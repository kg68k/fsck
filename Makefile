# Makefile for fsck (convert source code from UTF-8 to Shift_JIS)
#   Do not use non-ASCII characters in this file.

MKDIR_P = mkdir -p
U8TOSJ = u8tosj

SRCDIR_MK = srcdir.mk
SRC_DIR = src
-include $(SRCDIR_MK)

BLD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*)
SJ_SRCS = $(subst $(SRC_DIR)/,$(BLD_DIR)/,$(SRCS))


.PHONY: all clean

all: directories $(SJ_SRCS)

directories: $(BLD_DIR)

# Do not use $(SRCDIR_MK) as the target name to prevent automatic remaking of the makefile.
srcdir_mk:
	rm -f $(SRCDIR_MK)
	echo "SRC_DIR = $(CURDIR)/src" > $(SRCDIR_MK)

$(BLD_DIR):
	$(MKDIR_P) $@

# convert src/* (UTF-8) to build/* (Shift_JIS)
$(BLD_DIR)/%: $(SRC_DIR)/%
	$(U8TOSJ) < $^ >! $@


clean:
	rm -f $(SJ_SRCS)

# EOF
