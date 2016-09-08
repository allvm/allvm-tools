# Normally the target would be named "all", but that's reserved by CMake (for
# the default "make all" target). So we name the target "liball" and set the
# actual output name to "all".
add_library(liball Allexe.cpp)
set_target_properties(liball PROPERTIES OUTPUT_NAME all)
target_link_libraries(liball archive-rw)
