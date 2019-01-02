#ifndef ALLVM_ALLVMLINKER
#define ALLVM_ALLVMLINKER

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

namespace llvm {
class Error;
} // end namespace llvm

namespace allvm {

/// This class provides a linker interface to be used by the static code
/// generation.
class ALLVMLinker {
public:
  /// Links the given object files into an executable with the given file name.
  virtual llvm::Error link(llvm::ArrayRef<llvm::StringRef> ObjectFilenames,
                           llvm::StringRef CrtBits,
                           llvm::StringRef OutFilename) const = 0;
  virtual ~ALLVMLinker();

protected:
  /// Creates the command line arguments given to the linker and pushes them
  /// back to the given LinkerArgs vector.
  void
  createLinkerArguments(llvm::ArrayRef<llvm::StringRef> ObjectFilenames,
                        llvm::Optional<llvm::StringRef> CrtBits,
                        llvm::StringRef OutFilename,
                        llvm::SmallVectorImpl<std::string> &LinkerArgs) const;

  /// Calls the given linker program with the given arguments as an external
  /// process and waits that process.
  ///
  /// \returns Error::success() on success.
  llvm::Error
  callLinkerAsExternalProcess(llvm::StringRef LinkerProgram,
                              llvm::ArrayRef<llvm::StringRef> LinkerArgv) const;
};

/// Implementation of the linker interface that uses an ld-like linker that
/// should be available in the user's PATH.
class PathLinker : public ALLVMLinker {
private:
  llvm::StringRef Linker;
  bool GccLikeDriver;

public:
  PathLinker(llvm::StringRef LinkerName);

  llvm::Error link(const llvm::ArrayRef<llvm::StringRef> ObjectFilenames,
                   llvm::StringRef CrtBits,
                   llvm::StringRef OutFilename) const override;
};

/// Implementation of the linker interface that uses the alld linker that is
/// built with the other allvm tools.
class InternalLinker : public ALLVMLinker {
private:
  llvm::StringRef Alld;

public:
  InternalLinker(llvm::StringRef AlldPath);

  llvm::Error link(const llvm::ArrayRef<llvm::StringRef> ObjectFilenames,
                   llvm::StringRef CrtBits,
                   llvm::StringRef OutFilename) const override;
};

} // end namespace allvm

#endif // ALLVM_ALLVMLINKER
