#include "Allexe.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace allvm;
using namespace llvm;

static std::unique_ptr<MemoryBuffer> moduleToBuffer(const Module *M) {
  std::string X;
  raw_string_ostream OS(X);
  WriteBitcodeToFile(M, OS);
  return MemoryBuffer::getMemBufferCopy(OS.str());
}

Allexe::Allexe(std::unique_ptr<ZipArchive> zipArchive)
    : archive(std::move(zipArchive)) {}

size_t Allexe::getNumModules() const { return archive->listFiles().size(); }

// TODO: Move this to using llvm::Error, and Expected
ErrorOr<std::unique_ptr<Allexe>> Allexe::open(StringRef filename,
                                              bool overwrite) {
  auto archive = ZipArchive::open(filename, overwrite);
  if (!archive) {
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto A = std::unique_ptr<Allexe>(new Allexe(std::move(*archive)));
  if (A->getNumModules() > 0 && A->getModuleName(0) != ALLEXE_MAIN)
    return std::make_error_code(std::errc::invalid_argument);
  return std::move(A);
}

ErrorOr<std::unique_ptr<Module>>
Allexe::getModule(size_t idx, LLVMContext &ctx, uint32_t *crc,
                  bool shouldLoadLazyMetaData) {
  assert(idx < getNumModules() && "invalid module idx");
  auto bitcode = archive->getEntry(idx, crc);
  return getLazyBitcodeModule(std::move(bitcode), ctx, shouldLoadLazyMetaData);
}

uint32_t Allexe::getModuleCRC(size_t idx) {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->getEntryCRC(idx);
}

StringRef Allexe::getModuleName(size_t idx) const {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->listFiles()[idx];
}

bool Allexe::updateModule(size_t idx, std::unique_ptr<llvm::Module> m) {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->updateEntry(idx, moduleToBuffer(m.get()));
}

bool Allexe::addModule(std::unique_ptr<llvm::Module> m, StringRef moduleName) {
  StringRef entryName = !moduleName.empty() ? moduleName : m->getName();
  return archive->addEntry(moduleToBuffer(m.get()), entryName);
}

bool Allexe::addModule(StringRef filename, StringRef moduleName) {
  auto buf = MemoryBuffer::getFile(filename);
  StringRef entryName = !moduleName.empty() ? moduleName : filename;
  return buf && archive->addEntry(std::move(buf.get()), entryName);
}
