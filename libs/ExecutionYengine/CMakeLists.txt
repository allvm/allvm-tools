# TODO: Trim these!
set(LLVM_LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  ExecutionEngine
  IRReader
  MC
  MIRParser
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
)

add_llvm_library(ExecutionYengine
  JITExecutionYengine.cpp
  LinuxJIT.cpp # select this based on platform
  StaticExecutionYengine.cpp

  LINK_LIBS
  liball
  Cache
  StaticCodeGen
  rt
)

add_definitions(${LLVM_DEFINITIONS})
