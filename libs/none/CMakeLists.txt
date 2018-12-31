enable_language(ASM)

# TODO: read from 'VERSION' file
set(MUSL_VERSION 1.1.20)

set(MUSL "musl-${MUSL_VERSION}")

set(MUSL_SRC "${PROJECT_SOURCE_DIR}/third_party/musl")
set(MUSL_PATCHES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/musl-patches")

set(MUSL_PATCHES
  #${MUSL_PATCHES_DIR}/0005-Another-weak_alias-victim-destructors-for-stdio-and-.patch
  ${MUSL_PATCHES_DIR}/0009-configure-don-t-force-disable-unwind-tables-let-TC-d.patch
  ${MUSL_PATCHES_DIR}/0010-configure-don-t-add-fomit-frame-pointer.patch
)

set(MUSL_CFLAGS_EXTRA "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # Bug in LLVM 7.0 causes compiler to crash!
  #if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
  #  list(APPEND MUSL_PATCHES
  #    ${MUSL_PATCHES_DIR}/0001-Build-with-fPIC-large-code-model-to-workaround-RTDyL.patch
  #  )
  #endif()
  # Don't complain about 'unused' arguments
  # This happens when building assembly files using same CFLAGS used for C files,
  # lots of them don't make any sense for assembly code so they go unused.
  set(MUSL_CFLAGS_EXTRA "-Qunused-arguments")
  # Disable other warnings that are stylistic than actual problems
  set(MUSL_CFLAGS_EXTRA "${MUSL_CFLAGS_EXTRA} -Wno-parentheses -Wno-tautological-compare")
  set(MUSL_CFLAGS_EXTRA "${MUSL_CFLAGS_EXTRA} -Wno-string-plus-int")
  # Further disable warnings about unsupported attrs/pragmas, in theory musl supports clang.
  set(MUSL_CFLAGS_EXTRA "${MUSL_CFLAGS_EXTRA} -Wno-ignored-attributes -Wno-unknown-pragmas")
endif()

set(MUSL_CFLAGS_EXTRA "${MUSL_CFLAGS_EXTRA} -fstack-protector-all")

set(MUSL_CFLAGS "-fPIC ${MUSL_CFLAGS_EXTRA}" CACHE string "CFLAGS for building musl")

set(MUSL_PATCH_COMMANDS COMMAND echo "Applying musl patches...")

foreach(patch ${MUSL_PATCHES})
  list(APPEND MUSL_PATCH_COMMANDS
    COMMAND echo "Applying patch '${patch}'..."
    COMMAND patch -p1 -d "${MUSL}" -i ${patch}
  )
endforeach(patch)

add_custom_command(OUTPUT ${MUSL}/config.mak
  COMMAND rm -rf "${MUSL}"
  COMMAND cp -r "${MUSL_SRC}" "${MUSL}"
  ${MUSL_PATCH_COMMANDS}
  COMMAND cd ${MUSL} && CC=${CMAKE_C_COMPILER} ./configure --disable-shared CFLAGS=${MUSL_CFLAGS} --enable-debug
  MAIN_DEPENDENCY ${MUSL_TARBALL}
  DEPENDS ${MUSL_PATCHES}
  COMMENT "Configuring ${MUSL}"
)

# Kludge to handle parallelism when using make,
# without breaking compat when using other generators (like ninja)
if ("${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles")
  set(MAKECMD $(MAKE) AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB})
else()
  set(MAKECMD make -j8 AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB})
endif()

add_custom_command(OUTPUT ${MUSL}/lib/libc.a
  COMMAND hardeningDisable=stackprotector ${MAKECMD} -C ${MUSL}
  DEPENDS ${MUSL}/config.mak
  COMMENT "Building ${MUSL}..."
)
add_custom_target(musl DEPENDS ${MUSL}/lib/libc.a)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${MUSL}")

# Compile our little assembly bits into a library, then merge in musl's libc.a.
add_library(none STATIC unwind/UnwindRegistersRestore.S unwind/UnwindRegistersSave.S dso_handle.S dynamic.S)
install(TARGETS none DESTINATION lib)
set_output_directory(none BINARY_DIR ${LLVM_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${LLVM_LIBRARY_OUTPUT_INTDIR})
add_dependencies(none musl)
# http://stackoverflow.com/questions/3821916/how-to-merge-two-ar-static-libraries-into-one
configure_file(merge.ar.in merge.ar)
add_custom_command(TARGET none POST_BUILD
  COMMAND ${CMAKE_AR} -M <merge.ar
  COMMAND mv libnone.a.merged ${LLVM_LIBRARY_OUTPUT_INTDIR}/libnone.a
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded ${LLVM_LIBRARY_OUTPUT_INTDIR}/libnone.a
  DEPENDS merge.ar
)

# Install crt bits from musl, since they are useful for static linking.
set(MUSL_ABS ${CMAKE_CURRENT_BINARY_DIR}/${MUSL})
set(CRTFILES
  ${MUSL_ABS}/lib/crt1.o
  ${MUSL_ABS}/lib/crti.o
  ${MUSL_ABS}/lib/crtn.o
  ${MUSL_ABS}/lib/rcrt1.o
  ${MUSL_ABS}/lib/Scrt1.o
)
add_custom_command(TARGET musl POST_BUILD
  COMMAND mkdir -p ${LLVM_LIBRARY_OUTPUT_INTDIR}/crt
  COMMAND cp -t ${LLVM_LIBRARY_OUTPUT_INTDIR}/crt ${CRTFILES}
)
set_property(DIRECTORY APPEND PROPERTY
  ADDITIONAL_MAKE_CLEAN_FILES "${LLVM_LIBRARY_OUTPUT_INTDIR}/crt")
install(FILES ${CRTFILES} DESTINATION lib/crt)
