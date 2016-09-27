#ifndef ALLVM_CACHE_LOCATION_H
#define ALLVM_CACHE_LOCATION_H

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <string>

namespace allvm {

static const auto CACHE_ENV_VAR = "ALLVM_CACHE_DIR";
static const char *CacheDirEnvValue = std::getenv(CACHE_ENV_VAR);
static const std::string CacheDirEnvOverride =
    CacheDirEnvValue ? CacheDirEnvValue : "";

static inline std::string getDefaultCacheDir(const llvm::Twine &CacheID) {
  llvm::SmallString<20> CacheDir;

  // If user sets the environment variable, use it as cache directory
  // (Note that we trust the user at this point, hopefully it's writable , etc.)
  if (!CacheDirEnvOverride.empty()) {
    CacheDir = CacheDirEnvOverride;
    llvm::sys::path::append(CacheDir, CacheID);
    return CacheDir.str();
  }

  if (!llvm::sys::path::user_cache_directory(CacheDir, "allvm", CacheID))
    return ("allvm-cache-" + CacheID).str();
  return CacheDir.str();
}

} // end namespace allvm

#endif // ALLVM_CACHE_LOCATION_H
