#include "alley.h"

#include "ImageExecutor.h"
#include "JITCache.h" // For naming, TODO: better design

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

/****************************************************************
 * Name:        execWithJITCompilation
 *
 * Input:       A source file in allexe format & program’s environment.
 *
 * Output:      Create an JIT Executaion Engine which takes the mainmodule
 *              and library modules (embedded in the input file) and executes
 *it.
 ****************************************************************/
int allvm::execWithJITCompilation(allvm::Allexe &allexe,
                                  llvm::StringRef Filename,
                                  llvm::ArrayRef<std::string> Args,
                                  const char **envp) {

  auto mainFile = allexe.getModuleName(0);

  if (mainFile != ALLEXE_MAIN) {
    errs() << "Could not open " << Filename << ": ";
    errs() << "First entry was '" << mainFile << "',";
    errs() << " expected '" << ALLEXE_MAIN << "'\n";
    return 1;
  }

  LLVMContext context;
  auto LoadModule = [&](size_t idx) {
    uint32_t crc;
    auto M = allexe.getModule(idx, context, &crc);
    auto name = allexe.getModuleName(idx);
    if (!M) {
      errs() << "Could not read " << Filename << ": " << name << "\n";
      // XXX: Do something with M's error
      exit(1);
    }
    M.get()->setModuleIdentifier(JITCache::generateName(name, crc));
    return std::move(M.get());
  };

  // get main module
  auto executor = make_unique<ImageExecutor>(LoadModule(0));

  // Add supporting libraries
  for (size_t i = 1, e = allexe.getNumModules(); i != e; ++i) {
    executor->addModule(LoadModule(i));
  }

  return executor->runHostedBinary(Args, envp, getLibNone());
}
