#include "alley.h"

#include "ImageCache.h" // For naming, TODO: better design
#include "ImageExecutor.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

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

static cl::opt<bool> DisableJIT(
    "disableJIT", cl::init(false),
    cl::desc("Choose between JIT or static compilation (default: JIT)"));

StringRef allvm::getLibNone() { return LibNone; }

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

  StringRef ProgName = InputFilename;
  if (sys::path::has_extension(InputFilename))
    ProgName = ProgName.drop_back(sys::path::extension(ProgName).size());
  InputArgv.insert(InputArgv.begin(), ProgName);

  // Choose the code generation mode Dynamic (using JIT) or Static
  if (DisableJIT) {
    return execWithStaticCompilation(allexe, InputFilename, InputArgv, envp);
  } else {
    return execWithJITCompilation(allexe, InputFilename, InputArgv, envp);
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
int allvm::execWithJITCompilation(allvm::Allexe &allexe,
                                  llvm::StringRef Filename,
                                  llvm::ArrayRef<std::string> Args,
                                  const char **envp) {

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

  return executor->runHostedBinary(InputArgv, envp, LibNone);
}
