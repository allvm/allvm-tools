
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
)

# Surely there's a better way?
list(APPEND ALLVM_TEST_DEPENDS
  wllvm-extract
  wllvm-dump
  alley
  alltogether
  bc2allvm
  none
)
set(ALLVM_TEST_PARAMS
  allvm_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
)
set(LIT_REPORT ${CMAKE_CURRENT_BINARY_DIR}/report.xml)
set(ALLVM_TEST_EXTRA_ARGS -sv --xunit-xml-output=${LIT_REPORT})

# try to find 'lit'
find_program(LIT_COMMAND llvm-lit)
if (EXISTS ${LLVM_INSTALL_PREFIX}/lit/lit.py)
  set(LIT_COMMAND ${LLVM_INSTALL_PREFIX}/lit/lit.py)
endif()
if (NOT LIT_COMMAND)
  set(LIT_COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/utils/lit/lit.py)
endif()

# Check we have FileCheck utility, as indicator of the LLVM utilities.
find_program(FILECHECK_PATH FileCheck HINTS ${LLVM_TOOLS_BINARY_DIR})
if (NOT FILECHECK_PATH)
  message(STATUS "LLVM Tools path: ${LLVM_TOOLS_BINARY_DIR}")
  message(FATAL_ERROR "
     Unable to find required 'FileCheck' utility.
     Check that LLVM_INSTALL_UTILS was enabled when you built LLVM.")
else()
  message(STATUS "Found FileCheck: ${FILECHECK_PATH}")
endif()

add_lit_testsuite(check "Running ALLVM regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  PARAMS ${ALLVM_TEST_PARAMS}
  DEPENDS ${ALLVM_TEST_DEPENDS}
  ARGS ${ALLVM_TEST_EXTRA_ARGS}
)
add_lit_testsuites(ALLVM ${CMAKE_CURRENT_SOURCE_DIR}
  PARAMS ${ALLVM_TEST_PARAMS}
  DEPENDS ${ALLVM_TEST_DEPENDS}
)

message(STATUS "Using lit command: ${LIT_COMMAND}")
