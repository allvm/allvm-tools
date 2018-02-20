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

#include "allvm/ToolCommon.h"
#include "allvm/ResourceAnchor.h"

#include "allvm/Allexe.h"
#include "allvm/ExitOnError.h"
#include "allvm/FileRemoverPlus.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/ToolOutputFile.h>

using namespace allvm;
using namespace llvm;

namespace {
ALLVMTool AT("allopt");
cl::OptionCategory AllOptOptCat("allopt options");
cl::opt<std::string> InputFilename("i", cl::init("-"),
                                   cl::desc("<input allexe>"),
                                   AT.getCat());
cl::opt<std::string> OutputFilename("o", cl::init("-"),
                                    cl::desc("<output allexe>"),
                                    AT.getCat());
cl::opt<bool> ForceOutput("f", cl::desc("Replace output allexe if it exists"),
                          cl::init(false), AT.getCat());
cl::opt<bool>
    AnalyzeOnly("analyze",
                cl::desc("don't expect bitcode as output of pipeline"),
                cl::init(false), AT.getCat());
cl::opt<std::string> Pipeline(cl::Positional, cl::Required,
                              cl::desc("<pipeline command>"),
                              AT.getCat());
cl::list<std::string> Args(cl::ConsumeAfter, cl::desc("<pipeline arguments>"),
    AT.getCat());

allvm::ExitOnError ExitOnErr;

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

  AT.parseCLOpts(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  // Create temporary files for bitcode input and output
  SmallString<32> TempIn, TempOut, TempExe;
  ExitOnErr(errorCodeToError(
      sys::fs::createTemporaryFile("allopt-in", "bc", TempIn)));
  ExitOnErr(errorCodeToError(
      sys::fs::createTemporaryFile("allopt-out", "bc", TempOut)));
  ExitOnErr(errorCodeToError(
      sys::fs::createTemporaryFile("allopt-allexe", "allexe", TempExe)));
  FileRemoverPlus InRemover(TempIn), OutRemover(TempOut), ExeRemover(TempExe);

  if (OutputFilename == "-" && !ForceOutput && !AnalyzeOnly &&
      sys::Process::StandardOutIsDisplayed()) {
    ExitOnErr(make_error<StringError>(
        "refusing to scribble to display stdout, use -f to override",
        errc::invalid_argument));
  }

  // Put the allexe's bitcode into tempin
  std::error_code EC;
  {
    FileRemoverPlus ExeLocalRemover(TempExe);
    StringRef InputPath = InputFilename;
    if (InputFilename == "-") {
      auto InputBuf = ExitOnErr(errorOrToExpected(MemoryBuffer::getSTDIN()));
      raw_fd_ostream ExeS(TempExe, EC, sys::fs::F_None);
      ExitOnErr(errorCodeToError(EC));
      ExeS.write(InputBuf->getBufferStart(), InputBuf->getBufferSize());
      InputPath = TempExe;
    }

    auto exe = ExitOnErr(Allexe::openForReading(InputPath, RP));

    if (exe->getNumModules() != 1)
      ExitOnErr(make_error<StringError>(
          "allopt only supports single-module allexe's for now",
          errc::invalid_argument));

    LLVMContext C;
    auto M = ExitOnErr(exe->getModule(0, C));
    ExitOnErr(M->materializeAll());
    raw_fd_ostream InS(TempIn, EC, sys::fs::F_None);
    ExitOnErr(errorCodeToError(EC));
    WriteBitcodeToFile(M.get(), InS);
  }

  // Run the pipeline
  ExitOnErr(runPipeline(TempIn, TempOut));

  // Write new allexe
  if (!AnalyzeOnly) {
    StringRef OutputPath = OutputFilename;
    if (OutputFilename == "-")
      OutputPath = TempExe;

    // Limited lifetime, write the allexe on dtor
    {
      auto outexe = ExitOnErr(Allexe::open(OutputPath, RP, ForceOutput));
      ExitOnErr(outexe->addModule(TempOut, ALLEXE_MAIN));
    }

    if (OutputFilename == "-") {
      auto OutData =
          ExitOnErr(errorOrToExpected(MemoryBuffer::getFile(OutputPath)));
      outs().write(OutData->getBufferStart(), OutData->getBufferSize());
    }
  } else {
    tool_output_file out(OutputFilename, EC, sys::fs::F_None);
    ExitOnErr(errorCodeToError(EC));

    auto OutData = ExitOnErr(errorOrToExpected(MemoryBuffer::getFile(TempOut)));
    out.os().write(OutData->getBufferStart(), OutData->getBufferSize());
  }

  return 0;
}
