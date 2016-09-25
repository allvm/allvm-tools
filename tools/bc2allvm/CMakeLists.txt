set(LLVM_LINK_COMPONENTS
  BitReader
  BitWriter
  Core
  Support
)

add_llvm_tool(bc2allvm
  bc2allvm.cpp
)
target_link_libraries(bc2allvm liball)

add_definitions(${LLVM_DEFINITIONS})
