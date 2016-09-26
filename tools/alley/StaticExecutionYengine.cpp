#include "alley.h"

#include "StaticBinaryCache.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Errc.h>

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEBUG_TYPE "alley"

using namespace allvm;
using namespace llvm;

/****************************************************************
 * Name:        execWithStaticCompilation
 *
 * Input:       A source file in allexe format & program's
 *              arguments and environment.
 *              On a cache miss, DoStaticCodeGenIfNeeded determines
 *              whether compilation should be performed.
 *
 * Output:      Uses the StaticBinaryCache to look up and execute the native
 *              object code for the given .allexe program.  If the code is
 *              not available in the cache:
 *              If DoStaticCodeGenIfNeeded is true, the code is compiled
 *              and added to the cache.
 *              If DoStaticCodeGenIfNeeded is false, return success.
 *              If execution succeeds this function does not return,
 *              and returns and error if any are encountered.
 *
 * Assumptions: The allexe embeds a single module and is obtained
 *              by tool like alltogether. The key for getting a single
 *              module is to merge all the libraries with the main module,
 *              which is what alltogether does.
 ****************************************************************/

Error allvm::tryStaticExec(ExecutionInfo &EI, bool DoStaticCodeGenIfNeeded) {
  auto &allexe = EI.allexe;
  assert(allexe.getNumModules() == 1 &&
         "The input must be an allexe with a single module");
  LLVMContext context;

  const CompilationOptions Options;

  // Set a unique name for the module using StaticBinaryCache's naming scheme
  auto crc = allexe.getModuleCRC(0);
  auto name = allexe.getModuleName(0); // == ALLEXE_MAIN

  auto CacheKey = StaticBinaryCache::generateName(name, crc, &Options);

  // Setting Up the Cache
  std::unique_ptr<StaticBinaryCache> Cache(new StaticBinaryCache());

  // Query the cache for an existing Image, which would avoid native code-gen
  int execFD = Cache->getObjectFileDesc(CacheKey);
  bool isCached = (execFD >= 0);

  DEBUG(dbgs() << (isCached ? "Found in cache\n" : "Not in cache!\n"));

  if (!isCached) {
    if (!DoStaticCodeGenIfNeeded) {
      // No error encountered, but returning means we didn't exec() anything
      return Error::success();
    }
    // Generate native code for the specified .allexe into a temp file.
    // C++ doesn't have a portable way to create a temp file: using a C idiom.
    // FIXME: tmpnam() should not be used because there is a small chance
    // the temp file is created by another process or thread before being
    // opened by us, but that cannot be fixed here: the real problem
    // is that StaticCodeGen() wants a file name and then writes to it,
    // creating an opening for the race condition.  Instead, it should have
    // an option to leave the file name unspecified and create it internally.
    // XXX: see LLVM's createUniqueFile, maybe?
    DEBUG(dbgs() << "Starting static compilation...\n");
    char tempFileName[L_tmpnam];
    (void)tmpnam(tempFileName);
    auto binary = compileAndLinkAllexeWithLlcDefaults(
        allexe, EI.LibNone, "clang", tempFileName, context);
    if (!binary)
      return make_error<StringError>("error during compilation/linking",
                                     binary.getError());
    DEBUG(dbgs() << "Compiled successfully into " << tempFileName << "\n");

    // Now copy the executable to the cache location and delete the temp file
    Cache->notifyObjectCompiled(CacheKey, tempFileName);
    sys::fs::remove(Twine(tempFileName));
    execFD = Cache->getObjectFileDesc(CacheKey);
    assert(execFD != -1 && "Failed to retrieve native code from cache!");
  }

  // Convert Args from ArrayRef<std::string> to char*[]
  SmallVector<const char *, 16> argv;
  for (auto &arg : EI.Args)
    argv.push_back(arg.data());
  argv.push_back(nullptr); // null-terminate argv

  // Almost ready to launch this sucker
  DEBUG(dbgs() << "fexecve: " << execFD << ": " << argv[0] << "\n");
  fexecve(execFD, const_cast<char **>(argv.data()),
          const_cast<char **>(EI.envp));

  perror("fexecve failed!"); // fexecve never returns if successful!
  // XXX: FIXME
  return make_error<StringError>("Error fexecve!", errc::invalid_argument);
}
