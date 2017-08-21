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

#include "CtorUtils.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
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
#include <llvm/Transforms/Utils/BuildLibCalls.h>

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
cl::opt<bool> NoInternalize("no-internalize",
                            cl::desc("Don't internalize main modules."),
                            cl::init(false));

allvm::ExitOnError ExitOnErr;

struct Entry {
  std::unique_ptr<Allexe> A;
  std::unique_ptr<Module> Main;
  StringRef Filename;
  StringRef Base;
  std::string MainName;
  std::string getCtorsName() const { return formatv("ctors_{0}", Base); }
  std::string getDtorsName() const { return formatv("dtors_{0}", Base); }
};

std::string getLibCtorsName(Allexe &A, size_t i) {
  return formatv("ctors_${0}", A.getModuleCRC(i));
}
std::string getLibDtorsName(Allexe &A, size_t i) {
  return formatv("dtors_${0}", A.getModuleCRC(i));
}

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

  // Implement "__select" function
  // declare i32 @__select(i8*, i32, i8**, i8**) local_unnamed_addr #1
  auto *SFn = MuxMain->getFunction("__select");
  assert(SFn);

  auto SAI = SFn->arg_begin();
  auto *Name = &*SAI++;

  IRBuilder<> Builder(C);
  Builder.SetInsertPoint(BasicBlock::Create(C, "entry", SFn));

  // XXX: Grab 'main' function prototype from existing main
  auto *Main = MuxMain->getFunction("main");
  auto *MainFnTy = Main->getFunctionType();

  TargetLibraryInfoImpl TLII(Triple(MuxMain->getTargetTriple()));
  TargetLibraryInfo TLI(TLII);
  DataLayout DL(MuxMain.get());

  // XXX: pre-sort list and do a binary search?
  for (auto &E : Es) {
    // Compare name with this entry
    auto Chars = Builder.getInt64(E.Base.size() + 1);
    auto *NameV = cast<Constant>(Builder.CreateGlobalStringPtr(E.Base));
    auto *Cmp = emitStrNCmp(Name, NameV, Chars, Builder, DL, &TLI);
    auto *Match = Builder.CreateIsNull(Cmp);

    auto *MatchBB = BasicBlock::Create(C, "match-" + E.Base, SFn);
    auto *ContBB = BasicBlock::Create(C, "", SFn);
    Builder.CreateCondBr(Match, MatchBB, ContBB);

    Builder.SetInsertPoint(MatchBB);
    auto *RealMain = E.Main->getFunction("main" /* E.MainName */);
    auto *RealMainTy = RealMain->getFunctionType();
    assert(!RealMainTy->isVarArg());
    auto *RealMainDecl = MuxMain->getOrInsertFunction(E.MainName, RealMainTy);

    auto AI = SAI, AE = SFn->arg_end();
    auto PI = RealMainTy->param_begin(), PE = RealMainTy->param_end();
    assert((std::distance(AI, AE) >= std::distance(PI, PE)) &&
           "real main has more arguments than we provide");
    SmallVector<Value *, 4> Args;
    for (; AI != AE && PI != PE; ++AI, ++PI) {
      assert(AI->getType() == *PI);
      if (AI->getType() != *PI)
        abort();
      Args.push_back(&*AI);
    }

    auto callCtorDtor = [&](auto FName) {
      auto *Decl =
          MuxMain->getOrInsertFunction(FName, Builder.getVoidTy(), nullptr);
      Builder.CreateCall(Decl);
    };

    for (size_t i = E.A->getNumModules() - 1; i >= 1; --i)
      callCtorDtor(getLibCtorsName(*E.A, i));

    callCtorDtor(E.getCtorsName());

    auto *Call = Builder.CreateCall(RealMainDecl, Args);

    // XXX: This only handles the cases where main() returns
    // TODO: Handle this properly!
    // Idea: add a single llvm.global_dtors entry for the mux'd main
    // which invokes the dtorfn stored in a global variable we write
    // to once we know which program we're running.
    callCtorDtor(E.getDtorsName());

    for (size_t i = 1, e = E.A->getNumModules(); i < e; ++i)
      callCtorDtor(getLibDtorsName(*E.A, i));

    Value *Ret = Call;
    if (Call->getType()->isVoidTy())
      Ret = Constant::getNullValue(MainFnTy->getReturnType());
    Builder.CreateRet(Ret);

    Builder.SetInsertPoint(ContBB);
  }

  // XXX: abort?
  Builder.CreateRet(ConstantInt::getSigned(MainFnTy->getReturnType(), -1));

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

void replaceCtorsDtorsGV(GlobalVariable *GV, Module &M, StringRef Name) {
  std::vector<Constant *> CtorsDtors;
  if (GV) {
    CtorsDtors = parseGlobalCtorDtors(GV);
    GV->eraseFromParent();
  }
  createCtorDtorFunc(CtorsDtors, M, Name);
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
  SmallVector<Entry, 16> Entries;
  StringSet<> Basenames;
  for (auto &I : InputFiles) {
    auto A = ExitOnErr(Allexe::openForReading(I, RP));
    auto Main = ExitOnErr(A->getModule(0, C));

    auto Base = sys::path::filename(I);
    if (!Basenames.insert(Base).second) {
      errs() << formatv("error: Duplicate basename '{0}' encountered\n", Base);
      return -1;
    }
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
      if (!NoInternalize) {
        internalizeModule(*E.Main, [&MainF](auto &GV) { return &GV == MainF; });

        // Don't export any definitions other than the renamed main
        for (auto &F : *E.Main)
          if (&F != MainF)
            processGlobal(F);
        for (auto &GV : E.Main->globals())
          processGlobal(GV);
        for (auto &GA : E.Main->aliases())
          processGlobal(GA);
      }

      // Grab ctors/dtors

      auto CtorsGV = ExitOnErr(findGlobalCtors(*E.Main));
      replaceCtorsDtorsGV(CtorsGV, *E.Main, E.getCtorsName());

      auto DtorsGV = ExitOnErr(findGlobalDtors(*E.Main));
      replaceCtorsDtorsGV(DtorsGV, *E.Main, E.getDtorsName());

      ExitOnErr(Output->addModule(std::move(E.Main), E.MainName + ".bc"));
    }

    // The code below does the trick *sometimes* but not if the allexes
    // have dependencies on code that provide conflicting symbols
    // (and even then might be wrong re:subtle linking behavior)
    //
    // Idea for handling situations like:
    // X a b c
    // Y a b d
    // Group conflicts:
    // (X c) a b
    // (Y d) a b
    // {(X c), (Y d) } a b
    // XXX: works if X/Y are only bits needing c/d;
    // won't work if a and b need one of c/d; at least not with
    // the current approach below that internalizes X/Y.

    // Add supporting libs, don't add same lib twice

    errs() << "Adding libs...\n";
    DenseSet<uint32_t> CRCs;
    for (auto &E : Entries) {
      for (size_t i = 1; i < E.A->getNumModules(); ++i)
        if (CRCs.insert(E.A->getModuleCRC(i)).second) {
          auto LibMod = ExitOnErr(E.A->getModule(i, C));
          auto Name = E.A->getModuleName(i);
          ExitOnErr(LibMod->materializeAll());

          auto CtorsGV = ExitOnErr(findGlobalCtors(*LibMod));
          replaceCtorsDtorsGV(CtorsGV, *LibMod, getLibCtorsName(*E.A, i));

          auto DtorsGV = ExitOnErr(findGlobalDtors(*LibMod));
          replaceCtorsDtorsGV(DtorsGV, *LibMod, getLibDtorsName(*E.A, i));

          ExitOnErr(Output->addModule(std::move(LibMod), Name));
        }
    }
  }

  return 0;
}
