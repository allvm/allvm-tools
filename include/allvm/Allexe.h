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

/// An ALLVM Executable
class Allexe {
  std::unique_ptr<ZipArchive> archive;

  Allexe(std::unique_ptr<ZipArchive> zipArchive);

public:
  /// returns the number of modules in the allexe
  size_t getNumModules() const;

  /// return a Module object corresponding to module at specified index \p idx,
  /// optionally returning the CRC of the module in \p crc.
  ///
  /// Convenience wrapper for Allexe::getModuleBuffer.
  ///
  /// \param idx index of module, must be valid
  /// \param c Context to read bitcode into for the Module
  /// \param crc optional pointer to location to write CRC of file in zip
  /// \param shouldLoadLazyMetaData (exposed from LLVM's option of same name)
  llvm::Expected<std::unique_ptr<llvm::Module>>
  getModule(size_t idx, llvm::LLVMContext &c, uint32_t *crc = nullptr,
            bool shouldLoadLazyMetaData = true) const;

  /// return a buffer containing the bitcode for the module at the specified
  /// index \p idx, optionally returning the CRC of the module
  ///
  /// \param idx index of module, must be valid
  /// \param crc optional pointer to location to write CRC of file in zip
  std::unique_ptr<llvm::MemoryBuffer>
  getModuleBuffer(size_t idx, uint32_t *crc = nullptr) const;

  /// return the CRC for the module at index \p idx
  ///
  /// \param idx index of module, must be valid
  uint32_t getModuleCRC(size_t idx) const;

  /// return the (uncompressed) size of the module at index \p idx
  ///
  /// \param idx index of module, must be valid
  uint64_t getModuleSize(size_t idx) const;

  /// return the name of the module at index \p idx
  ///
  /// \param idx index of module, must be valid
  llvm::StringRef getModuleName(size_t idx) const;

  /// add a module to this allexe
  ///
  /// \param m Module to add
  /// \param moduleName optional name in allexe, else Module's name is used
  llvm::Error addModule(std::unique_ptr<llvm::Module> m,
                        llvm::StringRef moduleName = "");

  /// load a module from disk and add it to this allexe
  ///
  /// \param filename path of bitcode module to add
  /// \param moduleName optionally specify name, else \p filename is used
  llvm::Error addModule(llvm::StringRef filename,
                        llvm::StringRef moduleName = "");

  /// update/replace a module, return error on failure
  ///
  /// \param idx index of module to update, must be valid
  /// \param m Module to use at index \p idx
  llvm::Error updateModule(size_t idx, std::unique_ptr<llvm::Module> m);

  /// \name Open or create Allexe's, factory methods
  /// @{

  /// open a allexe for reading only
  static llvm::Expected<std::unique_ptr<const Allexe>>
  openForReading(llvm::StringRef, const ResourcePaths &);

  /// open a allexe for reading and writing
  static llvm::Expected<std::unique_ptr<Allexe>>
  open(llvm::StringRef, const ResourcePaths &, bool overwrite = false);
  /// @}
};

const llvm::StringRef ALLEXE_MAIN = "main.bc";

} // end namespace allvm

#endif // ALLVM_ALLEXE
