#ifndef _ALLVM_STATIC_CODE_GEN_
#define _ALLVM_STATIC_CODE_GEN_

/*
#include <llvm/Support/ErrorOr.h>

#include <memory>
#include <string>
#include <vector>
*/

namespace llvm {
class LLVMContext;
class raw_pwrite_stream;
}

namespace allvm {
class Allexe;
}

namespace allvm {

void compileAllexeWithLlcDefaults(Allexe &, llvm::raw_pwrite_stream &, llvm::LLVMContext &);

}

#endif // _ALLVM_STATIC_CODE_GEN_
