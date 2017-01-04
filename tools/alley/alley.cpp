#include "ExecutionYengine.h"

#include "ALLVMContextAnchor.h"
#include "ALLVMLinker.h"
#include "ALLVMVersion.h"
#include "AOTCompile.h"

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

namespace {
cl::opt<std::string> LibNone("libnone", cl::desc("Path of libnone.a"));

cl::opt<std::string> CrtBits("crtbits",
                             cl::desc("Path to the crt* object files"));

cl::opt<std::string> Linker("linker",
                            cl::desc("Linker to use for static compilation")
#ifndef ALLVM_alld_available
                                ,
                            cl::init("ld")
#endif
                                );

cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                   cl::desc("<input allvm file>"));

cl::list<std::string> InputArgv(cl::ConsumeAfter,
                                cl::desc("<program arguments>..."));

// TODO: Enable forcing use of the JIT even when we have a static version cached
// cl::opt<bool> ForceJIT("force-jit", cl::init(false),
//                              cl::desc("Force using the JIT"));

cl::opt<bool> ForceStatic("force-static", cl::init(false),
                          cl::desc("Force using static code path"));

cl::opt<bool> NoExec("noexec", cl::desc("Don't actually execute the program"),
                     cl::init(false), cl::Hidden);

ExitOnError ExitOnErr;

} // end anonymous namespace

int main(int argc, const char **argv, const char **envp) {
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  ALLVMContext AC = ALLVMContext::getAnchored(argv[0]);
  LibNone.setInitialValue(AC.LibNonePath);
  CrtBits.setInitialValue(AC.CrtBitsPath);

  cl::ParseCommandLineOptions(argc, argv, "allvm runtime executor");
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  auto allexe = ExitOnErr(Allexe::openForReading(InputFilename, AC));

  const CompilationOptions Options; // TODO: Let use specify these?
  if (ForceStatic) {
    if (allexe->getNumModules() != 1) {
      errs() << "Allexe contains too many modules for static code path!\n";
      errs() << "Hint: Use 'alltogether' first!\n";
      return 1;
    }
    StaticBinaryCache Cache;
    std::unique_ptr<ALLVMLinker> TheLinker;
    if (Linker.empty())
      TheLinker = llvm::make_unique<InternalLinker>(AC.AlldPath);
    else
      TheLinker = llvm::make_unique<PathLinker>(Linker);
    ExitOnErr(AOTCompileIfNeeded(Cache, *allexe, LibNone, CrtBits, *TheLinker,
                                 Options));
  }

  // Fixup argv[0] to the allexe name without the allexe suffix.
  StringRef ProgName = InputFilename;
  if (sys::path::has_extension(InputFilename))
    ProgName = ProgName.drop_back(sys::path::extension(ProgName).size());
  InputArgv.insert(InputArgv.begin(), ProgName);

  ExecutionYengine EY({*allexe, InputArgv, envp, LibNone, NoExec});

  // TODO: Don't encode ["modules == 1" <--> static] logic everywhere
  if (allexe->getNumModules() == 1)
    ExitOnErr(EY.tryStaticExec(Linker, Options));

  // If we made it to here, we're JIT'ing the code
  ExitOnErr(EY.doJITExec());

  return 0;
}
