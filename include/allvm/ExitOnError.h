#ifndef ALLVM_EXITONERROR
#define ALLVM_EXITONERROR

#include <llvm/Support/Error.h>
#include <llvm/Support/Signals.h>

#include <string>

namespace allvm {

using ExitOnError = llvm::ExitOnError;

} // end namespace allvm

#endif // ALLVM_EXITONERROR
