set(LLVM_LINK_COMPONENTS
  Core
  Support
)

add_llvm_library(Context
  Context.cpp
)

add_definitions(${LLVM_DEFINITIONS})
