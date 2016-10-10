# Get a description of the git rev we're building from,
# and embed that information in ALLVMVersion.h
include(GetGitRevisionDescription)

git_describe(GITVERSION --tags --always --dirty)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/ALLVMVersion.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/ALLVMVersion.h
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")


