set(LLVM_LINK_COMPONENTS
  Core
  LTO
  ${LLVM_TARGETS_TO_BUILD}
)

add_llvm_tool(alltogether
  alltogether.cpp
)
target_link_libraries(alltogether liball)

add_definitions(${LLVM_DEFINITIONS})
