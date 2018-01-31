//===-- alld.cpp ----------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Linker wrapper for lld.
//
//===----------------------------------------------------------------------===//
#include "allvm/GitVersion.h"

#include <lld/Common/Driver.h>

#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>

using namespace allvm;
using namespace llvm;

int main(int Argc, const char **Argv) {
  sys::PrintStackTraceOnErrorSignal(Argv[0]);
  PrettyStackTraceProgram StackPrinter(Argc, Argv);
  llvm_shutdown_obj Shutdown;

  ArrayRef<const char *> Args(Argv, Argv + Argc);
  return !lld::elf::link(Args, /* CanExitEarly */ true);
}
