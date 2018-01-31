//===-- wllvm-extract.cpp -------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Extract bitcode from an WLLVM file.
//
//===----------------------------------------------------------------------===//

#include "allvm/Allexe.h"
#include "allvm/DeInlineAsm.h"
#include "allvm/GitVersion.h"
#include "allvm/ModuleFlags.h"
#include "allvm/ResourceAnchor.h"
#include "allvm/WLLVMFile.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/DebugInfo.h>
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
               clEnumValN(OutputKind::Allexe, "allexe", "ALLEXE format")));

cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input file built with wllvm>"));

cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"));

cl::opt<bool> StripDebug("strip-debug",
                         cl::desc("Strip debugging information from bitcode"),
                         cl::init(false));

cl::opt<bool> PreserveAsm("preserve-asm",
                          cl::desc("Don't attempt to replace inline ASM code"),
                          cl::init(false));

cl::opt<bool> ForceOutput("f", cl::desc("Replace output allexe if it exists"),
                          cl::init(false));

void runDeInlineAsm(Module &M) {
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  MPM.addPass<DeInlineAsm>(DeInlineAsm());
  MPM.run(M, MAM);
}

} // end anonymous namespace

static std::string getDefaultSuffix(OutputKind K) {
  switch (K) {
  case OutputKind::SingleBitcode:
    return ".bc";
  case OutputKind::BitcodeArchive:
    return ".bc.a";
  case OutputKind::Allexe:
    return ".allexe";
  }
  llvm_unreachable("invalid output kind");
}

static Error writeAsSingleBC(const WLLVMFile &File, StringRef Filename) {
  // Initialize output file, error early if unavailable
  std::unique_ptr<ToolOutputFile> Out;
  std::error_code EC;
  Out.reset(new ToolOutputFile(Filename, EC, sys::fs::F_None));
  if (EC)
    return make_error<StringError>(
        "error opening output file '" + Filename + "'", EC);

  LLVMContext C;
  auto Composite = File.getLinkedModule(C);
  if (!Composite)
    return Composite.takeError();

  if (StripDebug)
    StripDebugInfo(**Composite);
  if (!PreserveAsm)
    runDeInlineAsm(**Composite);

  setWLLVMSource(Composite->get(), InputFilename);

  WriteBitcodeToFile(Composite->get(), Out->os(),
                     false,   // ShouldPreserveUseListOrder
                     nullptr, // ModuleSummaryIndex (ThinLTO)
                     true     // Generate Hash
  );
  // We made it this far without error, keep the result.
  Out->keep();
  return Error::success();
}

static Error writeAsBitcodeArchive(const WLLVMFile &File, StringRef Filename) {
  if (StripDebug)
    return make_error<StringError>(
        "stripping debug information not supported when emitting archives",
        errc::invalid_argument);
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
  return writeArchive(Filename, Members, true /* writeSymTab */, Archive::K_GNU,
                      true /* deterministic */, false /* thin */);
}

static Error writeAsAllexe(const WLLVMFile &File, StringRef Filename,
                           const ResourcePaths &RP) {
  LLVMContext C;

  auto Output = Allexe::open(Filename, RP, ForceOutput);
  if (!Output)
    return Output.takeError();

  auto Composite = File.getLinkedModule(C);
  if (!Composite)
    return Composite.takeError();

  if (StripDebug)
    StripDebugInfo(**Composite);
  if (!PreserveAsm)
    runDeInlineAsm(**Composite);

  return (*Output)->addModule(std::move(*Composite), ALLEXE_MAIN);
}

static Error writeAs(const WLLVMFile &File, StringRef Filename, OutputKind Kind,
                     const ResourcePaths &RP) {
  switch (Kind) {
  case OutputKind::SingleBitcode:
    return writeAsSingleBC(File, Filename);
  case OutputKind::BitcodeArchive:
    return writeAsBitcodeArchive(File, Filename);
  case OutputKind::Allexe:
    return writeAsAllexe(File, Filename, RP);
  }

  llvm_unreachable("unhandled outputkind");
}

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  auto WLLVMFile = WLLVMFile::open(InputFilename);
  if (!WLLVMFile) {
    errs() << "Error reading file: " << InputFilename << "\n";
    logAllUnhandledErrors(WLLVMFile.takeError(), errs(),
                          StringRef(argv[0]) + ": ");
    return 1;
  }

  // Figure out where we're writing...
  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-")
      OutputFilename = InputFilename + getDefaultSuffix(EmitOutputKind);
  }

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);
  Error E = writeAs(*WLLVMFile.get(), OutputFilename, EmitOutputKind, RP);
  if (E) {
    logAllUnhandledErrors(std::move(E), errs(), StringRef(argv[0]) + ": ");
    return 1;
  }

  return 0;
}
