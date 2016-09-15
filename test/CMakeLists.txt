
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
)
set(ALLVM_TEST_PARAMS
  allvm_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
)
set(LIT_REPORT ${CMAKE_CURRENT_BINARY_DIR}/report.xml)
set(ALLVM_TEST_EXTRA_ARGS -sv --xunit-xml-output=${LIT_REPORT})

# try to find 'lit'
find_program(LIT_COMMAND llvm-lit)
find_program(LIT_COMMAND lit)

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
