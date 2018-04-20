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
  orcJIT
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
)

add_allvm_library(ExecutionYengine
  JITExecutionYengine.cpp
  LinuxJIT.cpp # select this based on platform
  StaticExecutionYengine.cpp
  orcJIT.cpp
   
  LINK_LIBS
  liball
  Cache
  StaticCodeGen
  rt
)

add_definitions(${LLVM_DEFINITIONS})
