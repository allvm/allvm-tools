//===-- Error.h -----------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Error handling helpers
//
//===----------------------------------------------------------------------===//

#ifndef ALLVM_Error_h
#define ALLVM_Error_h

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_os_ostream.h>

namespace allvm {
using namespace llvm;

LLVM_ATTRIBUTE_NORETURN void reportError(Twine Msg) {
  errs() << "\nError reading file: " << Msg << ".\n";
  errs().flush();
  exit(1);
}

static void reportError(StringRef Input, std::error_code EC) {
  if (Input == "-")
    Input = "<stdin>";

  reportError(Twine(Input) + ": " + EC.message());
}

static void reportError(StringRef Input, StringRef Message) {
  if (Input == "-")
    Input = "<stdin>";

  reportError(Twine(Input) + ": " + Message);
}

static void reportError(StringRef Input, Error Err) {
  if (Input == "-")
    Input = "<stdin>";
  std::string ErrMsg;
  {
    raw_string_ostream ErrStream(ErrMsg);
    logAllUnhandledErrors(std::move(Err), ErrStream, Input + ": ");
  }
  reportError(ErrMsg);
}

} // end namespace allvm



#endif // ALLVM_Error_h
