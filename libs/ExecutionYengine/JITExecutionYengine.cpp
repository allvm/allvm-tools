#include "allvm/ExecutionYengine.h"
#include "allvm/JITCache.h"

#include "PlatformSpecificJIT.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;

namespace {

void libcxxabiKludge(Module &M) {
  if (StringRef(M.getModuleIdentifier()).contains("libc++abi")) {
    auto atexit_impl = M.getFunction("__cxa_thread_atexit_impl");
    assert(atexit_impl);
    assert(GlobalValue::isExternalWeakLinkage(atexit_impl->getLinkage()));
    errs() << "[alley] WARNING: Applying workaround for "
              "__cxa_thread_atexit_impl in "
           << M.getModuleIdentifier() << "\n";
    atexit_impl->replaceAllUsesWith(
        ConstantPointerNull::get(atexit_impl->getType()));
    atexit_impl->eraseFromParent();

    auto thread_atexit = M.getFunction("__cxa_thread_atexit");
    assert(thread_atexit);
    thread_atexit->eraseFromParent();

    auto dtors_run =
        M.getFunction("_ZN10__cxxabiv112_GLOBAL__N_19run_dtorsEPv");
    assert(dtors_run);
    dtors_run->eraseFromParent();

    auto dtors_mgr =
        M.getFunction("_ZN10__cxxabiv112_GLOBAL__N_112DtorsManagerD2Ev");
    assert(dtors_mgr);
    dtors_mgr->eraseFromParent();

    auto dtors = M.getNamedGlobal("_ZN10__cxxabiv112_GLOBAL__N_15dtorsE");
    assert(dtors);
    dtors->eraseFromParent();
    auto dtors_alive =
        M.getNamedGlobal("_ZN10__cxxabiv112_GLOBAL__N_111dtors_aliveE");
    assert(dtors_alive);
    dtors_alive->eraseFromParent();
  }
}

} // end anonymous namespace

Error ExecutionYengine::doJITExec() {
  LLVMContext context;
  auto &allexe = Info.allexe;
  auto LoadModule = [&](size_t idx) -> Expected<std::unique_ptr<Module>> {
    uint32_t crc;
    auto M = allexe.getModule(idx, context, &crc);
    auto name = allexe.getModuleName(idx);
    if (!M)
      return M.takeError();
    M.get()->setModuleIdentifier(JITCache::generateName(name, crc));
    return std::move(M.get());
  };

  // get main module
  auto MainMod = LoadModule(0);
  if (!MainMod)
    return MainMod.takeError();

  auto Cache = llvm::make_unique<JITCache>();

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

      libcxxabiKludge(**M);
    }
    EE->addModule(std::move(*M));
  }

  return runHosted(*EE, Info);
}
