enable_language(ASM)

set(MUSL musl-1.1.15)

set(MUSL_PATCHES
  musl-patches/0001-Build-with-fPIC-to-workaround-RTDyLd-relocation-limi.patch
  musl-patches/0002-Disable-vdso-support-on-x86_64-we-don-t-support-it-y.patch
  musl-patches/0003-Remove-simple-malloc-gets-in-way-of-real-malloc.patch
  musl-patches/0004-KLUDGE-More-workaround-weak-alias-this-time-tackling.patch
)

add_custom_command(OUTPUT ${MUSL}/config.mak
  COMMAND rm -rf "${MUSL}"
  COMMAND tar xf "${CMAKE_CURRENT_SOURCE_DIR}/${MUSL}.tar.gz"
  COMMAND for PATCH in ${MUSL_PATCHES} \; do patch -p1 -d "${MUSL}" -i "${CMAKE_CURRENT_SOURCE_DIR}/$$PATCH" \; done
  COMMAND cd ${MUSL} && CC=${CMAKE_C_COMPILER} ./configure --disable-shared
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
add_dependencies(none musl)
# http://stackoverflow.com/questions/3821916/how-to-merge-two-ar-static-libraries-into-one
configure_file(merge.ar merge.ar)
add_custom_command(TARGET none POST_BUILD
  COMMAND ar -M <merge.ar
  COMMAND mv libnone.a.merged libnone.a
  DEPENDS merge.ar
)
