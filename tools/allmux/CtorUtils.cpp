//===-- CtorUtils.cpp -----------------------------------------------------===//
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

#include "CtorUtils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/raw_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
Expected<GlobalVariable *> findGlobalCtorsDtors(Module &M, StringRef Name) {
  GlobalVariable *GV = M.getGlobalVariable(Name);
  if (!GV)
    return nullptr;

  // Verify that the initializer is simple enough for us to handle. We are
  // only allowed to optimize the initializer if it is unique.
  if (!GV->hasUniqueInitializer())
    return make_error<StringError>(
        "static ctor/dtor array does not have unique initializer",
        errc::invalid_argument);

  if (isa<ConstantAggregateZero>(GV->getInitializer()))
    return GV;
  ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());

  for (auto &V : CA->operands()) {
    if (isa<ConstantAggregateZero>(V))
      continue;
    ConstantStruct *CS = cast<ConstantStruct>(V);
    if (isa<ConstantPointerNull>(CS->getOperand(1)))
      continue;

    // Must have a function or null ptr.
    if (!isa<Function>(CS->getOperand(1)->stripPointerCasts()))
      return make_error<StringError>("static ctor/dtor initializer has invalid "
                                     "value where function pointer should be",
                                     errc::invalid_argument);

    // Init priority must be standard.
    ConstantInt *CI = cast<ConstantInt>(CS->getOperand(0));
    if (CI->getZExtValue() != 65535)
      return make_error<StringError>(
          "static ctor/dtor initializer has unsupported priority value",
          errc::invalid_argument);
  }

  return GV;
}
} // end anonymous namespace

Expected<GlobalVariable *> allvm::findGlobalCtors(Module &M) {
  return findGlobalCtorsDtors(M, "llvm.global_ctors");
}
Expected<GlobalVariable *> allvm::findGlobalDtors(Module &M) {
  return findGlobalCtorsDtors(M, "llvm.global_dtors");
}

/// Given a llvm.global_ctors list that we can understand,
/// return a list of the functions and null terminator as a vector.
std::vector<Constant *> allvm::parseGlobalCtorDtors(GlobalVariable *GV) {
  if (GV->getInitializer()->isNullValue())
    return {};
  ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());
  std::vector<Constant *> Result;
  Result.reserve(CA->getNumOperands());
  for (auto &V : CA->operands()) {
    ConstantStruct *CS = cast<ConstantStruct>(V);
    Result.push_back(CS->getOperand(1));
  }
  return Result;
}

Function *allvm::createCtorDtorFunc(ArrayRef<Constant *> Fns, Module &M,
                                    const Twine &Name) {
  auto &C = M.getContext();
  IRBuilder<> Builder(C);

  // TODO: Better handle case where function with this name already exists;
  // just create the function as whatever and then call setName() or something?
  auto NameS = Name.str();
  auto *F = cast<Function>(
      M.getOrInsertFunction(NameS, Builder.getVoidTy()));

  Builder.SetInsertPoint(BasicBlock::Create(C, "entry", F));

  for (auto *Entry : Fns) {
    if (Entry)
      Builder.CreateCall(Entry);
  }

  Builder.CreateRetVoid();

  return F;
}
