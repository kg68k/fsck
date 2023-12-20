/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/collect.h,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:46:47 $
 * $Log: collect.h,v $
 * Revision 1.1  1993/03/26  22:46:47  src
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

/* Copyright (C) 2023 TcbnErik */

#ifndef COLLECT_H
#define COLLECT_H

#include "dir.h"
#include "fsck.h"

extern void collect(directory *dir);
extern void adjust(disk *disk_ptr, directory *dir);
extern int is_there_more_duplication(disk *disk_ptr, directory **dir_ptr);
extern int its_duplicators(directory **dir_ptr);
extern int next_collection(directory **dir_ptr);

#endif
