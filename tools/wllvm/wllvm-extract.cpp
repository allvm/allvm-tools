//===-- wllvm-extract.cpp -------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Extract bitcode from an WLLVM file.
//
//===----------------------------------------------------------------------===//

#include "Error.h"
#include "WLLVMFile.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
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

  LLVMContext C;

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);

  // Create an empty module to link these into
  auto Composite = make_unique<Module>("linked", C);
  SMDiagnostic Err;
  Linker L(*Composite);
  for (auto &BCFilename : WLLVMFile->getBCFilenames()) {
    auto M = parseIRFile(BCFilename, Err, C);
    if (!M) {
      Err.print(argv[0], errs());
      return 1;
    }

    if (L.linkInModule(std::move(M)))
      reportError(BCFilename, "error linking module");
  }

  // TODO: Do something more useful than dump :)
  Composite->dump();

  return 0;
}
