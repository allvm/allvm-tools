set(LLVM_LINK_COMPONENTS
  BitReader
  BitWriter
  Core
  IPO
  Support
)

add_llvm_tool(allmux
  allmux.cpp
  CtorUtils.cpp
)
target_link_libraries(allmux PRIVATE liball Passes ResourcePaths)

set(MM_BC ${CMAKE_CURRENT_SOURCE_DIR}/main/mux_main.bc)
install(FILES ${MM_BC} DESTINATION lib)
add_custom_command(TARGET allmux POST_BUILD
  COMMAND cp -t ${LLVM_LIBRARY_OUTPUT_INTDIR} ${MM_BC}
)

add_definitions(${LLVM_DEFINITIONS})
