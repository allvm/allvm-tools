//===-- wllvm-dump.cpp ----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Dump information about a file built with WLLVM.
//
//===----------------------------------------------------------------------===//

#include "Error.h"

#include <llvm/Object/Archive.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/LineIterator.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace allvm;
using namespace llvm;
using namespace object;

const StringRef WLLVMSectionName = ".llvm_bc";

static cl::opt<std::string>
    InputFilename(cl::Positional, cl::Required,
                  cl::desc("<input file built with wllvm>"));

int main(int argc, const char **argv, const char **envp) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv);

  // Open the specified file
  Expected<OwningBinary<Binary>> BinaryOrErr = createBinary(InputFilename);
  if (!BinaryOrErr)
    reportError(InputFilename, BinaryOrErr.takeError());
  auto &Binary = *BinaryOrErr.get().getBinary();

  // Check the filetype, later we might want to support more of these.
  auto error = [](StringRef Msg) { reportError(InputFilename, Msg); };
  if (Archive *a = dyn_cast<Archive>(&Binary))
    error("archives are not yet supported");
  ObjectFile *o = dyn_cast<ObjectFile>(&Binary);
  if (!o)
    error(
        "invalid or unsupported file format, unable to read as an object file");
  if (!o->isELF())
    error("not an ELF object file");

  // Find the section and print its contents
  for (auto &S : o->sections()) {
    StringRef Name;
    auto check = [](std::error_code ec) {
      if (ec)
        reportError(InputFilename, ec);
    };
    check(S.getName(Name));

    if (Name == WLLVMSectionName) {
      if (S.getSize() == 0)
        error("WLLVM section found, but size was zero");

      StringRef Contents;
      check(S.getContents(Contents));

      auto Mem = MemoryBuffer::getMemBuffer(Contents);
      for (line_iterator I(*Mem, true), E; I != E; ++I) {
        assert(*I == I->trim() && "Unexpected whitespace");
        errs() << "Bitcode entry: [" << *I << "]\n";
      }

      return 0;
    }
  }

  error("Unable to find WLLVM section");
}
