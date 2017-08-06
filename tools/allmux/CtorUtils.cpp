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

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace allvm;
using namespace llvm;

/// Given a llvm.global_ctors list that we can understand,
/// return a list of the functions and null terminator as a vector.
std::vector<Function *> parseGlobalCtors(GlobalVariable *GV) {
  if (GV->getInitializer()->isNullValue())
    return std::vector<Function *>();
  ConstantArray *CA = cast<ConstantArray>(GV->getInitializer());
  std::vector<Function *> Result;
  Result.reserve(CA->getNumOperands());
  for (auto &V : CA->operands()) {
    ConstantStruct *CS = cast<ConstantStruct>(V);
    Result.push_back(dyn_cast<Function>(CS->getOperand(1)));
  }
  return Result;
}

/// Find the llvm.global_ctors list, verifying that all initializers have an
/// init priority of 65535.
GlobalVariable *findGlobalCtors(Module &M) {
  GlobalVariable *GV = M.getGlobalVariable("llvm.global_ctors");
  if (!GV)
    return nullptr;

  // Verify that the initializer is simple enough for us to handle. We are
  // only allowed to optimize the initializer if it is unique.
  if (!GV->hasUniqueInitializer())
    return nullptr;

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
    if (!isa<Function>(CS->getOperand(1)))
      return nullptr;

    // Init priority must be standard.
    ConstantInt *CI = cast<ConstantInt>(CS->getOperand(0));
    if (CI->getZExtValue() != 65535)
      return nullptr;
  }

  return GV;
}
