cmake_minimum_required (VERSION 3.4)

project (ALLVM CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-Wall)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
if (${LLVM_PACKAGE_VERSION} VERSION_LESS 4.0)
  message(FATAL_ERROR "LLVM 4.0 or newer is required")
endif()

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

#### PANDOC / BUILD_DOCS ####
if(NOT PANDOC_EXECUTABLE)
    find_program(PANDOC_EXECUTABLE pandoc)
    mark_as_advanced(PANDOC_EXECUTABLE)
endif()

if (PANDOC_EXECUTABLE)
  set(PANDOC_FOUND 1)
endif()

option(BUILD_DOCS "Build documentation (requires pandoc)" ${PANDOC_FOUND})

if (BUILD_DOCS)
  if(NOT PANDOC_EXECUTABLE)
    message(FATAL_ERROR
      "BUILD_DOCS requested, but pandoc not found.\n"
      "Install Pandoc or set cache variable PANDOC_EXECUTABLE.")
  endif()
  add_subdirectory(docs)
endif()
message(STATUS "Building documentation: ${BUILD_DOCS}")

##########

option(USE_CLANG_WERROR_FLAGS OFF)

if (USE_CLANG_WERROR_FLAGS)
  set(WARNING_FLAGS -Weverything
    -Wno-unknown-warning-option -Wno-c++98-compat -Wno-padded
    -Wno-global-constructors -Wno-exit-time-destructors -Wno-unused-macros)
  list(APPEND LLVM_DEFINITIONS ${WARNING_FLAGS} -Werror)
endif()

option(USE_QEMU_FEXECVE_WORKAROUND "Try to avoid using fexecve() as workaround for use with qemu user emulation. YMMV." OFF)
mark_as_advanced(USE_QEMU_FEXECVE_WORKAROUND)
if (USE_QEMU_FEXECVE_WORKAROUND)
  message(WARNING "Enabling unsupported fexecve() workaround...")
  add_definitions(-DUSE_QEMU_FEXECVE_WORKAROUND)
endif()

find_library(LLD_AVAILABLE_AS_LIB lldELF)
if ( LLD_AVAILABLE_AS_LIB )
  add_definitions(-DALLVM_alld_available)
else()
  message(WARNING
"lld was not built with found LLVM. "
"The internal linker (alld) will not be built.")
endif()


option(USE_GOLD_LINKER_IF_AVAILABLE ON)
if(USE_GOLD_LINKER_IF_AVAILABLE)
  check_cxx_compiler_flag("-fuse-ld=gold" CXX_SUPPORTS_GOLD)
  append_if(CXX_SUPPORTS_GOLD "-fuse-ld=gold"
    CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
endif()

# Put libs and bins in expected place.
# XXX: We don't setup CMAKE_CFG_INTDIR to distinguish Release/Debug/etc.
set(LLVM_RUNTIME_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${LLVM_LIBDIR_SUFFIX})

link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

### Headers

# Generated header path
set(ALLVM_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)

include_directories (${PROJECT_SOURCE_DIR}/include ${ALLVM_INCLUDE_DIR})

install(DIRECTORY include/allvm ${ALLVM_INCLUDE_DIR}/allvm
  DESTINATION include
  COMPONENT allvm-headers
  FILES_MATCHING PATTERN "*.h"
)
###

include(CheckFormat)
include(Package)
include(Version)

add_subdirectory (libs)
add_subdirectory (tools)
add_subdirectory (test)

