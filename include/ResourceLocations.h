#ifndef ALLVM_RESOURCE_LOCATIONS_H
#define ALLVM_RESOURCE_LOCATIONS_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

namespace allvm {
namespace resources {

static inline std::string findPrefixDir() {
  static int StaticSymbol;
  // Get path of current executable, may not work on non-linux?
  // XXX: This won't work when trying to find paths from libraries
  //      linked into executables not shipped with allvm-tools.
  auto Executable = llvm::sys::fs::getMainExecutable("allvm", &StaticSymbol);
  assert(!Executable.empty());
  assert(llvm::sys::path::has_parent_path(Executable));
  auto BinDir = llvm::sys::path::parent_path(Executable);
  assert(llvm::sys::path::has_parent_path(BinDir));
  return llvm::sys::path::parent_path(BinDir).str();
}
static std::string PrefixDir = findPrefixDir();

static inline std::string getPath(llvm::StringRef Dir, llvm::StringRef File) {
  llvm::SmallString<32> Path{PrefixDir};
  llvm::sys::path::append(Path, Dir, File);
  assert(llvm::sys::fs::exists(Path));
  return Path.str();
}

static std::string LibNonePath = getPath("lib", "libnone.a");
static std::string AlleyPath = getPath("bin", "alley");

} // end namespace resources
} // end namespace allvm

#endif // ALLVM_RESOURCE_LOCATIONS_H
