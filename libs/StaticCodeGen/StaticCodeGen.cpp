#include "allvm/StaticCodeGen.h"
#include "allvm/ALLVMLinker.h"
#include "allvm/Allexe.h"
#include "allvm/FileRemoverPlus.h"

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
#include <llvm/Support/FileUtilities.h>
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
    AttributeList Attrs = F.getAttributes();
    AttrBuilder NewAttrs;

    if (!CPU.empty())
      NewAttrs.addAttribute("target-cpu", CPU);

    if (!Features.empty())
      NewAttrs.addAttribute("target-features", Features);

    if (DisableFPElim.hasValue())
      NewAttrs.addAttribute("no-frame-pointer-elim",
                            DisableFPElim.getValue() ? "true" : "false");

    if (DisableTailCalls.hasValue())
      NewAttrs.addAttribute("disable-tail-calls",
                            toStringRef(DisableTailCalls.getValue()));

    if (StackRealign)
      NewAttrs.addAttribute("stackrealign");

    if (TrapFuncName.hasValue())
      for (auto &B : F)
        for (auto &I : B)
          if (auto *Call = dyn_cast<CallInst>(&I))
            if (const auto *Callee = Call->getCalledFunction())
              if (Callee->getIntrinsicID() == Intrinsic::debugtrap ||
                  Callee->getIntrinsicID() == Intrinsic::trap)
                Call->addAttribute(AttributeList::FunctionIndex,
                                   Attribute::get(Ctx, "trap-func-name",
                                                  TrapFuncName.getValue()));

    // Let NewAttrs override Attrs.
    F.setAttributes(
        Attrs.addAttributes(Ctx, AttributeList::FunctionIndex, NewAttrs));
  }
}

static Error makeStaticCodeGenError(const Twine &Msg, std::error_code EC) {
  return make_error<StringError>("ALLVM static code generator error: " + Msg,
                                 EC);
}

static Error makeStaticCodeGenError(const Twine &Msg) {
  return makeStaticCodeGenError(Msg, errc::invalid_argument);
}

static Error compileModule(std::unique_ptr<Module> &M, raw_pwrite_stream &OS,
                           const CompilationOptions &Options,
                           LLVMContext &Context) {
  assert(M && "no module provided for static code generation");

  // Verify module immediately to catch problems before doInitialization() is
  // called on any passes.
  if (!Options.NoVerify && verifyModule(*M, &errs())) {
    return makeStaticCodeGenError("input module is broken!");
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
    return makeStaticCodeGenError(Error);
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
    return makeStaticCodeGenError(
        "target does not support generation of object files!",
        errc::function_not_supported);
  }

  // Run passes to do compilation.
  PM.run(*M);

  auto HasError = *static_cast<bool *>(Context.getDiagnosticContext());
  // TODO: Produce better error message?
  if (HasError) {
    return makeStaticCodeGenError("unknown error compiling to object file");
  }

  return Error::success();
}


struct LLCDiagnosticHandler : public DiagnosticHandler {
  bool *HasError;
  LLCDiagnosticHandler(bool *HasErrorPtr) : HasError(HasErrorPtr) {}
  bool handleDiagnostics(const DiagnosticInfo &DI) override {
    if (DI.getSeverity() == DS_Error)
      *HasError = true;

    if (auto *Remark = dyn_cast<DiagnosticInfoOptimizationBase>(&DI))
      if (!Remark->isEnabled())
        return true;

    DiagnosticPrinterRawOStream DP(errs());
    errs() << LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity()) << ": ";
    DI.print(DP);
    errs() << "\n";
    return true;
  }
};

namespace allvm {

Error compileAllexe(Allexe &Input, raw_pwrite_stream &OS,
                    const CompilationOptions &Options, LLVMContext &Context) {
  assert(Input.getNumModules() == 1 &&
         "attempted static code gen for allexe with more than one modules");

  // Set a diagnostic handler that doesn't exit on the first error.
  bool HasError = false;
  Context.setDiagnosticHandler(
      llvm::make_unique<LLCDiagnosticHandler>(&HasError));

  // Get module to be compiled.
  auto ExpM = Input.getModule(0, Context);
  if (!ExpM)
    return ExpM.takeError();
  auto &M = ExpM.get();
  if (auto E = M->materializeAll())
    return E;

  // Compile module.
  return compileModule(M, OS, Options, Context);
}

// Initialize compilation flags to the llc default values.
CompilationOptions::CompilationOptions()
    : CMModel(CodeModel::Small), RelocModel(None), OLvl(CodeGenOpt::Default),
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
  TOptions.UseInitArray = true;
  TOptions.MCOptions.AsmVerbose = true;
}

std::string CompilationOptions::serializeCompilationOptions() const {
  std::string buffer;

  // Serialize TargetTriple
  buffer += TargetTriple.str();
  // Serialize MArch
  buffer += MArch.str();
  // Serialize MCPU
  buffer += MCPU.str();
  // Serialize MAttrs
  for (auto attr : MAttrs) {
    buffer += attr.str();
  }
  // Serialize CMModel
  buffer += std::to_string(CMModel);
  // Serialize RelocModel
  if (RelocModel.hasValue()) {
    buffer += std::to_string(RelocModel.getValue());
  }
  // Serialize OLvl
  buffer += std::to_string(OLvl);
#ifdef __x86_64__
  errs() << "Using incomplete serialization of compilation options, FIXME!\n";
#else
#warning                                                                       \
    "Unable to emit runtime warning that serialization of compilation options is incomplete!"
#endif
  // Serialize TargetOptions
  buffer += std::to_string(TOptions.PrintMachineCode);
  // buffer += std::to_string(TOptions.LessPreciseFPMADOption);
  buffer += std::to_string(TOptions.UnsafeFPMath);
  buffer += std::to_string(TOptions.NoInfsFPMath);
  buffer += std::to_string(TOptions.NoNaNsFPMath);
  buffer += std::to_string(TOptions.NoTrappingFPMath);
  buffer += std::to_string(TOptions.HonorSignDependentRoundingFPMathOption);
  buffer += std::to_string(TOptions.NoZerosInBSS);
  buffer += std::to_string(TOptions.GuaranteedTailCallOpt);
  buffer += std::to_string(TOptions.StackAlignmentOverride);
  buffer += std::to_string(TOptions.StackSymbolOrdering);
  buffer += std::to_string(TOptions.EnableFastISel);
  buffer += std::to_string(TOptions.UseInitArray);
  buffer += std::to_string(TOptions.DisableIntegratedAS);
  buffer += std::to_string(static_cast<int>(TOptions.CompressDebugSections));
  buffer += std::to_string(TOptions.RelaxELFRelocations);
  buffer += std::to_string(TOptions.FunctionSections);
  buffer += std::to_string(TOptions.DataSections);
  buffer += std::to_string(TOptions.UniqueSectionNames);
  buffer += std::to_string(TOptions.TrapUnreachable);
  buffer += std::to_string(TOptions.EmulatedTLS);
  buffer += std::to_string(TOptions.EnableIPRA);
  buffer += std::to_string(TOptions.FloatABIType);
  buffer += std::to_string(TOptions.AllowFPOpFusion);
  buffer += std::to_string(TOptions.ThreadModel);
  buffer += std::to_string(static_cast<int>(TOptions.EABIVersion));
  buffer += std::to_string(static_cast<int>(TOptions.DebuggerTuning));
  buffer += std::to_string(TOptions.FPDenormalMode);
  buffer += std::to_string(static_cast<int>(TOptions.ExceptionModel));
  buffer += std::to_string(TOptions.MCOptions.SanitizeAddress);
  buffer += std::to_string(TOptions.MCOptions.MCRelaxAll);
  buffer += std::to_string(TOptions.MCOptions.MCNoExecStack);
  buffer += std::to_string(TOptions.MCOptions.MCFatalWarnings);
  buffer += std::to_string(TOptions.MCOptions.MCNoWarn);
  buffer += std::to_string(TOptions.MCOptions.MCSaveTempLabels);
  buffer += std::to_string(TOptions.MCOptions.MCUseDwarfDirectory);
  buffer += std::to_string(TOptions.MCOptions.MCIncrementalLinkerCompatible);
  buffer += std::to_string(TOptions.MCOptions.ShowMCEncoding);
  buffer += std::to_string(TOptions.MCOptions.ShowMCInst);
  buffer += std::to_string(TOptions.MCOptions.AsmVerbose);
  buffer += std::to_string(TOptions.MCOptions.PreserveAsmComments);
  buffer += std::to_string(TOptions.MCOptions.DwarfVersion);
  buffer += TOptions.MCOptions.ABIName;
  // Serialize NoVerify
  buffer += std::to_string(NoVerify);
  // Serialize DisableSimplifyLibCalls
  buffer += std::to_string(DisableSimplifyLibCalls);
  // Serialize DisableFPElim
  if (DisableFPElim.hasValue()) {
    buffer += std::to_string(DisableFPElim.getValue());
  }
  // Serialize DisableTailCalls
  if (DisableTailCalls.hasValue()) {
    buffer += std::to_string(DisableTailCalls.getValue());
  }
  // Serialize StackRealign
  buffer += std::to_string(StackRealign);
  // Serialize TrapFuncName
  if (TrapFuncName.hasValue()) {
    buffer += TrapFuncName.getValue();
  }

  return buffer;
}

Error compileAllexeWithLlcDefaults(Allexe &Input, raw_pwrite_stream &OS,
                                   LLVMContext &Context) {
  CompilationOptions Options;
  return compileAllexe(Input, OS, Options, Context);
}

Expected<std::unique_ptr<ObjectFile>>
compileAllexe(Allexe &Input, StringRef Filename,
              const CompilationOptions &Options, LLVMContext &Context) {

  {
    // Open the output file.
    std::error_code EC;
    sys::fs::OpenFlags OpenFlags = sys::fs::F_None;
    auto FDOut = llvm::make_unique<ToolOutputFile>(Filename, EC, OpenFlags);
    if (EC) {
      return makeStaticCodeGenError("error opening compilation output", EC);
    }
    assert(FDOut->os().supportsSeeking());

    // Do static code generation.
    if (auto E = compileAllexe(Input, FDOut->os(), Options, Context))
      return std::move(E);

    FDOut->keep();
  }

  // Get a MemoryBuffer of the output file and use it to create the object file.
  auto ErrorOrBuffer = MemoryBuffer::getFile(Filename);
  if (!ErrorOrBuffer) {
    return makeStaticCodeGenError("error reading compiled result",
                                  ErrorOrBuffer.getError());
  }
  MemoryBufferRef Buffer(**ErrorOrBuffer);
  return ObjectFile::createObjectFile(Buffer);
}

Expected<std::unique_ptr<ObjectFile>>
compileAllexeWithLlcDefaults(Allexe &Input, StringRef Filename,
                             LLVMContext &Context) {
  CompilationOptions Options;
  return compileAllexe(Input, Filename, Options, Context);
}

Expected<std::unique_ptr<Binary>>
compileAndLinkAllexe(Allexe &Input, StringRef LibNone, StringRef CrtBits,
                     const ALLVMLinker &Linker, StringRef Filename,
                     const CompilationOptions &Options, LLVMContext &Context) {
  std::string ObjectFilename(Filename);
  ObjectFilename.append(".o");

  // Remove the created .o before leaving this function.
  FileRemoverPlus RemoveObject(ObjectFilename);

  // Compile the allexe.
  auto Err = compileAllexe(Input, ObjectFilename, Options, Context);
  if (!Err)
    return std::move(Err);

  // Link the allexe.
  SmallVector<StringRef, 2> Objects;
  Objects.push_back(ObjectFilename);
  Objects.push_back(LibNone);
  if (auto E = Linker.link(Objects, CrtBits, Filename))
    return std::move(E);

  // Get a MemoryBuffer of the output file and use it to create the binary file.
  auto ErrorOrBuffer = MemoryBuffer::getFile(Filename);
  if (!ErrorOrBuffer) {
    return makeStaticCodeGenError("error reading linker result",
                                  ErrorOrBuffer.getError());
  }
  MemoryBufferRef Buffer(**ErrorOrBuffer);
  return createBinary(Buffer);
}

Expected<std::unique_ptr<Binary>> compileAndLinkAllexeWithLlcDefaults(
    Allexe &Input, StringRef LibNone, StringRef CrtBits,
    const ALLVMLinker &Linker, StringRef Filename, LLVMContext &Context) {
  CompilationOptions Options;
  return compileAndLinkAllexe(Input, LibNone, CrtBits, Linker, Filename,
                              Options, Context);
}

} // end namespace allvm
