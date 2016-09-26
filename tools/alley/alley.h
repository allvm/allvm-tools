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
int execWithJITCompilation(allvm::Allexe &, llvm::StringRef Filename,
                           llvm::ArrayRef<std::string> Args, const char **);

// TODO: These belong elsewhere!
const llvm::StringRef ALLEXE_MAIN = "main.bc";
llvm::StringRef getLibNone();
}

#endif // ALLEY_H
