#include "ImageExecutor.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/Archive.h>
#include <llvm/Support/raw_os_ostream.h>

#include <elf.h>
#include <sys/types.h>
#include <unistd.h>

static const size_t INIT_STACK_MAX = 1024;


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
  // EE->setProcessAllSections(true); // XXX: is this needed/useful?

  // Get the binary as a OwningBinary<object::Archive>
  auto Pair = BinaryOrErr.get().takeBinary();
  auto AsArchive = std::unique_ptr<object::Archive>(
      cast<object::Archive>(Pair.first.release()));

  // Add archive to the execution engine!
  EE->addArchive({std::move(AsArchive), std::move(Pair.second)});

  // Use stack-allocated pointer here so that 32bits is enough
  // to represent the distance between the address of
  // this variable and the code we're running.
  // Fixes 32bit relocations from libnone.a as built
  // using older binutils/gcc although I'm not sure
  // of the exact details -- I don't think it's a bug.
  // Anyway we really should have direct control
  // over memory layout ourselves instead these things work.
  // (for example: Why is stack address closer?
  //  this might be brittle across platforms)
  void *dummy_ptr = nullptr;
  uint64_t dummy_addr = reinterpret_cast<uint64_t>(&dummy_ptr);

  // FIXME: Resolve these properly instead of hardcoding to our dummy pointer
  EE->addGlobalMapping("__init_array_start", dummy_addr);
  EE->addGlobalMapping("__init_array_end", dummy_addr);
  EE->addGlobalMapping("__fini_array_start", dummy_addr);
  EE->addGlobalMapping("__fini_array_end", dummy_addr);

  // TODO: Look into Orc's LocalCXXRuntimeOverrides for a better solution!
  EE->addGlobalMapping("__dso_handle", dummy_addr);

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
    stack_init.push_back(reinterpret_cast<uint64_t>(arg.data()));
  stack_init.push_back(0); // null-terminated list

  // Next comes the environment
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
  uint64_t *stack = static_cast<uint64_t*>(alloca(stack_size));
  memcpy(stack, stack_init.data(), stack_size);
  stack_init.clear();

  char **argv_ptr = reinterpret_cast<char **>(stack);
  assert(!argv.empty());
  int argc = static_cast<int>(argv.size());

  auto StartAddr = EE->getFunctionAddress("__libc_start_main");
  auto MainAddr = EE->getFunctionAddress("main");
  assert(StartAddr);
  assert(MainAddr);

  EE->finalizeObject();

  typedef int (*mainty)(int, char**, char**);
  typedef int (*startty)(mainty, int, char **);

  // TODO: Run static constructors, but AFTER initializing libc components...
  EE->runStaticConstructorsDestructors(false);

  // Note: __libc_start_main() calls exit() so we don't really return
  startty start = reinterpret_cast<startty>(StartAddr);
  return start(reinterpret_cast<mainty>(MainAddr), argc, argv_ptr);
}

} // end namespace allvm
