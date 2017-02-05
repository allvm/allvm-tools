#ifndef ALLVM_AOT_COMPILE_H
#define ALLVM_AOT_COMPILE_H

#include "allvm/Allexe.h"
#include "allvm/StaticBinaryCache.h"
#include "allvm/StaticCodeGen.h"

#include <llvm/Support/Error.h>

namespace allvm {

llvm::Error AOTCompileIfNeeded(StaticBinaryCache &Cache, Allexe &allexe,
                               llvm::StringRef LibNone, llvm::StringRef CrtBits,
                               const ALLVMLinker &Linker,
                               const CompilationOptions &Options);

} // end namespace allvm

#endif // ALLVM_AOT_COMPILE_H
