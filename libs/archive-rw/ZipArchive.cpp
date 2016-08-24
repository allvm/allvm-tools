#include "ZipArchive.h"

#include "zip.h"
using namespace llvm;

namespace allvm {

ZipArchive::~ZipArchive() {
  if (archive) {
    zip_close(archive);
  }
}

ErrorOr<std::unique_ptr<ZipArchive>> ZipArchive::open(
    const Twine &filename, bool overwrite) {
  std::unique_ptr<ZipArchive> zip(new ZipArchive);
  int err;
  int flags = ZIP_CREATE;
  if (overwrite) flags |= ZIP_TRUNCATE;
  zip_t *archive = zip_open(filename.str().c_str(), flags, &err);
  if (!archive) {
    return std::error_code{};
  }
  zip->archive = archive;

  auto num_files = zip_get_num_entries(archive, 0);
  for (unsigned i = 0; i < num_files; i++) {
    zip_stat_t statinfo;
    zip_stat_index(archive, i, 0, &statinfo);
    zip->files.emplace_back(statinfo.name);
  }

  return std::move(zip);
}

std::unique_ptr<MemoryBuffer> ZipArchive::getEntry(const Twine &entryName,
                                                   uint32_t *crcOut) {
  SmallVector<char, 256> str;
  StringRef name = entryName.toStringRef(str);
  auto entry = std::find_if(files.begin(), files.end(),
    [&](decltype(files[0]) &f) {
      return f == name;
    });
  if (entry == files.end())
    return nullptr;

  size_t index = static_cast<size_t>(std::distance(files.begin(), entry));
  return getEntry(index, crcOut);
}

std::unique_ptr<MemoryBuffer> ZipArchive::getEntry(size_t index, uint32_t *crcOut) {
  // Find the size of the file entry, and make a new MemoryBuffer of that size.
  zip_stat_t statinfo;
  zip_stat_index(archive, index, 0, &statinfo);
  std::unique_ptr<MemoryBuffer> buf =
    MemoryBuffer::getNewUninitMemBuffer(statinfo.size, files[index]);

  // Decompress the file into the buffer.
  zip_file_t *fd = zip_fopen_index(archive, index, 0);
  zip_fread(fd, const_cast<char*>(buf->getBufferStart()), statinfo.size);
  if (crcOut)
    *crcOut = statinfo.crc;
  zip_fclose(fd);

  return buf;
}

bool ZipArchive::addEntry(std::unique_ptr<MemoryBuffer> entry, StringRef entryName) {
  auto *zipBuffer = zip_source_buffer(archive, entry->getBufferStart(),
                                      entry->getBufferSize(), 0/*don't free*/);
  if (!zipBuffer) return false;

  buffers.emplace_back(std::move(entry));
  files.push_back(entryName);

  bool ok =
    zip_file_add(archive, entryName.data(), zipBuffer, ZIP_FL_ENC_UTF_8) >= 0;

  return ok;
}

ArrayRef<std::string> ZipArchive::listFiles() const { return files; }

} // namespace allvm
