#ifndef ALLVM_ARCHIVE_ZIPARCHIVE_H
#define ALLVM_ARCHIVE_ZIPARCHIVE_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/MemoryBuffer.h>

#include <memory>
#include <string>
#include <vector>

typedef struct zip zip_t;

namespace llvm {
class Twine;
}

namespace allvm {

class ZipArchive final {
  zip_t *archive;
  std::vector<std::string> files;

  // list of buffers to be compressed
  //
  // libzib takes address of these buffers but and don't write
  // the buffers until the archive is closed.
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> buffers;

  ZipArchive() : archive(nullptr) {}

public:
  ~ZipArchive();

  static llvm::ErrorOr<std::unique_ptr<ZipArchive>>
    open(const llvm::Twine &Filename, bool overwrite);

  std::unique_ptr<llvm::MemoryBuffer> getEntry(const llvm::Twine &Entry,
                                               uint32_t *CrcOut = nullptr);

  std::unique_ptr<llvm::MemoryBuffer> getEntry(size_t index,
                                               uint32_t *CrcOut = nullptr);
  llvm::ArrayRef<std::string> listFiles() const;

  // append an entry in the archive
  bool addEntry(std::unique_ptr<llvm::MemoryBuffer> entry, llvm::StringRef entryName);
};

}

#endif
