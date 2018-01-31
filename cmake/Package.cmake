set(CMAKE_CONFIG_DEST "lib/cmake/allvm")

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
  FILE ALLVMConfig.cmake
  DESTINATION ${CMAKE_CONFIG_DEST}
)
