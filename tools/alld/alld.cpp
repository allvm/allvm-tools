//===-- alld.cpp ----------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Linker wrapper for lld.
//
//===----------------------------------------------------------------------===//
#include <lld/Common/Driver.h>
//#include <llvm/Support/InitLLVM.h>

using namespace llvm;

int main(int Argc, const char **Argv) {
  //InitLLVM X(Argc, Argv);

  ArrayRef<const char *> Args(Argv, Argv + Argc);
  return !lld::elf::link(Args, /* CanExitEarly */ true);
}
