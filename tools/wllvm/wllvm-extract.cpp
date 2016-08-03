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

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
  // TODO: Emit "ALLEXE"/"ALLSO" format (zip)?
  enum class OutputKind { SingleBitcode, BitcodeArchive };

  cl::opt<OutputKind> EmitOutputKind(
      "output-kind", cl::desc("Choose output kind"),
      cl::init(OutputKind::SingleBitcode),
      cl::values(clEnumValN(OutputKind::SingleBitcode, "single-bc",
                            "Single bitcode file"),
                 clEnumValN(OutputKind::BitcodeArchive, "archive",
                            "Archive of multiple bitcode files"),
                 clEnumValEnd));

  cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                     cl::desc("<input file built with wllvm>"));

  cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                      cl::value_desc("filename"));

} // end anon namespace

std::string getDefaultSuffix(OutputKind K) {
  switch (K) {
  case OutputKind::SingleBitcode:
    return ".bc";
  case OutputKind::BitcodeArchive:
    return ".bc.a";
  }
}

int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  LLVMContext C;

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);

  // Figure out where we're writing...
  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-")
      OutputFilename = InputFilename + getDefaultSuffix(EmitOutputKind);
  }

  switch (EmitOutputKind) {
  case OutputKind::SingleBitcode: {
    SMDiagnostic Err;

    // Initialize output file, error early if unavailable
    std::unique_ptr<tool_output_file> Out;
    std::error_code EC;
    Out.reset(new tool_output_file(OutputFilename, EC, sys::fs::F_None));
    if (EC) {
      errs() << "Error opening file '" << OutputFilename << "': ";
      errs() << EC.message() << "\n";
      errs().flush();
      return 1;
    }

    // Create an empty module to link these into
    auto Composite = make_unique<Module>("linked", C);
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

    WriteBitcodeToFile(Composite.get(), Out->os());

    // We made it this far without error, keep the result.
    Out->keep();
    break;
  }
  case OutputKind::BitcodeArchive: {
    std::vector<NewArchiveMember> Members;
    for (auto &BCFilename : WLLVMFile->getBCFilenames()) {
      auto Member =
          NewArchiveMember::getFile(BCFilename, /* deterministic */ true);
      if (!Member)
        reportError(BCFilename, Member.takeError());
      Members.push_back(std::move(*Member));
    }
    auto result = writeArchive(OutputFilename, Members, true /* writeSymTab */,
                               Archive::K_GNU, true /* deterministic */,
                               false /* thin */);
    if (result.second)
      reportError(result.first, result.second);
    break;
  }
  }

  return 0;
}
