#ifndef ALLVM_ALLEXE
#define ALLVM_ALLEXE

#include "ZipArchive.h"

#include <llvm/Support/ErrorOr.h>

#include <memory>
#include <string>
#include <vector>

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
  size_t getNumModules() const;

  std::unique_ptr<llvm::MemoryBuffer> getModuleBuffer(size_t idx,
                                                      uint32_t *crc = nullptr) {
    return archive->getEntry(idx, crc);
  }

  llvm::ErrorOr<std::unique_ptr<llvm::Module>>
  getModule(size_t i, llvm::LLVMContext &, uint32_t *crc = nullptr,
            bool shouldLoadLazyMetaData = true);

  llvm::StringRef getModuleName(size_t idx) const;

  /// add a module to this allexe
  bool addModule(std::unique_ptr<llvm::Module>,
                 llvm::StringRef moduleName = "");

  /// load a module from disk and add it to this allexe
  bool addModule(llvm::StringRef filename, llvm::StringRef moduleName = "");

  /// update a module, return true if succeeds
  bool updateModule(size_t idx, std::unique_ptr<llvm::Module>);

  /// open a allexe for reading and writing
  static llvm::ErrorOr<std::unique_ptr<Allexe>> open(llvm::StringRef,
                                                     bool overwrite = false);
};
}

#endif // ALLVM_ALLEXE
