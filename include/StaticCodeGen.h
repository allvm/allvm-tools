#ifndef ALLVM_STATIC_CODE_GEN
#define ALLVM_STATIC_CODE_GEN

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetOptions.h>

#include <memory>
#include <string>

namespace llvm {
template <class T> class Expected;
class Error;
class LLVMContext;
class raw_pwrite_stream;
class StringRef;

namespace object {
class ObjectFile;
class Binary;
}
}

namespace allvm {
class Allexe;
class ALLVMLinker;
}

namespace allvm {

/// A collection of general compilation options. Other pass-specific options are
/// specified within corresponding compilation passes and target-specific
/// options are specified with the corresponding target machine.

/// TODO: Currently the pass-specific options and target-specific options can
/// only be tuned by command line flags. Maybe we will want to make these flags
/// part of the library API in the future.

/// NOTE: Any member variable added to this struct should be utilized for
/// computing the hash. This requires serialization of that member to bytes
/// which is done in member function serializeCompilationOptions.

struct CompilationOptions {
  llvm::StringRef TargetTriple;
  llvm::StringRef MArch;
  llvm::StringRef MCPU;
  std::vector<llvm::StringRef> MAttrs;
  llvm::CodeModel::Model CMModel;
  llvm::Optional<llvm::Reloc::Model> RelocModel;
  llvm::CodeGenOpt::Level OLvl;
  llvm::TargetOptions TOptions;
  bool NoVerify;
  bool DisableSimplifyLibCalls;
  llvm::Optional<bool> DisableFPElim;
  llvm::Optional<bool> DisableTailCalls;
  bool StackRealign;
  llvm::Optional<std::string> TrapFuncName;

  /// Default constructor that initializes the various options to the default
  /// values used by llc.
  CompilationOptions();

  /// All the members of struct CompilationOptions are used by StaticBinaryCache
  /// to compute a unique hash. This is done by converting the 'statically'
  /// allocated data pointed by the members of this struct into an array of
  /// bytes. This array is later used to compute the unique hash. As this
  // struct contains dynamic pointers in the form of vectors and strings, the
  /// raw bytes of this struct cannot be used to create the unique hash, as
  /// they will change with every time an instance of CompilationOptions is
  /// created even if the static data that the instance represents is the same.
  //===--------------------------Limitation-----------------------------------===//
  /// Currently the member CompilationOptions::TOptions::Reciprocals is not
  /// handled. Reciprocals is of type TargetRecip, whose members are defined by
  /// the target machine during Codegen. But certain hashing decisions (like, if
  /// the binary is already present in cache) need to be made before Codegen.
  std::string serializeCompilationOptions() const;
};

/// Compiles the module contained in the given allexe with the given options
/// and writes the corresponding object file to the provided stream. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
llvm::Error compileAllexe(Allexe &Input, llvm::raw_pwrite_stream &OS,
                          const CompilationOptions &Options,
                          llvm::LLVMContext &Context);

/// Compiles the module contained in the given allexe and writes the
/// corresponding object file to the provided stream. The various compilation
/// options are initialized to the default values used by llc. This function
/// assumes that the allexe has been merged and contains exactly one bitcode
/// module.
llvm::Error compileAllexeWithLlcDefaults(Allexe &Input,
                                         llvm::raw_pwrite_stream &OS,
                                         llvm::LLVMContext &Context);

/// Compiles the module contained in the given allexe with the given options
/// and writes the corresponding object file to the provided disk location. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns an llvm ObjectFile object on success.
llvm::Expected<std::unique_ptr<llvm::object::ObjectFile>>
compileAllexe(Allexe &Input, llvm::StringRef Filename,
              const CompilationOptions &Options, llvm::LLVMContext &Context);

/// Compiles the module contained in the given allexe and writes the
/// corresponding object file to the provided disk location. The various
/// compilation options are initialized to the default values used by llc. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns an llvm ObjectFile object on success.
llvm::Expected<std::unique_ptr<llvm::object::ObjectFile>>
compileAllexeWithLlcDefaults(Allexe &Input, llvm::StringRef Filename,
                             llvm::LLVMContext &Context);

/// Compiles the module contained in the given allexe with the given options,
/// links the resulted object file with the given libnone object and linker
/// driver, and writes the corresponding executable to the provided disk
/// location. This function assumes that the allexe has been merged and contains
/// exactly one bitcode module.
///
/// \returns an llvm Binary object on success.
llvm::Expected<std::unique_ptr<llvm::object::Binary>>
compileAndLinkAllexe(Allexe &Input, llvm::StringRef LibNone,
                     const ALLVMLinker &Linker, llvm::StringRef Filename,
                     const CompilationOptions &Options,
                     llvm::LLVMContext &Context);

/// Compiles the module contained in the given allexe, links the resulted object
/// file with the given libnone objecti and linker driver, and writes the
/// corresponding object file to the provided disk location. The various
/// compilation options are initialized to the default values used by llc. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns an llvm Binary object on success.
llvm::Expected<std::unique_ptr<llvm::object::Binary>>
compileAndLinkAllexeWithLlcDefaults(Allexe &Input, llvm::StringRef LibNone,
                                    const ALLVMLinker &Linker,
                                    llvm::StringRef Filename,
                                    llvm::LLVMContext &Context);

} // end namespace allvm

#endif // ALLVM_STATIC_CODE_GEN
