add_executable(wllvm-dump
  wllvm-dump.cpp
)

add_executable(wllvm-extract
  wllvm-extract.cpp
)

llvm_map_components_to_libnames(wllvm_llvm_libs core bitwriter object irreader linker)
target_link_libraries(wllvm-dump wllvm ${wllvm_llvm_libs})
target_link_libraries(wllvm-extract liball wllvm ${wllvm_llvm_libs})
