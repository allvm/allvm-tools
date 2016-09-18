set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  BitWriter
  Core
  ExecutionEngine
  MCJIT
  Option
)

add_llvm_tool(alley
  ExecutorMain.cpp
  HostedImageExecutor.cpp
  ImageCache.cpp
  ImageExecutor.cpp
  StaticBinaryCache.cpp
)
target_link_libraries(alley liball StaticCodeGen)

add_definitions(${LLVM_DEFINITIONS})
