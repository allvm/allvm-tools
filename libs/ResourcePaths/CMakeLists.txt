set(LLVM_LINK_COMPONENTS
  Core
  Support
)

add_allvm_library(ResourcePaths
  ResourcePaths.cpp
)

add_definitions(${LLVM_DEFINITIONS})
