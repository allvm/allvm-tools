add_llvm_tool(alld
  alld.cpp
)

target_link_libraries(alld PRIVATE ResourcePaths lldELF lldCommon)

add_definitions(${LLVM_DEFINITIONS})
