//===-- allready.cpp ------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Ensure this allexe has an entry in the static binary cache.
// That is to say, it's "all ready" for execution.
//
//===----------------------------------------------------------------------===//
#include "ExecutionYengine.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

static std::string getDefaultLibNone() {
  static int StaticSymbol;
  auto Executable = sys::fs::getMainExecutable("allvm_tool", &StaticSymbol);
  auto BinDir = sys::path::parent_path(Executable);
  return (BinDir + "/../lib/libnone.a").str();
}

static cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"),
                                    cl::init(getDefaultLibNone()));

static cl::opt<std::string>
    Linker("linker",
           cl::desc("Path of linker-driver to use for static compilation"),
           cl::init("clang"));

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input allvm file>"));

static ExitOnError ExitOnErr;

int main(int argc, const char **argv) {
  // w
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  cl::ParseCommandLineOptions(argc, argv, "allready static codegen -> cache");

  ExitOnErr.setBanner(std::string(argv[0]) + ":");

  auto allexeOrError = Allexe::open(InputFilename);
  if (!allexeOrError) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << allexeOrError.getError().message() << '\n';
    return 1;
  }

  auto &allexe = *allexeOrError.get();
  if (allexe.getNumModules() == 0) {
    errs() << "allexe contained no files!\n";
    return 1;
  }

  auto mainFile = allexe.getModuleName(0);

  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << InputFilename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  std::vector<std::string> DummyArgs;
  ExecutionYengine EY({allexe, DummyArgs, NULL, LibNone, true /* NoExec */});

  const CompilationOptions Options; // TODO: Let use specify these?
  ExitOnErr(EY.tryStaticExec(Linker, Options, true /* ForceStatic */));

  return 0;
}
