#include "ExecutionYengine.h"
#include "PlatformSpecificJIT.h"

#include "JITCache.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

Error ExecutionYengine::doJITExec() {
  LLVMContext context;
  auto &allexe = Info.allexe;
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

  auto Cache = make_unique<JITCache>();

  if (!Cache->hasObjectFor((*MainMod).get())) {
    if (auto E = (*MainMod)->materializeAll())
      return E;
  }

  // Create the EE using this module:
  EngineBuilder builder(std::move(*MainMod));
  builder.setEngineKind(EngineKind::JIT);
  std::string error;
  builder.setErrorStr(&error);
  std::unique_ptr<ExecutionEngine> EE(builder.create());
  if (!EE.get())
    return make_error<StringError>("Error building execution engine: " + error,
                                   errc::invalid_argument);
  EE->setObjectCache(Cache.get());

  // Add supporting libraries
  for (size_t i = 1, e = allexe.getNumModules(); i != e; ++i) {
    auto M = LoadModule(i);
    if (!M)
      return M.takeError();
    if (!Cache->hasObjectFor((*M).get())) {
      if (auto E = (*M)->materializeAll())
        return E;
    }
    EE->addModule(std::move(*M));
  }

  return runHosted(*EE, Info);
}
