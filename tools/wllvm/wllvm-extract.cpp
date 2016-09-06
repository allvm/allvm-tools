//===-- wllvm-extract.cpp -------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Extract bitcode from an WLLVM file.
//
//===----------------------------------------------------------------------===//

#include "Allexe.h"
#include "Error.h"
#include "WLLVMFile.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
enum class OutputKind { SingleBitcode, BitcodeArchive, Allexe };

cl::opt<OutputKind> EmitOutputKind(
    "output-kind", cl::desc("Choose output kind"),
    cl::init(OutputKind::SingleBitcode),
    cl::values(clEnumValN(OutputKind::SingleBitcode, "single-bc",
                          "Single bitcode file"),
               clEnumValN(OutputKind::BitcodeArchive, "archive",
                          "Archive of multiple bitcode files"),
               clEnumValN(OutputKind::Allexe, "allexe", "ALLEXE format"),
               clEnumValEnd));

cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input file built with wllvm>"));

cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"));

cl::opt<bool> InternalizeHidden(
    "internalize-hidden",
    cl::desc("Don't internalize hidden variables. Only for single bc/allexe."),
    cl::init(true));

cl::opt<bool> ForceOutput("f", cl::desc("Replace output allexe if it exists"),
                          cl::init(false));

} // end anon namespace

static std::string getDefaultSuffix(OutputKind K) {
  switch (K) {
  case OutputKind::SingleBitcode:
    return ".bc";
  case OutputKind::BitcodeArchive:
    return ".bc.a";
  case OutputKind::Allexe:
    return ".allexe";
  }
}

static int writeAsSingleBC(const WLLVMFile &File, StringRef Filename) {
  // Initialize output file, error early if unavailable
  std::unique_ptr<tool_output_file> Out;
  std::error_code EC;
  Out.reset(new tool_output_file(Filename, EC, sys::fs::F_None));
  if (EC) {
    errs() << "Error opening file '" << Filename << "': ";
    errs() << EC.message() << "\n";
    errs().flush();
    return 1;
  }

  LLVMContext C;
  auto Composite = File.getLinkedModule(C, InternalizeHidden);
  if (!Composite) {
    errs() << "Error linking into single module\n";
    errs().flush();
    return 1;
  }

  WriteBitcodeToFile(Composite.get(), Out->os());

  // We made it this far without error, keep the result.
  Out->keep();
  return 0;
}

static int writeAsBitcodeArchive(const WLLVMFile &File, StringRef Filename) {
  std::vector<NewArchiveMember> Members;
  std::vector<std::unique_ptr<MemoryBuffer>> Buffers;
  size_t unique_id = 0;
  for (auto &BCFilename : File.getBCFilenames()) {
    auto Member =
        NewArchiveMember::getFile(BCFilename, /* deterministic */ true);
    if (!Member)
      reportError(BCFilename, Member.takeError());

    // Make copy of buffer so we can give it a name :(
    Buffers.push_back(std::move(Member->Buf));
    Member->Buf = MemoryBuffer::getMemBufferCopy(
        Buffers.back()->getBuffer(),
        Twine(unique_id++) + "-" + sys::path::filename(BCFilename));
    Members.push_back(std::move(*Member));
  }
  auto result =
      writeArchive(Filename, Members, true /* writeSymTab */, Archive::K_GNU,
                   true /* deterministic */, false /* thin */);
  if (result.second)
    reportError(result.first, result.second);

  return 0;
}

static int writeAsAllexe(const WLLVMFile &File, StringRef Filename) {
  LLVMContext C;

  auto Output = Allexe::open(Filename, ForceOutput);
  if (!Output)
    reportError(Filename, Output.getError());

  auto Composite = File.getLinkedModule(C, InternalizeHidden);
  if (!Composite) {
    errs() << "Error linking into single module\n";
    return 1;
  }

  if (!(*Output)->addModule(std::move(Composite),
                      "main.bc" /* FIXME: magic string */)) {
    errs() << "Error adding module to allexe!\n";
    return 1;
  }

  // (Writes output in destructor of class Allexe)

  return 0;
}


int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);

  // Figure out where we're writing...
  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-")
      OutputFilename = InputFilename + getDefaultSuffix(EmitOutputKind);
  }

  switch (EmitOutputKind) {
  case OutputKind::SingleBitcode:
    return writeAsSingleBC(*WLLVMFile, OutputFilename);
  case OutputKind::BitcodeArchive:
    return writeAsBitcodeArchive(*WLLVMFile, OutputFilename);
  case OutputKind::Allexe:
    return writeAsAllexe(*WLLVMFile, OutputFilename);
  }

  // unreachable
  return 0;
}
