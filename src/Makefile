# Makefile for fsck.x

ifeq ($(notdir $(CURDIR)),src)
$(error do not execute make in src directory)
endif

U8TOSJ = u8tosj
SRCDIR_MK = ../srcdir.mk
SRC_DIR = ../src
-include $(SRCDIR_MK)

CC	= gcc
CFLAGS	= -O -Wall -fomit-frame-pointer -fstrength-reduce -D__DOS_INLINE__ -D__IOCS_INLINE__ \
	 -DFSCK


SRCS = main.c disk.c sector.c root.c fat.c dir.c dirsub.c collect.c malloc.c print.c error.c
OBJS = $(subst .c,.o,$(SRCS))

TARGETS = fsck.x


.PHONY : all clean archive

all: $(TARGETS)

clean:
	rm -f $(TARGETS) $(OBS)


archive: fsck_p.zip

fsck_p.zip: fsck.x README.txt CHANGELOG.txt
	strip fsck.x
	rm -f $@
	zip -9 $@ $^

#README.txt: $(SRC_DIR)/../README.md
#	$(U8TOSJ) < $^ >! $@
#
#CHANGELOG.txt: $(SRC_DIR)/../CHANGELOG.md
#	$(U8TOSJ) < $^ >! $@

%.txt: $(SRC_DIR)/../%.md
	$(U8TOSJ) < $^ >! $@


fsck.x: $(OBJS)
	$(CC) -o $@ $^


collect.o : collect.c fsck.h config.h dir.h collect.h 
dir.o : dir.c fsck.h config.h dir.h collect.h 
dirsub.o : dirsub.c fsck.h config.h dir.h 
disk.o : disk.c fsck.h config.h 
dump.o : dump.c fsck.h config.h 
error.o : error.c fsck.h config.h 
fat.o : fat.c fsck.h config.h 
main.o : main.c fsck.h config.h 
malloc.o : malloc.c fsck.h config.h 
print.o : print.c fsck.h config.h 
root.o : root.c fsck.h config.h dir.h 
sector.o : sector.c fsck.h config.h 


# EOF
