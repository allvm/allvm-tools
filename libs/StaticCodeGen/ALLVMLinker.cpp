#include "allvm/ALLVMLinker.h"

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

void ALLVMLinker::createLinkerArguments(
    ArrayRef<StringRef> ObjectFilenames,
    Optional<StringRef> CrtBits, StringRef OutFilename,
    SmallVectorImpl<std::string> &LinkerArgs) const {

  LinkerArgs.emplace_back("-o");
  LinkerArgs.emplace_back(OutFilename.str());
  LinkerArgs.emplace_back("-static");
  LinkerArgs.emplace_back("--eh-frame-hdr");
  if (CrtBits) {
    LinkerArgs.emplace_back((CrtBits.getValue() + "/crt1.o").str());
    LinkerArgs.emplace_back((CrtBits.getValue() + "/crti.o").str());
  }
  for (StringRef Object : ObjectFilenames) {
    LinkerArgs.emplace_back(Object.str());
  }
  if (CrtBits) {
    LinkerArgs.emplace_back((CrtBits.getValue() + "/crtn.o").str());
  }
}

Error ALLVMLinker::callLinkerAsExternalProcess(StringRef LinkerProgram,
                                               ArrayRef<StringRef> LinkerArgv) const {

  bool ExecutionFailed;
  std::string ErrorMsg;
  int Res = llvm::sys::ExecuteAndWait(LinkerProgram, LinkerArgv,
                                      /*env*/ None, /*Redirects*/ {},
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

PathLinker::PathLinker(llvm::StringRef LinkerName)
    : Linker(LinkerName),
      GccLikeDriver(LinkerName == "gcc" || LinkerName == "clang" ||
                    LinkerName == "cc") {}

Error PathLinker::link(const SmallVectorImpl<StringRef> &ObjectFilenames,
                       StringRef CrtBits, StringRef OutFilename) const {
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
  SmallVector<std::string, 8> LinkerArgs;
  Optional<StringRef> CrtBitsOption(None);
  if (!GccLikeDriver) {
    CrtBitsOption = CrtBits;
  }
  createLinkerArguments(ObjectFilenames, CrtBitsOption, OutFilename,
                        LinkerArgs);
  SmallVector<StringRef, 10> LinkerArgv;

  LinkerArgv.push_back(LinkerProgram);

  for (auto &Arg: LinkerArgs)
    LinkerArgv.push_back(Arg);

  // Call linker as external process.
  return callLinkerAsExternalProcess(LinkerProgram, LinkerArgv);
}

InternalLinker::InternalLinker(StringRef AlldPath) : Alld(AlldPath) {}

Error InternalLinker::link(const SmallVectorImpl<StringRef> &ObjectFilenames,
                           StringRef CrtBits, StringRef OutFilename) const {

  // Create linker arguments.
  SmallVector<std::string, 8> LinkerArgs;
  Optional<StringRef> CrtBitsOption(CrtBits);
  createLinkerArguments(ObjectFilenames, CrtBitsOption, OutFilename,
                        LinkerArgs);

  SmallVector<StringRef, 10> LinkerArgv;
  LinkerArgv.push_back(Alld);

  for (auto &Arg: LinkerArgs)
    LinkerArgv.push_back(Arg);

  // Call linker as external process.
  return callLinkerAsExternalProcess(Alld, LinkerArgv);
}

} // end namespace allvm
