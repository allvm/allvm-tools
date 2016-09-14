//===-- WLLVMFile.cpp -----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// WLLVM file wrapper.
//
//===----------------------------------------------------------------------===//

#include "WLLVMFile.h"

#include <llvm/Object/Archive.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/LineIterator.h>

using namespace allvm;

Expected<std::unique_ptr<WLLVMFile>> WLLVMFile::open(StringRef file) {

  auto BinaryOrErr = createBinary(file);
  if (!BinaryOrErr)
    return BinaryOrErr.takeError();
  auto &Binary = *BinaryOrErr.get().getBinary();

  if (Archive *a = dyn_cast<Archive>(&Binary))
    return make_error<StringError>("archives are not yet supported",
                                   errc::invalid_argument);
  ObjectFile *o = dyn_cast<ObjectFile>(&Binary);
  if (!o)
    return make_error<StringError>(
        "invalid or unsupported file format (expected object file)",
        errc::invalid_argument);
  if (!o->isELF())
    return make_error<StringError>("not an ELF object file", errc::invalid_argument);
  for (auto &S : o->sections()) {
    StringRef SecName;
    if (auto ec = S.getName(SecName))
      return make_error<StringError>("error reading name for section '" +
                                         SecName + "'",
                                     errc::invalid_argument);

    if (SecName == WLLVMSectionName) {
      if (S.getSize() == 0)
        return make_error<StringError>("empty WLLVM section found",
                                       errc::invalid_argument);

      StringRef Contents;
      if (auto ec = S.getContents(Contents))
        return make_error<StringError>("Error reading section contents: " +
                                           ec.message(),
                                       errc::invalid_argument);

      return make_unique<WLLVMFile>(file, Contents);
    }
  }
  return make_error<StringError>("unable to find WLLVM section", errc::invalid_argument);
}

void WLLVMFile::parseWLLVMSection(StringRef Contents) {
  // Make copy for editing, replace nulls with spaces
  SectionData.assign(Contents.begin(), Contents.end());

  for (auto &c : SectionData)
    if (!c)
      c = ' ';

  // Be sure the memory is null-terminated
  SectionData.push_back(0);

  // But don't include the null in the stringref, since
  // memory buffer seems to want to read one past it?
  auto Mem = MemoryBuffer::getMemBuffer(
      StringRef(SectionData.data(), SectionData.size() - 1));
  for (line_iterator I(*Mem, true), E; I != E; ++I) {
    StringRef Entry = I->trim();

    BCEntries.push_back(Entry);
  }
}
