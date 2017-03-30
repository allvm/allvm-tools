#ifndef ALLVM_FILEREMOVERPLUS
#define ALLVM_FILEREMOVERPLUS

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Signals.h>

namespace allvm {

class FileRemoverPlus {
  llvm::SmallString<128> filename;

public:
  FileRemoverPlus(llvm::StringRef Filename) : filename(Filename) {
    llvm::sys::RemoveFileOnSignal(filename);
  }

  ~FileRemoverPlus() {
    llvm::sys::fs::remove(filename);
    llvm::sys::DontRemoveFileOnSignal(filename);
  }
};

} // end namespace allvm

#endif // ALLVM_FILEREMOVERPLUS
