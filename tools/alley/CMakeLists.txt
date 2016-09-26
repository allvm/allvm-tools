set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  BitReader
  BitWriter
  Core
  ExecutionEngine
  MC
  MCJIT
  MCParser
  Object
  Option
  Support
)

add_llvm_tool(alley
  alley.cpp
  HostedImageExecutor.cpp
  JIT.cpp
  JITCache.cpp
  ImageExecutor.cpp
  StaticBinaryCache.cpp
  Static.cpp
)
target_link_libraries(alley liball StaticCodeGen)
target_link_libraries(alley rt)

add_definitions(${LLVM_DEFINITIONS})
