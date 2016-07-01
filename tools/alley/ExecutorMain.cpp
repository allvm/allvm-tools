#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

#include "ImageExecutor.h"
#include "ZipArchive.h"

using namespace allvm;
using namespace llvm;

static cl::opt<std::string> InputFilename(
  cl::Positional, cl::Required, cl::desc("<input allvm file>"));

static cl::list<std::string> InputArgv(cl::ConsumeAfter,
  cl::desc("<program arguments>..."));

int main(int argc, const char **argv, const char **envp) {
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  cl::ParseCommandLineOptions(argc, argv, "allvm runtime executor");

  LLVMContext context; 

  auto exezip = ZipArchive::openForReading(InputFilename);
  if (!exezip) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << exezip.getError().message() << '\n';
    return 1;
  }

  auto bitcode = (*exezip)->getEntry("main.bc");
  if (!bitcode) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "no main.bc present\n";
    return 1;
  }

  SMDiagnostic err;
  std::unique_ptr<Module> M = parseIR(bitcode->getMemBufferRef(), err, context);
  if (!M.get()) {
    err.print(argv[0], errs());
    return 1;
  }

  std::unique_ptr<ImageExecutor> executor(new ImageExecutor(std::move(M)));
  InputArgv.insert(InputArgv.begin(), InputFilename);
  return executor->runBinary(InputArgv, envp);
}
