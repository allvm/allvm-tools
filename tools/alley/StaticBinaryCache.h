#ifndef ALLVM_StaticBinaryCache_H
#define ALLVM_StaticBinaryCache_H

#include "StaticCodeGen.h"
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <string>

namespace allvm {

class StaticBinaryCache : public llvm::ObjectCache {
public:
  ~StaticBinaryCache() override {}

  StaticBinaryCache(llvm::StringRef _CacheDir);
  StaticBinaryCache();
  void notifyObjectCompiled(const llvm::Module *M,
                            llvm::MemoryBufferRef Obj) override;
  std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module *M) override;
  bool hasObjectFor(const llvm::Module *M);
  static std::string generateName(llvm::StringRef Name, uint32_t crc,
                                  const CompilationOptions *Options = NULL);

private:
  std::string CacheDir;

  std::unique_ptr<llvm::MemoryBuffer> getObject(llvm::StringRef Name);
  bool getCacheFilename(llvm::StringRef ModID, std::string &CacheName);
};

} // end namespace allvm

#endif // ALLVM_StaticBinaryCache_H
