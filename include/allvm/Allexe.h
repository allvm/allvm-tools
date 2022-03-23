#ifndef ALLVM_ALLEXE
#define ALLVM_ALLEXE

#include "allvm/ResourcePaths.h"
#include "allvm/ZipArchive.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class Module;
class LLVMContext;
} // end namespace llvm

namespace allvm {

class Allexe {
  std::unique_ptr<ZipArchive> archive;

  Allexe(std::unique_ptr<ZipArchive> zipArchive);

public:
  size_t getNumModules() const;

  llvm::Expected<std::unique_ptr<llvm::Module>>
  getModule(size_t i, llvm::LLVMContext &, uint32_t *crc = nullptr,
            bool shouldLoadLazyMetaData = true) const;

  std::unique_ptr<llvm::MemoryBuffer>
  getModuleBuffer(size_t idx, uint32_t *crc = nullptr) const;

  uint32_t getModuleCRC(size_t idx) const;
  uint64_t getModuleSize(size_t idx) const;

  llvm::StringRef getModuleName(size_t idx) const;

  /// add a module to this allexe
  llvm::Error addModule(std::unique_ptr<llvm::Module>,
                        llvm::StringRef moduleName = "");

  /// load a module from disk and add it to this allexe
  llvm::Error addModule(llvm::StringRef filename,
                        llvm::StringRef moduleName = "");

  /// update a module, return true if succeeds
  llvm::Error updateModule(size_t idx, std::unique_ptr<llvm::Module>);

  /// open a allexe for reading only
  static llvm::Expected<std::unique_ptr<const Allexe>>
  openForReading(llvm::StringRef, const ResourcePaths &);

  /// open a allexe for reading and writing
  static llvm::Expected<std::unique_ptr<Allexe>>
  open(llvm::StringRef, const ResourcePaths &, bool overwrite = false);
};

const llvm::StringRef ALLEXE_MAIN = "main.bc";

} // end namespace allvm

#endif // ALLVM_ALLEXE
