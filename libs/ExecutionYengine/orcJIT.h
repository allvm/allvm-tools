
#include <llvm/ADT/STLExtras.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/LazyEmittingLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/IndirectionUtils.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/OrcABISupport.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Object/Archive.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include "allvm/ExecutionYengine.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {
void dummy() {
  llvm::errs() << "Dummy Function called\n";
  abort();
}
}

namespace llvm {
namespace orc {


class OrcJIT{

private:
  std::unique_ptr<TargetMachine> TM;
  const DataLayout DL;
  typedef ObjectLinkingLayer<> ObjectLayerT;
  typedef IRCompileLayer<ObjectLayerT> CompileLayerT;
  typedef std::function<std::unique_ptr<Module>(std::unique_ptr<Module>)> TransformFtor;
  typedef IRTransformLayer<CompileLayerT, TransformFtor> IRDumpLayerT;
  //typedef LazyEmittingLayer<CompileLayerT> CODLayerT;

  ObjectLayerT ObjectLayer;
  CompileLayerT CompileLayer;
  //CODLayerT CODLayer;

  std::vector<object::OwningBinary<object::Archive>> Archives;

  std::unique_ptr<JITCompileCallbackManager> CompileCallbackManager;
  
  typedef orc::CompileOnDemandLayer<CompileLayerT, orc::JITCompileCallbackManager> CODLayerT;
  typedef CODLayerT::IndirectStubsManagerBuilderT IndirectStubsManagerBuilder;


  CODLayerT CODLayer;

  orc::LocalCXXRuntimeOverrides CXXRuntimeOverrides;
  std::vector<orc::CtorDtorRunner<CODLayerT>> IRStaticDestructorRunners;

public:

  OrcJIT(std::unique_ptr<TargetMachine> TM,
         std::unique_ptr<JITCompileCallbackManager> CompileCallbackMgr,
         IndirectStubsManagerBuilder IndirectStubsMgrBuilder)
        : TM(move(TM)), DL(this->TM->createDataLayout()),
          ObjectLayer(),
          CompileLayer(ObjectLayer, SimpleCompiler(*this->TM)),
          CompileCallbackManager(move(CompileCallbackMgr)),
          CODLayer(CompileLayer,
                        [](Function &F) {  return std::set<Function*>({&F}); },
                        *CompileCallbackManager, 
                        std::move(IndirectStubsMgrBuilder),
                        true),
          CXXRuntimeOverrides([this](const std::string &S) { return mangle(S); }) {}

  TargetMachine &getTargetMachine() { return *TM; }

  ~OrcJIT() {

    CXXRuntimeOverrides.runDestructors();
    for (auto &DtorRunner : IRStaticDestructorRunners)
       DtorRunner.runViaLayer(CODLayer);

  }

  bool addModule(std::unique_ptr<Module> M) {


    if (M->getDataLayout().isDefault())
      M->setDataLayout(DL);

    std::vector<std::string> CtorNames, DtorNames;
    unsigned CtorId = 0, DtorId = 0;

    for (auto Ctor : orc::getConstructors(*M)) {
       std::string NewCtorName = ("$static_ctor." + Twine(CtorId++)).str();
       Ctor.Func->setName(NewCtorName);
       Ctor.Func->setLinkage(GlobalValue::ExternalLinkage);
       Ctor.Func->setVisibility(GlobalValue::HiddenVisibility);
       CtorNames.push_back(mangle(NewCtorName));
    }

    for (auto Dtor : orc::getDestructors(*M)) {
       std::string NewDtorName = ("$static_dtor." + Twine(DtorId++)).str();
       Dtor.Func->setName(NewDtorName);
       Dtor.Func->setLinkage(GlobalValue::ExternalLinkage);
       Dtor.Func->setVisibility(GlobalValue::HiddenVisibility);
       DtorNames.push_back(mangle(NewDtorName));
    }

    auto Resolver = createLambdaResolver(
        [&] (const std::string &Name) -> JITSymbol {
           if (auto Sym = CODLayer.findSymbol(Name,false)) {
              return Sym;
           } else
              return CXXRuntimeOverrides.searchOverrides(Name);
        },
        [this] (const std::string &Name) {

          if (auto Sym = scanArchives(Name))
             return Sym;

          if (auto SymAddr =
                   RTDyldMemoryManager::getSymbolAddressInProcess(Name))
             return JITSymbol(SymAddr, JITSymbolFlags::Exported);

          if (Name == "__fini_array_end" || Name == "__fini_array_start" || Name == "__init_array_start" || Name == "__init_array_end")
             return JITSymbol(JITEvaluatedSymbol(reinterpret_cast<JITTargetAddress>(dummy),
                              JITSymbolFlags::Exported));

             return JITSymbol(nullptr);
        });
     
 
    std::vector<std::unique_ptr<Module>> Ms;
    Ms.push_back(std::move(M));
    
    auto H = CODLayer.addModuleSet(std::move(Ms),
                               make_unique<SectionMemoryManager>(),
                               std::move(Resolver));

    orc::CtorDtorRunner<CODLayerT> CtorRunner(std::move(CtorNames), H);

    CtorRunner.runViaLayer(CODLayer);

    IRStaticDestructorRunners.emplace_back(std::move(DtorNames), H);

    return true;

  }
  
  void addArchive(object::OwningBinary<object::Archive> A) {
    Archives.push_back(std::move(A));
  }

  std::string mangle(const Twine &T) {
    std::string MangledName;
    raw_string_ostream MangledNameStream(MangledName);
    Mangler::getNameWithPrefix(MangledNameStream, T.str(), DL);
    return MangledNameStream.str();
  }

  JITSymbol findSymbol(const std::string Name) {

   if (auto Sym = CODLayer.findSymbol(mangle(Name), false))
      return Sym;

   if (auto Sym = scanArchives(Name))
      return Sym;

  // FIXME: Resolve these properly instead of hardcoding to our dummy pointer
  if (Name == "__fini_array_end" || Name == "__fini_array_start" || Name == "__init_array_start" || Name == "__init_array_end")
             return JITSymbol(JITEvaluatedSymbol(reinterpret_cast<JITTargetAddress>(dummy),
                              JITSymbolFlags::Exported));

   return nullptr;
  }

  JITSymbol scanArchives(std::string Name) { 

    for (object::OwningBinary<object::Archive> &OB : Archives) {
      object::Archive * A = OB.getBinary();
      auto OptionalChildOrErr = A->findSym(Name);
      if (!OptionalChildOrErr)
        report_fatal_error(OptionalChildOrErr.takeError());
      auto &OptionalChild = *OptionalChildOrErr;
      if (OptionalChild) {
        Expected<std::unique_ptr<object::Binary>> ChildBinOrErr =
                OptionalChild->getAsBinary();
        if (!ChildBinOrErr) {
           consumeError(ChildBinOrErr.takeError());
           continue;
        }

        //auto N = OptionalChild->getFullName();
        //if (N) errs() << "Name: " << *N << "\n";
        //consumeError(N.takeError());

        std::unique_ptr<object::Binary> &ChildBin = ChildBinOrErr.get();
        if (ChildBin->isObject()) {


    auto Resolver = createLambdaResolver(
        [&] (const std::string &Name) -> JITSymbol {
           if (auto Sym = CODLayer.findSymbol(Name,false)) {
              return Sym;
           } else
              return CXXRuntimeOverrides.searchOverrides(Name);
           
        },
        [this] (const std::string &Name) {
          if (auto Sym = scanArchives(Name))
             return Sym;

          if (auto SymAddr =
                   RTDyldMemoryManager::getSymbolAddressInProcess(Name))
             return JITSymbol(SymAddr, JITSymbolFlags::Exported);

          if (Name == "__fini_array_end" || Name == "__fini_array_start" || Name == "__init_array_start" || Name == "__init_array_end")
             return JITSymbol(JITEvaluatedSymbol(reinterpret_cast<JITTargetAddress>(dummy),
                              JITSymbolFlags::Exported));        

             return JITSymbol(nullptr);
        });



           std::vector<std::unique_ptr<object::ObjectFile>> ObjSet;
           ObjSet.push_back(std::unique_ptr<object::ObjectFile>(
                  static_cast<object::ObjectFile *>(ChildBin.release())));
           ObjectLayer.addObjectSet(std::move(ObjSet),
                                    make_unique<SectionMemoryManager>(),
                                    std::move(Resolver));
           if (auto Sym = ObjectLayer.findSymbol(Name,false))
              return Sym;
           else {
              errs() << "We couldn't find " << Name << "\n";
              return JITSymbol(nullptr);
           }
        }

      }
   
    }

   return JITSymbol(nullptr);

  }

  //void removeModule(ModuleHandle H) { CODLayer.removeModuleSet(H) }


};
 
Error runOrcJIT(std::vector<std::unique_ptr<Module>> Ms,
            //  const std::vector<std::string> &Args,
              allvm::ExecutionYengine::ExecutionInfo &Info);

} // end namespace orc

} //end namespace llvm


