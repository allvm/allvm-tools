//===-- bc2allvm.cpp ------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Create an allexe file from a single bc file.
//
//===----------------------------------------------------------------------===//

#include "allvm/Allexe.h"
#include "allvm/ExitOnError.h"
#include "allvm/GitVersion.h"
#include "allvm/ModuleFlags.h"
#include "allvm/ResourceAnchor.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
cl::opt<std::string> MainFile(cl::Positional, cl::Required,
                              cl::desc("<main LLVM bitcode file (or ll)>"));
cl::list<std::string>
    InputFiles(cl::Positional, cl::ZeroOrMore,
               cl::desc("<input LLVM bitcode file (or ll)>..."));

cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"));
cl::opt<bool> ForceOutput("f", cl::desc("Replace output allexe if it exists"),
                          cl::init(false));

allvm::ExitOnError ExitOnErr;

} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);

  cl::ParseCommandLineOptions(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  // Figure out where we're writing the output
  if (OutputFilename.empty()) {
    StringRef Input = MainFile;
    if (Input != "-") {
      SmallString<64> Output{Input};
      sys::path::replace_extension(Output, "allexe");
      OutputFilename = Output.str();
    }
  }
  if (OutputFilename.empty()) {
    errs() << "No output filename given!\n";
    return 1;
  }
  if (sys::fs::exists(OutputFilename)) {
    // Reject directory path
    if (sys::fs::is_directory(OutputFilename)) {
      errs() << "Output path is a directory!\n";
      return 1;
    }
    // Reject file path if `-f` is not specified
    if (!ForceOutput) {
      errs() << "Output file exists. Use -f flag to force overwrite.\n";
      return 1;
    }
  }

  {
    // Try to open the output file first
    auto Output = ExitOnErr(Allexe::open(OutputFilename, RP, ForceOutput));

    // Use parseIRFile to handle both bitcode and textual IR
    // Create helper lambda for re-use below.
    auto addModule = [&](StringRef Filename, bool isMain = false) {
      LLVMContext C;
      SMDiagnostic Err;
      SmallString<128> AbsFilename = Filename;

      ExitOnErr(errorCodeToError(llvm::sys::fs::make_absolute(AbsFilename)));

      auto Module = parseIRFile(Filename, Err, C);
      if (!Module) {
        Err.print(argv[0], errs());
        exit(1);
      }
      if (isMain) {
        // Check that "main.bc" actually has, you know, 'main'!
        auto main = Module->getFunction("main");
        if (!main) {
          errs() << "Failed to find 'main' function in " << Filename << "!\n";
          exit(1);
        }
        if (main->isDeclaration()) {
          errs() << "'main' must not be a declaration in first module\n";
          exit(1);
        }
      }
      auto Sources = getALLVMSources(Module.get());
      if (Sources.empty())
        setALLVMSource(Module.get(), Filename);
      else
        errs() << "Warning: Module flag '" << MF_ALLVM_SOURCE
               << "' already set, not changing\n";
      auto Name = isMain ? ALLEXE_MAIN : "";
      ExitOnErr(Output->addModule(std::move(Module), Name));
    };

    addModule(MainFile, true);
    for (const auto &it : InputFiles)
      addModule(it);

    // TODO: Add (on by default?) feature for checking that
    // the resulting allexe is sane/reasonable/not-obviously-invalid.
    // Allexe::sanityCheck() ?
    // (Allexe destructor writes the file)
  }

  return 0;
}
