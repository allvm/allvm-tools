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
#include "allvm/ExecutionYengine.h"

#include "allvm/ALLVMLinker.h"
#include "allvm/AOTCompile.h"
#include "allvm/ExitOnError.h"
#include "allvm/GitVersion.h"
#include "allvm/ResourceAnchor.h"

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
cl::OptionCategory AllreadyOptCat("allready options");
cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"), cl::cat(AllreadyOptCat));

cl::opt<std::string> CrtBits("crtbits",
                             cl::desc("Path to the crt* object files"), cl::cat(AllreadyOptCat));

cl::opt<std::string> Linker("linker",
                            cl::desc("Linker to use for static compilation"),
#ifndef ALLVM_alld_available
                            cl::init("ld"),
#endif
                            cl::cat(AllreadyOptCat)
                                );

cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input allvm file>"), cl::cat(AllreadyOptCat));

allvm::ExitOnError ExitOnErr;
} // end anonymous namespace

int main(int argc, const char **argv) {
  // w
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);
  LibNone.setInitialValue(RP.LibNonePath);
  CrtBits.setInitialValue(RP.CrtBitsPath);

  cl::HideUnrelatedOptions(AllreadyOptCat);
  cl::ParseCommandLineOptions(argc, argv, "allready static codegen -> cache");

  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  auto allexe = ExitOnErr(Allexe::openForReading(InputFilename, RP));

  const CompilationOptions Options; // TODO: Let use specify these?
  StaticBinaryCache Cache;
  std::unique_ptr<ALLVMLinker> TheLinker;
  if (Linker.empty())
    TheLinker = llvm::make_unique<InternalLinker>(RP.AlldPath);
  else
    TheLinker = llvm::make_unique<PathLinker>(Linker);
  ExitOnErr(AOTCompileIfNeeded(Cache, *allexe, LibNone, CrtBits, *TheLinker,
                               Options));

  outs() << "Successfully cached static binary for '" << InputFilename
         << "'.\n";

  return 0;
}
