add_llvm_tool(alld
  alld.cpp
)

target_link_libraries(alld PRIVATE ResourcePaths lldELF lldCommon)

llvm_config(alld)
add_definitions(${LLVM_DEFINITIONS})
