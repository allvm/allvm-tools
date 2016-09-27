#ifndef ALLVM_CACHE_LOCATION_H
#define ALLVM_CACHE_LOCATION_H

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <string>

namespace allvm {

static inline std::string getDefaultCacheDir(const llvm::Twine &CacheID) {
  llvm::SmallString<20> CacheDir;
  if (!llvm::sys::path::user_cache_directory(CacheDir, "allvm", CacheID))
    return ("allvm-cache-" + CacheID).str();
  return CacheDir.str();
}


} // end namespace allvm

#endif // ALLVM_CACHE_LOCATION_H
