#include "AOTCompile.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Object/Binary.h>
#include <llvm/Support/Errc.h>

#define DEBUG_TYPE "alley"

using namespace allvm;
using namespace llvm;

Error allvm::AOTCompileIfNeeded(StaticBinaryCache &Cache, Allexe &allexe,
                                StringRef LibNone, StringRef Linker,
                                const CompilationOptions &Options) {
  if (allexe.getNumModules() != 1)
    return make_error<StringError>(
        "cannot execute allexe with more than one module",
        errc::invalid_argument);
  LLVMContext context;

  // Set a unique name for the module using StaticBinaryCache's naming scheme
  auto crc = allexe.getModuleCRC(0);
  auto name = allexe.getModuleName(0); // == ALLEXE_MAIN

  auto CacheKey = StaticBinaryCache::generateName(name, crc, &Options);

  // Query the cache for an existing Image, which would avoid native code-gen
  int execFD = Cache.getObjectFileDesc(CacheKey);
  bool isCached = (execFD >= 0);

  DEBUG(dbgs() << (isCached ? "Found in cache\n" : "Not in cache!\n"));

  if (!isCached) {
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
    auto binary = compileAndLinkAllexeWithLlcDefaults(allexe, LibNone, Linker,
                                                      tempFileName, context);
    if (!binary)
      return make_error<StringError>("error during compilation/linking",
                                     binary.getError());
    DEBUG(dbgs() << "Compiled successfully into " << tempFileName << "\n");

    // Now copy the executable to the cache location and delete the temp file
    Cache.notifyObjectCompiled(CacheKey, tempFileName);
    sys::fs::remove(Twine(tempFileName));
    execFD = Cache.getObjectFileDesc(CacheKey);
    assert(execFD != -1 && "Failed to retrieve native code from cache!");
  }

  return Error::success();
}
