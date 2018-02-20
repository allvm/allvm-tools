//===-- wllvm-dump.cpp ----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Dump information about a file built with WLLVM.
//
//===----------------------------------------------------------------------===//

#include "allvm/GitVersion.h"
#include "allvm/WLLVMFile.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
cl::OptionCategory WllvmDumpOptCat("wllvm-dump options");
cl::opt<std::string>
    InputFilename(cl::Positional, cl::Required,
                  cl::desc("<input file built with wllvm>"), cl::cat(WllvmDumpOptCat));
} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::HideUnrelatedOptions(WllvmDumpOptCat);
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  auto Input = WLLVMFile::open(InputFilename);
  if (!Input) {
    logAllUnhandledErrors(Input.takeError(), errs(), StringRef(argv[0]) + ": ");
    return 1;
  }

  for (auto &BCFilename : (*Input)->getBCFilenames()) {
    errs() << "Entry: [" << BCFilename << "]\n";
  }

  return 0;
}
