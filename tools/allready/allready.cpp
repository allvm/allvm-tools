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

#include "ALLVMContextAnchor.h"
#include "ALLVMVersion.h"
#include "AOTCompile.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {
cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"));

cl::opt<std::string>
    Linker("linker",
           cl::desc("Path of linker-driver to use for static compilation"),
           cl::init("clang"));

cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input allvm file>"));

ExitOnError ExitOnErr;
} // end anonymous namespace

int main(int argc, const char **argv) {
  // w
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  ALLVMContext AC = ALLVMContext::getAnchored(argv[0]);
  LibNone.setInitialValue(AC.LibNonePath);

  cl::ParseCommandLineOptions(argc, argv, "allready static codegen -> cache");

  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  auto allexe = ExitOnErr(Allexe::openForReading(InputFilename, AC));

  const CompilationOptions Options; // TODO: Let use specify these?
  StaticBinaryCache Cache;
  ExitOnErr(AOTCompileIfNeeded(Cache, *allexe, LibNone, Linker, Options));

  outs() << "Successfully cached static binary for '" << InputFilename << ".\n";

  return 0;
}
