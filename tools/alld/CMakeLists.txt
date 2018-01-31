add_llvm_tool(alld
  alld.cpp
)

target_link_libraries(alld PRIVATE ResourcePaths lldELF lldConfig lldCore)

llvm_config(alld
# These are taken directly from lld/ELF/CMakeLists.txt:
  ${LLVM_TARGETS_TO_BUILD}
  Analysis
  BitReader
  BitWriter
  Codegen
  Core
  DebugInfoDWARF
  Demangle
  IPO
  Linker
  LTO
  Object
  Option
  Passes
  MC
  Support
  Target
  TransformUtils
)

add_definitions(${LLVM_DEFINITIONS})
