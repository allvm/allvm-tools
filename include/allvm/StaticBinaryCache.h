#ifndef ALLVM_StaticBinaryCache_H
#define ALLVM_StaticBinaryCache_H

#include "allvm/StaticCodeGen.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>

#include <string>

namespace allvm {

class StaticBinaryCache {
public:
  ~StaticBinaryCache() {}

  StaticBinaryCache(llvm::StringRef _CacheDir);
  StaticBinaryCache();

  int getObjectFileDesc(llvm::StringRef Name) const;
  void notifyObjectCompiled(llvm::StringRef CacheKey,
                            llvm::StringRef execFilePath);

  static std::string generateName(llvm::StringRef Name, uint32_t crc,
                                  const CompilationOptions *Options = NULL);

private:
  std::string CacheDir;

  bool getCacheFilename(llvm::StringRef ModID, std::string &CacheName) const;
};

} // end namespace allvm

#endif // ALLVM_StaticBinaryCache_H
