#ifndef ALLVM_PLATFORM_SPECIFIC_JIT_H
#define ALLVM_PLATFORM_SPECIFIC_JIT_H

#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include "ExecutionYengine.h"
#include "JITCache.h"

namespace allvm {

// This is implemented per-platform
llvm::Error runHosted(llvm::ExecutionEngine &EE,
                      ExecutionYengine::ExecutionInfo &Info, JITCache &Cache);

} // end namespace allvm

#endif // ALLVM_PLATFORM_SPECIFIC_JIT_H
