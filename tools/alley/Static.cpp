#include "alley.h"

#include "StaticBinaryCache.h" // For naming, TODO: better design

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include <unistd.h>
//#include <types.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace allvm;
using namespace llvm;

static cl::opt<bool> DoStaticCodeGen(
    "doCodeGen", cl::init(true),
    cl::desc("Do static native code generation if necessary (default: ON)"));

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

// TODO: Return an Error instead of tr
int allvm::execWithStaticCompilation(allvm::Allexe &allexe,
                                     StringRef InputFilename,
                                     ArrayRef<std::string> Args,
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

  DEBUG(dbgs() << (isCached ? "Found in cache\n" : "Not in cache!\n"));

  if (!isCached && DoStaticCodeGen) {
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
    (void)tmpnam(tempFileName);
    ErrorOr<std::unique_ptr<object::Binary>> binary =
        compileAndLinkAllexeWithLlcDefaults(allexe, getLibNone(), "clang",
                                            tempFileName, context);
    if (!binary) {
      errs() << "Compile/link failed for allexe " << allexe.getModuleName(0)
             << "\n";
      return -1;
    }
    DEBUG(dbgs() << "Compiled successfully into " << tempFileName << "\n");

    // Now copy the executable to the cache location and delete the temp file
    Cache->notifyObjectCompiled(Mod, tempFileName);
    sys::fs::remove(Twine(tempFileName));
    execFD = Cache->getObjectFileDesc(Mod);
    assert(execFD != -1 && "Failed to retrieve native code from cache!");
  }

  // Convert Args from ArrayRef<std::string> to char*[].
  // This is all so low-level ... need an equivalent of execve in llvm::sys.
  char **argvArray = (char **)malloc((Args.size() + 1) * sizeof(char *));
  for (int i = 0, e = Args.size(); i < e; i++)
    argvArray[i] = (char *)Args[i].c_str();
  argvArray[Args.size()] = (char *)0; // null-terminate argvArray[]

  // Almost ready to launch this sucker
  DEBUG(dbgs() << "fexecve: " << execFD << ": " << argvArray[0] << "\n");
  fexecve(execFD, argvArray, const_cast<char **>(envp));

  perror("fexecve failed!"); // fexecve never returns if successful!
  return -1;
}
