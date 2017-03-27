include(pandocology)

add_document(
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
    --smart
    --tab-stop=2
    --number-sections
    -V documentclass:scrartcl
    -V geometry:margin=1in
    -V classoption:twocolumn
    -V title:"ALLVM Package Format Design and Specification"
    -V subtitle:"Version: ${GITVERSION}"
  NO_EXPORT_PRODUCT
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ALLVMFormat.pdf DESTINATION share/doc)
