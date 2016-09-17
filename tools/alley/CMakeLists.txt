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
  ImageExecutor.cpp
  HostedImageExecutor.cpp
  ImageCache.cpp
  StaticBinaryCache.cpp
)
target_link_libraries(alley liball StaticCodeGen)

add_definitions(${LLVM_DEFINITIONS})
