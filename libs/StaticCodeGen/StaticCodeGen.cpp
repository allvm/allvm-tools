#include "StaticCodeGen.h"
#include "Allexe.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DiagnosticInfo.h>
#include "llvm/IR/DiagnosticPrinter.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>

#include <string>

using namespace allvm;
using namespace llvm;

static inline std::string getCPUStr(StringRef MCPU) {
  // If user asked for the 'native' CPU, autodetect here. If autodection fails,
  // this will set the CPU to an empty string which tells the target to
  // pick a basic default.
  if (MCPU == "native")
    return sys::getHostCPUName();

  return MCPU;
}

static inline std::string getFeaturesStr(StringRef MCPU,
                                         const std::vector<StringRef> &MAttrs) {
  SubtargetFeatures Features;

  // If user asked for the 'native' CPU, we need to autodetect features.
  // This is necessary for x86 where the CPU might not support all the
  // features the autodetected CPU name lists in the target. For example,
  // not all Sandybridge processors support AVX.
  if (MCPU == "native") {
    StringMap<bool> HostFeatures;
    if (sys::getHostCPUFeatures(HostFeatures))
      for (auto &F : HostFeatures)
        Features.AddFeature(F.first(), F.second);
  }

  for (unsigned i = 0; i != MAttrs.size(); ++i)
    Features.AddFeature(MAttrs[i]);

  return Features.getString();
}

static inline void setFunctionAttributes(StringRef CPU, StringRef Features,
                                         Optional<bool> DisableFPElim,
                                         Optional<bool> DisableTailCalls,
                                         bool StackRealign,
                                         Optional<std::string> TrapFuncName,
                                         Module &M) {
  for (auto &F : M) {
    auto &Ctx = F.getContext();
    AttributeSet Attrs = F.getAttributes(), NewAttrs;

    if (!CPU.empty())
      NewAttrs = NewAttrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
                                       "target-cpu", CPU);

    if (!Features.empty())
      NewAttrs = NewAttrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
                                       "target-features", Features);

    if (DisableFPElim.hasValue())
      NewAttrs =
        NewAttrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
                              "no-frame-pointer-elim",
                              DisableFPElim.getValue() ? "true" : "false");

    if (DisableTailCalls.hasValue())
      NewAttrs =
        NewAttrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
                              "disable-tail-calls",
                              toStringRef(DisableTailCalls.getValue()));

    if (StackRealign)
      NewAttrs = NewAttrs.addAttribute(Ctx, AttributeSet::FunctionIndex,
                                       "stackrealign");

    if (TrapFuncName.hasValue())
      for (auto &B : F)
        for (auto &I : B)
          if (auto *Call = dyn_cast<CallInst>(&I))
            if (const auto *F = Call->getCalledFunction())
              if (F->getIntrinsicID() == Intrinsic::debugtrap ||
                  F->getIntrinsicID() == Intrinsic::trap)
                Call->addAttribute(llvm::AttributeSet::FunctionIndex,
                                   "trap-func-name", TrapFuncName.getValue());

    // Let NewAttrs override Attrs.
    NewAttrs = Attrs.addAttributes(Ctx, AttributeSet::FunctionIndex, NewAttrs);
    F.setAttributes(NewAttrs);
  }
}

static int compileModule(std::unique_ptr<Module> &M, raw_pwrite_stream &OS,
                         StringRef TargetTriple,
                         StringRef MArch, StringRef MCPU,
                         const std::vector<StringRef> &MAttrs,
                         CodeModel::Model CMModel,
                         Optional<Reloc::Model> RelocModel,
                         CodeGenOpt::Level OLvl, const TargetOptions &Options,
                         bool NoVerify,
                         bool DisableSimplifyLibCalls,
                         Optional<bool> DisableFPElim,
                         Optional<bool> DisableTailCalls,
                         bool StackRealign,
                         Optional<std::string> TrapFuncName,
                         LLVMContext &Context) {
  assert(M && "no module provided for static code generation");

  // Verify module immediately to catch problems before doInitialization() is
  // called on any passes.
  if (!NoVerify && verifyModule(*M, &errs())) {
    errs() << "allvm static code generator: input module is broken!\n";
    return 1;
  }

  // Get the target triple.
  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty())
    M->setTargetTriple(Triple::normalize(TargetTriple));
  Triple TheTriple = Triple(M->getTargetTriple());
  if (TheTriple.getTriple().empty())
    TheTriple.setTriple(sys::getDefaultTargetTriple());

  // Get the target specific parser.
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(MArch, TheTriple,
                                                         Error);
  if (!TheTarget) {
    errs() << "allvm static code generator: " << Error;
    return 1;
  }

  std::string CPUStr = getCPUStr(MCPU);
  std::string FeaturesStr = getFeaturesStr(MCPU, MAttrs);

  std::unique_ptr<TargetMachine> Target(
      TheTarget->createTargetMachine(TheTriple.getTriple(), CPUStr, FeaturesStr,
                                     Options, RelocModel, CMModel, OLvl));

  assert(Target && "Could not allocate target machine!");

  // Build up all of the passes that we want to do to the module.
  legacy::PassManager PM;

  // Add an appropriate TargetLibraryInfo pass for the module's triple.
  TargetLibraryInfoImpl TLII(Triple(M->getTargetTriple()));

  // The DisableSimplifyLibCalls flag actually disables all builtin optzns.
  if (DisableSimplifyLibCalls)
    TLII.disableAllFunctions();
  PM.add(new TargetLibraryInfoWrapperPass(TLII));

  // Add the target data from the target machine, if it exists, or the module.
  M->setDataLayout(Target->createDataLayout());

  // Override function attributes based on CPUStr, FeaturesStr, and other flags.
  setFunctionAttributes(CPUStr, FeaturesStr, DisableFPElim, DisableTailCalls,
                        StackRealign, TrapFuncName, *M);

  // Ask the target to add backend passes as necessary.
  if (Target->addPassesToEmitFile(PM, OS, TargetMachine::CGFT_ObjectFile,
                                  NoVerify)) {
    errs() << "allvm static code generator: target does not support"
           << "generation of object files!\n";
    return 1;
  }

  PM.run(*M);
  assert(false);

  auto HasError = *static_cast<bool *>(Context.getDiagnosticContext());
  if (HasError)
    return 1;

  return 0;
}

static void DiagnosticHandler(const DiagnosticInfo &DI, void *Context) {
  bool *HasError = static_cast<bool *>(Context);
  if (DI.getSeverity() == DS_Error)
    *HasError = true;

  DiagnosticPrinterRawOStream DP(errs());
  errs() << LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity()) << ": ";
  DI.print(DP);
  errs() << "\n";
}

namespace allvm {

void compileAllexeWithLlcDefaults(Allexe &Input, raw_pwrite_stream &OS,
                                  LLVMContext &Context) {
  assert(Input.getNumModules() == 1 &&
         "attempted static code gen for allexe with more than one modules");

  // Set a diagnostic handler that doesn't exit on the first error.
  bool HasError = false;
  Context.setDiagnosticHandler(DiagnosticHandler, &HasError);

  // Get module to be compiled.
  auto ErrorOrM = Input.getModule(0, Context);
  assert(ErrorOrM && "failed to extract single module from allexe");

  // Initialize compilation flags to the llc default values.
  std::string TargetTriple;
  std::string MArch;  
  std::string MCPU;
  std::vector<StringRef> MAttrs;
  CodeModel::Model CMModel{CodeModel::Default};
  Optional<Reloc::Model> RelocModel{None};
  CodeGenOpt::Level OLvl{CodeGenOpt::Default};
  TargetOptions Options;
  //Options.PrintMachineCode = true;
  Options.UseInitArray = true;
  Options.MCOptions.AsmVerbose = true;
  bool NoVerify{false};
  bool DisableSimplifyLibCalls{false};
  Optional<bool> DisableFPElim{None};
  Optional<bool> DisableTailCalls{None};
  bool StackRealign{false};
  Optional<std::string> TrapFuncName{None};

  // Compile module.
  if (!compileModule(ErrorOrM.get(), OS, TargetTriple, MArch, MCPU, MAttrs,
                     CMModel, RelocModel, OLvl, Options, NoVerify,
                     DisableSimplifyLibCalls, DisableFPElim, DisableTailCalls,
                     StackRealign, TrapFuncName, Context))
    assert(false && "compilation failed");
}

}
