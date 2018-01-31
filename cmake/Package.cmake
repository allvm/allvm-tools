
set(ALLVM_INCLUDE_INSTALL_DIR include)
set(ALLVM_LIB_INSTALL_DIR lib)
set(ALLVM_PREFIX .)

set(CMAKE_CONFIG_DEST "${ALLVM_LIB_INSTALL_DIR}/cmake/allvm")

macro(add_allvm_library name)
  llvm_add_library(${name} ${ARGN})
  install(
    TARGETS ${name}
    EXPORT ALLVM
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    COMPONENT ${name}
  )
endmacro(add_allvm_library name)

install(
  EXPORT ALLVM
  FILE ALLVMTargets.cmake
  DESTINATION ${CMAKE_CONFIG_DEST}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  cmake/ALLVMConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ALLVMConfig.cmake
  INSTALL_DESTINATION ${ALLVM_LIB_INSTALL_DIR}/cmake/allvm
  PATH_VARS ALLVM_INCLUDE_INSTALL_DIR ALLVM_LIB_INSTALL_DIR ALLVM_PREFIX
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ALLVMConfig.cmake
  DESTINATION ${ALLVM_LIB_INSTALL_DIR}/cmake/allvm
)
