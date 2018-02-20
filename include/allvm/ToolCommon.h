#ifndef ALLVM_TOOL_COMMON_H
#define ALLVM_TOOL_COMMON_H

#include "allvm/GitVersion.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

namespace {

class ALLVMTool {
  std::string Name, Overview;
  std::string CatName{Name + " options"};
  llvm::cl::OptionCategory ALLVMOptCat{CatName};

  auto getVersionPrinter() {
    static std::string CapturedName = Name;
    return []() {
      llvm::raw_ostream &OS = llvm::outs();
      OS << CapturedName << " (ALLVM Tools) " << allvm::getALLVMVersion()
         << "\n";
      OS << "  ALLVM Project (http://allvm.org)\n";
      OS << "  LLVM version " << LLVM_VERSION_STRING << "\n";

      // Following is mostly from lib/Support/CommandLine.cpp
      OS << "  ";
#ifndef __OPTIMIZE__
      OS << "DEBUG build";
#else
      OS << "Optimized build";
#endif
#ifndef NDEBUG
      OS << " with assertions";
#endif
      std::string CPU = llvm::sys::getHostCPUName();
      if (CPU == "generic")
        CPU = "(unknown)";
      OS << ".\n"
         << "  Default target: " << llvm::sys::getDefaultTargetTriple() << '\n'
         << "  Host CPU: " << CPU << '\n';

    };
  }

public:
  ALLVMTool(llvm::StringRef Name, llvm::StringRef Overview = "")
      : Name(Name), Overview(Overview) {
    llvm::cl::SetVersionPrinter(getVersionPrinter());
  }

  bool parseCLOpts(int argc, const char *const *argv) {
    llvm::cl::HideUnrelatedOptions(ALLVMOptCat);
    return llvm::cl::ParseCommandLineOptions(argc, argv, Overview);
  }

  auto getCat() { return llvm::cl::cat(ALLVMOptCat); }
  const auto &getName() { return Name; }
  const auto &getOverview() { return Overview; }
};

} // end anonymous namespace

#endif // ALLVM_TOOL_COMMON_H
