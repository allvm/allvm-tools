cmake_minimum_required (VERSION 3.6)

project (ALLVM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-Wall)

find_package(LLVM 4.0.0 REQUIRED CONFIG)
if(NOT LLVM_CONFIG_HAS_RTTI)
  list(APPEND CMAKE_CXX_FLAGS -fno-rtti)
endif()
link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

include_directories ("${PROJECT_SOURCE_DIR}/include")

add_subdirectory (libs)
add_subdirectory (tools)
