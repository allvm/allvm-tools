set(LLVM_LINK_COMPONENTS
  Core
  BitWriter
  Object
  IRReader
  Linker
)

llvm_add_library(wllvm
  WLLVMFile.cpp
  WLLVMLink.cpp
)

add_definitions(${LLVM_DEFINITIONS})
