//===-- wllvm-dump.cpp ----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Dump information about a file built with WLLVM.
//
//===----------------------------------------------------------------------===//

#include "allvm/ToolCommon.h"
#include "allvm/WLLVMFile.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
ALLVMTool AT("wllvm-dump", "Dump information about a file built with WLLVM");
cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input file built with wllvm>"),
                                   AT.getCat());
} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  AT.parseCLOpts(argc, argv);

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
