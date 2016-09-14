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

static bool internalizeHidden(GlobalValue &GV) {
  if (!GV.hasHiddenVisibility())
    return false;

  // Don't mess with comdat globals,
  // they're designed to handle being linked together.
  if (GV.hasComdat())
    return false;

  // Don't internalize references to externally defined symbols
  if (GV.isDeclaration())
    return false;

  // These are useful for opts, and should be dropped while linking.
  if (GV.hasAvailableExternallyLinkage())
    return false;

  // Pretty sure weak-any will break things if we don't do something
  // See FunctionImportUtils.cpp for some details.
  // (it might make sense to try to leverage that work, part of ThinLTO)
  assert(!GV.hasWeakAnyLinkage());

  // Hopefully that's most of the things we'll run into,
  // go ahead and convert this global to private (and non-hidden):
  GV.setLinkage(GlobalValue::PrivateLinkage);

  // (Setting local linkage type should change visibility)
  assert(!GV.hasHiddenVisibility());

  return true;
}

static void internalizeHidden(Module *M) {
  for (auto &Func : *M)
    if (Func.getName() != "main")
      internalizeHidden(Func);
  for (auto &Global : M->globals())
    internalizeHidden(Global);
  for (auto &Alias : M->aliases())
    internalizeHidden(Alias);
}

Expected<std::unique_ptr<llvm::Module>>
WLLVMFile::getLinkedModule(LLVMContext &C, bool InternalizeHidden) const {
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

  if (InternalizeHidden)
    internalizeHidden(Composite.get());

  return std::move(Composite);
}
