#ifndef ALLVM_ALLVMLINKER
#define ALLVM_ALLVMLINKER

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

namespace llvm {
class Error;
}

namespace allvm {

/// This class provides a linker interface to be used by the static code
/// generation. 
class ALLVMLinker {
public:
  /// Links the given object files into an exacutable with the given file name.
  virtual llvm::Error link(
    const llvm::SmallVectorImpl<llvm::StringRef> &ObjectFilenames,
    llvm::StringRef Filename) const =0;
  virtual ~ALLVMLinker();
};

/// Implementation of the linker interface that uses a gcc-like driver that
/// should be available in the user's PATH.
class PathLinker : public ALLVMLinker {
private:
  llvm::StringRef Linker;

public:
  PathLinker(llvm::StringRef LinkerName) : Linker(LinkerName) { }

  llvm::Error link(
    const llvm::SmallVectorImpl<llvm::StringRef> &ObjectFilenames,
    llvm::StringRef Filename) const override;
};

/// Implementation of the linker interface that uses the lld driver that
/// should have been built with LLVM.
class LldLinker : public ALLVMLinker {
public:
  llvm::Error link(
    const llvm::SmallVectorImpl<llvm::StringRef> &ObjectFilenames,
    llvm::StringRef Filename) const override;
};

} // end namespace allvm

#endif // ALLVM_ALLVMLINKER
