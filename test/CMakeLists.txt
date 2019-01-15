
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

# Surely there's a better way?
list(APPEND ALLVM_TEST_DEPENDS
  all-info
  alley
  allmux
  allopt
  allready
  alltogether
  bc2allvm
  none
  wllvm-dump
  wllvm-extract
)
if ( LLD_AVAILABLE_AS_LIB )
  list(APPEND ALLVM_TEST_DEPENDS alld)
endif()
set(ALLVM_TEST_PARAMS
  allvm_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
)
set(LIT_REPORT ${CMAKE_CURRENT_BINARY_DIR}/report.xml)
set(ALLVM_TEST_EXTRA_ARGS -sv --xunit-xml-output=${LIT_REPORT})

set(LLVM_EXTERNAL_LIT ${CMAKE_SOURCE_DIR}/third_party/lit/lit.py)

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
