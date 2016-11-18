//===-- alltogether.cpp ---------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Converts allexes into allexe's containing a single bitcode module.
//
//===----------------------------------------------------------------------===//

#include "ALLVMContextAnchor.h"
#include "ALLVMVersion.h"
#include "Allexe.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LTO/legacy/LTOCodeGenerator.h>
#include <llvm/LTO/legacy/LTOModule.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
cl::opt<bool> Overwrite("f", cl::desc("overwrite existing alltogether'd file"),
                        cl::init(false));
cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input allvm file>"));

cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"));

cl::opt<bool> DisableOpt("disable-opt",
                         cl::desc("Disable optimizations, only link"),
                         cl::init(false));

cl::opt<bool> Quiet("quiet", cl::desc("Don't print informational messages"));
cl::alias QuietA("q", cl::desc("Alias for -quiet"), cl::aliasopt(Quiet));

inline void info(const Twine &Message) {
  if (!Quiet) {
    outs() << Message;
    outs().flush();
  }
}

ExitOnError ExitOnErr;

} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  // Initialize the configured targets
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  ALLVMContext AC = ALLVMContext::getAnchored(argv[0]);
  if (OutputFilename.empty()) {
    if (StringRef(InputFilename) != "-") {
      SmallString<64> Output{StringRef(InputFilename)};
      sys::path::replace_extension(Output, "merged.allexe");
      OutputFilename = Output.str();
    }
  }
  if (OutputFilename.empty()) {
    errs() << "No output filename given!\n";
    return 1;
  }

  info("Combining referenced modules in allexe\n");
  info("  Input: " + InputFilename + "\n");
  info("  Output: " + OutputFilename + "\n");

  LLVMContext Context;

  auto exe = ExitOnErr(Allexe::openForReading(InputFilename, AC));

  SMDiagnostic Err;
  LTOCodeGenerator CodeGen(Context);

  auto Options = InitTargetOptionsFromCodeGenFlags();
  CodeGen.setTargetOptions(Options);
  // TODO: Set other LTOCodeGenerator options?

  // Ensure we keep our entry point! :)
  // XXX: Is this really the right way to do this?
  CodeGen.addMustPreserveSymbol("main");

  // Extract modules and add to LTOCodeGen
  info("Extracting and merging modules...\n");
  for (size_t i = 0, e = exe->getNumModules(); i != e; ++i) {
    auto bcEntry = exe->getModuleName(i);
    info("  " + bcEntry + ":\n");
    info("    Loading...");
    auto bitcode = exe->getModuleBuffer(i);
    if (!bitcode) {
      errs() << "Could not open " << InputFilename << ": ";
      errs() << "error extracting '" << bcEntry << "'\n";
      return 1;
    }
    auto ErrOrMod =
        LTOModule::createFromBuffer(Context, bitcode->getBufferStart(),
                                    bitcode->getBufferSize(), Options, bcEntry);
    info("Adding...");
    auto &LTOMod = *ErrOrMod;
    if (!CodeGen.addModule(LTOMod.get())) {
      errs() << "Error adding '" << bcEntry << "'\n";
      return 1;
    }
    info("Success.\n");
  }

  if (!DisableOpt) {

    info("Running optimizations...\n");

    // Run optimizations
    // (Misc flags we're expected to set, for now just do all the things)
    bool DisableVerify = false, DisableInline = false,
         DisableGVNLoadPRE = false, DisableLTOVectorization = false;
    if (!CodeGen.optimize(DisableVerify, DisableInline, DisableGVNLoadPRE,
                          DisableLTOVectorization)) {
      errs() << "error optimizing code!\n";
      return 1;
    }
  }

  info("Creating merged module...\n");

  {
    SmallString<32> TempBCPath;
    if (auto ec =
            sys::fs::createTemporaryFile("allvm-merged", "bc", TempBCPath)) {
      errs() << "Error creating temporary file\n";
      return 1;
    }
    FileRemover Remover(TempBCPath);

    // Save merged bitcode file to temporary path
    // (Ideally we would just generate this in memory instead!)
    if (!CodeGen.writeMergedModules(TempBCPath.c_str())) {
      errs() << "Error writing merged module\n";
      return 1;
    }

    auto alltogether = ExitOnErr(Allexe::open(OutputFilename, AC, Overwrite));
    ExitOnErr(alltogether->addModule(TempBCPath, ALLEXE_MAIN));
  }
  info("Successfully wrote to '" + OutputFilename + "'!\n");

  return 0;
}
