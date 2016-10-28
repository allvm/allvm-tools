#include "ALLVMContext.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

using namespace llvm;
using namespace allvm;

namespace {

std::string getPrefixDir(const char *Argv, void *Main) {
  auto Executable = sys::fs::getMainExecutable(Argv, Main);
  assert(!Executable.empty());
  assert(sys::path::has_parent_path(Executable));
  auto BinDir = sys::path::parent_path(Executable);
  assert(sys::path::has_parent_path(BinDir));
  return sys::path::parent_path(BinDir).str();
}

std::string getPath(StringRef PrefixDir, StringRef Dir, StringRef File) {
  SmallString<32> Path{PrefixDir};
  sys::path::append(Path, Dir, File);
  auto EC = sys::fs::make_absolute(Path);
  assert(!EC && "Failed to create absolute path for resource file");
  assert(sys::fs::exists(Path));
  return Path.str();
}

} // end anonymous namespace

ALLVMContext ALLVMContext::get(const char *Argv, void *Main) {
  return ALLVMContext::get(getPrefixDir(Argv, Main));
}

ALLVMContext ALLVMContext::get(StringRef PrefixDir) {
  return {PrefixDir, getPath(PrefixDir, "lib", "libnone.a"),
          getPath(PrefixDir, "lib/crt", "."),
          getPath(PrefixDir, "bin", "alley")};
}
