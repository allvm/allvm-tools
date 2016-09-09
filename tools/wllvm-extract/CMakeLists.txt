set(LLVM_LINK_COMPONENTS
  BitWriter
  Core
  IRReader
  Linker
  Object
)

add_llvm_tool(wllvm-extract
  wllvm-extract.cpp
)
target_link_libraries(wllvm-extract liball wllvm)

add_definitions(${LLVM_DEFINITIONS})
