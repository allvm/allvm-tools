//===-- WLLVMFile.h -------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// WLLVM file wrapper.
//
//===----------------------------------------------------------------------===//

#ifndef ALLVM_WLLVMFile_h
#define ALLVM_WLLVMFile_h

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/ObjectFile.h>

#include "Error.h"

namespace allvm {

using namespace llvm;
using namespace object;

const StringRef WLLVMSectionName = ".llvm_bc";

class WLLVMFile {
  StringRef Name;
  OwningBinary<Binary> File;
  std::vector<StringRef> BCEntries;

public:
  WLLVMFile(StringRef Name, OwningBinary<Binary> Binary)
      : Name(Name), File(std::move(Binary)) {
    parseWLLVMSection();
  }
  static std::unique_ptr<WLLVMFile> open(StringRef Input);

  SectionRef getWLLVMSection();

  ArrayRef<StringRef> getBCFilenames() const {
    return BCEntries;
  }

private:
  void parseWLLVMSection();
  LLVM_ATTRIBUTE_NORETURN void error(StringRef Msg) { reportError(Name, Msg); }
  void check(std::error_code ec) {
    if (ec)
      reportError(Name, ec);
  }
};

} // end namespace allvm


#endif // ALLVM_WLLVMFile_h
