set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Core
)

add_allvm_library(Passes
  DeInlineAsm.cpp
)

add_definitions(${LLVM_DEFINITIONS})
