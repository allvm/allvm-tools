
PANDOC := pandoc

PANDOC_OPTIONS := --smart --tab-stop=2 --number-sections
#PANDOC_OPTIONS += -V documentclass:paper
PANDOC_OPTIONS += -V documentclass:scrartcl
PANDOC_OPTIONS += -V geometry:margin=1in
PANDOC_OPTIONS += -V classoption:twocolumn
PANDOC_OPTIONS += -V title:"ALLVM Package Format Design and Specification"
PANDOC_OPTIONS += -V date:"$(shell date +'%d %B, %Y')"

ALLVM_FORMAT_SRC := \
	ALLVMFormat-Overview.md \
	ALLVMFormat-Design.md \
	ALLVMFormat-NonGoals.md \
	ALLVMFormat-Contents.md \
	ALLVMFormat-Platform.md \
	ALLVMFormat-TODO.md

TARGETS := ALLVMFormat.pdf

all: $(TARGETS)

ALLVMFormat.pdf: $(ALLVM_FORMAT_SRC)
	$(PANDOC) $(PANDOC_OPTIONS) -t latex -o $@ $^

# For experimenting with out outputs, note 'clean' won't remove these!
ALLVMFormat.%: $(ALLVM_FORMAT_SRC)
	$(PANDOC) $(PANDOC_OPTIONS) -t $* -o $@ $^

clean:
	-rm -f $(TARGETS)
