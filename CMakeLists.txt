cmake_minimum_required (VERSION 3.4)

project (ALLVM CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-Wall)

find_package(LLVM 4.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(Python_ADDITIONAL_VERSIONS 2.7)
include(FindPythonInterp)
if( NOT PYTHONINTERP_FOUND )
  message(FATAL_ERROR
"Unable to find Python interpreter, required for builds and testing.

Please install Python or specify the PYTHON_EXECUTABLE CMake variable.")
endif()

if( ${PYTHON_VERSION_STRING} VERSION_LESS 2.7 )
  message(FATAL_ERROR "Python 2.7 or newer is required")
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(AddLLVM)
include(HandleLLVMOptions)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LLVM_BUILD_TOOLS ON)

find_package( ZLIB REQUIRED )

option(USE_CLANG_WERROR_FLAGS OFF)

if (USE_CLANG_WERROR_FLAGS)
  set(WARNING_FLAGS -Weverything
    -Wno-unknown-warning-option -Wno-c++98-compat -Wno-padded
    -Wno-global-constructors -Wno-exit-time-destructors)
  list(APPEND LLVM_DEFINITIONS ${WARNING_FLAGS} -Werror)
endif()

find_library(LLD_AVAILABLE_AS_LIB lldELF)
if ( LLD_AVAILABLE_AS_LIB )
  add_definitions(-DALLVM_alld_available)
else()
  message(WARNING
"lld was not built with found LLVM. "
"The internal linker (alld) will not be built.")
endif()

# Put libs and bins in expected place.
# XXX: We don't setup CMAKE_CFG_INTDIR to distinguish Release/Debug/etc.
set(LLVM_RUNTIME_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${LLVM_LIBDIR_SUFFIX})

link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

include_directories ("${PROJECT_SOURCE_DIR}/include")

include(CheckFormat)
include(Version)

add_subdirectory (libs)
add_subdirectory (tools)
add_subdirectory (test)

