# Normally the target would be named "all", but that's reserved by CMake (for
# the default "make all" target). So we name the target "liball" and set the
# actual output name to "all".
llvm_add_library(liball Allexe.cpp
  OUTPUT_NAME all
  LINK_LIBS archive-rw)
