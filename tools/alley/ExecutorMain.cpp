#include "Allexe.h"
#include "ImageCache.h" // For naming, TODO: better design
#include "ImageExecutor.h"
#include "StaticBinaryCache.h" // For naming, TODO: better design

#include "llvm/Object/ObjectFile.h"
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
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "alley"

#include <unistd.h>
//#include <types.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

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

static cl::opt<bool> DisableJIT(
    "disableJIT", cl::init(false),
    cl::desc("Choose between JIT or static compilation (default: JIT)"));

static cl::opt<bool> DoStaticCodeGen(
    "doCodeGen", cl::init(true),
    cl::desc("Do static native code generation if necessary (default: ON)"));

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
  if (DisableJIT) {
    return execWithStaticCompilation(allexe, envp);
  } else {
    return execWithJITCompilation(allexe, envp);
  }
}


// Helper function for execWith{JIT,Static}Compilation:
// Add name of file as argv[0]
void AddProgName()
{
  std::string theName(InputFilename.getValue()); // make a copy to edit
  StringRef ProgName = theName;	// wrap in a StringRef for easier operations
  if (sys::path::has_extension(InputFilename))
    ProgName = ProgName.drop_back(sys::path::extension(ProgName).size());
  InputArgv.insert(InputArgv.begin(), ProgName);
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
  AddProgName();

  return executor->runHostedBinary(InputArgv, envp, LibNone);
}


/****************************************************************
 * Name:        execWithStaticCompilation
 *
 * Input:       A source file in allexe format & program’s environment.
 *
 * Output:      Uses the StaticBinaryCache to look up and execute the native
 *              object code for the given .allexe program.  If the code is
 *              not available in the cache, this should return an error, but
 *              (for convenience and for experiments) will compile the 
 *              module embedded in allexe to create an object file, and then
 *              notifies the cache to save it before executing it. 
 *
 * Assumptions: The allexe embeds a single module and is obtained
 *              by tool like alltogether. The key for getting a single
 *              module is to merge all the libraries with the main module,
 *              which is what alltogether does.
 ****************************************************************/

int execWithStaticCompilation(allvm::Allexe &allexe,
                              const char **envp) {

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
    // FIXME: Do something with M's error
    exit(1);
  }

  // Set a unique name for the module using StaticBinaryCache's naming scheme
  const CompilationOptions Options;
  M.get()->setModuleIdentifier(
      StaticBinaryCache::generateName(name, crc, &Options));

  // Setting Up the Cache
  std::unique_ptr<StaticBinaryCache> Cache(new StaticBinaryCache());

  // Query the cache for an existing Image, which would avoid native code-gen
  Module *Mod = M.get().get();
  int execFD = Cache->getObjectFileDesc(Mod);
  bool isCached = (execFD >= 0);

  DEBUG(dbgs() << (isCached? "Found in cache\n" : "Not in cache!\n"));

  if (! isCached && DoStaticCodeGen) {
    // Generate native code for the specified .allexe into a temp file.
    // C++ doesn't have a portable way to create a temp file: using a C idiom.
    // FIXME: tmpnam() should not be used because there is a small chance
    // the temp file is created by another process or thread before being 
    // opened by us, but that cannot be fixed here: the real problem
    // is that StaticCodeGen() wants a file name and then writes to it,
    // creating an opening for the race condition.  Instead, it should have
    // an option to leave the file name unspecified and create it internally.
    // 
    char tempFileName[L_tmpnam];
    (void) tmpnam(tempFileName);
    ErrorOr<std::unique_ptr<object::Binary>> binary =
      compileAndLinkAllexeWithLlcDefaults(allexe, LibNone, "clang",
					  tempFileName, context);
    if (! binary) {
      errs() << "Compile/link failed for allexe " 
	     << allexe.getModuleName(0) << "\n";
      return -1;
    }
    DEBUG(dbgs() << "Compiled successfully into " << tempFileName << "\n");

    // Now copy the executable to the cache location and delete the temp file
    Cache->notifyObjectCompiled(Mod, tempFileName);
    sys::fs::remove(Twine(tempFileName));
    execFD = Cache->getObjectFileDesc(Mod);
    assert(execFD != -1 && "Failed to retrieve native code from cache!");
  }

  // Add the name of the allexe file (without the extension) as InputArgv[0].
  // Do NOT use the native code name: that is not a meaningful name.
  // 
  AddProgName();
  
  // Convert InputArgv[] from cl::list<std::string> to char*[].
  // This is all so low-level ... need an equivalent of execve in llvm::sys.
  char** argvArray = (char**) malloc((InputArgv.size() + 1) * sizeof(char*));
  for (int i=0, e = InputArgv.size(); i < e; i++)
    argvArray[i] = (char*) InputArgv[i].c_str();
  argvArray[InputArgv.size()] = (char*) 0; // null-terminate argvArray[]

  // Almost ready to launch this sucker
  DEBUG(dbgs() << "fexecve: " << execFD << ": " << argvArray[0] << "\n");
  fexecve(execFD, argvArray, const_cast<char**>(envp));

  perror("fexecve failed!");	// fexecve never returns if successful!
  return -1;
}
