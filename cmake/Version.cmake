# Get a description of the git rev we're building from,
# and embed that information in ALLVMVersion.h

if(NOT GITVERSION)
  include(GetGitRevisionDescription)
  git_describe(GITVERSION --tags --always --dirty)
endif()

if (NOT GITVERSION)
  message(FATAL_ERROR "Unable to find 'git'! Is it installed?")
else()
  message(STATUS "Detected ALLVM Tools source version: ${GITVERSION}")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/ALLVMVersion.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/ALLVMVersion.h
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")


