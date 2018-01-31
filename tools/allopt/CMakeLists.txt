set(LLVM_LINK_COMPONENTS
  BitReader
  BitWriter
  Core
  Support
)

add_llvm_tool(allopt
  allopt.cpp
)

target_link_libraries(allopt PRIVATE liball ResourcePaths)

add_definitions(${LLVM_DEFINITIONS})
