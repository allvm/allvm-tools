set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Analysis
  AsmPrinter
  CodeGen
  Core
  IRReader
  MC
  MIRParser
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
)

llvm_add_library(StaticCodeGen
  StaticCodeGen.cpp
  LINK_LIBS liball
)
