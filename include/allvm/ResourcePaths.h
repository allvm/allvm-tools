#ifndef ALLVM_RESOURCEPATHS_H
#define ALLVM_RESOURCEPATHS_H

#include <llvm/ADT/StringRef.h>

#include <string>

namespace allvm {
struct ResourcePaths {
  std::string PrefixDir;
  std::string LibNonePath;
  std::string CrtBitsPath;
  std::string AlleyPath;
  std::string AlldPath;
  // To use this, include ResourceAnchor in your tool.
  static ResourcePaths getAnchored(const char *Argv0);
  // Otherwise, use these and help us find where we're installed:
  static ResourcePaths get(const char *Argv0, void *Main);
  static ResourcePaths get(llvm::StringRef Prefix);

private:
  static char Anchor;
};
} // end namespace allvm

#endif // ALLVM_RESOURCEPATHS_H
