# TODO: Trim these!
set(LLVM_LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  ExecutionEngine # ObjectCache
  IRReader
  MC
  MIRParser
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
)

add_llvm_library(Cache
  JITCache.cpp
  StaticBinaryCache.cpp
  LINK_LIBS
  StaticCodeGen # CompilationOptions
)

include_directories (${ZLIB_INCLUDE_DIRS})
target_link_libraries (Cache PUBLIC ${ZLIB_LIBRARIES})
