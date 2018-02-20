#include "allvm/Allexe.h"
#include "allvm/ExitOnError.h"
#include "allvm/ToolCommon.h"
#include "allvm/ResourceAnchor.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace allvm;

namespace {
ALLVMTool AT("all-info");
cl::opt<std::string> InputFilename(cl::Positional,
                                   cl::desc("<input Allexe file>"),
                                   AT.getCat());
allvm::ExitOnError ExitOnErr;

std::string crcToHex(uint32_t crc) {
  // Get fixed-width hex string for the crc
  auto crcHex = utohexstr(crc);
  while (crcHex.size() < 8)
    crcHex = "0" + crcHex;
  return crcHex;
}
} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  AT.parseCLOpts(argc, argv);

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);
  auto Input = ExitOnErr(Allexe::openForReading(InputFilename, RP));

  outs() << "Modules:\n";
  for (size_t i = 0; i < Input->getNumModules(); i++) {
    auto name = Input->getModuleName(i);
    auto hexcrc = crcToHex(Input->getModuleCRC(i));
    outs() << "\t" << name << " (" << hexcrc << ")\n";
  }

  return 0;
}
