#include "ImageExecutor.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;

namespace allvm {

ImageExecutor::ImageExecutor(std::unique_ptr<Module> &&module)
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
}
