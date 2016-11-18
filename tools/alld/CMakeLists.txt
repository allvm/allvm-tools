add_llvm_tool(alld
  alld.cpp
)

target_link_libraries(alld Context lldELF)

add_definitions(${LLVM_DEFINITIONS})
