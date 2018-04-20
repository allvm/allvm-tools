#include "orcJIT.h"

#include <llvm/ExecutionEngine/Orc/OrcABISupport.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Archive.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/raw_ostream.h>


#include <cstdio>
#include <system_error>
#include <elf.h>
#include <sys/types.h>
#include <unistd.h>

using namespace llvm;
using namespace orc;

static const size_t INIT_STACK_MAX = 16384;

template <typename PtrTy>
static PtrTy fromTargetAddress(JITTargetAddress Addr) {
  return reinterpret_cast<PtrTy>(static_cast<uintptr_t>(Addr));
}

Error llvm::orc::runOrcJIT(std::vector<std::unique_ptr<Module>> Ms,
                         //const std::vector<std::string> &Args,
                         allvm::ExecutionYengine::ExecutionInfo &Info) {

  // Grab a target machine and try to build a factory function for the
  // target-specific Orc callback manager.
  EngineBuilder EB;
  auto TM = std::unique_ptr<TargetMachine>(EB.selectTarget());
  Triple T(TM->getTargetTriple());
  auto CompileCallbackMgr = orc::createLocalCompileCallbackManager(T, 0); 

  // If we couldn't build the factory function then there must not be a callback
  // manager for this target. Bail out.
  if (!CompileCallbackMgr) {
    errs() << "No callback manager available for target '"
           << TM->getTargetTriple().str() << "'.\n";
    return Error::success();
  }

  auto IndirectStubsMgrBuilder = orc::createLocalIndirectStubsManagerBuilder(T);

  // If we couldn't build a stubs-manager-builder for this target then bail out.
  if (!IndirectStubsMgrBuilder) {
    errs() << "No indirect stubs manager available for target '"
           << TM->getTargetTriple().str() << "'.\n";
    return Error::success();
  }

  const DataLayout DL(TM->createDataLayout());

  OrcJIT J(std::move(TM), std::move(CompileCallbackMgr), IndirectStubsMgrBuilder);

  auto BinaryOrErr = object::createBinary(Info.LibNone);
  if (!BinaryOrErr)
    return Error::success();

  auto Pair = BinaryOrErr.get().takeBinary();
  auto AsArchive = std::unique_ptr<object::Archive>(
      cast<object::Archive>(Pair.first.release()));

  J.addArchive({std::move(AsArchive), std::move(Pair.second)});

  // Just construct on heap for convenience, abort if run over
  SmallVector<uint64_t, 64> stack_init;

  // First come the arguments
  // TODO: We are populating these with pointers to our memory,
  // which won't work if trying to JIT remotely or whatnot.
  // See ExecutionEngine's ArgvArray for how to do this
  // using EE's memory interface if that becomes important.
  for (auto &arg : Info.Args)
    stack_init.push_back(reinterpret_cast<uint64_t>(arg.data()));
  stack_init.push_back(0); // null-terminated list

  // Next comes the environment
  auto *envp = Info.envp;
  while (*envp)
    stack_init.push_back(reinterpret_cast<uint64_t>(*envp++));
  stack_init.push_back(0); // null-terminated list

  // Finally, the auxv stuff:
  // XXX: getauxval would make this easier, but doesn't exist on presto :(
  auto addAuxv = [&](uint64_t type, uint64_t val) {
    stack_init.push_back(type);
    stack_init.push_back(val);
  };
  addAuxv(AT_UID, getuid());
  addAuxv(AT_EUID, geteuid());
  addAuxv(AT_GID, getgid());
  addAuxv(AT_EGID, getegid());
  addAuxv(AT_PAGESZ, static_cast<uint64_t>(getpagesize()));
  addAuxv(AT_SECURE, 0);
  addAuxv(AT_HWCAP, 0);  // I guess?
  addAuxv(AT_RANDOM, 0); // pointer to 16 bytes of random :(
  // Others we might want:
  // * AT_BASE/AT_PHDR/related
  // * AT_EXECFN, AT_ENTRY
  // * AT_PLATFORM?
  // Look into these if have issues with tls or something.
  stack_init.push_back(AT_NULL);

  // Move all this into a stack allocation:
  if (stack_init.size() >= INIT_STACK_MAX)
    return make_error<StringError>("too many variables; stack size too large",
                                   errc::invalid_argument);

  size_t stack_size = stack_init.size() * sizeof(stack_init[0]);
  uint64_t *stack = static_cast<uint64_t *>(alloca(stack_size));
  memcpy(stack, stack_init.data(), stack_size);
  stack_init.clear();

  char **argv_ptr = reinterpret_cast<char **>(stack);
  assert(!Info.Args.empty());
  int argc = static_cast<int>(Info.Args.size());

 

  //typedef int (*mainty)(int, char **, char **);
  //typedef int (*startty)(mainty, int, char **);

  

  std::vector<const char *> ArgV;
  for (auto &Arg : Info.Args)
    ArgV.push_back(Arg.c_str());

   if (Info.NoExec) {
    errs() << "'noexec' option set, skipping execution...\n";
//    errs() << "hello2\n";
    return Error::success();
  }

   errs() << "hellso\n";


  for (auto &M : Ms) {
    //M->setDataLayout(DL);
    J.addModule(std::move(M));
  }

  auto StartSym = J.findSymbol("__libc_start_main");
  auto MainSym = J.findSymbol("main");

  typedef int (*MainFnPtr)(int, char *[]);
  typedef int (*StartFnPtr)(MainFnPtr, int, char *[]);

  if (!MainSym) {
    errs() << "Could not find main function.\n";
    return Error::success();
  }

  auto Main = fromTargetAddress<MainFnPtr>(MainSym.getAddress());
  auto Start = fromTargetAddress<StartFnPtr>(StartSym.getAddress());
//  return Main(static_cast<int>(ArgV.size()), ArgV.data());

  Start(Main, static_cast<int>(argc), argv_ptr);
  return make_error<StringError>("libc returned instead of exiting directly?!",
                                 errc::invalid_argument);
  //return Start(Main, static_cast<int>(argc), argv_ptr);//(static_cast<int>(ArgV.size()), ArgV.data());

   //startty start = reinterpret_cast<startty>(StartAddr);
   //start(reinterpret_cast<mainty>(MainSym), argc, argv_ptr);
  /* return make_error<StringError>("libc returned instead of exiting directly?!",
                                  errc::invalid_argument);*/
}


