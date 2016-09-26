#ifndef ALLVM_EXECUTION_YENGINE_H
#define ALLVM_EXECUTION_YENGINE_H

#include "Allexe.h"
#include "StaticBinaryCache.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

namespace allvm {

class ExecutionYengine final {

public:
  struct ExecutionInfo {
    allvm::Allexe &allexe;
    llvm::ArrayRef<std::string> Args;
    const char **envp;
    llvm::StringRef LibNone;
  };

  ExecutionYengine(ExecutionInfo EI) : Info(EI) {}

  // Execute statically-compiled binary if available in cache.
  llvm::Error tryStaticExec(llvm::StringRef Linker,
                            const CompilationOptions &Options,
                            bool DoStaticCodeGenIfNeeded);

  // Execute using JIT
  llvm::Error doJITExec();

private:
  ExecutionInfo Info;
};

} // end namespace allvm

#endif // ALLVM_EXECUTION_YENGINE_H
