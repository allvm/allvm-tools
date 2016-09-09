set(LLVM_LINK_COMPONENTS
  Core
  BitWriter
  IRReader
)
# Normally the target would be named "all", but that's reserved by CMake (for
# the default "make all" target). So we name the target "liball" and set the
# actual output name to "all".
add_llvm_library(liball Allexe.cpp
  OUTPUT_NAME all
  LINK_LIBS PUBLIC archive-rw)
