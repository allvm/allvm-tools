#include "CacheLocation.h"

#include "allvm/StaticBinaryCache.h"

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

StaticBinaryCache::StaticBinaryCache(StringRef _CacheDir)
    : CacheDir(_CacheDir) {
  // Add trailing '/' to cache dir if necessary.
  if (!CacheDir.empty() && CacheDir[CacheDir.size() - 1] != '/') {
    CacheDir += '/';
  }
}

StaticBinaryCache::StaticBinaryCache()
    : StaticBinaryCache(getDefaultCacheDir("static_binaries")) {}

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
  LLVM_DEBUG(dbgs() << "Copying binary for " << CacheKey << " from "
                    << execFilePath << " to " << CacheName << "\n");
  std::string CacheNameTmp = CacheName + ".tmp";
  auto EC = sys::fs::copy_file(execFilePath, CacheNameTmp);
  if (EC) {
    errs() << "StaticBinaryCache: file copy failed: " << EC.message() << "\n";
    exit(1);
  }
  int ret = chmod(CacheNameTmp.data(), S_IRUSR | S_IXUSR);
  if (ret) {
    errs() << "StaticBinaryCache: Error changing permissions of file!\n";
    exit(1);
  }
  EC = sys::fs::rename(CacheNameTmp, CacheName);
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
  LLVM_DEBUG(dbgs() << "Exec cache file name: " << CacheName << "\n");

  // Open the cache file in read-only mode and return the file descriptor.
  int execFD = open(CacheName.c_str(), O_PATH | O_CLOEXEC);
  if (execFD == -1) { // Not an error: just a cache miss
    LLVM_DEBUG(dbgs() << "Executable does not exist in file cache " << CacheDir
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
  LLVM_DEBUG(dbgs() << "For Module " << ModID
                    << " : Cache file name: " << CacheName << "\n");
  return true;
}

std::string StaticBinaryCache::generateName(StringRef Name, uint32_t crc,
                                            const CompilationOptions *Options) {
  if (nullptr != Options) {
    std::string buffer = Options->serializeCompilationOptions();
    size_t size = buffer.length();
    auto *raw_bytes = reinterpret_cast<const Bytef *>(buffer.data());
    assert(size <= UINT32_MAX);
    crc = static_cast<uint32_t>(
        crc32(crc, raw_bytes, static_cast<uint32_t>(size)));
  }
  // We only even add the 'Name' part for human readability,
  // so if it's a full path only use the filename portion
  if (sys::path::has_filename(Name))
    Name = sys::path::filename(Name);
  std::string crcHex = utohexstr(crc);
  return ("allexe:" + crcHex + "-" + Name).str();
}

} // end namespace allvm
