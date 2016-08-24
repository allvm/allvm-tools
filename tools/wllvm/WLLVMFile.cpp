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

#include "Error.h"

#include <llvm/Object/Archive.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/LineIterator.h>

using namespace allvm;

std::unique_ptr<WLLVMFile> WLLVMFile::open(StringRef file) {

  auto BinaryOrErr = createBinary(file);
  if (!BinaryOrErr)
    reportError(file, BinaryOrErr.takeError());
  return make_unique<WLLVMFile>(file, std::move(BinaryOrErr.get()));
}

SectionRef WLLVMFile::getWLLVMSection() {
  // Check the filetype, later we might want to support more of these.
  auto &Binary = *File.getBinary();
  if (Archive *a = dyn_cast<Archive>(&Binary))
    error("archives are not yet supported");
  ObjectFile *o = dyn_cast<ObjectFile>(&Binary);
  if (!o)
    error(
        "invalid or unsupported file format, unable to read as an object file");
  if (!o->isELF())
    error("not an ELF object file");
  for (auto &S : o->sections()) {
    StringRef SecName;
    check(S.getName(SecName));

    if (SecName == WLLVMSectionName) {
      if (S.getSize() == 0)
        error("WLLVM section found, but size was zero");

      return S;
    }
  }
  error("Unable to find WLLVM section");
}

void WLLVMFile::parseWLLVMSection() {
  auto S = getWLLVMSection();

  StringRef Contents;
  check(S.getContents(Contents));

  // Make copy for editing, replace nulls with spaces
  SectionData.assign(Contents.begin(), Contents.end());

  for (auto &c : SectionData)
    if (!c) c = ' ';

  // Must be null-terminated for line_iterator
  SectionData.push_back(0);

  auto Mem = MemoryBuffer::getMemBuffer(
      StringRef(SectionData.data(), SectionData.size()));
  for (line_iterator I(*Mem, true), E; I != E; ++I) {
    StringRef Entry = I->trim();

    BCEntries.push_back(Entry);
  }
}
