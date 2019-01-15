add_llvm_tool(alld
  alld.cpp
)

target_link_libraries(alld PRIVATE ResourcePaths lldELF lldCommon)

llvm_config(alld
# These are taken directly from lld/ELF/CMakeLists.txt:
#${LLVM_TARGETS_TO_BUILD}
  #  BinaryFormat
  #  BitWriter
  #  Core
  #  DebugInfoDWARF
  #  LTO
  #  MC
  #  Object
  #  Option
  #  Support

  #DEPENDS
  #ELFOptionsTableGen
)

add_definitions(${LLVM_DEFINITIONS})
