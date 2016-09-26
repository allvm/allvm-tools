#include "alley.h"

#include "ImageExecutor.h"
#include "JITCache.h" // For naming, TODO: better design

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

/****************************************************************
 * Name:        execWithJITCompilation
 *
 * Input:       A source file in allexe format & the program's
 *              arguments and environment.
 *
 * Output:      Create an JIT Execution Engine which takes the mainmodule
 *              and library modules (embedded in the input file) and executes
 *it.
 ****************************************************************/
Error allvm::execWithJITCompilation(allvm::Allexe &allexe,
                                    llvm::ArrayRef<std::string> Args,
                                    const char **envp) {
  LLVMContext context;
  auto LoadModule = [&](size_t idx) -> Expected<std::unique_ptr<Module>> {
    uint32_t crc;
    auto M = allexe.getModule(idx, context, &crc);
    auto name = allexe.getModuleName(idx);
    if (!M)
      return make_error<StringError>("Could not read module '" + name + "'",
                                     errc::invalid_argument);
    M.get()->setModuleIdentifier(JITCache::generateName(name, crc));
    return std::move(M.get());
  };

  // get main module
  auto MainMod = LoadModule(0);
  if (!MainMod)
    return MainMod.takeError();
  auto executor = make_unique<ImageExecutor>(std::move(*MainMod));

  // Add supporting libraries
  for (size_t i = 1, e = allexe.getNumModules(); i != e; ++i) {
    auto M = LoadModule(i);
    if (!M)
      return M.takeError();
    executor->addModule(std::move(*M));
  }

  executor->runHostedBinary(Args, envp, getLibNone());
  // TODO: Clarify return value significance of runHostedBinary()
  return Error::success();
}
