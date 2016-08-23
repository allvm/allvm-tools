#ifndef ALLVM_ARCHIVE_ZIPWRITER_H
#define ALLVM_ARCHIVE_ZIPWRITER_H

typedef struct zip zip_t;

#include <memory>
#include <string>

namespace llvm {
class MemoryBuffer;
class Twine;
}

namespace allvm {

// Simplest thing for now, stop-gap until 'liball' does things right.
class ZipWriter final {
public:
  static bool writeToZip(std::unique_ptr<llvm::MemoryBuffer> Main,
                         const llvm::Twine &Filename);
};


} // end namespace allvm

#endif // ALLVM_ARCHIVE_ZIP

