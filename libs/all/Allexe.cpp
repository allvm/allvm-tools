#include "Allexe.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/raw_ostream.h>

using namespace allvm;
using namespace llvm;

static Error makeOpenError(const StringRef Filename, const Twine &Msg,
                           std::error_code EC) {
  return make_error<StringError>(
      "Could not open allexe '" + Filename + "': " + Msg, EC);
}

static std::unique_ptr<MemoryBuffer> moduleToBuffer(const Module *M) {
  std::string X;
  raw_string_ostream OS(X);
  WriteBitcodeToFile(M, OS, true /* ShouldPreserveUseListOrder */, nullptr, true /* Generate hash info */);
  return MemoryBuffer::getMemBufferCopy(OS.str());
}

Allexe::Allexe(std::unique_ptr<ZipArchive> zipArchive)
    : archive(std::move(zipArchive)) {}

size_t Allexe::getNumModules() const { return archive->listFiles().size(); }

Expected<std::unique_ptr<Allexe>>
Allexe::openForReading(StringRef filename, const ALLVMContext &AC) {
  auto Allexe = Allexe::open(filename, AC, false);
  if (!Allexe)
    return Allexe.takeError();
  if ((*Allexe)->getNumModules() == 0)
    return makeOpenError(filename, "empty or invalid allexe",
                         errc::invalid_argument);
  return std::move(*Allexe);
}

Expected<std::unique_ptr<Allexe>>
Allexe::open(StringRef filename, const ALLVMContext &AC, bool overwrite) {
  auto archive = ZipArchive::open(filename, overwrite);
  if (!archive)
    // TODO: Improve error handling reported by ZipArchive
    // so we can return a useful error here!
    return makeOpenError(filename, "unknown reason", errc::invalid_argument);
  if (!(*archive)->setPrefixStr("#!" + AC.AlleyPath + "\n"))
    return makeOpenError(filename, "unable to set prefix string",
                         errc::invalid_argument);

  auto A = std::unique_ptr<Allexe>(new Allexe(std::move(*archive)));
  if (A->getNumModules() > 0 && A->getModuleName(0) != ALLEXE_MAIN)
    return makeOpenError(filename,
                         "invalid allexe: First entry was: '" +
                             A->getModuleName(0) + "', expected: '" +
                             ALLEXE_MAIN + "'",
                         errc::invalid_argument);

  return std::move(A);
}

Expected<std::unique_ptr<Module>>
Allexe::getModule(size_t idx, LLVMContext &ctx, uint32_t *crc,
                  bool shouldLoadLazyMetaData) {
  assert(idx < getNumModules() && "invalid module idx");
  auto bitcode = archive->getEntry(idx, crc);
  // TODO: Give error here more context about module that caused the error
  return getOwningLazyBitcodeModule(std::move(bitcode), ctx,
                                    shouldLoadLazyMetaData);
}

uint32_t Allexe::getModuleCRC(size_t idx) {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->getEntryCRC(idx);
}

StringRef Allexe::getModuleName(size_t idx) const {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->listFiles()[idx];
}

uint64_t Allexe::getModuleSize(size_t idx) {
  assert(idx < getNumModules() && "invalid module idx");
  return archive->getEntryUncompressedSize(idx);
}

Error Allexe::updateModule(size_t idx, std::unique_ptr<Module> m) {
  assert(idx < getNumModules() && "invalid module idx");
  if (!archive->updateEntry(idx, moduleToBuffer(m.get())))
    return make_error<StringError>("Error updating module in allexe",
                                   errc::invalid_argument);
  return Error::success();
}

Error Allexe::addModule(std::unique_ptr<Module> m, StringRef moduleName) {
  StringRef entryName = !moduleName.empty() ? moduleName : m->getName();
  if (!archive->addEntry(moduleToBuffer(m.get()), entryName))
    return make_error<StringError>("Error adding module to allexe",
                                   errc::invalid_argument);
  return Error::success();
}

Error Allexe::addModule(StringRef filename, StringRef moduleName) {
  auto buf_or_err = MemoryBuffer::getFile(filename);
  if (!buf_or_err)
    return make_error<StringError>("unable to open file " + filename,
                                   buf_or_err.getError());
  auto buf = std::move(buf_or_err.get());
  if (!llvm::isBitcode(
          reinterpret_cast<const unsigned char *>(buf->getBufferStart()),
          reinterpret_cast<const unsigned char *>(buf->getBufferEnd())))
    return make_error<StringError>("file is not bitcode: " + filename,
                                   errc::invalid_argument);
  StringRef entryName = !moduleName.empty() ? moduleName : filename;
  if (!archive->addEntry(std::move(buf), entryName))
    return make_error<StringError>("Error adding module to allexe",
                                   errc::invalid_argument);
  return Error::success();
}
