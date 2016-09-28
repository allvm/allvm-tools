#include "ExecutionYengine.h"

#include "AOTCompile.h"
#include "StaticBinaryCache.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Binary.h>
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
 * Output:      Uses the StaticBinaryCache to look up and execute the native
 *              object code for the given .allexe program.
 *              If the code is not available in the cache, return success.
 *              If execution succeeds this function does not return,
 *              and returns an error if any are encountered.
 *
 * Assumptions: The allexe embeds a single module and is obtained
 *              by tool like alltogether. The key for getting a single
 *              module is to merge all the libraries with the main module,
 *              which is what alltogether does.
 ****************************************************************/

Error ExecutionYengine::tryStaticExec(StringRef Linker LLVM_ATTRIBUTE_UNUSED,
                                      const CompilationOptions &Options) {
  // TODO: Include 'Linker' and 'LibNone' and anything else relevant in the
  // cache lookup--changing these changes the resulting binary!
  // (And remove the LLVM_ATTRIBUTE_UNUSED bit from the argument above)

  auto &allexe = Info.allexe;
  if (allexe.getNumModules() != 1)
    return make_error<StringError>(
        "cannot execute allexe with more than one module",
        errc::invalid_argument);

  // Setting Up the Cache
  std::unique_ptr<StaticBinaryCache> Cache(new StaticBinaryCache());

  // Convert Args from ArrayRef<std::string> to char*[]
  SmallVector<const char *, 16> argv;
  for (auto &arg : Info.Args)
    argv.push_back(arg.data());
  argv.push_back(nullptr); // null-terminate argv

  // Set a unique name for the module using StaticBinaryCache's naming scheme
  auto crc = allexe.getModuleCRC(0);
  auto name = allexe.getModuleName(0); // == ALLEXE_MAIN

  auto CacheKey = StaticBinaryCache::generateName(name, crc, &Options);

  // Query the cache for an existing Image, which would avoid native code-gen
  int execFD = Cache->getObjectFileDesc(CacheKey);
  bool isCached = (execFD >= 0);

  DEBUG(dbgs() << (isCached ? "Found in cache\n" : "Not in cache!\n"));

  if (!isCached) {
    // No error encountered, but returning means we didn't exec() anything
    return Error::success();
  }

  if (Info.NoExec) {
    errs() << "'noexec' option set, skipping execution...\n";
    exit(0); // TODO: returning here would be nice
  }

  // Almost ready to launch this sucker
  DEBUG(dbgs() << "fexecve: " << execFD << ": " << argv[0] << "\n");
  fexecve(execFD, const_cast<char **>(argv.data()),
          const_cast<char **>(Info.envp));

  perror("fexecve failed!"); // fexecve never returns if successful!
  // XXX: FIXME
  return make_error<StringError>("Error fexecve!", errc::invalid_argument);
}
