#include "ZipWriter.h"

#include <llvm/Support/MemoryBuffer.h>

#include "zip.h"
using namespace llvm;
using namespace allvm;

bool ZipWriter::writeToZip(std::unique_ptr<llvm::MemoryBuffer> Main,
                           const llvm::Twine &Filename) {
  int err;
  auto *zip = zip_open(Filename.str().c_str(), ZIP_CREATE | ZIP_EXCL, &err);
  if (!zip)
    return false;

  auto *source =
      zip_source_buffer(zip, Main->getBufferStart(), Main->getBufferSize(),
                        0 /* don't free this memory */);
  if (!source)
    return false;

  if (zip_file_add(zip, "main.bc", source, ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(source);
    return false;
  }

  return zip_close(zip) == 0;
}
