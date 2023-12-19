/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/dirsub.c,v $
 * $Author: kawamoto $
 * $Revision: 1.3 $
 * $Date: 1993/10/24 13:18:45 $
 * $Log: dirsub.c,v $
 *	Revision 1.3  1993/10/24  13:18:45  kawamoto
 *	lost+found 関係のバグフィックス
 *	-isLittleEndian サポート
 *
 *	Revision 1.2  1993/05/15  07:50:12  kawamoto
 *	ちょっとだけ修正
 *
 *	Revision 1.1  1993/03/26  22:47:23  src
 *	version 0.05a
 *
 *	Revision 2.3  1993/01/09  09:46:01  kawamoto
 *	version 0.5
 *
 *	Revision 2.2  1993/01/05  17:42:23  kawamoto
 *	デバッグオプションを外して再コンパイル
 *	「ドライブが用意されていない」エラーを回避
 *
 *	Revision 2.1  1992/12/19  11:42:08  kawamoto
 *	公表バージョン (ver 0.04, rev 2.1)
 *
 *	Revision 1.5  1992/12/19  11:33:35  kawamoto
 *	ver 0.04
 *
 *	Revision 1.4  1992/12/12  11:54:46  kawamoto
 *	version 0.04
 *
 *	Revision 1.3  1992/11/28  11:46:10  kawamoto
 *	version 0.03b
 *
 *	Revision 1.2  1992/11/15  17:29:36  kawamoto
 *	Ver 0.03 without entering bad sectors.
 *
 *	Revision 1.1  1992/11/14  22:26:41  kawamoto
 *	Initial revision
 */

/* Copyright (C) 2023 TcbnErik */

#include <iocslib.h>
#include <jctype.h>

#include "fsck.h"
#include "dir.h"

unsigned long current_datetime(void)
{
  unsigned long datetime;
  unsigned long date, time;
  int value;

  date = DATEBIN(BINDATEGET());
  value = ((date & 0x0fff0000) >> 16) - 1980;
  datetime = (value & 0x7f) << 25;
  value = (date & 0x0000ff00) >> 8;
  datetime += (value & 0x0f) << 21;
  value = date & 0x000000ff;
  datetime += (value & 0x1f) << 16;
  time = TIMEBIN(TIMEGET());
  value = (time & 0x00ff0000) >> 16;
  datetime += (value & 0x1f) << 11;
  value = (time & 0x0000ff00) >> 8;
  datetime += (value & 0x3f) << 5;
  value = time & 0x000000ff;
  datetime += (value & 0x3f) >> 1;
  return datetime;
}

directory *rootdir(disk *disk_ptr)
{
  directory *ptr;
  char *str;

  ptr = Malloc(sizeof(directory));
  ptr->buffer = Malloc(disk_ptr->sector.length);
  ptr->sector = disk_ptr->root.top_sector;
  ptr->offset = 0;
  ptr->total_offset = 0;
  ptr->cluster = 0;
  read_sector(ptr->buffer, disk_ptr, ptr->sector, 1);
  ptr->entry = (direntry *)ptr->buffer;
  ptr->entry_attribute_memo = 0;
  ptr->entry_cluster_memo = 0;
  ptr->offset -= sizeof(direntry);
  ptr->total_offset--;
  ptr->entry--;
  str = Malloc(3);
  str[0] = 'a' + disk_ptr->drive_no;
  str[1] = ':';
  str[2] = 0;
  ptr->dir_path = str;
  return ptr;
}

directory *subdir(disk *disk_ptr, const char *dir_path, unsigned short cluster)
{
  directory *ptr;

  ptr = Malloc(sizeof(directory));
  ptr->buffer = Malloc(disk_ptr->cluster.length);
  ptr->cluster = cluster;
  ptr->offset = 0;
  ptr->total_offset = 0;
  ptr->sector = -1;
  read_cluster(ptr->buffer, disk_ptr, ptr->cluster, 1);
  ptr->entry = (direntry *)ptr->buffer;
  ptr->entry_attribute_memo = 0;
  ptr->entry_cluster_memo = 0;
  ptr->offset -= sizeof(direntry);
  ptr->total_offset--;
  ptr->entry--;
  ptr->dir_path = dir_path;
  return ptr;
}

void freedir(directory *ptr)
{
  if (ptr->buffer)
    Free(ptr->buffer);
  if (ptr->sector != -1 && ptr->dir_path)
    Free((void *)ptr->dir_path);
  Free(ptr);
}

static int is_illegal_code(int data, int *last_is_kanji)
{
  data &= 0xff;
  if (0 <= data && data <= 0x20)
    return 1;
  if (data == '?' || data == '*' || data == 0x7f)
    return 1;
  if (*last_is_kanji)
    {
      if (SFTJIS((*last_is_kanji << 8) | data) >= 0)
	*last_is_kanji = 0;
      else
	return 1;
    }
  else
    {
      if (iskanji(data))
	*last_is_kanji = data;
    }
  return 0;
}

static int is_legal_filename(direntry *entry)
{
  int offset;
  int data;
  int last_is_kanji;

  data = (unsigned char)entry->fileName[0];
  last_is_kanji = 0;
  if (is_illegal_code((data == 5) ? 0xe5 : data, &last_is_kanji))
    return 0;
  for (offset = 1; offset < 8; ++offset)
    {
      data = (unsigned char)entry->fileName[offset];
      if (data == ' ')
	{
	  if (is_illegal_code('1', &last_is_kanji))
	    return 0;
	  while (++offset < 8)
	    if (entry->fileName[offset] != ' ')
	      return 0;
	  /* 先頭が $00 なら良しとする(Windows の拡張ファイルエントリ対策) */
	  return (entry->fileName2[0] == 0) ? 1 : 0;
	}
      if (is_illegal_code(data, &last_is_kanji))
	return 0;
    }
  for (offset = 0; offset < 10; ++offset)
    {
      data = (unsigned char)entry->fileName2[offset];
      if (data == 0)
	{
	  if (is_illegal_code('1', &last_is_kanji))
	    return 0;
	  /* 先頭が $00 なら良しとする(Windows の拡張ファイルエントリ対策) */
	  if (offset == 0)
	    return 1;
	  while (++offset < 10)
	    if (entry->fileName2[offset] != 0)
	      return 0;
	  return 1;
	}
      if (is_illegal_code(data, &last_is_kanji))
	return 0;
    }
  return !is_illegal_code('1', &last_is_kanji);
}

static int is_legal_ext(direntry *entry)
{
  int offset;
  int data;
  int last_is_kanji;

  last_is_kanji = 0;
  for (offset = 0; offset < 3; ++offset)
    {
      data = (unsigned char)entry->ext[offset];
      if (data == ' ')
	{
	  if (is_illegal_code('1', &last_is_kanji))
	    return 0;
	  while (++offset < 3)
	    if (entry->ext[offset] != ' ')
	      return 0;
	  return 1;
	}
      if (is_illegal_code(data, &last_is_kanji))
	return 0;
    }
  return !is_illegal_code('1', &last_is_kanji);
}

static int is_legal_attr(direntry *entry)
{
  int attr;

  attr = entry->atr;
  if (is_volume(attr))
    {
      if (is_directory(attr))
	return 0;
      if (is_file_or_symlink(attr))
	return 0;
      return 1;
    }
  if (is_directory(attr))
    {
      if (is_file_or_symlink(attr))
	return 0;
      return 1;
    }
  if (is_file(attr))
    return 1;
  return 0;
}

#define NEXT_BUFEND 0
#define NEXT_ENTRYEND 1
#define NEXT_DELETED 2
#define NEXT_IGNORED 3
#define NEXT_NORMALENTRY 4

static int _nextentry(disk *disk_ptr, directory *ptr)
{
  ptr->entry++;
  ptr->offset += sizeof(direntry);
  ptr->total_offset++;
  if (ptr->sector == -1)
    {
      if (ptr->offset == disk_ptr->cluster.length)
	{
	  ptr->offset = 0;
	  if (disk_ptr->FAT.buf[ptr->cluster].refer != FAT_REFER_OK)
	    return NEXT_BUFEND;
	  ptr->cluster = disk_ptr->FAT.buf[ptr->cluster].value;
	  read_cluster(ptr->buffer, disk_ptr, ptr->cluster, 1);
	  ptr->entry = (direntry *)ptr->buffer;
	}
      else if (ptr->buffer == NULL)
	{
	  ptr->buffer = Malloc(disk_ptr->cluster.length);
	  read_cluster(ptr->buffer, disk_ptr, ptr->cluster, 1);
	  ptr->entry = (direntry *)(ptr->buffer + ptr->offset);
	}
    }
  else
    {
      if (ptr->offset == disk_ptr->sector.length)
	{
	  ptr->offset = 0;
	  ptr->sector++;
	  if (ptr->sector == disk_ptr->root.top_sector + disk_ptr->root.sector_num)
	    return NEXT_BUFEND;
	  read_sector(ptr->buffer, disk_ptr, ptr->sector, 1);
	  ptr->entry = (direntry *)ptr->buffer;
	}
      else if (ptr->buffer == NULL)
	{
	  ptr->buffer = Malloc(disk_ptr->sector.length);
	  read_sector(ptr->buffer, disk_ptr, ptr->sector, 1);
	  ptr->entry = (direntry *)(ptr->buffer + ptr->offset);
	}
    }
  ptr->entry_cluster_memo = get_cluster(ptr->entry);
  ptr->entry_attribute_memo = ptr->entry->atr;
  if (ptr->entry->fileName[0] == 0)
    {
      if (ptr->offset + sizeof(direntry) <
	  ((ptr->sector == -1) ? disk_ptr->cluster.length : disk_ptr->sector.length))
	ptr->entry[1].fileName[0] = 0;
      return NEXT_ENTRYEND;
    }
  if (ptr->entry->fileName[0] == (char)0xe5)
    return NEXT_DELETED;
  if (ptr->dir_path)
    ptr->file_path[0] = 0;
  if (is_legal_filename(ptr->entry))
    if (is_legal_ext(ptr->entry))
      {
	if (ptr->dir_path)
	  {
	    direntry *entry;
	    char *name;
	    int offset;

	    entry = ptr->entry;
	    strcpy(ptr->file_path, ptr->dir_path);
	    strcat(ptr->file_path, "/");
	    name = ptr->file_path + strlen(ptr->file_path);
	    *name = entry->fileName[0];
	    if (*name == 0x05)
	      *name = 0xe5;
	    ++name;
	    for (offset = 1; offset < 8 && entry->fileName[offset] != ' '; ++offset)
	      *name++ = entry->fileName[offset];
	    for (offset = 0; offset < 10 && entry->fileName2[offset]; ++offset)
	      *name++ = entry->fileName2[offset];
	    if (entry->ext[0] != ' ')
	      {
		*name++ = '.';
		for (offset = 0; offset < 3 && entry->ext[offset] != ' '; ++offset)
		  *name++ = entry->ext[offset];
	      }
	    *name = 0;
	  }
	if (is_legal_attr(ptr->entry))
	  return NEXT_NORMALENTRY;
      }
  if (ptr->file_path[0] == 0)
    {
      prerr_str("ディレクトリ %s の ", "directory %s has illegal ", ptr->dir_path);
      prerr_int("%d 番目に不正エントリが有ります", "%d th entry", ptr->total_offset + 1);
    }
  else
    prerr_str("%s は不正エントリです", "%s is an illegal entry", ptr->file_path);

  end_record();
  if (flags.writing)
    {
      prerr_("これを削除します", "remove it");
      ptr->entry->fileName[0] = 0xe5;
      writeentry(disk_ptr, ptr);
      end_line();
      return NEXT_DELETED;
    }
  else
    {
      prerr_("これを無視します", "ignore it");
      end_line();
      return NEXT_IGNORED;
    }
}

int nextentry(disk *disk_ptr, directory *ptr)
{
  while (1)
    {
      int flag;

      flag = _nextentry(disk_ptr, ptr);
      if (flag == NEXT_BUFEND)
	break;
      if (flag == NEXT_ENTRYEND)
	break;
      if (flag == NEXT_IGNORED)
	continue;
      if (flag == NEXT_DELETED)
	continue;
#ifdef DEBUG
      if (debug.path)
	{
	  print_str("[%s]", SAME, ptr->file_path);
	  if (!debug.link)
	    end_line();
	}
#endif
      return 1;
    }
  return 0;
}

int nextfreeentry(disk *disk_ptr, directory *ptr)
{
  int flag;

  while (1)
    {
      flag = _nextentry(disk_ptr, ptr);
      if (flag == NEXT_BUFEND)
	break;
      if (flag == NEXT_IGNORED)
	continue;
      if (flag == NEXT_NORMALENTRY)
	continue;
      strcpy(ptr->file_path, ptr->dir_path);
      strcat(ptr->file_path, "/");
      return 1;
    }
  return 0;
}

directory *copyentry(directory *source)
{
  directory *destination;

  destination = Malloc(sizeof(directory));
  destination->buffer = NULL;
  destination->sector = source->sector;
  destination->offset = source->offset;
  destination->total_offset = source->total_offset;
  destination->cluster = source->cluster;
  destination->entry = NULL;
  destination->entry_attribute_memo = source->entry_attribute_memo;
  destination->entry_cluster_memo = source->entry_cluster_memo;
  strcpy(destination->file_path, source->file_path);
  destination->dir_path = NULL;
  return destination;
}

void readentry(disk *disk_ptr, directory *ptr)
{
  ptr->total_offset--;
  ptr->offset -= sizeof(direntry);
  ptr->entry--;
  _nextentry(disk_ptr, ptr);
}

void freeentrybuffer(disk *disk_ptr, directory *ptr)
{
  if (ptr->buffer)
    Free(ptr->buffer);
  ptr->buffer = NULL;
  ptr->entry = NULL;
}

void writeentry(disk *disk_ptr, directory *ptr)
{
  if (ptr->sector == -1)
    write_cluster(ptr->buffer, disk_ptr, ptr->cluster, 1);
  else
    write_sector(ptr->buffer, disk_ptr, ptr->sector, 1);
}
