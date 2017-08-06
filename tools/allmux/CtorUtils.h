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

namespace llvm {
  class Function;
  class GlobalVariable;
  class Module;
} // end namespace llvm

namespace allvm {

llvm::GlobalVariable *findGlobalCtors(llvm::Module &M);
std::vector<llvm::Function *> parseGlobalCtors(llvm::GlobalVariable *GV);

} // end namespace allvm

#endif // ALLVM_CTORUTILS
