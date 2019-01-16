//===-- alltogether.cpp ---------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Converts allexes into allexe's containing a single bitcode module.
//
//===----------------------------------------------------------------------===//

#include "allvm/Allexe.h"
#include "allvm/ExitOnError.h"
#include "allvm/FileRemoverPlus.h"
#include "allvm/ResourceAnchor.h"
#include "allvm/ToolCommon.h"

// XXX: Revisit this!
#include <llvm/CodeGen/CommandFlags.inc>

#include <llvm/ADT/SmallString.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LTO/legacy/LTOCodeGenerator.h>
#include <llvm/LTO/legacy/LTOModule.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
ALLVMTool AT("alltogether");
cl::opt<bool> Overwrite("f", cl::desc("overwrite existing alltogether'd file"),
                        cl::init(false), AT.getCat());
cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input allvm file>"), AT.getCat());

cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"), AT.getCat());

cl::opt<bool> DisableOpt("disable-opt",
                         cl::desc("Disable optimizations, only link"),
                         cl::init(false), AT.getCat());

cl::opt<bool> Quiet("quiet", cl::desc("Don't print informational messages"));
cl::alias QuietA("q", cl::desc("Alias for -quiet"), cl::aliasopt(Quiet),
                 AT.getCat());

cl::opt<bool>
    NoInternalizeHidden("no-internalize-hidden",
                        cl::desc("Don't internalize hidden variables."),
                        cl::init(false), AT.getCat());

inline void info(const Twine &Message) {
  if (!Quiet) {
    outs() << Message;
    outs().flush();
  }
}

allvm::ExitOnError ExitOnErr;

} // end anonymous namespace

static void processGlobal(GlobalValue &GV) {
  if (!NoInternalizeHidden && GV.hasHiddenVisibility() && !GV.isDeclaration())
    GV.setLinkage(GlobalValue::InternalLinkage);
}

int main(int argc, const char **argv) {
  InitLLVM X(argc, argv);

  AT.parseCLOpts(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  // Initialize the configured targets
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);
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

  auto exe = ExitOnErr(Allexe::openForReading(InputFilename, RP));

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

    for (auto &F : LTOMod->getModule())
      if (F.getName() != "main")
        processGlobal(F);
    for (auto &GV : LTOMod->getModule().globals())
      processGlobal(GV);
    for (auto &GA : LTOMod->getModule().aliases())
      processGlobal(GA);

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
    FileRemoverPlus Remover(TempBCPath);

    // Save merged bitcode file to temporary path
    // (Ideally we would just generate this in memory instead!)
    if (!CodeGen.writeMergedModules(TempBCPath.c_str())) {
      errs() << "Error writing merged module\n";
      return 1;
    }

    auto alltogether = ExitOnErr(Allexe::open(OutputFilename, RP, Overwrite));
    ExitOnErr(alltogether->addModule(TempBCPath, ALLEXE_MAIN));
  }
  info("Successfully wrote to '" + OutputFilename + "'!\n");

  return 0;
}
