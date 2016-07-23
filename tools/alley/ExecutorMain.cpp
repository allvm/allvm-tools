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

static cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"));

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input allvm file>"));

static cl::list<std::string> InputArgv(cl::ConsumeAfter,
                                       cl::desc("<program arguments>..."));

const StringRef ALLEXE_MAIN = "main.bc";

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

  auto bcFiles = (*exezip)->listFiles();
  if (bcFiles.empty()) {
    errs() << "allexe contained no files!\n";
    return 1;
  }

  auto mainFile = bcFiles.front();
  auto supportFiles = bcFiles.drop_front();
  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  auto bitcode = (*exezip)->getEntry(ALLEXE_MAIN);
  if (!bitcode) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "no main.bc present\n";
    return 1;
  }

  SMDiagnostic err;
  auto M = parseIR(bitcode->getMemBufferRef(), err, context);
  if (!M.get()) {
    err.print(argv[0], errs());
    return 1;
  }

  auto executor = make_unique<ImageExecutor>(std::move(M));

  // Add supporting libraries
  for (auto &lib : supportFiles) {
    auto lib_bitcode = (*exezip)->getEntry(lib);
    if (!lib_bitcode) {
      errs() << "Could not open " << InputFilename << ": ";
      errs() << "Failed to load '" << lib << "'\n";
      return 1;
    }
    auto LibM = parseIR(lib_bitcode->getMemBufferRef(), err, context);
    if (!LibM.get()) {
      err.print(argv[0], errs());
      return 1;
    }
    executor->addModule(std::move(LibM));
  }

  // Add name of file as argv[0]
  InputArgv.insert(InputArgv.begin(), InputFilename);

  if (LibNone.empty())
    return executor->runBinary(InputArgv, envp);

  return executor->runHostedBinary(InputArgv, envp, LibNone);
}
