#include "allvm/ResourcePaths.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

using namespace llvm;
using namespace allvm;

namespace {

std::string getPrefixDir(const char *Argv0, void *Main) {
  SmallString<128> Executable = StringRef(Argv0);

  // For now, prefer Argv[0] as indication of prefix dir (#53)
  auto EC = sys::fs::make_absolute(Executable);
  if (EC || !sys::fs::exists(Executable))
    Executable = sys::fs::getMainExecutable(Argv0, Main);

  assert(sys::fs::exists(Executable));
  assert(!Executable.empty());
  assert(sys::path::has_parent_path(Executable));
  auto BinDir = sys::path::parent_path(Executable);
  assert(sys::path::has_parent_path(BinDir));
  return sys::path::parent_path(BinDir).str();
}

std::string getPath(StringRef PrefixDir, StringRef Dir, StringRef File) {
  SmallString<128> Path{PrefixDir};
  sys::path::append(Path, Dir, File);
  if (auto EC = sys::fs::make_absolute(Path)) {
    // IF assertions are enabled, this will hint as to why things failed
    assert(0 && "Failed to create absolute path for resource file");
    // Otherwise.. abort?
    abort();
  }
  assert(sys::fs::exists(Path));
  return Path.str();
}

} // end anonymous namespace

ResourcePaths ResourcePaths::get(const char *Argv0, void *Main) {
  return ResourcePaths::get(getPrefixDir(Argv0, Main));
}

ResourcePaths ResourcePaths::get(StringRef PrefixDir) {
  return {PrefixDir, getPath(PrefixDir, "lib", "libnone.a"),
          getPath(PrefixDir, "lib/crt", "."),
          getPath(PrefixDir, "bin", "alley"),
#ifdef ALLVM_alld_available
          getPath(PrefixDir, "bin", "alld")
#else
          ""
#endif
  };
}
