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
target_link_libraries(wllvm-extract PRIVATE liball ResourcePaths wllvm Passes)

add_definitions(${LLVM_DEFINITIONS})
