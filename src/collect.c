/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/collect.c,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:46:35 $
 * $Log: collect.c,v $
 * Revision 1.1  1993/03/26  22:46:35  src
 * version 0.05a
 *
 * Revision 2.3  1993/01/09  09:46:01  kawamoto
 * version 0.5
 *
 * Revision 2.2  1993/01/05  17:42:23  kawamoto
 * デバッグオプションを外して再コンパイル
 * 「ドライブが用意されていない」エラーを回避
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

#include "collect.h"

#include "dir.h"
#include "fsck.h"

static int collection_num = 0;
static directory **collection_buffer = NULL;
static int buffer_length = 0;

#ifdef DEBUG
void print_collections() {
  int flag;
  int num;
  directory **ptr;

  print_int("collection_num %d", SAME, collection_num);
  end_line();
  flag = 0;
  for (num = 0, ptr = collection_buffer; num < collection_num; ++num, ++ptr) {
    print_int("[address %08X", SAME, (int)*ptr);
    print_int(" sector %d]", SAME, (*ptr)->sector);
    flag = 1;
  }
  if (flag) end_line();
}
#endif

void collect(directory *dir) {
  int num;
  directory **ptr;

  dir = copyentry(dir);
  for (num = 0, ptr = collection_buffer; num < collection_num; ++num, ++ptr)
    if (strcmp((*ptr)->file_path, dir->file_path) == 0) return;
  if (collection_num >= buffer_length) {
    directory **work;
    int work_length;

    work = collection_buffer;
    work_length = buffer_length;
    buffer_length = buffer_length ? buffer_length << 1 : 16;
    collection_buffer =
        (directory **)Malloc(sizeof(directory *) * buffer_length);
    if (work) {
      int offset;

      for (offset = 0; offset < work_length; ++offset)
        collection_buffer[offset] = work[offset];
      Free(work);
    }
  }
  collection_buffer[collection_num++] = dir;
#ifdef DEBUG
  if (debug.collect) {
    print_int("[collect sector %d", SAME, dir->sector);
    print_str(" %s]", SAME, dir->file_path);
  }
#endif
}

void adjust(disk *disk_ptr, directory *dir) {
  int num, offset;
  int length;
  directory **ptr;
  unsigned char *src, *dst;

  if (dir->sector == -1)
    length = disk_ptr->cluster.length;
  else
    length = disk_ptr->sector.length;
  for (num = 0, ptr = collection_buffer; num < collection_num; ++num, ++ptr) {
    if (dir->sector != (*ptr)->sector) continue;
    if (dir->sector == -1)
      if (dir->cluster != (*ptr)->cluster) continue;
    if ((dst = (*ptr)->buffer) == NULL) continue;
    src = dir->buffer;
    for (offset = 0; offset < length; ++offset) *dst++ = *src++;
  }
}

static void set_batting(disk *disk_ptr) {
  int num;
  directory **ptr;

  for (num = 0, ptr = collection_buffer; num < collection_num; ++num, ++ptr) {
    unsigned short cluster;
    unsigned short batting;

    batting = 0;
    cluster = (*ptr)->entry_cluster_memo;
    while (1) {
      if (disk_ptr->FAT.buf[cluster].refered == FAT_REFERED_MANY)
        if (cluster > batting) batting = cluster;
      if (disk_ptr->FAT.buf[cluster].refer != FAT_REFER_OK) break;
      if (disk_ptr->FAT.buf[cluster].loop != FAT_LOOP_NO) break;
      cluster = disk_ptr->FAT.buf[cluster].value;
    }
    (*ptr)->batting_cluster_max = batting;
  }
}

static void sort_by_batting(directory **collection_buffer, int collection_num) {
  unsigned short batting;

  if (collection_num <= 1) return;
  if (collection_num == 2) {
    unsigned short top_batting, bottom_batting;

    top_batting = collection_buffer[0]->batting_cluster_max;
    bottom_batting = collection_buffer[1]->batting_cluster_max;
    batting = (top_batting + bottom_batting) / 2;
  } else {
    unsigned short top_batting, middle_batting, bottom_batting;

    top_batting = collection_buffer[0]->batting_cluster_max;
    middle_batting =
        collection_buffer[(collection_num - 1) / 2]->batting_cluster_max;
    bottom_batting = collection_buffer[collection_num - 1]->batting_cluster_max;
    batting = (top_batting + middle_batting + bottom_batting) / 3;
  }
  {
    int left_num;
    directory **source;
    directory **greaters, **equals, **lesses;
    directory **sptr, **gptr, **eptr, **lptr;

    sptr = source = collection_buffer;
    gptr = greaters = Malloc(sizeof(directory *) * collection_num);
    eptr = equals = Malloc(sizeof(directory *) * collection_num);
    lptr = lesses = Malloc(sizeof(directory *) * collection_num);
    left_num = collection_num;
    while (left_num--) {
      if ((*sptr)->batting_cluster_max < batting)
        *lptr++ = *sptr++;
      else if ((*sptr)->batting_cluster_max == batting)
        *eptr++ = *sptr++;
      else
        *gptr++ = *sptr++;
    }
    sptr = source;
    if (gptr != greaters) {
      int num;

      num = gptr - greaters;
      sort_by_batting(greaters, num);
      gptr = greaters;
      while (num--) *sptr++ = *gptr++;
    }
    if (eptr != equals) {
      int num, num_save;

      num_save = eptr - equals;
      eptr = equals;
      num = num_save;
      while (num--)
        if (is_directory((*eptr)->entry_attribute_memo))
          *sptr++ = *eptr++;
        else
          eptr++;
      eptr = equals;
      num = num_save;
      while (num--)
        if (is_directory((*eptr)->entry_attribute_memo))
          eptr++;
        else
          *sptr++ = *eptr++;
    }
    if (lptr != lesses) {
      int num;

      num = lptr - lesses;
      sort_by_batting(lesses, num);
      lptr = lesses;
      while (num--) *sptr++ = *lptr++;
    }
    Free(greaters);
    Free(equals);
    Free(lesses);
  }
}

static int duplication_offset;

int is_there_more_duplication(disk *disk_ptr, directory **dir_ptr) {
  set_batting(disk_ptr);
  sort_by_batting(collection_buffer, collection_num);
  if (collection_num == 0) return 0;
  if (collection_buffer[0]->batting_cluster_max) {
    *dir_ptr = collection_buffer[0];
    duplication_offset = 1;
    return 1;
  }
  return 0;
}

int its_duplicators(directory **dir_ptr) {
  unsigned short one, theOther;

  if (duplication_offset == collection_num) return 0;
  one = collection_buffer[0]->batting_cluster_max;
  theOther = collection_buffer[duplication_offset]->batting_cluster_max;
  if (one != theOther) return 0;
  *dir_ptr = collection_buffer[duplication_offset++];
  return 1;
}

static int shipout_offset = 0;

int next_collection(directory **dir_ptr) {
  if (shipout_offset == collection_num) return 0;
  *dir_ptr = collection_buffer[shipout_offset++];
  return 1;
}
