#ifndef ALLEY_H
#define ALLEY_H

#include "Allexe.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <string>

namespace allvm {

// Attempt to run the given allexe using statically-compiled code.
// return if error or statically compiled code is not available in the cache.
llvm::Error tryStaticExec(allvm::Allexe &, llvm::ArrayRef<std::string> Args,
                          const char **envp, bool DoStaticCodeGenIfNeeded);
llvm::Error execWithJITCompilation(allvm::Allexe &,
                                   llvm::ArrayRef<std::string> Args,
                                   const char **envp);

// TODO: These belong elsewhere!
llvm::StringRef getLibNone();
}

#endif // ALLEY_H
