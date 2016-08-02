#ifndef ALLVM_ImageExecutor_h
#define ALLVM_ImageExecutor_h

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class ExecutionEngine;
class Module;
class StringRef;
class MemoryBufferRef;
}

namespace allvm {

class ImageCache;

class ImageExecutor final {
  llvm::Module *M;
  std::unique_ptr<llvm::ExecutionEngine> EE;
  std::unique_ptr<ImageCache> Cache;
public:
  ImageExecutor(std::unique_ptr<llvm::Module> &&mainModule,
                bool UseCache = true);
  ~ImageExecutor();
  void addModule(std::unique_ptr<llvm::Module> M);

  int runBinary(const std::vector<std::string> &argv, const char **envp);
  int runHostedBinary(const std::vector<std::string> &argv, const char **envp,
                      llvm::StringRef LibNone);
};

}

#endif
