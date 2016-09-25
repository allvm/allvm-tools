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

std::string StaticBinaryCache::getAndResetCacheFile(const Module *M,
                                                    bool doReset = true) {
  const std::string &ModuleID = M->getModuleIdentifier();
  std::string CacheName;
  if (!getCacheFilename(ModuleID, CacheName))
    return std::string("");
  if (!CacheDir.empty()) { // Create user-defined cache dir.
    SmallString<128> dir(sys::path::parent_path(CacheName));
    sys::fs::create_directories(Twine(dir));
  }
  // Delete existing file with this name, if any, because it is out of date
  if (doReset)
    sys::fs::remove(Twine(CacheName)); // safe even if file does not exist.
  return CacheName;
}

// Copy binary code from a MemoryBuffer to the cache location
void StaticBinaryCache::notifyObjectCompiled(const Module *M,
                                             MemoryBufferRef Obj) {
  std::string CacheName = getAndResetCacheFile(M, /*doReset*/ true);
  DEBUG(dbgs() << "Caching MemoryBuffer for " << M->getModuleIdentifier()
               << " at " << CacheName << "\n");
  std::error_code EC;
  raw_fd_ostream outfile(CacheName, EC, sys::fs::F_None);
  outfile.write(Obj.getBufferStart(), Obj.getBufferSize());
  outfile.close();
}

// Copy binary code from an executable file to the cache location.
// execFilePath must be a valid path to a readable file.
void StaticBinaryCache::notifyObjectCompiled(const Module *M,
                                             const std::string &execFilePath) {
  std::string CacheName = getAndResetCacheFile(M, /*doReset*/ false);
  DEBUG(dbgs() << "Copying binary for " << M->getModuleIdentifier() << " from "
               << execFilePath << " to " << CacheName << "\n");
  std::error_code EC = sys::fs::rename(execFilePath, Twine(CacheName));
  if (EC.value() < 0)
    errs() << "StaticBinaryCache: file copy failed: " << EC.message() << "\n";
}

#if 0
void StaticBinaryCache::notifyObjectCompiled(const Module *M,
			     std::unique_ptr<object::Binary> binary) {
  std::string CacheName = getAndResetCacheFile(M);
  DEBUG(dbgs() << "Caching binary for " << M->getModuleIdentifier()
	<< " at " << CacheName << "\n");
  MemoryBufferRef Obj = binary.get()->getMemoryBufferRef();
  std::error_code EC;
  raw_fd_ostream outfile(CacheName, EC, sys::fs::F_None);
  outfile.write(Obj.getBufferStart(), Obj.getBufferSize());
  outfile.close();
}
#endif

std::unique_ptr<MemoryBuffer> StaticBinaryCache::getObject(const Module *M) {
  return getObject(M->getModuleIdentifier());
}

int StaticBinaryCache::getObjectFileDesc(const Module *M) const {
  return getObjectFileDesc(M->getModuleIdentifier());
}

bool StaticBinaryCache::hasObjectFor(const Module *M) {
  std::string CacheName;
  // FIXME: Avoid loading file altogether!
  if (!getCacheFilename(M->getModuleIdentifier(), CacheName))
    return false;
  // Load the object from the cache filename
  ErrorOr<std::unique_ptr<MemoryBuffer>> IRObjectBuffer =
      MemoryBuffer::getFile(CacheName.c_str(), -1, false);
  // Return true if the file is there
  return !!IRObjectBuffer;
}

std::unique_ptr<MemoryBuffer>
StaticBinaryCache::getObject(StringRef Name) const {
  std::string CacheName;
  if (!getCacheFilename(Name, CacheName))
    return nullptr;
  // Load the object from the cache filename
  ErrorOr<std::unique_ptr<MemoryBuffer>> IRObjectBuffer =
      MemoryBuffer::getFile(CacheName.c_str(), -1, false);
  // If the file isn't there, that's OK.
  if (!IRObjectBuffer)
    return nullptr;
  // MCJIT will want to write into this buffer, and we don't want that
  // because the file has probably just been mmapped.  Instead we make
  // a copy.  The filed-based buffer will be released when it goes
  // out of scope.
  return MemoryBuffer::getMemBufferCopy(IRObjectBuffer.get()->getBuffer());
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
