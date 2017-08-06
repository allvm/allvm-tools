//===-- CtorUtils.h -------------------------------------------------------===//
//
// **Largely based on "CtorUtils.cpp" from LLVM 4.0**
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines functions that are used to process llvm.global_ctors.
//
//===----------------------------------------------------------------------===//

#ifndef ALLVM_CTORUTILS
#define ALLVM_CTORUTILS

#include <vector>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Error.h>

namespace llvm {
class Function;
class GlobalVariable;
class Module;
} // end namespace llvm

namespace allvm {

llvm::Expected<llvm::GlobalVariable *> findGlobalCtors(llvm::Module &M);
llvm::Expected<llvm::GlobalVariable *> findGlobalDtors(llvm::Module &M);
std::vector<llvm::Function *> parseGlobalCtorDtors(llvm::GlobalVariable *GV);

llvm::Function *createCtorDtorFunc(llvm::ArrayRef<llvm::Function *> Fns, llvm::Module &M, const llvm::Twine & Name);

} // end namespace allvm

#endif // ALLVM_CTORUTILS
