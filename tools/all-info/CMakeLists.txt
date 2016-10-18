set(LLVM_LINK_COMPONENTS
  Support
)

add_llvm_tool(all-info
  all-info.cpp
)

target_link_libraries(all-info liball Context)

add_definitions(${LLVM_DEFINITIONS})
