#ifndef _ALLVM_STATIC_CODE_GEN_
#define _ALLVM_STATIC_CODE_GEN_

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetOptions.h>

#include <memory>
#include <string>

namespace llvm {
template <class T> class ErrorOr;
class LLVMContext;
class raw_pwrite_stream;
class StringRef;

namespace object {
class ObjectFile;
}
}

namespace allvm {
class Allexe;
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

  /// All the members of struct CompilationOptions is used by StaticBinaryCache 
  /// to compute a unique hash. This is done by converting the 'statically' allocated
  /// data pointed by the members of this struct into an array of bytes. This array
  /// is later used to compute the unique hash.
  /// As this struct contains dynamic pointers in the form of vectors and strings, 
  /// the raw bytes of this strcut cannot be used to create the unique hash, as they
  /// will change with every time an instance of CompilationOptions is created even
  /// if the static data that the insatnce represents is the same. 
  //===--------------------------Limitation-----------------------------------===//
  /// Curretly the member CompilationOptions::TOptions::Reciprocals is not handled.
  /// Reciprocals is of type TargetRecip whose member are defined by the target   
  /// machine during Codegen. But certain hashing decisons (like, if the binary is already
  /// present in cache) need to be made before Codegen.
  /// 
  std::string serializeCompilationOptions();
};

/// Compiles the module contained in the given allexe with the given options
/// and writes the corresponding object file to the provided stream. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns 0 on success.
int compileAllexe(Allexe &, llvm::raw_pwrite_stream &,
                  const CompilationOptions &, llvm::LLVMContext &);

/// Compiles the module contained in the given allexe and writes the
/// corresponding object file to the provided stream. The various compilation
/// options are initialized to the default values used by llc. This function
/// assumes that the allexe has been merged and contains exactly one bitcode
/// module.
///
/// \returns 0 on success.
int compileAllexeWithLlcDefaults(Allexe &, llvm::raw_pwrite_stream &,
                                 llvm::LLVMContext &);

/// Compiles the module contained in the given allexe with the given options
/// and writes the corresponding object file to the provided disk location. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns an llvm ObjectFile object on success.
llvm::ErrorOr<std::unique_ptr<llvm::object::ObjectFile>>
compileAllexe(Allexe &, llvm::StringRef, const CompilationOptions &,
              llvm::LLVMContext &);

/// Compiles the module contained in the given allexe and writes the
/// corresponding object file to the provided disk location. The various
/// compilation options are initialized to the default values used by llc. This
/// function assumes that the allexe has been merged and contains exactly one
/// bitcode module.
///
/// \returns an llvm ObjectFile object on success.
llvm::ErrorOr<std::unique_ptr<llvm::object::ObjectFile>>
compileAllexeWithLlcDefaults(Allexe &, llvm::StringRef, llvm::LLVMContext &);

} // end namespace allvm

#endif // _ALLVM_STATIC_CODE_GEN_
