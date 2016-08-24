#include <llvm/ADT/Twine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include "Allexe.h"

using namespace allvm;
using namespace llvm;

const StringRef ALLEXE_MAIN = "main.bc";

Allexe::Allexe(std::unique_ptr<ZipArchive> zipArchive)
  : archive(std::move(zipArchive)) {
}

unsigned Allexe::getNumModules() const {
  return archive->listFiles().size();
}

ErrorOr<std::unique_ptr<Allexe>> Allexe::open(StringRef filename, bool overwrite) {
  auto archive = ZipArchive::open(filename, overwrite);
  if (!archive) {
    return std::error_code{};
  }
  return std::unique_ptr<Allexe>(new Allexe(std::move(*archive)));
}

ErrorOr<std::unique_ptr<Module>> Allexe::getModule(unsigned idx, LLVMContext &ctx, uint32_t *crc) {
  assert(idx < getNumModules() && "invalid module idx");
  auto bitcode = archive->getEntry(idx, crc);
  // @wdietz: should we move the lazy flag up as an argument?
  return getLazyBitcodeModule(std::move(bitcode), ctx, /* lazy metadata */true);
}

StringRef Allexe::getModuleName(unsigned idx) const {
  return archive->listFiles()[idx];
}

bool Allexe::updateModule(unsigned idx, std::unique_ptr<llvm::Module> m) {
  assert(idx < getNumModules() && "invalid module idx");
}

bool Allexe::addModule(std::unique_ptr<llvm::Module> m, StringRef moduleName) {
  std::string x;
  raw_string_ostream os(x);
  WriteBitcodeToFile(m.get(), os); 
  StringRef entryName = !moduleName.empty() ? moduleName : m->getName();
  auto buf = MemoryBuffer::getMemBufferCopy(os.str());
  return archive->addEntry(std::move(buf), entryName);
}

bool Allexe::addModule(StringRef filename, StringRef moduleName) {
  auto buf = MemoryBuffer::getFile(filename);
  StringRef entryName = !moduleName.empty() ? moduleName : filename;
  return buf && archive->addEntry(std::move(buf.get()), entryName);
}
