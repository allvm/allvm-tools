#ifndef ALLVM_DEINLINEASM_H
#define ALLVM_DEINLINEASM_H

#include <llvm/IR/PassManager.h>

#include <string>

namespace allvm {

class DeInlineAsm final : public llvm::PassInfoMixin<DeInlineAsm> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

} // end namespace allvm

#endif // ALLVM_DEINLINEASM_H
