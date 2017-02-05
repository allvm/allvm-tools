set(LLVM_LINK_COMPONENTS
  Core
  Support
)

add_llvm_library(ResourcePaths
  ResourcePaths.cpp
)

add_definitions(${LLVM_DEFINITIONS})
