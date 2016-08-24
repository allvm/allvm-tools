#ifndef _ALLVM_ALLEXE_
#define _ALLVM_ALLEXE_

#include "llvm/Support/ErrorOr.h"
#include <memory>
#include <vector>
#include <string>

#include "ZipArchive.h"

namespace llvm {
class Module;
class StringRef;
class LLVMContext;
}

namespace allvm {

class Allexe {
  std::unique_ptr<ZipArchive> archive;

  Allexe(std::unique_ptr<ZipArchive> zipArchive);

public:
  unsigned getNumModules() const;

  std::unique_ptr<llvm::MemoryBuffer> getModuleBuffer(unsigned idx, uint32_t *crc=nullptr) { return archive->getEntry(idx, crc); }

  llvm::ErrorOr<std::unique_ptr<llvm::Module>> getModule(unsigned i, llvm::LLVMContext &, uint32_t *crc=nullptr);

  llvm::StringRef getModuleName(unsigned idx) const;

  /// add a module to this allexe
  bool addModule(std::unique_ptr<llvm::Module>,
                 llvm::StringRef moduleName=llvm::StringRef());

  /// load a module from disk and add it to this allexe
  bool addModule(llvm::StringRef filename,
                 llvm::StringRef moduleName=llvm::StringRef());

  /// update a module, return true if succeeds
  bool updateModule(unsigned idx, std::unique_ptr<llvm::Module>);

  /// open a allexe for reading and writign
  static llvm::ErrorOr<std::unique_ptr<Allexe>> open(llvm::StringRef, bool overwrite=false);
};
}

#endif // _ALLVM_ALLEXE_
