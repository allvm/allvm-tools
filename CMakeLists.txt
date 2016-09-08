cmake_minimum_required (VERSION 3.6)

project (ALLVM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM 4.0.0 REQUIRED CONFIG)
list(APPEND CMAKE_CXX_FLAGS -fno-rtti) # TODO: what if someone builds llvm with RTTI enabled?
link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

include_directories ("${PROJECT_SOURCE_DIR}/include")

add_subdirectory (libs)
add_subdirectory (tools)

# TODO: -Wall
