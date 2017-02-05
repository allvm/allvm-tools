#ifndef ALLVM_MODULEFLAGS_H
#define ALLVM_MODULEFLAGS_H

#include <llvm/IR/Module.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Metadata.h>

namespace allvm {

static const llvm::StringRef MF_ALLVM_SOURCE = "ALLVM Source";
static const llvm::StringRef MF_WLLVM_SOURCE = "WLLVM Source";

inline void setModuleFlag(llvm::Module *M, llvm::StringRef Key, llvm::StringRef Source) {
	llvm::LLVMContext &C = M->getContext();
	M->addModuleFlag(llvm::Module::AppendUnique, Key, llvm::MDNode::get(C,
llvm::MDString::get(C, Source)));
}

inline void setWLLVMSource(llvm::Module *M, llvm::StringRef Source) {
  setModuleFlag(M, MF_WLLVM_SOURCE, Source);
}

inline void setALLVMSource(llvm::Module *M, llvm::StringRef Source) {
  setModuleFlag(M, MF_ALLVM_SOURCE, Source);
}

} // end namespace allvm

#endif // ALLVM_MODULEFLAGS_H

