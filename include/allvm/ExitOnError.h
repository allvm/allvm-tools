#ifndef ALLVM_EXITONERROR
#define ALLVM_EXITONERROR

#include <llvm/Support/Error.h>
#include <llvm/Support/Signals.h>

#include <string>

namespace allvm {

// Copy of LLVM's 'ExitOnError' only it runs interrupt handlers
// on exit, like report_fatal_error().
// This ensures temporary files are removed.

/// Helper for check-and-exit error handling.
///
/// For tool use only. NOT FOR USE IN LIBRARY CODE.
///
class ExitOnError {
public:
  /// Create an error on exit helper.
  ExitOnError(std::string B = "", int DefaultErrorExitCode = 1)
      : Banner(std::move(B)),
        GetExitCode([=](const llvm::Error &) { return DefaultErrorExitCode; }) {
  }

  /// Set the banner string for any errors caught by operator().
  void setBanner(std::string B) { this->Banner = std::move(B); }

  /// Set the exit-code mapper function.
  void
  setExitCodeMapper(std::function<int(const llvm::Error &)> GetExitCodeFn) {
    this->GetExitCode = std::move(GetExitCodeFn);
  }

  /// Check Err. If it's in a failure state log the error(s) and exit.
  void operator()(llvm::Error Err) const { checkError(std::move(Err)); }

  /// Check E. If it's in a success state then return the contained value.
  /// If it's in a failure state log the error(s) and exit.
  template <typename T> T operator()(llvm::Expected<T> &&E) const {
    checkError(E.takeError());
    return std::move(*E);
  }

  /// Check E. If it's in a success state then return the contained reference.
  /// If it's in a failure state log the error(s) and exit.
  template <typename T> T &operator()(llvm::Expected<T &> &&E) const {
    checkError(E.takeError());
    return *E;
  }

private:
  void checkError(llvm::Error Err) const {
    if (Err) {
      int ExitCode = GetExitCode(Err);
      logAllUnhandledErrors(std::move(Err), llvm::errs(), Banner);
      llvm::sys::RunInterruptHandlers();
      exit(ExitCode);
    }
  }

  std::string Banner;
  std::function<int(const llvm::Error &)> GetExitCode;
};

} // end namespace allvm

#endif // ALLVM_EXITONERROR
