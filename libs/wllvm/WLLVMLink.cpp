//===-- WLLVMLink.cpp -----------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Link contents of an WLLVMFile
//
//===----------------------------------------------------------------------===//

#include "WLLVMFile.h"

#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/SourceMgr.h>

using namespace allvm;
using namespace llvm;

Expected<std::unique_ptr<Module>>
WLLVMFile::getLinkedModule(LLVMContext &C) const {
  // TODO: Rework how WLLVMFile* handles errors!
  SMDiagnostic Err;

  // Create an empty module to link these into
  auto Composite = make_unique<Module>("linked", C);
  Linker L(*Composite);
  for (auto &BCFilename : getBCFilenames()) {
    auto M = parseIRFile(BCFilename, Err, C);
    if (!M) {
      Err.print("WLLVM-link", errs());
      return make_error<StringError>("Error opening referenced module '" +
                                         BCFilename + "'",
                                     errc::invalid_argument);
    }
    if (L.linkInModule(std::move(M)))
      return make_error<StringError>(BCFilename + ": error linking module",
                                     errc::invalid_argument);
  }

  return std::move(Composite);
}
