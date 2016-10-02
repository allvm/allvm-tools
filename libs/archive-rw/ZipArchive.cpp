#include "ZipArchive.h"

#include "zip.h"

using namespace llvm;

namespace allvm {

// TODO: Don't do this, instead point to our specific alley!
static const char *SHEBANG = "#!/usr/bin/env alley\n";

ZipArchive::~ZipArchive() {
  if (archive) {
    // If the archive was changed, this writes those changes now.
    // (If the archive was not changed, this will do nothing)
    // We specifiy the 'shebang' as prefix data to write to the zip
    // so that it can be executed easily.
    zip_close_shebang(archive, SHEBANG);
  }
}

ErrorOr<std::unique_ptr<ZipArchive>> ZipArchive::open(const Twine &filename,
                                                      bool overwrite) {
  std::unique_ptr<ZipArchive> zip(new ZipArchive);
  int err;
  int flags = ZIP_CREATE;
  if (overwrite)
    flags |= ZIP_TRUNCATE;
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
                            [&](decltype(files[0]) &f) { return f == name; });
  if (entry == files.end())
    return nullptr;

  size_t index = static_cast<size_t>(std::distance(files.begin(), entry));
  return getEntry(index, crcOut);
}

std::unique_ptr<MemoryBuffer> ZipArchive::getEntry(size_t index,
                                                   uint32_t *crcOut) {
  // Find the size of the file entry, and make a new MemoryBuffer of that size.
  zip_stat_t statinfo;
  zip_stat_index(archive, index, 0, &statinfo);
  std::unique_ptr<MemoryBuffer> buf =
      MemoryBuffer::getNewUninitMemBuffer(statinfo.size, files[index]);

  // Decompress the file into the buffer.
  zip_file_t *fd = zip_fopen_index(archive, index, 0);
  zip_fread(fd, const_cast<char *>(buf->getBufferStart()), statinfo.size);
  if (crcOut)
    *crcOut = statinfo.crc;
  zip_fclose(fd);

  return buf;
}

uint32_t ZipArchive::getEntryCRC(size_t index) {
  zip_stat_t statinfo;
  zip_stat_index(archive, index, 0, &statinfo);

  return statinfo.crc;
}

bool ZipArchive::updateEntry(size_t idx, std::unique_ptr<MemoryBuffer> entry,
                             StringRef newEntryName) {
  return writeBufferToEntry(idx, std::move(entry), newEntryName);
}

bool ZipArchive::addEntry(std::unique_ptr<MemoryBuffer> entry,
                          StringRef entryName) {
  files.push_back(entryName);
  return writeBufferToEntry(ZIP_INDEX_LAST, std::move(entry), entryName);
}

bool ZipArchive::writeBufferToEntry(size_t idx,
                                    std::unique_ptr<MemoryBuffer> buf,
                                    StringRef entryName) {
  auto *zipBuffer = zip_source_buffer(archive, buf->getBufferStart(),
                                      buf->getBufferSize(), 0 /*don't free*/);
  if (!zipBuffer)
    return false;

  writeBuffers.emplace_back(std::move(buf));

  bool ok;
  const auto encoding = ZIP_FL_ENC_UTF_8;
  if (idx != ZIP_INDEX_LAST) {
    ok = zip_file_replace(archive, idx, zipBuffer, encoding) >= 0;
    if (!entryName.empty())
      ok = ok && zip_file_rename(archive, idx, entryName.data(), encoding);
  } else {
    ok = zip_file_add(archive, entryName.data(), zipBuffer, encoding) >= 0;
  }

  return ok;
}

ArrayRef<std::string> ZipArchive::listFiles() const { return files; }

} // namespace allvm
