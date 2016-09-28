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

add_llvm_library(StaticCodeGen
  StaticCodeGen.cpp
  LINK_LIBS liball
)

add_definitions(${LLVM_DEFINITIONS})
