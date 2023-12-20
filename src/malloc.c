/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/malloc.c,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:48:58 $
 * $Log: malloc.c,v $
 * Revision 1.1  1993/03/26  22:48:58  src
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

#include <stdlib.h>

#include "fsck.h"

void *Malloc(int size) {
  char *ptr, *ptr_save;

  ptr = malloc(size);
  if (ptr == 0) {
    prerr_("メモリが足りません", "No memory space");
    cleanup_exit(255);
  }
  ptr_save = ptr;
  while (size--) *ptr++ = 0;
  return ptr_save;
}

void Free(void *ptr) { free(ptr); }
