#include "StaticBinaryCache.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace allvm {

StaticBinaryCache::StaticBinaryCache(llvm::StringRef _CacheDir) : CacheDir(_CacheDir) {
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

  StringRef toStrRef = CDir;
  CacheDir = toStrRef;

  //Add trailing '/' to cache dir if necessary.
  if (!CacheDir.empty() && CacheDir[CacheDir.size() - 1] != '/') {
      CacheDir += '/';
  }
}

void StaticBinaryCache::notifyObjectCompiled(const Module *M, MemoryBufferRef Obj) {
  const std::string &ModuleID = M->getModuleIdentifier();
  std::string CacheName;
  if (!getCacheFilename(ModuleID, CacheName))
    return;
  if (!CacheDir.empty()) { // Create user-defined cache dir.
    SmallString<128> dir(sys::path::parent_path(CacheName));
    sys::fs::create_directories(Twine(dir));
  }
  std::error_code EC;
  raw_fd_ostream outfile(CacheName, EC, sys::fs::F_None);
  outfile.write(Obj.getBufferStart(), Obj.getBufferSize());
  outfile.close();
}

std::unique_ptr<MemoryBuffer> StaticBinaryCache::getObject(const Module *M) {
  return getObject(M->getModuleIdentifier());
}

bool StaticBinaryCache::hasObjectFor(const Module *M) {
  // TODO: Avoid loading file altogether!
  std::string CacheName;
  if (!getCacheFilename(M->getModuleIdentifier(), CacheName))
    return false;
  // Load the object from the cache filename
  ErrorOr<std::unique_ptr<MemoryBuffer>> IRObjectBuffer =
      MemoryBuffer::getFile(CacheName.c_str(), -1, false);
  // If the file isn't there, that's OK.
  return !!IRObjectBuffer;
}

std::unique_ptr<MemoryBuffer> StaticBinaryCache::getObject(StringRef Name) {
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

bool StaticBinaryCache::getCacheFilename(StringRef ModID, std::string &CacheName) {
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
  CacheName.replace(pos, CacheName.length() - pos, ".o");
  return true;
}

std::string StaticBinaryCache::generateName(StringRef Name, uint32_t crc, CompilationOptions*  Options) {
  if(NULL != Options) {
    std::string buffer = Options->serializeCompilationOptions();
    size_t size = buffer.length();
    const unsigned char* raw_bytes = (const unsigned char*) buffer.c_str();
    crc = crc32(crc, raw_bytes, size);
  }
  std::string crcHex = utohexstr(crc);
  return ("allexe:" + crcHex + "-" + Name).str();
}

} // end namespace allvm
