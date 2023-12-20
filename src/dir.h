/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/dir.h,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:47:12 $
 * $Log: dir.h,v $
 * Revision 1.1  1993/03/26  22:47:12  src
 * version 0.05a
 *
 * Revision 2.4  1993/01/19  21:28:41  kawamoto
 * volume 名を認識しないバグをフィックス
 *
 * Revision 2.3  1993/01/09  21:44:08  kawamoto
 * archive attribute が立っていなくても通常ファイルだとみなす
 *
 * Revision 2.2  1993/01/09  09:46:01  kawamoto
 * version 0.5
 *
 * Revision 2.1  1992/12/19  11:42:08  kawamoto
 * 公表バージョン (ver 0.04, rev 2.1)
 *
 * Revision 1.3  1992/12/12  11:54:46  kawamoto
 * version 0.04
 *
 * Revision 1.2  1992/11/15  17:29:36  kawamoto
 * Ver 0.03 without entering bad sectors.
 *
 * Revision 1.1  1992/11/14  22:26:41  kawamoto
 * Initial revision
 */

/* Copyright (C) 2023 TcbnErik */

#ifndef DIR_H
#define DIR_H

#include <string.h>

#include "fsck.h"

typedef struct {
  char fileName[8];
  char ext[3];
  unsigned char atr;
  char fileName2[10];
  unsigned char time[2];
  unsigned char date[2];
  unsigned char fatNo[2];
  unsigned char fileLen[4];
} direntry;

unsigned long current_datetime(void);

static inline unsigned short get_cluster(direntry *entry) {
  return (entry->fatNo[1] << 8) + entry->fatNo[0];
}

static inline int get_length(direntry *entry) {
  return (entry->fileLen[3] << 24) + (entry->fileLen[2] << 16) +
         (entry->fileLen[1] << 8) + (entry->fileLen[0]);
}

static inline unsigned long get_datetime(direntry *entry) {
  return (entry->date[1] << 24) + (entry->date[0] << 16) +
         (entry->time[1] << 8) + (entry->time[0]);
}

static inline void put_cluster(direntry *entry, unsigned short cluster) {
  entry->fatNo[1] = cluster >> 8;
  entry->fatNo[0] = cluster;
}

static inline void put_length(direntry *entry, int length) {
  entry->fileLen[3] = length >> 24;
  entry->fileLen[2] = length >> 16;
  entry->fileLen[1] = length >> 8;
  entry->fileLen[0] = length;
}

static inline void put_datetime(direntry *entry, unsigned long datetime) {
  entry->date[1] = datetime >> 24;
  entry->date[0] = datetime >> 16;
  entry->time[1] = datetime >> 8;
  entry->time[0] = datetime;
}

static inline int is_directory(int atr) { return (atr & 0x10) != 0; }

static inline int is_file(int atr) {
  return ((atr & 0x78) == 0 || (atr & 0x20) != 0);
}

static inline int is_file_or_directory(int atr) {
  return ((atr & 0x78) == 0 || (atr & 0x30) != 0);
}

static inline int is_symlink(int atr) { return (atr & 0x40) != 0; }

static inline int is_file_or_symlink(int atr) {
  return ((atr & 0x78) == 0 || (atr & 0x60) != 0);
}

static inline int is_volume(int atr) { return (atr & 0x08) != 0; }

typedef struct {
  int sector;
  unsigned short cluster;
  int offset;
  int total_offset;
  const char *dir_path;
  direntry *entry;
  unsigned char entry_attribute_memo;
  unsigned short entry_cluster_memo;
  unsigned short batting_cluster_max;
  char *file_path;
  unsigned char *buffer;
} directory;

extern directory *rootdir(disk *disk_ptr);
extern directory *subdir(disk *disk_ptr, const char *dir_path,
                         unsigned short cluster);
extern void freedir(directory *ptr);
extern int nextentry(disk *disk_ptr, directory *ptr);
extern int nextfreeentry(disk *disk_ptr, directory *ptr);
extern directory *copyentry(directory *source);
extern void readentry(disk *disk_ptr, directory *ptr);
extern void freeentrybuffer(disk *disk_ptr, directory *ptr);
extern void writeentry(disk *disk_ptr, directory *ptr);

#endif
