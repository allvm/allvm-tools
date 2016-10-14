#ifndef ALLVM_ALLEXE
#define ALLVM_ALLEXE

#include "ALLVMContext.h"
#include "ZipArchive.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class Module;
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

  llvm::Expected<std::unique_ptr<llvm::Module>>
  getModule(size_t i, llvm::LLVMContext &, uint32_t *crc = nullptr,
            bool shouldLoadLazyMetaData = true);

  uint32_t getModuleCRC(size_t idx);

  llvm::StringRef getModuleName(size_t idx) const;

  // TODO: bool -> llvm::Error
  /// add a module to this allexe
  llvm::Error addModule(std::unique_ptr<llvm::Module>,
                        llvm::StringRef moduleName = "");

  /// load a module from disk and add it to this allexe
  llvm::Error addModule(llvm::StringRef filename,
                        llvm::StringRef moduleName = "");

  /// update a module, return true if succeeds
  llvm::Error updateModule(size_t idx, std::unique_ptr<llvm::Module>);

  /// open a allexe for reading only
  static llvm::Expected<std::unique_ptr<Allexe>>
  openForReading(llvm::StringRef, const ALLVMContext &);

  /// open a allexe for reading and writing
  static llvm::Expected<std::unique_ptr<Allexe>>
  open(llvm::StringRef, const ALLVMContext &, bool overwrite = false);
};

const llvm::StringRef ALLEXE_MAIN = "main.bc";
}

#endif // ALLVM_ALLEXE
