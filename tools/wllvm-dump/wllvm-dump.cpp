//===-- wllvm-dump.cpp ----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Dump information about a file built with WLLVM.
//
//===----------------------------------------------------------------------===//

#include "WLLVMFile.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

static cl::opt<std::string>
    InputFilename(cl::Positional, cl::Required,
                  cl::desc("<input file built with wllvm>"));

int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);

  for (auto &BCFilename : WLLVMFile->getBCFilenames()) {
    errs() << "Entry: [" << BCFilename << "]\n";
  }

  return 0;
}
