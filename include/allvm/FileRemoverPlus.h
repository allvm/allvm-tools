#ifndef ALLVM_FILEREMOVERPLUS
#define ALLVM_FILEREMOVERPLUS

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Signals.h>

namespace allvm {

using FileRemoverPlus = llvm::FileRemover;

} // end namespace allvm

#endif // ALLVM_FILEREMOVERPLUS
