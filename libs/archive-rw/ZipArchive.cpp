#include "ZipArchive.h"

#include <llvm/Support/MemoryBuffer.h>

#include "zip.h"
using namespace llvm;

namespace allvm {

ZipArchive::~ZipArchive() {
  if (archive) {
    zip_discard(archive);
  }
}

ErrorOr<std::unique_ptr<ZipArchive>> ZipArchive::openForReading(
    const Twine &filename) {
  std::unique_ptr<ZipArchive> zip(new ZipArchive);
  int err;
  zip_t *archive = zip_open(filename.str().c_str(), ZIP_RDONLY, &err);
  if (!archive) {
    return std::error_code{};
  }
  zip->archive = archive;

  int num_files = zip_get_num_entries(archive, 0);
  for (int i = 0; i < num_files; i++) {
    zip_stat_t statinfo;
    zip_stat_index(archive, i, 0, &statinfo);
    zip->files.emplace_back(statinfo.name);
  }

  return std::move(zip);
}

std::unique_ptr<MemoryBuffer> ZipArchive::getEntry(const Twine &entryName) {
  SmallVector<char, 256> str;
  StringRef name = entryName.toStringRef(str);
  auto entry = std::find_if(files.begin(), files.end(),
    [&](decltype(files[0]) &f) {
      return f == name;
    });
  if (entry == files.end())
    return nullptr;

  // Find the size of the file entry, and make a new MemoryBuffer of that size.
  int index = entry - files.begin();
  zip_stat_t statinfo;
  zip_stat_index(archive, index, 0, &statinfo);
  std::unique_ptr<MemoryBuffer> buf =
    MemoryBuffer::getNewUninitMemBuffer(statinfo.size, entryName);

  // Decompress the file into the buffer.
  zip_file_t *fd = zip_fopen_index(archive, index, 0);
  zip_fread(fd, const_cast<char*>(buf->getBufferStart()), statinfo.size);
  zip_fclose(fd);

  return buf;
}

ArrayRef<std::string> ZipArchive::listFiles() const { return files; }
}
