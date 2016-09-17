
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
find_program(LIT_COMMAND lit)
if (EXISTS ${LLVM_INSTALL_PREFIX}/lit/lit.py)
  set(LIT_COMMAND ${LLVM_INSTALL_PREFIX}/lit/lit.py)
endif()
if (NOT LIT_COMMAND) # if it ends in the suffix -NOTFOUND -> false! Marvelous.
  message(FATAL_ERROR "
  Unable to find location of LLVM lit!

  If you have it available please either add it to your PATH or specify its location
  by setting the 'LIT_COMMAND' option:
    cmake ... -DLIT_COMMAND=/path/to/llvm-lit
      OR
    cmake ... -DLIT_COMMAND=/path/to/lit
      OR
    cmake ... -DLIT_COMMAND=/path/to/llvm-source/utils/lit/lit.py

  Alternatively you may install it as a python package:
    pip install lit
      OR
    easy_install /path/to/llvm-source/utils/lit

    (If you're wondering why it isn't install with LLVM, see: http://llvm.org/PR8496)
  ")
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
