#ifndef ALLVM_CONTEXT_H
#define ALLVM_CONTEXT_H

#include <llvm/ADT/StringRef.h>

#include <string>

namespace allvm {
struct ALLVMContext {
  std::string PrefixDir;
  std::string LibNonePath;
  std::string AlleyPath;
  static ALLVMContext get(const char *Argv0, void *Main);
  static ALLVMContext get(llvm::StringRef Prefix);
};
} // end namespace allvm

#endif // ALLVM_CONTEXT_H
