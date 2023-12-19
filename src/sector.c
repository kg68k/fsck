/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/sector.c,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:49:41 $
 * $Log: sector.c,v $
 *	Revision 1.1  1993/03/26  22:49:41  src
 *	version 0.05a
 *
 *	Revision 2.2  1993/01/09  09:46:01  kawamoto
 *	version 0.5
 *
 *	Revision 2.1  1992/12/19  11:42:08  kawamoto
 *	公表バージョン (ver 0.04, rev 2.1)
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

#include <doslib.h>
#include "fsck.h"

void check_read_sector(disk *disk_ptr)
{
  int cur_sector;
  int check_length;
  int max_sector;
  int offset;
  unsigned char *dummy;

  if (!flags.check_reading_sectors)
    return;
  if (flags.writing && flags.check_writing_sectors)
    return;
  set_error_mode(1);
  dummy = (unsigned char *)Malloc(32 * disk_ptr->sector.length);
  prhead("セクタ読み込みチェック中です", "Checking sector reading");
  cur_sector = 0;
  check_length = 32;
  max_sector = disk_ptr->sector.num;
  while (cur_sector < max_sector)
    {
      if (cur_sector + check_length > max_sector)
	check_length = max_sector - cur_sector;
#ifdef DEBUG
      if (debug.sector)
	{
	  print_int("チェック %d", "check %d", cur_sector);
	  print_int(" - %d", SAME, cur_sector + check_length - 1);
	  end_line();
	}
#endif
      if (diskred(disk_ptr, dummy, cur_sector, check_length))
	{
	  for (offset = 0; offset < check_length; ++offset)
	    {
#ifdef DEBUG
	      if (debug.sector)
		{
		  print_int("もう一度チェック %d", "more check %d", cur_sector);
		  end_record();
		}
#endif
	      if (diskred(disk_ptr, dummy, cur_sector, 1))
		{
		  prerr_int("不良セクタ %d です (読み込み出来ません)",
			    "bad sector %d (cannot read)", cur_sector);
		  end_line();
		  set_bad_sector(disk_ptr, cur_sector);
		}
	      cur_sector++;
	    }
	}
      else
	cur_sector += check_length;
    }
  Free(dummy);
  end_section();
  set_error_mode(0);
}

void check_write_sector(disk *disk_ptr)
{
  int cur_sector;
  int check_length;
  int check_left_num;
  int max_sector;
  int offset, offset2;
  unsigned char *dummy, *ptr;
  unsigned char *dummy2, *ptr2;

  if (!flags.writing || !flags.check_writing_sectors)
    return;
  set_error_mode(1);
  dummy = (unsigned char *)Malloc(32 * disk_ptr->sector.length);
  dummy2 = (unsigned char *)Malloc(32 * disk_ptr->sector.length);
  if (dummy == 0 || dummy2 == 0)
    return;
  prhead("セクタ書き出しチェック中です", "Checking sector writing");
  cur_sector = 0;
  check_length = 32;
  check_left_num = 1;
  max_sector = disk_ptr->sector.num;
  while (cur_sector < max_sector)
    {
      if (--check_left_num <= 0)
	{
	  check_length = 32;
	  check_left_num = 1;
	}
      if (cur_sector + check_length > max_sector)
	check_length = max_sector - cur_sector;
      for (offset = 0; offset < check_length; ++offset)
	if (is_bad_sector(disk_ptr, cur_sector + offset))
	  check_length = offset;
      if (check_length == 0)
	{
	  ++cur_sector;
	  continue;
	}
#ifdef DEBUG
      if (debug.sector)
	{
	  print_int("チェック %d", "check %d", cur_sector);
	  print_int(" - %d", SAME, cur_sector + check_length - 1);
	  end_line();
	}
#endif
      ptr = dummy;
      for (offset = 0; offset < check_length * disk_ptr->sector.length; ++offset)
	*ptr++ = 0;
      if (diskred(disk_ptr, dummy, cur_sector, check_length) ||
	  ({
	    ptr = dummy;
	    ptr2 = dummy2;
	    for (offset = 0; offset < check_length * disk_ptr->sector.length; ++offset)
	      *ptr2++ = *ptr++ ^ 0xff;
	    0;
	  }) ||
	  diskwrt(disk_ptr, dummy, cur_sector, check_length) ||
	  diskred(disk_ptr, dummy2, cur_sector, check_length))
	{
	  if (check_length != 1)
	    {
	      check_left_num = check_length;
	      check_length = 1;
	      continue;
	    }
	  prerr_int("不良セクタ %d です (書き出し出来ません)",
		    "bad sector %d (cannot write)", cur_sector);
	  end_line();
	  set_bad_sector(disk_ptr, cur_sector);
	  ++cur_sector;
	  continue;
	}
      for (offset = 0; offset < check_length; ++offset)
	{
	  ptr = dummy + offset * disk_ptr->sector.length;
	  ptr2 = dummy2 + offset * disk_ptr->sector.length;
	  for (offset2 = 0; offset2 < disk_ptr->sector.length; ++offset2)
	    if (*ptr++ != *ptr2++)
	      break;
	  if (offset2 < disk_ptr->sector.length)
	    {
	      prerr_int("不良セクタ %d です (読み書きデータ比較エラーです)",
			"bad sector %d (comparison error of r/w data)", cur_sector);
	      end_line();
	      set_bad_sector(disk_ptr, cur_sector);
	    }
	  ++cur_sector;
	}
    }
  Free(dummy);
  Free(dummy2);
  end_section();
  set_error_mode(0);
}

int set_bad_sector(disk *disk_ptr, int sector_offset)
{
  int mask, offset8;

  if (disk_ptr->sector.num <= sector_offset)
    return ERROR_OUT_OF_SECTOR;
  mask = 0x80 >> (sector_offset & 7);
  offset8 = sector_offset >> 3;
  disk_ptr->sector.bads[offset8] |= mask;
  return 0;
}

int is_bad_sector(disk *disk_ptr, int sector_offset)
{
  int mask, offset8;

  if (disk_ptr->sector.num <= sector_offset)
    return ERROR_OUT_OF_SECTOR;
  mask = 0x80 >> (sector_offset & 7);
  offset8 = sector_offset >> 3;
  return ((disk_ptr->sector.bads[offset8] & mask) != 0);
}

int is_bad_cluster(disk *disk_ptr, int cluster_offset)
{
  int cur, from, to;

  from = (cluster_offset - 2) * disk_ptr->cluster.sectors_per +
	 disk_ptr->data.top_sector;
  to = from + disk_ptr->cluster.sectors_per;
  for (cur = from ; cur < to; ++cur)
    if (is_bad_sector(disk_ptr, cur))
      return 1;
  return 0;
}

void read_sector(void *buffer, disk *disk_ptr, int from, int num)
{
  int to = from + num;
  int real_to;

  while (from < to)
    {
      for (real_to = from; real_to < to; real_to++)
	if (is_bad_sector(disk_ptr, real_to))
	  break;
      if (from < real_to)
	{
	  diskred(disk_ptr, buffer, from, real_to - from);
	  buffer += (real_to - from) * disk_ptr->sector.length;
	  from = real_to;
	}
      else
	{
	  int counter;

	  for (counter = 0; counter < disk_ptr->cluster.length; ++counter)
	    *((char *)buffer)++ = 0;
	  from++;
	}
    }
}

void read_cluster(void *buffer, disk *disk_ptr,
		  unsigned short from, unsigned short num)
{
  read_sector(buffer, disk_ptr, (from - 2) * disk_ptr->cluster.sectors_per +
				disk_ptr->data.top_sector,
				num * disk_ptr->cluster.sectors_per);
}

void write_sector(const void *buffer, disk *disk_ptr, int from, int num)
{
  int to = from + num;
  int real_to;

  if (!flags.writing)
    return;
  while (from < to)
    {
      for (real_to = from; real_to < to; real_to++)
	if (is_bad_sector(disk_ptr, real_to))
	  break;
      if (from < real_to)
	{
	  diskwrt(disk_ptr, buffer, from, real_to - from);
	  buffer += (real_to - from) * disk_ptr->sector.length;
	  from = real_to;
	}
      else
	{
	  prerr_int("セクタ %d に書き出し出来ません",
		    "can not write to sector %d", from);
	      end_line();
	  from++;
	}
    }
}

void write_cluster(const void *buffer, disk *disk_ptr,
		   unsigned short from, unsigned short num)
{
  if (!flags.writing)
    return;
  write_sector(buffer, disk_ptr, (from - 2) * disk_ptr->cluster.sectors_per +
				 disk_ptr->data.top_sector,
				 num * disk_ptr->cluster.sectors_per);
}
