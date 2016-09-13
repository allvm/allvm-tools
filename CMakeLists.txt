cmake_minimum_required (VERSION 3.4)

project (ALLVM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-Wall)

find_package(LLVM 4.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include(HandleLLVMOptions)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LLVM_BUILD_TOOLS ON)

# Put libs and bins in expected place.
# XXX: We don't setup CMAKE_CFG_INTDIR to distinguish Release/Debug/etc.
set(LLVM_RUNTIME_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${LLVM_LIBDIR_SUFFIX})

link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

include_directories ("${PROJECT_SOURCE_DIR}/include")

add_subdirectory (libs)
add_subdirectory (tools)

# format checking
# (based on polly's format checking)
# Build list of all clang-format'able files
file(GLOB_RECURSE files include/*.h tools/*.h tools/*.c tools/*.cpp libs/*.h libs/*.c libs/*.cpp)
# But skip files that we didn't write ourselves
file(GLOB_RECURSE archive_files libs/archive-rw/*.c libs/archive-rw/*.h)
file(GLOB_RECURSE musl_files libs/none/musl-*/*)
list(REMOVE_ITEM files ${archive_files} ${build_tree} ${musl_files})

# Command use to format a file
set(CLANGFORMAT_COMMAND clang-format -sort-includes -style=llvm)

# cmake for loop, let's do this
set(i 0)
foreach(file IN LISTS files)
  add_custom_command(OUTPUT allvm-check-format${i}
    COMMAND ${CLANGFORMAT_COMMAND} ${file} | diff -u ${file} -
    VERBATIM
    COMMENT "Checking format of ${file}..."
  )
  list(APPEND check_format_depends "allvm-check-format${i}")

  add_custom_command(OUTPUT allvm-update-format${i}
    COMMAND ${CLANGFORMAT_COMMAND} -i ${file}
    VERBATIM
    COMMENT "Updating format of ${file}..."
  )
  list(APPEND update_format_depends "allvm-update-format${i}")

  math(EXPR i ${i}+1)
endforeach()

add_custom_target(allvm-check-format DEPENDS ${check_format_depends})
set_target_properties(allvm-check-format PROPERTIES FOLDER "ALLVM")

add_custom_target(allvm-update-format DEPENDS ${update_format_depends})
set_target_properties(allvm-update-format PROPERTIES FOLDER "ALLVM")

