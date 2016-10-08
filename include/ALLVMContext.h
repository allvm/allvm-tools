#ifndef ALLVM_CONTEXT_H
#define ALLVM_CONTEXT_H

#include <llvm/ADT/StringRef.h>

#include <string>

namespace allvm {
struct ALLVMContext {
  std::string PrefixDir;
  std::string LibNonePath;
  std::string AlleyPath;
  // To use this, include ALLVMContextAnchor in your tool.
  static ALLVMContext getAnchored(const char *Argv0);
  // Otherwise, use these and help us find where we're installed:
  static ALLVMContext get(const char *Argv0, void *Main);
  static ALLVMContext get(llvm::StringRef Prefix);

private:
  static char Anchor;
};
} // end namespace allvm

#endif // ALLVM_CONTEXT_H
