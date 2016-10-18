#include "ALLVMContextAnchor.h"
#include "ALLVMVersion.h"
#include "Allexe.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace allvm;

namespace {
cl::opt<std::string> InputFilename(cl::Positional,
                                   cl::desc("<input Allexe file>"));
ExitOnError ExitOnErr;
}

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  cl::ParseCommandLineOptions(argc, argv);

  ALLVMContext AC = ALLVMContext::getAnchored(argv[0]);
  auto Input = ExitOnErr(Allexe::openForReading(InputFilename, AC));

  outs() << "Modules:\n";
  for (size_t i = 0; i < Input->getNumModules(); i++) {
    outs() << "\t" << Input->getModuleName(i) << "\n";
  }

  return 0;
}
