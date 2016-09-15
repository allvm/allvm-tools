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
#include "WLLVMFile.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Errc.h>
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

static Error writeAsSingleBC(const WLLVMFile &File, StringRef Filename) {
  // Initialize output file, error early if unavailable
  std::unique_ptr<tool_output_file> Out;
  std::error_code EC;
  Out.reset(new tool_output_file(Filename, EC, sys::fs::F_None));
  if (EC)
    return make_error<StringError>(
        "error opening output file '" + Filename + "'", EC);

  LLVMContext C;
  auto Composite = File.getLinkedModule(C, InternalizeHidden);
  if (!Composite)
    return Composite.takeError();

  WriteBitcodeToFile((*Composite).get(), Out->os());

  // We made it this far without error, keep the result.
  Out->keep();
  return Error::success();
}

static Error writeAsBitcodeArchive(const WLLVMFile &File, StringRef Filename) {
  std::vector<NewArchiveMember> Members;
  std::vector<std::unique_ptr<MemoryBuffer>> Buffers;
  size_t unique_id = 0;
  for (auto &BCFilename : File.getBCFilenames()) {
    auto Member =
        NewArchiveMember::getFile(BCFilename, /* deterministic */ true);
    if (!Member)
      return Member.takeError();

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
    return make_error<StringError>(result.first, result.second);

  return Error::success();
}

static Error writeAsAllexe(const WLLVMFile &File, StringRef Filename) {
  LLVMContext C;

  auto Output = errorOrToExpected(Allexe::open(Filename, ForceOutput));
  if (!Output)
    return Output.takeError();

  auto Composite = File.getLinkedModule(C, InternalizeHidden);
  if (!Composite)
    return Composite.takeError();

  if (!(*Output)->addModule(std::move(*Composite),
                            "main.bc" /* FIXME: magic string */))
    // "invalid argument"? :(
    return make_error<StringError>("error adding module to allexe",
                                   errc::invalid_argument);

  // (Writes output in destructor of class Allexe)

  return Error::success();
}

Error writeAs(const WLLVMFile &File, StringRef OutputFilename,
              OutputKind Kind) {
  switch (EmitOutputKind) {
  case OutputKind::SingleBitcode:
    return writeAsSingleBC(File, OutputFilename);
  case OutputKind::BitcodeArchive:
    return writeAsBitcodeArchive(File, OutputFilename);
  case OutputKind::Allexe:
    return writeAsAllexe(File, OutputFilename);
  }

  llvm_unreachable("unhandled outputkind");
}

int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);
  if (!WLLVMFile) {
    errs() << "Error reading file: " << InputFilename << "\n";
    logAllUnhandledErrors(WLLVMFile.takeError(), errs(), argv[0]);
    return 1;
  }

  // Figure out where we're writing...
  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-")
      OutputFilename = InputFilename + getDefaultSuffix(EmitOutputKind);
  }

  Error E = writeAs(*WLLVMFile.get(), OutputFilename, EmitOutputKind);
  if (E) {
    logAllUnhandledErrors(std::move(E), errs(), StringRef(argv[0]) + ": ");
    return 1;
  }

  return 0;
}