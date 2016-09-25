#ifndef ALLEY_H
#define ALLEY_H

#include "Allexe.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ObjectFile.h>

#include <string>

namespace allvm {

int execWithStaticCompilation(allvm::Allexe &, llvm::StringRef Filename,
                              llvm::ArrayRef<std::string> Args, const char **);
int execWithJITCompilation(allvm::Allexe &, llvm::StringRef Filename,
                           llvm::ArrayRef<std::string> Args, const char **);

// TODO: These belong elsewhere!
const llvm::StringRef ALLEXE_MAIN = "main.bc";
llvm::StringRef getLibNone();
}

#define DEBUG_TYPE "alley"
#endif // ALLEY_H
