set(LLVM_LINK_COMPONENTS
  BitReader
  BitWriter
  Core
  IRReader
  Linker
  Object
  Support
)

add_llvm_tool(wllvm-extract
  wllvm-extract.cpp
)
target_link_libraries(wllvm-extract liball ResourcePaths wllvm)

add_definitions(${LLVM_DEFINITIONS})
