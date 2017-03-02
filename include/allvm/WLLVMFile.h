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
#include <llvm/Support/Error.h>

namespace llvm {
class Module;
} // end namespace llvm

namespace allvm {

using namespace llvm;
using namespace object;

const StringRef WLLVMSectionName = ".llvm_bc";

class WLLVMFile {
  StringRef Name;
  std::vector<StringRef> BCEntries;
  std::vector<char> SectionData;

public:
  WLLVMFile(StringRef _Name, StringRef BCSectionContents) : Name(_Name) {
    parseWLLVMSection(BCSectionContents);
  }
  static llvm::Expected<std::unique_ptr<WLLVMFile>> open(StringRef Input);

  ArrayRef<StringRef> getBCFilenames() const { return BCEntries; }

  llvm::Expected<std::unique_ptr<llvm::Module>>
  getLinkedModule(LLVMContext &C) const;

private:
  void parseWLLVMSection(StringRef Contents);
};

} // end namespace allvm

#endif // ALLVM_WLLVMFile_h
