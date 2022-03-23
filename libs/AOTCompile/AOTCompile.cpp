#include "allvm/AOTCompile.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Object/Binary.h>
#include <llvm/Support/Errc.h>

#define DEBUG_TYPE "alley"

using namespace allvm;
using namespace llvm;

Error allvm::AOTCompileIfNeeded(StaticBinaryCache &Cache, const Allexe &allexe,
                                StringRef LibNone, StringRef CrtBits,
                                const ALLVMLinker &Linker,
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
    DEBUG(dbgs() << "Starting static compilation...\n");
    SmallString<20> tempFileName;
    if (auto EC = sys::fs::createTemporaryFile("allvm", "aot", tempFileName))
      return make_error<StringError>("Unable to create temporary file", EC);

    auto binary = compileAndLinkAllexeWithLlcDefaults(
        allexe, LibNone, CrtBits, Linker, tempFileName, context);
    if (!binary)
      return binary.takeError();
    DEBUG(dbgs() << "Compiled successfully into " << tempFileName << "\n");

    // Now copy the executable to the cache location and delete the temp file
    Cache.notifyObjectCompiled(CacheKey, tempFileName);
    sys::fs::remove(Twine(tempFileName));
    execFD = Cache.getObjectFileDesc(CacheKey);
    assert(execFD != -1 && "Failed to retrieve native code from cache!");
  }

  return Error::success();
}
