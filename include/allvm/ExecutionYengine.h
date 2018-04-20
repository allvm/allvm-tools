#ifndef ALLVM_EXECUTION_YENGINE_H
#define ALLVM_EXECUTION_YENGINE_H

#include "allvm/Allexe.h"
#include "allvm/StaticCodeGen.h" // for 'CompilationOptions'

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
    bool NoExec; // do everything but actually execute
  };

  ExecutionYengine(ExecutionInfo EI) : Info(EI) {}

  // Execute statically-compiled binary if available in cache.
  llvm::Error tryStaticExec(llvm::StringRef Linker,
                            const CompilationOptions &Options);

  // Execute using JIT
  llvm::Error doJITExec();

  // Execute using OrcJIT
  llvm::Error doOrcJITExec();

private:
  ExecutionInfo Info;
};

} // end namespace allvm

#endif // ALLVM_EXECUTION_YENGINE_H
