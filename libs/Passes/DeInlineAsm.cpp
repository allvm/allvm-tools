#include "allvm/DeInlineAsm.h"

#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/AtomicOrdering.h>

using namespace allvm;
using namespace llvm;

namespace {

bool matches(ArrayRef<StringRef> splitString, ArrayRef<StringRef> args) {
  if (splitString.size() != args.size())
    return false;
  for (unsigned i = 0; i < splitString.size(); i++) {
    if (splitString[i] != args[i])
      return false;
  }
  return true;
}

class InlineAsmVisitor : public InstVisitor<InlineAsmVisitor> {
public:
  void visitCallInst(CallInst &I) {
    if (!I.isInlineAsm())
      return;

    InlineAsm *inlineAsm = cast<InlineAsm>(I.getCalledValue());
    auto asmString = inlineAsm->getAsmString();

    Module *M = I.getParent()->getParent()->getParent();
    Value *replacement = nullptr;

    if (asmString.empty()) {
      // Handle "compiler barrier" idiom
      StringRef constraintString = inlineAsm->getConstraintString();
      if (constraintString == "~{memory},~{dirflag},~{fpsr},~{flags}") {
        replacement =
            new FenceInst(M->getContext(), AtomicOrdering::AcquireRelease,
                          SyncScope::SingleThread, &I);
      }
    } else {
      SmallVector<StringRef, 3> asmPieces;
      StringRef(asmString).split(asmPieces, ' ', -1, false);
      if (matches(asmPieces, {"bswap", "$0"}) ||
          matches(asmPieces, {"bswapl", "$0"}) ||
          matches(asmPieces, {"bswapq", "$0"})) {
        Function *intrinsicFunc = Intrinsic::getDeclaration(
            M, Intrinsic::bswap, I.getFunctionType()->params());
        replacement = CallInst::Create(intrinsicFunc, I.getArgOperand(0),
                                       I.getName(), &I);
      }
    }

    if (replacement) {
      I.replaceAllUsesWith(replacement);
      toRemove.push_back(&I);
    }
  }
  // Do nothing for everything else.
  void visitInstruction(Instruction &) {}

  void removeInstructions() {
    for (auto inst : toRemove)
      inst->eraseFromParent();
  }

private:
  std::vector<Instruction *> toRemove;
};

} // end anonymous namespace

PreservedAnalyses DeInlineAsm::run(Module &M, ModuleAnalysisManager &) {
  InlineAsmVisitor visitor;
  visitor.visit(M);
  visitor.removeInstructions();
  return PreservedAnalyses::all();
}
