#ifndef ALLVM_JITCACHE_H
#define ALLVM_JITCACHE_H

#include <llvm/ExecutionEngine/ObjectCache.h>

#include <string>

namespace allvm {

class JITCache : public llvm::ObjectCache {
public:
  ~JITCache() override {}

  JITCache(llvm::StringRef _CacheDir) : CacheDir(_CacheDir) {
    // Add trailing '/' to cache dir if necessary.
    if (!CacheDir.empty() && CacheDir[CacheDir.size() - 1] != '/')
      CacheDir += '/';
  }

  void notifyObjectCompiled(const llvm::Module *M,
                            llvm::MemoryBufferRef Obj) override;
  std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module *M) override;
  bool hasObjectFor(const llvm::Module *M);
  static std::string generateName(llvm::StringRef Name, uint32_t crc);

private:
  std::string CacheDir;

  std::unique_ptr<llvm::MemoryBuffer> getObject(llvm::StringRef Name);
  bool getCacheFilename(llvm::StringRef ModID, std::string &CacheName);
};

} // end namespace allvm

#endif // ALLVM_JITCACHE_H
