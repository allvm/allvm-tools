//===-- alld.cpp ----------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Linker wrapper for lld.
//
//===----------------------------------------------------------------------===//
#include "ALLVMContextAnchor.h"
#include "ALLVMVersion.h"

#include <lld/Driver/Driver.h>

using namespace allvm;
using namespace llvm;

int main(int argc, const char **argv) {
  ArrayRef<const char *> Args(argv, argv + argc);
  return lld::elf::link(Args, /* CanExitEarly */ true);
}
