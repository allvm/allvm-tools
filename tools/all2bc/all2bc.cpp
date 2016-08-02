//===-- all2bc.cpp --------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Build single bitcode file from an allexe.
//
//===----------------------------------------------------------------------===//

#include "ZipArchive.h"

#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LTO/legacy/LTOCodeGenerator.h>
#include <llvm/LTO/legacy/LTOModule.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;


namespace {
// TODO: Refactor everything in this ns to a common location!
std::string getDefaultLibNone() {
  static int StaticSymbol;
  auto Executable = sys::fs::getMainExecutable("allvm_tool", &StaticSymbol);
  auto BinDir = sys::path::parent_path(Executable);
  return (BinDir + "/../lib/libnone.a").str();
}

static cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"),
                                    cl::init(getDefaultLibNone()));
const StringRef ALLEXE_MAIN = "main.bc";
} // end namespace REFACTORME

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input allvm file>"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Override output filename"),
               cl::value_desc("filename"));

int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Initialize the configured targets
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-")
      OutputFilename = InputFilename + ".bc";
  }

  LLVMContext Context;

  auto exezip = ZipArchive::openForReading(InputFilename);
  if (!exezip) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << exezip.getError().message() << '\n';
    return 1;
  }

  auto bcFiles = (*exezip)->listFiles();
  if (bcFiles.empty()) {
    errs() << "allexe contained no files!\n";
    return 1;
  }

  auto mainFile = bcFiles.front();
  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  SMDiagnostic Err;
  LTOCodeGenerator CodeGen(Context);

  auto Options = InitTargetOptionsFromCodeGenFlags();
  CodeGen.setTargetOptions(Options);
  // TODO: Set other LTOCodeGenerator options?

  // Ensure we keep our entry point! :)
  // XXX: Is this really the right way to do this?
  CodeGen.addMustPreserveSymbol("main");

  // Extract modules and add to LTOCodeGen
  uint32_t crc;
  for (auto &bcEntry : bcFiles) {
    auto bitcode = (*exezip)->getEntry(bcEntry, &crc);
    if (!bitcode) {
      errs() << "Could not open " << InputFilename << ": ";
      errs() << "error extracting '" << bcEntry << "'\n";
      return 1;
    }
    auto ErrOrMod =
        LTOModule::createFromBuffer(Context, bitcode->getBufferStart(),
                                    bitcode->getBufferSize(), Options, bcEntry);
    auto &LTOMod = *ErrOrMod;
    if (!CodeGen.addModule(LTOMod.get())) {
      errs() << "Error adding '" << bcEntry << "'\n";
      return 1;
    }
  }

  // Run optimizations
  // (Misc flags we're expected to set, for now just do all the things)
  bool DisableVerify = false, DisableInline = false, DisableGVNLoadPRE = false,
       DisableLTOVectorization = false;
  if (!CodeGen.optimize(DisableVerify, DisableInline, DisableGVNLoadPRE,
                        DisableLTOVectorization)) {
    errs() << "error optimizing code!\n";
    return 1;
  }

  // Save merged bitcode file
  if (!CodeGen.writeMergedModules(OutputFilename.c_str())) {
    errs() << "Error writing merged module\n";
    return 1;
  }

  return 0;
}
