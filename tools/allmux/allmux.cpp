//===-- allmux.cpp --------------------------------------------------------===//
//
// Author: Will Dietz (WD), wdietz2@uiuc.edu
//
//===----------------------------------------------------------------------===//
//
// Combine allexe's into a single "multiplexed" allexe.
//
//===----------------------------------------------------------------------===//

#include "allvm/Allexe.h"
#include "allvm/DeInlineAsm.h"
#include "allvm/ExitOnError.h"
#include "allvm/GitVersion.h"
#include "allvm/ModuleFlags.h"
#include "allvm/ResourceAnchor.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/IPO/Internalize.h>

using namespace allvm;
using namespace llvm;

namespace {

// TODO: "two-or-more"
cl::list<std::string> InputFiles(cl::Positional, cl::OneOrMore,
                                 cl::desc("<input allexes>"));
cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"),
                                    cl::value_desc("filename"));
cl::opt<bool> ForceOutput("f", cl::desc("Replace output allexe if it exists"),
                          cl::init(false));

allvm::ExitOnError ExitOnErr;

struct Entry {
  std::unique_ptr<Allexe> A;
  std::unique_ptr<Module> Main;
  StringRef Filename;
  StringRef Base;
  std::string MainName;
};

Error verifyModule(Module &M) {
  std::string err;
  raw_string_ostream OS(err);
  if (verifyModule(M, &OS))
    return make_error<StringError>(OS.str(), errc::invalid_argument);
  return Error::success();
}

Expected<std::unique_ptr<Module>> genMain(ArrayRef<Entry> Es, LLVMContext &C,
                                          const ResourcePaths &RP) {
  SMDiagnostic Err;
  auto Main = parseIRFile(RP.MuxMainPath, Err, C);
  if (!Main) {
    // TODO: Fix, return an Error instead
    Err.print("allmux", errs());
    exit(1);
  }
  ExitOnErr(Main->materializeAll());

  // Replace "mains" array

  // %struct.main_info = type { i32 (i32, i8**)*, i8* }
  // @mains = external local_unnamed_addr global [0 x %struct.main_info], align
  // 8
  auto *MainsG = Main->getNamedValue("mains");
  assert(MainsG->getType()->isArrayTy());
  auto *MITy = Main->getTypeByName("struct.main_info");

  IRBuilder<> Builder(C);
  // Set insert point so things are added to the right module
  auto *MFn = Main->getFunction("main");
  Builder.SetInsertPoint(MFn->getEntryBlock().getTerminator());

  auto *MainPtrTy = cast<PointerType>(MITy->getTypeAtIndex(unsigned{0}));
  auto *MainFnTy = cast<FunctionType>(MainPtrTy->getElementType());

  SmallVector<Constant *, 4> MainInfos;
  for (auto &E : Es) {
    auto *MainDecl = Main->getOrInsertFunction(E.MainName, MainFnTy);
    assert(MainDecl->getType() == MainPtrTy);

    auto *NameV = cast<Constant>(Builder.CreateGlobalStringPtr(E.Base));

    MainInfos.push_back(ConstantStruct::get(MITy, {MainDecl, NameV}));
  }

  MainInfos.push_back(
      ConstantStruct::get(MITy,
                          {ConstantPointerNull::get(MainPtrTy),
                           ConstantPointerNull::get(Type::getInt8PtrTy(C))}));

  auto *ATy = ArrayType::get(MITy, MainInfos.size());
  auto *MainsInit = ConstantArray::get(ATy, MainInfos);

  auto *MainsArray = new GlobalVariable(*Main, MainsInit->getType(), true,
                                        GlobalVariable::ExternalLinkage,
                                        MainsInit, "mains_array");

  auto *MainsCasted =
      ConstantExpr::getPointerCast(MainsArray, MainsG->getType());
  MainsG->replaceAllUsesWith(MainsCasted);
  MainsG->eraseFromParent();

  if (auto E = verifyModule(*Main))
    return std::move(E);

  return std::move(Main);
}

} // end anonymous namespace

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  ResourcePaths RP = ResourcePaths::getAnchored(argv[0]);

  cl::ParseCommandLineOptions(argc, argv);
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  LLVMContext C;
  SmallVector<Entry, 2> Entries;
  for (auto &I : InputFiles) {
    auto A = ExitOnErr(Allexe::openForReading(I, RP));
    auto Main = ExitOnErr(A->getModule(0, C));

    auto Base = sys::path::filename(I);
    Entries.push_back(
        {std::move(A), std::move(Main), I, Base, formatv("main_{0}", Base)});
  }

  {
    auto Output = ExitOnErr(Allexe::open(OutputFilename, RP, ForceOutput));

    // Generate new "main"
    auto Main = ExitOnErr(genMain(Entries, C, RP));

    errs() << "Adding generated main..\n";
    ExitOnErr(Output->addModule(std::move(Main), ALLEXE_MAIN));

    // Add mains, renaming them:
    errs() << "Adding mains...\n";
    for (auto &E : Entries) {
      errs() << " - " << E.MainName << "\n";
      ExitOnErr(E.Main->materializeAll());
      auto *MainF = E.Main->getFunction("main");
      MainF->setName(E.MainName);
      MainF->setLinkage(GlobalValue::ExternalLinkage);
      internalizeModule(*E.Main, [&MainF](auto &GV) { return &GV == MainF; });
      ExitOnErr(Output->addModule(std::move(E.Main), E.MainName + ".bc"));
    }

    // Add supporting libs, don't add same lib twice

    errs() << "Adding libs...\n";
    DenseSet<uint32_t> CRCs;
    for (auto &E : Entries) {
      for (size_t i = 1; i < E.A->getNumModules(); ++i)
        if (CRCs.insert(E.A->getModuleCRC(i)).second) {
          auto LibMod = ExitOnErr(E.A->getModule(i, C));
          auto Name = E.A->getModuleName(i);
          ExitOnErr(LibMod->materializeAll());
          ExitOnErr(Output->addModule(std::move(LibMod), Name));
        }
    }
  }

  return 0;
}
