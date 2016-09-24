enable_language(ASM)

set(MUSL musl-1.1.15)

set(MUSL_PATCHES
  musl-patches/0002-Disable-vdso-support-on-x86_64-we-don-t-support-it-y.patch
  musl-patches/0003-Remove-simple-malloc-gets-in-way-of-real-malloc.patch
  musl-patches/0004-KLUDGE-More-workaround-weak-alias-this-time-tackling.patch
  musl-patches/0005-Another-weak_alias-victim-destructors-for-stdio-and-.patch
  musl-patches/0006-configure-improve-tryflag-to-reject-ignored-opt-argu.patch
)

set(CFLAGS_QUNUSED "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND MUSL_PATCHES
    musl-patches/0001-Build-with-fPIC-large-code-model-to-workaround-RTDyL.patch
  )
  # Don't complain about 'unused' arguments
  # This happens when building assembly files using same CFLAGS used for C files,
  # lots of them don't make any sense for assembly code so they go unused.
  set(CFLAGS_QUNUSED -Qunused-arguments)
endif()

set(MUSL_CFLAGS -O2 -pipe -fPIC ${CFLAGS_QUNUSED} CACHE string "CFLAGS for building musl")


add_custom_command(OUTPUT ${MUSL}/config.mak
  COMMAND rm -rf "${MUSL}"
  COMMAND tar xf "${CMAKE_CURRENT_SOURCE_DIR}/${MUSL}.tar.gz"
  COMMAND for PATCH in ${MUSL_PATCHES} \; do patch -p1 -d "${MUSL}" -i "${CMAKE_CURRENT_SOURCE_DIR}/$$PATCH" \; done
  COMMAND cd ${MUSL} && CC=${CMAKE_C_COMPILER} ./configure --disable-shared CFLAGS="${MUSL_CFLAGS}"
  MAIN_DEPENDENCY ${MUSL}.tar.gz
  DEPENDS ${MUSL_PATCHES}
  COMMENT "Configuring ${MUSL}"
)
add_custom_command(OUTPUT ${MUSL}/lib/libc.a
  COMMAND $(MAKE) -C ${MUSL}
  DEPENDS ${MUSL}/config.mak
  COMMENT "Building ${MUSL}..."
)
add_custom_target(musl DEPENDS ${MUSL}/lib/libc.a)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MUSL}")

# Compile our little libunwind bits into a library, then merge in musl's libc.a.
add_library(none STATIC UnwindRegistersRestore.S UnwindRegistersSave.S)
install(TARGETS none DESTINATION lib)
set_output_directory(none BINARY_DIR ${LLVM_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${LLVM_LIBRARY_OUTPUT_INTDIR})
add_dependencies(none musl)
# http://stackoverflow.com/questions/3821916/how-to-merge-two-ar-static-libraries-into-one
configure_file(merge.ar.in merge.ar)
add_custom_command(TARGET none POST_BUILD
  COMMAND ar -M <merge.ar
  COMMAND mv libnone.a.merged ${LLVM_LIBRARY_OUTPUT_INTDIR}/libnone.a
  DEPENDS merge.ar
)
