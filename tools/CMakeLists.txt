add_subdirectory(alley)
add_subdirectory(alltogether)
add_subdirectory(wllvm)

add_custom_target(bc2allvm ALL)
add_custom_command(
  TARGET bc2allvm POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/bc2allvm
          ${CMAKE_CURRENT_BINARY_DIR}/bc2allvm)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES bc2allvm)
