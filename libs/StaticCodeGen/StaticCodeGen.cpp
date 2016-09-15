#include "StaticCodeGen.h"
#include "Allexe.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetMachine.h>

using namespace allvm;
using namespace llvm;
using namespace object;

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
      NewAttrs = NewAttrs.addAttribute(
          Ctx, AttributeSet::FunctionIndex, "no-frame-pointer-elim",
          DisableFPElim.getValue() ? "true" : "false");

    if (DisableTailCalls.hasValue())
      NewAttrs = NewAttrs.addAttribute(
          Ctx, AttributeSet::FunctionIndex, "disable-tail-calls",
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
                                   Attribute::get(Ctx, "trap-func-name",
                                                  TrapFuncName.getValue()));

    // Let NewAttrs override Attrs.
    NewAttrs = Attrs.addAttributes(Ctx, AttributeSet::FunctionIndex, NewAttrs);
    F.setAttributes(NewAttrs);
  }
}

// Compiles the given module and return 0 on success.
static int compileModule(std::unique_ptr<Module> &M, raw_pwrite_stream &OS,
                         const CompilationOptions &Options,
                         LLVMContext &Context) {
  assert(M && "no module provided for static code generation");

  // Verify module immediately to catch problems before doInitialization() is
  // called on any passes.
  if (!Options.NoVerify && verifyModule(*M, &errs())) {
    errs() << "allvm static code generator: input module is broken!\n";
    return 1;
  }

  // Get the target triple.
  // If we are supposed to override the target triple, do so now.
  if (!Options.TargetTriple.empty())
    M->setTargetTriple(Triple::normalize(Options.TargetTriple));
  Triple TheTriple = Triple(M->getTargetTriple());
  if (TheTriple.getTriple().empty())
    TheTriple.setTriple(sys::getDefaultTargetTriple());

  // Get the target specific parser.
  std::string Error;
  const Target *TheTarget =
      TargetRegistry::lookupTarget(Options.MArch, TheTriple, Error);
  if (!TheTarget) {
    errs() << "allvm static code generator: " << Error;
    return 1;
  }

  std::string CPUStr = getCPUStr(Options.MCPU);
  std::string FeaturesStr = getFeaturesStr(Options.MCPU, Options.MAttrs);

  std::unique_ptr<TargetMachine> Target(TheTarget->createTargetMachine(
      TheTriple.getTriple(), CPUStr, FeaturesStr, Options.TOptions,
      Options.RelocModel, Options.CMModel, Options.OLvl));

  assert(Target && "Could not allocate target machine!");

  // Build up all of the passes that we want to do to the module.
  legacy::PassManager PM;

  // Add an appropriate TargetLibraryInfo pass for the module's triple.
  TargetLibraryInfoImpl TLII(Triple(M->getTargetTriple()));

  // The DisableSimplifyLibCalls flag actually disables all builtin optzns.
  if (Options.DisableSimplifyLibCalls)
    TLII.disableAllFunctions();
  PM.add(new TargetLibraryInfoWrapperPass(TLII));

  // Add the target data from the target machine, if it exists, or the module.
  M->setDataLayout(Target->createDataLayout());

  // Override function attributes based on CPUStr, FeaturesStr, and other
  // compilation options.
  setFunctionAttributes(CPUStr, FeaturesStr, Options.DisableFPElim,
                        Options.DisableTailCalls, Options.StackRealign,
                        Options.TrapFuncName, *M);

  // Ask the target to add backend passes as necessary.
  if (Target->addPassesToEmitFile(PM, OS, TargetMachine::CGFT_ObjectFile,
                                  Options.NoVerify)) {
    errs() << "allvm static code generator: target does not support"
           << "generation of object files!\n";
    return 1;
  }

  // Run passes to do compilation.
  PM.run(*M);

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

int compileAllexe(Allexe &Input, raw_pwrite_stream &OS,
                  const CompilationOptions &Options, LLVMContext &Context) {
  assert(Input.getNumModules() == 1 &&
         "attempted static code gen for allexe with more than one modules");

  // Set a diagnostic handler that doesn't exit on the first error.
  bool HasError = false;
  Context.setDiagnosticHandler(DiagnosticHandler, &HasError);

  // Get module to be compiled.
  auto ErrorOrM = Input.getModule(0, Context);
  if (!ErrorOrM)
    return 1;
  auto &M = ErrorOrM.get();
  M->materializeAll();

  // Compile module.
  return compileModule(M, OS, Options, Context);
}

// Initialize compilation flags to the llc default values.
CompilationOptions::CompilationOptions()
    : CMModel(CodeModel::Default), RelocModel(None), OLvl(CodeGenOpt::Default),
      NoVerify(false), DisableSimplifyLibCalls(false), DisableFPElim(None),
      DisableTailCalls(None), StackRealign(false), TrapFuncName(None) {

  // The following options are iniitialized as desired by their default
  // constructors:
  //  TargetTriple
  //  MArch
  //  MCPU
  //  MAttrs

  // The default constructor of TargetOptions initializes the object as desired
  // except for two members.
  TargetOptions Options;
  Options.UseInitArray = true;
  Options.MCOptions.AsmVerbose = true;
}

int compileAllexeWithLlcDefaults(Allexe &Input, raw_pwrite_stream &OS,
                                 LLVMContext &Context) {
  CompilationOptions Options;
  return compileAllexe(Input, OS, Options, Context);
}

llvm::ErrorOr<std::unique_ptr<ObjectFile>>
compileAllexe(Allexe &Input, StringRef Filename,
              const CompilationOptions &Options, LLVMContext &Context) {

  {
    // Open the output file.
    std::error_code EC;
    sys::fs::OpenFlags OpenFlags = sys::fs::F_None;
    auto FDOut = llvm::make_unique<tool_output_file>(Filename, EC, OpenFlags);
    if (EC)
      return EC;
    assert(FDOut->os().supportsSeeking());

    // Do static code generation.
    if (compileAllexe(Input, FDOut->os(), Options, Context)) {
      return make_error_code(errc::invalid_argument);
    }
    FDOut->keep();
  }

  // Get a MemoryBuffer of the output file and use it to create the object file.
  auto ErrorOrBuffer = MemoryBuffer::getFile(Filename);
  if (!ErrorOrBuffer)
    return ErrorOrBuffer.getError();
  MemoryBufferRef Buffer(**ErrorOrBuffer);
  return expectedToErrorOr(ObjectFile::createObjectFile(Buffer));
}

llvm::ErrorOr<std::unique_ptr<ObjectFile>>
compileAllexeWithLlcDefaults(Allexe &Input, StringRef Filename,
                             LLVMContext &Context) {
  CompilationOptions Options;
  return compileAllexe(Input, Filename, Options, Context);
}

} // end namespace allvm