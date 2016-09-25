#ifndef ALLVM_StaticBinaryCache_H
#define ALLVM_StaticBinaryCache_H

#include "StaticCodeGen.h"
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <string>

namespace allvm {

// ******************
// FIXME: Cache interface should be indexed with the allexe file, not
// LLVM::Module*, since this cache is the ALLVM cache for allexe -> native,
// and an LLVM Module is just an intermediate (temporary) object along the
// way.  For example, a cache hit should be possible to check without ever 
// extracting the LLVM Module from the allexe.
// ******************

class StaticBinaryCache : public llvm::ObjectCache {
public:
  ~StaticBinaryCache() override {}

  StaticBinaryCache(llvm::StringRef _CacheDir);
  StaticBinaryCache();

  // Insert machine code for a given module into the cache.
  // The machine code may come from a MemoryBuffer or a file.
  void notifyObjectCompiled(const llvm::Module *M,
                            llvm::MemoryBufferRef Obj) override;
  void notifyObjectCompiled(const llvm::Module *M,
                            const std::string& execFilePath);

  // Retrieve machine code for a given module from the cache.
  // Get this as either a MemoryBuffer object or a file descriptor.
  // Both could be const but base class function for first is not marked const.
  std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module *M) override;
  int getObjectFileDesc(const llvm::Module *M) const;
  
  // Query the cache to check whether the machine code for a module exists
  bool hasObjectFor(const llvm::Module *M);
  
  static std::string generateName(llvm::StringRef Name, uint32_t crc,
                                  const CompilationOptions *Options = NULL);

private:
  std::string CacheDir;

  std::unique_ptr<llvm::MemoryBuffer> getObject(llvm::StringRef Name) const;
  int getObjectFileDesc(llvm::StringRef Name) const;
  bool getCacheFilename(llvm::StringRef ModID, std::string &CacheName) const;
  std::string getAndResetCacheFile(const llvm::Module* M, bool doReset);
};

} // end namespace allvm

#endif // ALLVM_StaticBinaryCache_H
