set(LLVM_LINK_COMPONENTS
  BitReader
  Core
  LTO
  MC
  Support
  Target
  ${LLVM_TARGETS_TO_BUILD}
)

add_llvm_tool(alltogether
  alltogether.cpp
)
target_link_libraries(alltogether liball Context)

add_definitions(${LLVM_DEFINITIONS})
