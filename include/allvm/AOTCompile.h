#ifndef ALLVM_AOTCOMPILE_H
#define ALLVM_AOTCOMPILE_H

#include "allvm/Allexe.h"
#include "allvm/StaticBinaryCache.h"
#include "allvm/StaticCodeGen.h"

#include <llvm/Support/Error.h>

namespace allvm {

llvm::Error AOTCompileIfNeeded(StaticBinaryCache &Cache, const Allexe &allexe,
                               llvm::StringRef LibNone, llvm::StringRef CrtBits,
                               const ALLVMLinker &Linker,
                               const CompilationOptions &Options);

} // end namespace allvm

#endif // ALLVM_AOTCOMPILE_H
