include(pandocology)

add_custom_target(docs DEPENDS allvm-format)

add_document(
  TARGET
    allvm-format
  OUTPUT_FILE
    ALLVMFormat.pdf
  SOURCES
    ALLVMFormat-Overview.md
    ALLVMFormat-Design.md
    ALLVMFormat-NonGoals.md
    ALLVMFormat-Contents.md
    ALLVMFormat-Platform.md
    ALLVMFormat-TODO.md
  PANDOC_DIRECTIVES
    -t latex
    --tab-stop=2
    --number-sections
    -V documentclass:scrartcl
    -V geometry:margin=1in
    -V classoption:twocolumn
    -V title:"ALLVM Package Format Design and Specification"
    -V subtitle:"Version: ${GITVERSION}"
)

install(FILES ${CMAKE_BINARY_DIR}/product/ALLVMFormat.pdf DESTINATION share/doc/allvm)
