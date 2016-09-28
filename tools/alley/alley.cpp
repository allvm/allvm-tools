#include "ExecutionYengine.h"

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

static cl::list<std::string> InputArgv(cl::ConsumeAfter,
                                       cl::desc("<program arguments>..."));

// TODO: Enable forcing use of the JIT even when we have a static version cached
// static cl::opt<bool> ForceJIT("force-jit", cl::init(false),
//                              cl::desc("Force using the JIT"));

static cl::opt<bool> ForceStatic("force-static", cl::init(false),
                                 cl::desc("Force using static code path"));

static cl::opt<bool> NoExec("noexec",
                            cl::desc("Don't actually execute the program"),
                            cl::init(false), cl::Hidden);

static ExitOnError ExitOnErr;

int main(int argc, const char **argv, const char **envp) {
  // Link in necessary libraries
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  cl::ParseCommandLineOptions(argc, argv, "allvm runtime executor");

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

  const CompilationOptions Options; // TODO: Let use specify these?
  if (ForceStatic) {
    if (allexe.getNumModules() != 1) {
      errs() << "Allexe contains too many modules for static code path!\n";
      errs() << "Hint: Use 'alltogether' first!\n";
      return 1;
    }
    StaticBinaryCache Cache;
    ExitOnErr(AOTCompileIfNeeded(Cache, allexe, LibNone, Linker, Options));
  }

  // Fixup argv[0] to the allexe name without the allexe suffix.
  StringRef ProgName = InputFilename;
  if (sys::path::has_extension(InputFilename))
    ProgName = ProgName.drop_back(sys::path::extension(ProgName).size());
  InputArgv.insert(InputArgv.begin(), ProgName);

  ExecutionYengine EY({allexe, InputArgv, envp, LibNone, NoExec});

  // TODO: Don't encode ["modules == 1" <--> static] logic everywhere
  if (allexe.getNumModules() == 1)
    ExitOnErr(EY.tryStaticExec(Linker, Options));

  // If we made it to here, we're JIT'ing the code
  ExitOnErr(EY.doJITExec());

  return 0;
}
