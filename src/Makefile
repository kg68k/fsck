#
# $Project: X68k File System ChecKer $
# $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
# $Source: /home/src/cvs/fsck/Makefile,v $
# $Author: kawamoto $
# $Revision: 1.4 $
# $Date: 1993/12/22 07:56:07 $
# $Log: Makefile,v $
#	Revision 1.4  1993/12/22  07:56:07  kawamoto
#	Ver 1.02a ... エラー表示に、対象セクタ番号範囲を追加
#
#	Revision 1.3  1993/12/12  14:01:35  kawamoto
#	Ver 1.02 メディアバイト 0xfff0 (IBM format MO) に対応化。
#
#	Revision 1.2  1993/10/24  13:18:28  kawamoto
#	lost+found 関係のバグフィックス
#	-isLittleEndian サポート
#
#	Revision 1.1  1993/03/26  22:46:24  src
#	version 0.05a
#
#	Revision 2.3  1993/01/09  21:59:10  kawamoto
#	install を追加。-lhupair を追加。
#
#	Revision 2.2  1993/01/09  09:46:01  kawamoto
#	version 0.5
#
#	Revision 2.1  1992/12/19  11:42:08  kawamoto
#	公表バージョン (ver 0.04, rev 2.1)
#
#	Revision 1.4  1992/12/12  11:54:46  kawamoto
#	version 0.04
#
#	Revision 1.3  1992/11/28  11:52:00  kawamoto
#	version 0.03b
#
#	Revision 1.2  1992/11/15  17:29:36  kawamoto
#	Ver 0.03 without entering bad sectors.
#
#	Revision 1.1  1992/11/14  22:46:24  kawamoto
#	Initial revision
#

MARIKO = A
LOADLIBES = -lhupair -ldos -liocs
CFLAGS = -O -Wall -fomit-frame-pointer -fstrength-reduce
SOURCES = disk.c sector.c root.c fat.c dump.c dir.c dirsub.c collect.c \
	  malloc.c print.c error.c
OBJECTS = $(subst .c,.o,$(SOURCES))

.PHONY : all depend clean breakon install

all : breakon fsck.x dumpfat.x

install : all
	install -cp -m 775 fsck.x /etc/bin/fsck

breakon :
	command break on

depend :
	makedepend $(SOURCES)

clean :
	rm -f $(OBJECTS) fsck.a fsck.x dumpfat.x fsck.o dumpfat.o *.bak *~

fsck.a : $(OBJECTS)
	-$(AR) $(ARFLAGS) $@ $?

fsck.o : main.c fsck.h
	$(COMPILE.c) -DFSCK $< $(OUTPUT_OPTION)

fsck.x : fsck.o fsck.a

dumpfat.o : main.c fsck.h
	$(COMPILE.c) -DDUMPFAT $< $(OUTPUT_OPTION)

dumpfat.x : dumpfat.o fsck.a

# DO NOT DELETE THIS LINE -- make depend depends on it.

disk.o: A:/usr/gcc-include/doslib.h fsck.h config.h
sector.o: A:/usr/gcc-include/doslib.h fsck.h config.h
root.o: fsck.h config.h dir.h A:/usr/gcc-include/string.h
root.o: A:/usr/gcc-include/sys/types.h
fat.o: A:/usr/gcc-include/stdlib.h A:/usr/gcc-include/sys/types.h
fat.o: A:/usr/gcc-include/doslib.h fsck.h config.h
dump.o: A:/usr/gcc-include/stdio.h fsck.h config.h
dir.o: fsck.h config.h dir.h A:/usr/gcc-include/string.h
dir.o: A:/usr/gcc-include/sys/types.h collect.h
dirsub.o: A:/usr/gcc-include/iocslib.h A:/usr/gcc-include/sys/types.h fsck.h
dirsub.o: config.h dir.h A:/usr/gcc-include/string.h
dirsub.o: A:/usr/gcc-include/jctype.h
collect.o: fsck.h config.h dir.h A:/usr/gcc-include/string.h
collect.o: A:/usr/gcc-include/sys/types.h collect.h
malloc.o: A:/usr/gcc-include/stdlib.h A:/usr/gcc-include/sys/types.h fsck.h
malloc.o: config.h
print.o: A:/usr/gcc-include/iocslib.h A:/usr/gcc-include/sys/types.h
print.o: A:/usr/gcc-include/stdio.h fsck.h config.h
error.o: A:/usr/gcc-include/stdlib.h A:/usr/gcc-include/sys/types.h
error.o: A:/usr/gcc-include/stdio.h A:/usr/gcc-include/doslib.h fsck.h
error.o: config.h
