#ifndef ALLVM_ImageExecutor_h
#define ALLVM_ImageExecutor_h

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class ExecutionEngine;
class Module;
}

namespace allvm {

class ImageExecutor final {
  llvm::Module *M;
  std::unique_ptr<llvm::ExecutionEngine> EE;
public:
  ImageExecutor(std::unique_ptr<llvm::Module> &&mainModule);
  ~ImageExecutor();
  void addModule(std::unique_ptr<llvm::Module> M);

  int runBinary(const std::vector<std::string> &argv, const char **envp);
};

}

#endif
