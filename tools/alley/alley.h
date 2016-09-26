#ifndef ALLEY_H
#define ALLEY_H

#include "Allexe.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <string>

namespace allvm {

struct ExecutionInfo {
  allvm::Allexe &allexe;
  llvm::ArrayRef<std::string> Args;
  const char **envp;
  llvm::StringRef LibNone;
};

// Attempt to run the given allexe using statically-compiled code.
// return if error or statically compiled code is not available in the cache.
llvm::Error tryStaticExec(ExecutionInfo &EI, bool DoStaticCodeGenIfNeeded);
llvm::Error execWithJITCompilation(ExecutionInfo &EI);
}

#endif // ALLEY_H
