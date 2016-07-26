#ifndef ALLVM_ImageCache_h
#define ALLVM_ImageCache_h

#include <llvm/ExecutionEngine/ObjectCache.h>

#include <string>

namespace allvm {

class ImageCache : public llvm::ObjectCache {
public:
  ~ImageCache() override{};

  ImageCache(const std::string &CacheDir) : CacheDir(CacheDir) {
    // Add trailing '/' to cache dir if necessary.
    if (!this->CacheDir.empty() &&
        this->CacheDir[this->CacheDir.size() - 1] != '/')
      this->CacheDir += '/';
  }

  void notifyObjectCompiled(const llvm::Module *M,
                            llvm::MemoryBufferRef Obj) override;
  std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module *M) override;
  std::unique_ptr<llvm::MemoryBuffer> getObject(llvm::StringRef Name,
                                                uint32_t crc);
  static std::string generateName(llvm::StringRef Name, uint32_t crc);

private:
  std::string CacheDir;

  std::unique_ptr<llvm::MemoryBuffer> getObject(llvm::StringRef Name);
  bool getCacheFilename(llvm::StringRef ModID, std::string &CacheName);
};

} // end namespace allvm

#endif // ALLVM_ImageCache_h
