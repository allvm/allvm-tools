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
#include <llvm/ADT/StringSwitch.h>
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
  auto MuxMain = parseIRFile(RP.MuxMainPath, Err, C);
  if (!MuxMain) {
    // TODO: Fix, return an Error instead
    Err.print("allmux", errs());
    exit(1);
  }
  ExitOnErr(MuxMain->materializeAll());

  // Replace "mains" array

  // %struct.main_info = type { i32 (i32, i8**)*, i8* }
  // @mains = external local_unnamed_addr global [0 x %struct.main_info], align
  // 8
  auto *OriginalArray = MuxMain->getNamedValue("mains");
  auto *MITy = MuxMain->getTypeByName("struct.main_info");

  IRBuilder<> Builder(C);
  // Set insert point so things are added to the right module
  auto *Main = MuxMain->getFunction("main");
  Builder.SetInsertPoint(Main->getEntryBlock().getTerminator());

  auto *MainPtrTy = cast<PointerType>(MITy->getTypeAtIndex(unsigned{0}));
  auto *MainFnTy = cast<FunctionType>(MainPtrTy->getElementType());

  SmallVector<Constant *, 4> MainInfos;
  for (auto &E : Es) {
    auto *MainWrapper = cast<Function>(
        MuxMain->getOrInsertFunction(E.MainName + "_wrapper", MainFnTy));
    auto *BB = BasicBlock::Create(C, "entry", MainWrapper);
    Builder.SetInsertPoint(BB);
    SmallVector<Value *, 2> Args;

    auto *RealMain = E.Main->getFunction("main" /* E.MainName */);
    auto *RealMainTy = RealMain->getFunctionType();
    assert(!RealMainTy->isVarArg());
    auto *RealMainDecl = MuxMain->getOrInsertFunction(E.MainName, RealMainTy);

    auto AI = MainWrapper->arg_begin(), AE = MainWrapper->arg_end();
    auto PI = RealMainTy->param_begin(), PE = RealMainTy->param_end();

    assert((std::distance(AI, AE) >= std::distance(PI, PE)) &&
           "real main has more arguments than we provide");

    for (; AI != AE && PI != PE; ++AI, ++PI) {
      assert(AI->getType() == *PI);
      if (AI->getType() != *PI)
        abort();
      Args.push_back(&*AI);
    }

    auto *Call = Builder.CreateCall(RealMainDecl, Args);
    Value *Ret = Call;
    if (Call->getType()->isVoidTy())
      Ret = Constant::getNullValue(MainFnTy->getReturnType());
    Builder.CreateRet(Ret);
    for (auto &A : MainWrapper->args())
      Args.push_back(&A);

    auto *NameV = cast<Constant>(Builder.CreateGlobalStringPtr(E.Base));
    MainInfos.push_back(ConstantStruct::get(MITy, {MainWrapper, NameV}));
  }

  MainInfos.push_back(
      ConstantStruct::get(MITy,
                          {ConstantPointerNull::get(MainPtrTy),
                           ConstantPointerNull::get(Type::getInt8PtrTy(C))}));

  auto *ATy = ArrayType::get(MITy, MainInfos.size());
  auto *MainsInit = ConstantArray::get(ATy, MainInfos);

  auto *MainsArray = new GlobalVariable(*MuxMain, MainsInit->getType(), true,
                                        GlobalVariable::ExternalLinkage,
                                        MainsInit, "mains_array");

  // Replace original array with new array casted to its type
  auto *MainsCasted =
      ConstantExpr::getPointerCast(MainsArray, OriginalArray->getType());
  OriginalArray->replaceAllUsesWith(MainsCasted);
  OriginalArray->eraseFromParent();

  if (auto E = verifyModule(*MuxMain))
    return std::move(E);

  return std::move(MuxMain);
}

void processGlobal(GlobalValue &GV) {
  // Don't internalize these symbols,
  // list taken from "AlwaysPreserved" StringSet in Internalize.cpp
  auto AlwaysPreserved =
      StringSwitch<bool>(GV.getName())
          .Cases("llvm.used", "llvm.compiler.used", true)
          .Cases("llvm.global_ctors", "llvm.global_dtors", true)
          .Case("llvm.global.annotations", true)
          .Cases("__stack_chk_fail", "__stack_chk_guard", true)
          .Default(false);

  if (AlwaysPreserved)
    return;

  if (!GV.isDeclaration())
    GV.setLinkage(GlobalValue::InternalLinkage);
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
      MainF->setVisibility(GlobalValue::DefaultVisibility);
      internalizeModule(*E.Main, [&MainF](auto &GV) { return &GV == MainF; });

      // Don't export any definitions other than the renamed main
      for (auto &F : *E.Main)
        if (&F != MainF)
          processGlobal(F);
      for (auto &GV : E.Main->globals())
        processGlobal(GV);
      for (auto &GA : E.Main->aliases())
        processGlobal(GA);

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
