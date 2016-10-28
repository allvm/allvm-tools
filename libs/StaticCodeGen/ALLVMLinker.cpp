#include "ALLVMLinker.h"

#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/Program.h>

using namespace allvm;
using namespace llvm;

static Error makeALLVMLinkerError(const Twine &Msg, std::error_code EC) {
  return make_error<StringError>("ALLVM linker error: " + Msg, EC);
}

static Error makeALLVMLinkerError(const Twine &Msg) {
  return makeALLVMLinkerError(Msg, errc::invalid_argument);
}

namespace allvm {

ALLVMLinker::~ALLVMLinker() {}

PathLinker::PathLinker(llvm::StringRef LinkerName)
    : Linker(LinkerName),
      GccLikeDriver(LinkerName == "gcc" || LinkerName == "clang" ||
                    LinkerName == "cc") {}

Error PathLinker::link(const SmallVectorImpl<StringRef> &ObjectFilenames,
                       StringRef CrtBits, StringRef Filename) const {
  if (GccLikeDriver) {
    errs() << "\n"
           << "ALLVM linker warning: Using the " << Linker << " driver as "
           << "linker.\n"
           << "Drivers such as " << Linker << " may invoke an underlying "
           << "linker with flags that will\n"
           << "add additional code to the executable other than code coming "
           << "from the allexe\n"
           << "and libnone, i.e. a different libc.\n"
           << "Please consider using a naked linker such as ld.\n\n";
  }

  // Find linker.
  auto ErrorOrLinkerProgram = llvm::sys::findProgramByName(Linker);
  if (!ErrorOrLinkerProgram) {
    return makeALLVMLinkerError("Linker driver " + Linker + " was not found",
                                ErrorOrLinkerProgram.getError());
  }
  std::string &LinkerProgram = ErrorOrLinkerProgram.get();

  // Create linker arguments.
  std::string FilenameStr = Filename.str();
  std::string Crt1Str = (CrtBits + "/crt1.o").str();
  std::string CrtiStr = (CrtBits + "/crti.o").str();
  std::string CrtnStr = (CrtBits + "/crtn.o").str();
  SmallVector<std::string, 2> ObjectStrs;

  SmallVector<const char *, 10> LinkerArgv;

  LinkerArgv.push_back(LinkerProgram.c_str());
  LinkerArgv.push_back("-o");
  LinkerArgv.push_back(FilenameStr.c_str());
  LinkerArgv.push_back("-static");
  if (!GccLikeDriver) {
    LinkerArgv.push_back(Crt1Str.c_str());
    LinkerArgv.push_back(CrtiStr.c_str());
  }
  for (StringRef Object : ObjectFilenames) {
    ObjectStrs.push_back(Object.str());
    LinkerArgv.push_back(ObjectStrs.back().c_str());
  }
  if (!GccLikeDriver) {
    LinkerArgv.push_back(CrtnStr.c_str());
  }
  LinkerArgv.push_back(nullptr);

  // Call linker as external process.
  bool ExecutionFailed;
  std::string ErrorMsg;
  int Res = llvm::sys::ExecuteAndWait(LinkerProgram, LinkerArgv.data(),
                                      /*env*/ nullptr, /*Redirects*/ nullptr,
                                      /*secondsToWait*/ 0, /*memoryLimit*/ 0,
                                      &ErrorMsg, &ExecutionFailed);

  if (!ErrorMsg.empty()) {
    assert(Res < 0 && "Error string set with non-negative result");
    if (Res == -1) {
      return makeALLVMLinkerError("Failed to invoke the linker: " + ErrorMsg);
    } else {
      assert(Res == -2 && "Unexpected result");
      return makeALLVMLinkerError("Linker process crashed or timed out: " +
                                  ErrorMsg);
    }
  }

  if (Res > 0) {
    return makeALLVMLinkerError("Linking failed");
  }

  assert(Res == 0 && "Error string not set with negative result");
  return Error::success();
}

Error LldLinker::link(const SmallVectorImpl<StringRef> &ObjectFilenames,
                      StringRef CrtBits, StringRef Filename) const {

  // Create driver arguments.
  SmallVector<const char *, 5> LinkerArgs;

  LinkerArgs.push_back("lld");

  LinkerArgs.push_back("-o");

  std::string FilenameStr = Filename.str();
  LinkerArgs.push_back(FilenameStr.c_str());

  SmallVector<std::string, 2> ObjectStrs;
  for (StringRef Object : ObjectFilenames) {
    ObjectStrs.push_back(Object.str());
    LinkerArgs.push_back(ObjectStrs.back().c_str());
  }

  // Call lld entry point.
  bool LinkingDone = false; // lld::elf::link(LinkerArgs);

  if (!LinkingDone) {
    return makeALLVMLinkerError("Linking failed");
  }

  return Error::success();
}

} // end namespace allvm
