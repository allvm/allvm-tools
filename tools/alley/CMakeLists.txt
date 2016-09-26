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
  JITCache.cpp
  JITExecutionYengine.cpp
  LinuxJIT.cpp # select this based on platform
  StaticBinaryCache.cpp
  StaticExecutionYengine.cpp
)
target_link_libraries(alley liball StaticCodeGen)
target_link_libraries(alley rt)

add_definitions(${LLVM_DEFINITIONS})
