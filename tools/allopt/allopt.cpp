//===-- alld.cpp ----------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// "opt" for allexes
// Invokes custom external pipeline on bitcode contents of an allexe
//
//===----------------------------------------------------------------------===//

#include "allvm/GitVersion.h"
#include "allvm/ResourceAnchor.h"

#include "allvm/Allexe.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>

using namespace allvm;
using namespace llvm;

namespace {
cl::opt<std::string> InputFilename("i", cl::Required,
                                   cl::desc("<input allexe>"));
cl::opt<std::string> OutputFilename("o", cl::Required,
                                    cl::desc("<output allexe>"));
cl::opt<std::string> Pipeline(cl::Positional, cl::Required,
                              cl::desc("<pipeline command>"));
cl::list<std::string> Args(cl::ConsumeAfter, cl::desc("<pipeline arguments>"));

ExitOnError ExitOnErr;

Error runPipeline(StringRef Input, StringRef Output) {
  SmallVector<const char *, 4> ArgStrs;

  ArgStrs.push_back(Pipeline.data());
  for (auto &A : Args)
    ArgStrs.push_back(A.data());
  ArgStrs.push_back(nullptr);

  // Empty environment
  const char *env[] = {nullptr};

  const StringRef *redirects[] = {&Input, &Output, nullptr};

  // XXX: Make these cl::opt's?
  unsigned secondsToWait = 0;
  unsigned memoryLimit = 0;

  std::string err;
  auto Prog = ExitOnErr(errorOrToExpected(sys::findProgramByName(Pipeline)));
  auto ret = sys::ExecuteAndWait(Prog, ArgStrs.data(), env, redirects,
                                 secondsToWait, memoryLimit, &err);
  if (ret < 0) {
    return make_error<StringError>(err, errc::invalid_argument);
  }

  return Error::success();
}

} // end anonymous namespace

int main(int argc, const char *argv[]) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);

  cl::ParseCommandLineOptions(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  // Create temporary files for bitcode input and output
  SmallString<32> TempIn, TempOut;
  ExitOnErr(errorCodeToError(
      sys::fs::createTemporaryFile("allopt-in", "bc", TempIn)));
  ExitOnErr(errorCodeToError(
      sys::fs::createTemporaryFile("allopt-out", "bc", TempOut)));
  FileRemover InRemover(TempIn), OutRemover(TempOut);

  // Put the allexe's bitcode into tempin
  {
    auto exe = ExitOnErr(Allexe::openForReading(InputFilename, RP));

    if (exe->getNumModules() != 1)
      ExitOnErr(make_error<StringError>(
          "allopt only supports single-module allexe's for now",
          errc::invalid_argument));

    LLVMContext C;
    auto M = ExitOnErr(exe->getModule(0, C));
    ExitOnErr(M->materializeAll());
    std::error_code EC;
    raw_fd_ostream InS(TempIn, EC, sys::fs::F_None);
    ExitOnErr(errorCodeToError(EC));
    WriteBitcodeToFile(M.get(), InS);
  }

  // Run the pipeline
  ExitOnErr(runPipeline(TempIn, TempOut));

  // Write new allexe
  {
    auto outexe = ExitOnErr(Allexe::open(OutputFilename, RP));
    ExitOnErr(outexe->addModule(TempOut, ALLEXE_MAIN));
  }

  return 0;
}
