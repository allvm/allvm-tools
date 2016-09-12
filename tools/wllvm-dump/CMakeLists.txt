set(LLVM_LINK_COMPONENTS
  BitWriter
  Core
  IRReader
  Linker
  Object
)

add_llvm_tool(wllvm-dump
  wllvm-dump.cpp
)
target_link_libraries(wllvm-dump wllvm)

add_definitions(${LLVM_DEFINITIONS})
