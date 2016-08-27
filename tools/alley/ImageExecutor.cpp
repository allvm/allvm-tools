#include "ImageExecutor.h"

#include "ImageCache.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;

namespace allvm {

ImageExecutor::ImageExecutor(std::unique_ptr<Module> &&module, bool UseCache)
    : M(module.get()) {
  EngineBuilder builder(std::move(module));
  builder.setEngineKind(EngineKind::JIT);
  std::string error;
  builder.setErrorStr(&error);
  EE.reset(builder.create());
  if (!EE.get()) {
    errs() << "Error building execution engine: " << error << '\n';
    exit(1);
  }

  SmallString<20> CacheDir;
  if (!sys::path::user_cache_directory(CacheDir, "allvm", "objects"))
    CacheDir = "allvm-cache";

  if (UseCache) {
    Cache.reset(new ImageCache(CacheDir));
    EE->setObjectCache(Cache.get());
  }
  if (!UseCache || !Cache->hasObjectFor(M))
    M->materializeAll();
}

ImageExecutor::~ImageExecutor() {}

int ImageExecutor::runBinary(const std::vector<std::string> &argv,
                             const char **envp) {
  Function *main = M->getFunction("main");
  EE->finalizeObject();
  EE->runStaticConstructorsDestructors(false);
  int result = EE->runFunctionAsMain(main, argv, envp);
  EE->runStaticConstructorsDestructors(true);
  // XXX: lli runs exit to make sure atexit() stuff gets handled properly.
  return result;
}

void ImageExecutor::addModule(std::unique_ptr<llvm::Module> Mod) {
  if (!Cache || !Cache->hasObjectFor(Mod.get()))
    Mod->materializeAll();
  EE->addModule(std::move(Mod));
}

} // end namespace allvm
