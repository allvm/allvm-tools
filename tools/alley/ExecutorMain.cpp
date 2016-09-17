#include "Allexe.h"
#include "ImageCache.h" // For naming, TODO: better design
#include "StaticBinaryCache.h" // For naming, TODO: better design
#include "ImageExecutor.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Object/ObjectFile.h"

using namespace allvm;
using namespace llvm;

int execWithStaticCompilation(allvm::Allexe &, const char **);
int execWithJITCompilation(allvm::Allexe &, const char **);

static std::string getDefaultLibNone() {
  static int StaticSymbol;
  auto Executable = sys::fs::getMainExecutable("allvm_tool", &StaticSymbol);
  auto BinDir = sys::path::parent_path(Executable);
  return (BinDir + "/../lib/libnone.a").str();
}

static cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"),
                                    cl::init(getDefaultLibNone()));

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input allvm file>"));

static cl::list<std::string> InputArgv(cl::ConsumeAfter,
                                       cl::desc("<program arguments>..."));

static cl::opt<bool> EnableJIT(
    "enableJIT", cl::init(true),
    cl::desc("Choose between JIT or static compilation (default: JIT)"));

const StringRef ALLEXE_MAIN = "main.bc";

int main(int argc, const char **argv, const char **envp) {
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  cl::ParseCommandLineOptions(argc, argv, "allvm runtime executor");

  auto allexeOrError = Allexe::open(InputFilename);
  if (!allexeOrError) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << allexeOrError.getError().message() << '\n';
    return 1;
  }

  auto &allexe = *allexeOrError.get();
  if (allexe.getNumModules() == 0) {
    errs() << "allexe contained no files!\n";
    return 1;
  }

  // Choose the code generation mode Dynamic (using JIT) or Static
  if (EnableJIT) {
    return execWithJITCompilation(allexe, envp);
  } else {
    return execWithStaticCompilation(allexe, envp);
  }
}

/****************************************************************
 * Name:        execWithJITCompilation
 *
 * Input:       A source file in allexe format & program’s environment.
 *
 * Output:      Create an JIT Executaion Engine which takes the mainmodule
 *              and library modules (embedded in the input file) and executes
 *it.
 ****************************************************************/
int execWithJITCompilation(allvm::Allexe &allexe, const char **envp) {

  auto mainFile = allexe.getModuleName(0);

  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  LLVMContext context;
  auto LoadModule = [&](size_t idx) {
    uint32_t crc;
    auto M = allexe.getModule(idx, context, &crc);
    auto name = allexe.getModuleName(idx);
    if (!M) {
      errs() << "Could not read " << InputFilename << ": " << name << "\n";
      // XXX: Do something with M's error
      exit(1);
    }
    M.get()->setModuleIdentifier(ImageCache::generateName(name, crc));
    return std::move(M.get());
  };

  // get main module
  auto executor = make_unique<ImageExecutor>(LoadModule(0));

  // Add supporting libraries
  for (size_t i = 1, e = allexe.getNumModules(); i != e; ++i) {
    executor->addModule(LoadModule(i));
  }

  // Add name of file as argv[0]
  StringRef ProgName = InputFilename;
  if (sys::path::has_extension(InputFilename))
    ProgName = ProgName.drop_back(sys::path::extension(ProgName).size());
  InputArgv.insert(InputArgv.begin(), ProgName);

  return executor->runHostedBinary(InputArgv, envp, LibNone);
}

/****************************************************************
 * Name:        execWithStaticCompilation
 *
 * Input:       A source file in allexe format & program’s environment.
 *
 * Output:      Compiles the 'single' module embedded in allexe and creates
 *              an object file out of it.
 *
 * Assumptions: The allexe embeds a single module and is obtained
 *              by tool like alltogether. The key for getting a single
 *              module is to merge all the librraies with the main module
 *              which is what alltogether does.
 ****************************************************************/
int execWithStaticCompilation(allvm::Allexe &allexe, const char **envp) {

  assert(allexe.getNumModules() == 1 &&
         "The input must be an allexe with a single module");
  auto mainFile = allexe.getModuleName(0);

  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  LLVMContext context;
  // Setting up hash key as the Module identifier
  uint32_t crc;
  auto M = allexe.getModule(0, context, &crc);
  auto name = allexe.getModuleName(0);
  if (!M) {
    errs() << "Could not read " << InputFilename << ": " << name << "\n";
    // XXX: Do something with M's error
    exit(1);
  }
  CompilationOptions Options;
  M.get()->setModuleIdentifier(StaticBinaryCache::generateName(name, crc, &Options));

  // Setting Up the Cache
  std::unique_ptr<StaticBinaryCache> Cache(new StaticBinaryCache());

  // Query the cache before the static compilation for an existing Image
  Module *Mod = M.get().get();
  bool isCached = Cache->hasObjectFor(Mod);
  llvm::errs() << "\nisPresent:" << isCached << "\n";

  if (isCached) {
    // Cache Hit; Execute
    return 0;
  }

  // The following is just for testing the current implementation of caching.
  llvm::ErrorOr<std::unique_ptr<object::ObjectFile>> ObjFile = 
    compileAllexe(allexe, "test.out", Options, context);

  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFile("test.out");
  if (std::error_code EC = FileOrErr.getError()) {
    errs() << " Error reading fbject file to buffer '" << ALLEXE_MAIN << "'\n";
    return 1;
  }
  std::unique_ptr<MemoryBuffer> Buffer = std::move(FileOrErr.get());
  Cache->notifyObjectCompiled(Mod, Buffer->getMemBufferRef());

  return 0;
}
