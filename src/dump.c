/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/dump.c,v $
 * $Author: kawamoto $
 * $Revision: 1.2 $
 * $Date: 1993/05/15 07:50:28 $
 * $Log: dump.c,v $
 * Revision 1.2  1993/05/15  07:50:28  kawamoto
 * ちょっとだけ修正
 *
 * Revision 1.1  1993/03/26  22:47:47  src
 * version 0.05a
 *
 * Revision 2.2  1993/01/09  09:46:01  kawamoto
 * version 0.5
 *
 * Revision 2.1  1992/12/19  11:42:08  kawamoto
 * 公表バージョン (ver 0.04, rev 2.1)
 *
 * Revision 1.2  1992/11/15  17:29:36  kawamoto
 * Ver 0.03 without entering bad sectors.
 *
 * Revision 1.1  1992/11/14  22:26:41  kawamoto
 * Initial revision
 */

#include <stdio.h>

#include "fsck.h"

void dump_FAT(disk *disk_ptr) {
  unsigned short no;

  for (no = 0; no < disk_ptr->cluster.num; ++no) {
    if ((no & 255) == 0)
      printf(
          "\n    +0000 0001 0002 0003 0004 0005 0006 0007"
          " 0008 0009 000A 000B 000C 000D 000E 000F\n");
    if ((no & 15) == 0) printf("%04X", no);
    printf(" %04X", disk_ptr->FAT.buf[no].value);
    if ((no & 15) == 15) printf("\n");
  }
  if ((no & 15) != 0) printf("\n");
}

void diff_and_write_FAT(disk *disk_ptr) {
  char buffer[128];

  while (fgets(buffer, 126, stdin)) {
    int data[16], addr, num, offset;

    num = sscanf(buffer, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X",
                 &addr, data + 0, data + 1, data + 2, data + 3, data + 4,
                 data + 5, data + 6, data + 7, data + 8, data + 9, data + 10,
                 data + 11, data + 12, data + 13, data + 14, data + 15);
    if (--num < 1) continue;
    for (offset = 0; offset < num; ++offset) {
      unsigned short no;

      no = addr + offset;
      if (disk_ptr->cluster.num <= no) continue;
      if (disk_ptr->FAT.buf[no].value == data[offset]) continue;
      disk_ptr->FAT.buf[no].value = data[offset];
      write_FAT(disk_ptr, no);
    }
  }
}
