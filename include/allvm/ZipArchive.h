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
} // end namespace llvm

namespace allvm {

static const size_t ZIP_INDEX_LAST = static_cast<size_t>(-1);

class ZipArchive final {
  zip_t *archive;
  std::vector<std::string> files;

  // list of buffers to be compressed
  //
  // libzib doesn't write these buffers until the archive is closed.
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> writeBuffers;

  // helper function to write `buf` to an archive entry
  // if `idx` is ZIP_INDEX_LAST, it will append the buf as a new entry.
  bool writeBufferToEntry(size_t idx, std::unique_ptr<llvm::MemoryBuffer> buf,
                          llvm::StringRef);

  ZipArchive() : archive(nullptr) {}

public:
  ~ZipArchive();

  static llvm::ErrorOr<std::unique_ptr<ZipArchive>>
  open(const llvm::Twine &Filename, bool overwrite);

  std::unique_ptr<llvm::MemoryBuffer>
  getEntry(const llvm::Twine &Entry, uint32_t *CrcOut = nullptr) const;

  std::unique_ptr<llvm::MemoryBuffer>
  getEntry(size_t index, uint32_t *CrcOut = nullptr) const;

  uint32_t getEntryCRC(size_t index) const;
  uint64_t getEntryUncompressedSize(size_t index) const;

  llvm::ArrayRef<std::string> listFiles() const;

  // update content of an entry, can optionally rename said entry
  bool updateEntry(size_t idx, std::unique_ptr<llvm::MemoryBuffer> entry,
                   llvm::StringRef newEntryName = "");

  // append an entry in the archive
  bool addEntry(std::unique_ptr<llvm::MemoryBuffer> entry,
                llvm::StringRef entryName);

  // Set the "prefix" data to write before the zip file contents,
  // used when writing this zip to the disk.
  // (Will not be used if no changes are made to an existing archive)
  // The terminating null will not be written.
  // The resulting file will still be a valid zip.  Hopefully.
  //
  // Returns true on success.
  bool setPrefixStr(const llvm::Twine &Prefix);
};
} // end namespace allvm

#endif
