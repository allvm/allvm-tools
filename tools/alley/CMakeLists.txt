add_executable(alley
  ExecutorMain.cpp
  ImageExecutor.cpp
  HostedImageExecutor.cpp
  ImageCache.cpp
)

llvm_map_components_to_libnames(alley_llvm_libs
  core option executionengine bitwriter x86codegen x86asmparser x86asmprinter mcjit)
target_link_libraries(alley liball StaticCodeGen ${alley_llvm_libs})
