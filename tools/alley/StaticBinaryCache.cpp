#include "StaticBinaryCache.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Binary.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "cache"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

using namespace llvm;

namespace allvm {

StaticBinaryCache::StaticBinaryCache(llvm::StringRef _CacheDir)
    : CacheDir(_CacheDir) {
  // Add trailing '/' to cache dir if necessary.
  if (!CacheDir.empty() && CacheDir[CacheDir.size() - 1] != '/') {
    CacheDir += '/';
  }
}

StaticBinaryCache::StaticBinaryCache() {

  SmallString<20> CDir;
  if (!sys::path::user_cache_directory(CDir, "allvm", "static_binaries")) {
    CDir = "allvm-cache";
  }

  CacheDir = CDir.str();

  // Add trailing '/' to cache dir if necessary.
  if (!CacheDir.empty() && CacheDir[CacheDir.size() - 1] != '/') {
    CacheDir += '/';
  }
}

// Copy binary code from an executable file to the cache location.
// execFilePath must be a valid path to a readable file.
void StaticBinaryCache::notifyObjectCompiled(StringRef CacheKey,
                                             StringRef execFilePath) {
  std::string CacheName;
  if (!getCacheFilename(CacheKey, CacheName)) {
    errs() << "Static cache given file it couldn't produce filename for!\n";
    errs() << "CacheKey: " << CacheKey << "\n";
    assert(0 && "This shouldn't happen!");
    exit(1);
  }
  if (!CacheDir.empty()) { // Create user-defined cache dir.
    SmallString<128> dir(sys::path::parent_path(CacheName));
    sys::fs::create_directories(Twine(dir));
  }
  DEBUG(dbgs() << "Copying binary for " << CacheKey << " from " << execFilePath
               << " to " << CacheName << "\n");
  std::error_code EC = sys::fs::rename(execFilePath, CacheName);
  if (EC) {
    errs() << "StaticBinaryCache: file copy failed: " << EC.message() << "\n";
    exit(1);
  }
}

int StaticBinaryCache::getObjectFileDesc(StringRef Name) const {
  std::string CacheName;
  if (!getCacheFilename(Name, CacheName))
    return -1;

  // FIXME: Need to ensure we have an absolute path name so exec is not
  // subject to name resolution by searching through a PATH.
  DEBUG(dbgs() << "Exec cache file name: " << CacheName << "\n");

  // Open the cache file in read-only mode and return the file descriptor.
  int execFD = open(CacheName.c_str(), S_IRUSR /*perm: 00400*/);
  if (execFD == -1) { // Not an error: just a cache miss
    DEBUG(dbgs() << "Executable does not exist in file cache " << CacheDir
                 << "\n");
  }

  return execFD;
}

bool StaticBinaryCache::getCacheFilename(StringRef ModID,
                                         std::string &CacheName) const {
  std::string Prefix("allexe:");
  size_t PrefixLength = Prefix.length();
  if (ModID.substr(0, PrefixLength) != Prefix)
    return false;
  std::string CacheSubdir = ModID.substr(PrefixLength);
#if defined(_WIN32)
  // Transform "X:\foo" => "/X\foo" for convenience.
  if (isalpha(CacheSubdir[0]) && CacheSubdir[1] == ':') {
    CacheSubdir[1] = CacheSubdir[0];
    CacheSubdir[0] = '/';
  }
#endif
  CacheName = CacheDir + CacheSubdir;
  size_t pos = CacheName.rfind('.');
  CacheName.replace(pos, CacheName.length() - pos, "");
  DEBUG(dbgs() << "For Module " << ModID << " : Cache file name: " << CacheName
               << "\n");
  return true;
}

std::string StaticBinaryCache::generateName(StringRef Name, uint32_t crc,
                                            const CompilationOptions *Options) {
  if (NULL != Options) {
    std::string buffer = Options->serializeCompilationOptions();
    size_t size = buffer.length();
    auto *raw_bytes = reinterpret_cast<const Bytef *>(buffer.data());
    assert(size <= UINT32_MAX);
    crc = static_cast<uint32_t>(
        crc32(crc, raw_bytes, static_cast<uint32_t>(size)));
  }
  std::string crcHex = utohexstr(crc);
  return ("allexe:" + crcHex + "-" + Name).str();
}

} // end namespace allvm
