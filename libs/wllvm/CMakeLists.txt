llvm_add_library(wllvm
  WLLVMFile.cpp
  WLLVMLink.cpp
)
llvm_map_components_to_libnames(libwllvm_llvm_libs core bitwriter object irreader linker)
target_link_libraries(wllvm ${libwllvm_llvm_libs})
add_definitions(${LLVM_DEFINITIONS})
