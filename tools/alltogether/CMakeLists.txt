add_executable(alltogether
  alltogether.cpp
)

target_link_libraries(alltogether liball)
llvm_map_components_to_libnames(alltogether_llvm_libs all)
target_link_libraries(alltogether ${alltogether_llvm_libs})
