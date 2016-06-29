#ifndef ALLVM_ARCHIVE_ZIPARCHIVE_H
#define ALLVM_ARCHIVE_ZIPARCHIVE_H

#include "llvm/Support/ErrorOr.h"

#include <memory>
#include <string>
#include <vector>

typedef struct zip zip_t;

namespace llvm {
class MemoryBuffer;
class Twine;
}

namespace allvm {

class ZipArchive final {
  zip_t *archive;
  std::vector<std::string> files;

  ZipArchive() : archive(nullptr) {}

public:
  ~ZipArchive();

  static llvm::ErrorOr<std::unique_ptr<ZipArchive>>
    openForReading(const llvm::Twine &Filename);

  std::unique_ptr<llvm::MemoryBuffer> getEntry(const llvm::Twine &Entry);
  const std::vector<std::string> & listFiles() const;
};

}

#endif
