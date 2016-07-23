#include "ImageExecutor.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Archive.h>
#include <llvm/Support/raw_os_ostream.h>

#include <elf.h>
#include <sys/types.h>
#include <unistd.h>

static const size_t INIT_STACK_MAX = 1024;

static const void *dummy_ptr = nullptr;

using namespace llvm;
namespace allvm {
int ImageExecutor::runHostedBinary(const std::vector<std::string> &argv,
                                   const char **envp, StringRef LibNone) {

  auto BinaryOrErr = object::createBinary(LibNone);
  if (!BinaryOrErr) {
    errs() << "Could not open " << LibNone << "\n";
    return 1;
  }

  EE->DisableSymbolSearching();
  EE->setProcessAllSections(true); // XXX: is this needed/useful?

  // Get the binary as a OwningBinary<object::Archive>
  auto Pair = BinaryOrErr.get().takeBinary();
  auto AsArchive = std::unique_ptr<object::Archive>(
      cast<object::Archive>(Pair.first.release()));

  // Add archive to the execution engine!
  EE->addArchive({std::move(AsArchive), std::move(Pair.second)});

  // FIXME: Resolve these properly instead of hardcoding to our dummy pointer
  EE->addGlobalMapping("__init_array_start", (uint64_t)&dummy_ptr);
  EE->addGlobalMapping("__init_array_end", (uint64_t)&dummy_ptr);
  EE->addGlobalMapping("__fini_array_start", (uint64_t)&dummy_ptr);
  EE->addGlobalMapping("__fini_array_end", (uint64_t)&dummy_ptr);

  // Setup our stack for running the libc initialization code
  // This needs to actually be stack-allocated as musl code
  // uses that property for some checks.

  // Just construct on heap for convenience, abort if run over
  SmallVector<uint64_t, 64> stack_init;

  // First come the arguments
  // TODO: We are populating these with pointers to our memory,
  // which won't work if trying to JIT remotely or whatnot.
  // See ExecutionEngine's ArgvArray for how to do this
  // using EE's memory interface if that becomes important.
  for (auto &arg : argv)
    stack_init.push_back((uint64_t)arg.data());
  stack_init.push_back(0); // null-terminated list

  // Next comes the environment
  while (*envp)
    stack_init.push_back((uint64_t)*envp++);
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
  addAuxv(AT_PAGESZ, (uint64_t)getpagesize());
  addAuxv(AT_SECURE, 0);
  addAuxv(AT_HWCAP, 0); // I guess?
  addAuxv(AT_RANDOM, 0); // pointer to 16 bytes of random :(
  // Others we might want:
  // * AT_BASE/AT_PHDR/related
  // * AT_EXECFN, AT_ENTRY
  // * AT_PLATFORM?
  // Look into these if have issues with tls or something.
  stack_init.push_back(AT_NULL);

  // Move all this into a stack allocation:
  assert(stack_init.size() < INIT_STACK_MAX && "Too many vars!");
  size_t stack_size = stack_init.size() * sizeof(stack_init[0]);
  uint64_t *stack = (uint64_t*)alloca(stack_size);
  memcpy(stack, stack_init.data(), stack_size);
  stack_init.clear();

  char **argv_ptr = (char **)stack;
  assert(!argv.empty());
  int argc = argv.size();

  auto StartAddr = EE->getFunctionAddress("__libc_start_main");
  auto MainAddr = EE->getFunctionAddress("main");
  assert(StartAddr);
  assert(MainAddr);

  EE->finalizeObject();

  typedef int (*mainty)(int, char**, char**);
  typedef int (*startty)(mainty, int, char **);

  // TODO: Static constructors/destructors?

  // Can't run here, need to initialize libc first.

  // Note: __libc_start_main() calls exit() so we don't really return
  startty start = (startty)StartAddr;
  return start((mainty)MainAddr, argc, argv_ptr);
};

} // end namespace allvm
