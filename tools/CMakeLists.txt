add_subdirectory(alley)
add_subdirectory(alltogether)
add_subdirectory(wllvm-extract)
add_subdirectory(wllvm-dump)

add_custom_target(bc2allvm ALL)
add_custom_command(
  TARGET bc2allvm POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/bc2allvm
          ${LLVM_RUNTIME_OUTPUT_INTDIR}/bc2allvm
)
install(PROGRAMS bc2allvm DESTINATION bin)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${LLVM_RUNTIME_OUTPUT_INTDIR}/bc2allvm)
