/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/fat.c,v $
 * $Author: kawamoto $
 * $Revision: 1.4 $
 * $Date: 1993/12/12 14:01:46 $
 * $Log: fat.c,v $
 *	Revision 1.4  1993/12/12  14:01:46  kawamoto
 *	Ver 1.02 メディアバイト 0xfff0 (IBM format MO) に対応化。
 *
 *	Revision 1.3  1993/10/24  13:19:02  kawamoto
 *	lost+found 関係のバグフィックス
 *	-isLittleEndian サポート
 *
 *	Revision 1.2  1993/04/17  11:28:14  src
 *	directory にループがある場合に暴走するのを直した
 *	正式版（Ver 1.00）
 *
 *	Revision 1.1  1993/03/26  22:48:09  src
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

/* Copyright (C) 2023 TcbnErik */

#include <stdlib.h>
#include <doslib.h>

#include "fsck.h"

void remove_bad_cluster(disk *disk_ptr)
{
  unsigned short no;

  prhead("使用不能クラスタサーチ中です", "Searching unusable clusters");
  for (no = 2; no < disk_ptr->cluster.num; ++no)
    {
      if (disk_ptr->FAT.buf[no].badsec == FAT_BADSEC_OK)
	continue;
      if (disk_ptr->FAT.buf[no].value == 0xfff7)
	continue;
      prerr_int("クラスタ %04X は、使用不可能です",
		"cluster %04X is unusable", no);
      disk_ptr->FAT.buf[no].value = 0xfff7;
      if (flags.writing)
	{
	  end_record();
	  prerr_("「使用不能クラスタ」として登録します",
		 "enter it as `unusable cluster'");
	  write_FAT(disk_ptr, no);
	}
      end_line();
    }
  end_section();
}

void read_FAT(disk *disk_ptr)
{
  int no, ctr;
  unsigned char *dummy;
  unsigned char *src;
  FATbuf *dst;

  prhead("ＦＡＴ読み込み中です", "Reading FAT");
  if (!flags.force)
    {
      int ok;

      ok = 1;
      no = disk_ptr->FAT.top_sector;
      for (ctr = 0; ctr < disk_ptr->FAT.sector_num; ++ctr)
	{
	  if (is_bad_sector(disk_ptr, no))
	    ok = 0;
	  ++no;
	}
      if (!ok)
	{
	  prerr_("ＦＡＴは読み書き出来ません", "FAT is not readable");
	  cleanup_exit(1);
	}
    }
  dummy = (unsigned char *)Malloc(disk_ptr->sector.length * disk_ptr->FAT.sector_num);
  if (diskred(disk_ptr, dummy, disk_ptr->FAT.top_sector, disk_ptr->FAT.sector_num))
    {
      prerr_("ＦＡＴは読み書き出来ません", "FAT is not readable");
      cleanup_exit(1);
    }
  src = dummy;
  dst = disk_ptr->FAT.buf;
  if (disk_ptr->FAT.is_2bytes)
    {
      if (disk_ptr->FAT.little_endian)
	{
	  for (no = 0; no < disk_ptr->cluster.num; ++no)
	    {
	      dst->value = *src++;
	      dst++->value |= *src++ << 8;
	    }
	}
      else
	{
	  for (no = 0; no < disk_ptr->cluster.num; ++no)
	    {
	      dst->value = *src++ << 8;
	      dst++->value |= *src++;
	    }
	}
    }
  else
    {
      for (no = 0; no < disk_ptr->cluster.num; ++no)
	{
	  dst->value = *src++;
	  dst->value |= (*src & 0x0f) << 8;
	  if (dst->value > 0xff6)
	    dst->value |= 0xf000;
	  ++dst;
	  if (++no == disk_ptr->cluster.num)
	    break;
	  dst->value = (*src++ & 0xf0) >> 4;
	  dst->value |= *src++ << 4;
	  if (dst->value > 0xff6)
	    dst->value |= 0xf000;
	  ++dst;
	}
    }
  Free(dummy);
  end_section();
}

void write_FAT(disk *disk_ptr, unsigned short cluster)
{
  unsigned short no;
  int write_top, write_length, write_num;
  unsigned char *dummy;
  unsigned char *dst;
  FATbuf *src;

  if (!flags.writing)
    return;
#ifdef FATwriting
  end_record();
  print_("ＦＡＴ書き出し中です", "Writing FAT");
#endif
  if (disk_ptr->FAT.is_2bytes)
    {
      int from, to, half_of_length;

      write_top = disk_ptr->FAT.top_sector;
      write_num = 1;
      write_length = disk_ptr->sector.length;
      half_of_length = write_length / 2;
      dummy = (unsigned char *)Malloc(write_length);
      src = disk_ptr->FAT.buf;
      dst = dummy;
      from = cluster / half_of_length;
      write_top += from;
      from *= half_of_length;
      src += from;
      to = from + half_of_length;
      if (to > disk_ptr->cluster.num)
	to = disk_ptr->cluster.num;
      if (disk_ptr->FAT.little_endian)
	{
	  for (no = from; no < to; ++no)
	    {
	      *dst++ = src->value;
	      *dst++ = src++->value >> 8;
	    }
	}
      else
	{
	  for (no = from; no < to; ++no)
	    {
	      *dst++ = src->value >> 8;
	      *dst++ = src++->value;
	    }
	}
    }
  else
    {
      write_top = disk_ptr->FAT.top_sector;
      write_num = disk_ptr->FAT.sector_num;
      write_length = disk_ptr->sector.length * write_num;
      dummy = (unsigned char *)Malloc(write_length);
      src = disk_ptr->FAT.buf;
      dst = dummy;
      for (no = 0; no < disk_ptr->cluster.num; ++no)
	{
	  *dst++ = src->value;
	  *dst = (src++->value & 0x0f00) >> 8;
	  if (++no == disk_ptr->cluster.num)
	    break;
	  *dst++ |= (src->value & 0x000f) << 4;
	  *dst++ = (src++->value & 0x0ff0) >> 4;
	}
    }
  diskwrt(disk_ptr, dummy, write_top, write_num);
  Free(dummy);
#ifdef FATwriting
  end_line();
#endif
}

static void check_FAT_set(disk *disk_ptr)
{
  unsigned short no;
  unsigned short next;

  for (no = 2; no < disk_ptr->cluster.num; ++no)
    {
      next = disk_ptr->FAT.buf[no].value;
      disk_ptr->FAT.buf[no].refered = FAT_REFERED_NO;
      disk_ptr->FAT.buf[no].loop = FAT_LOOP_UNDECIDED;
      disk_ptr->FAT.buf[no].badsec = FAT_BADSEC_OK;
      disk_ptr->FAT.buf[no].subdir = FAT_SUBDIR_NOTYET;
      if (next == 0x0000)
	{
	  disk_ptr->FAT.buf[no].type = FAT_TYPE_FREE;
	  disk_ptr->FAT.buf[no].refer = FAT_REFER_END;
	}
      else if (next == 0xfff7)
	{
	  disk_ptr->FAT.buf[no].type = FAT_TYPE_OTHERS;
	  disk_ptr->FAT.buf[no].refer = FAT_REFER_OUTOF;
	  disk_ptr->FAT.buf[no].badsec = FAT_BADSEC_YES;
	}
      else if (0xfff8 <= next && next <= 0xfffe)
	{
	  disk_ptr->FAT.buf[no].type = FAT_TYPE_OTHERS;
	  disk_ptr->FAT.buf[no].refer = FAT_REFER_OUTOF;
	}
      else if (next == 0xffff)
	{
	  disk_ptr->FAT.buf[no].type = FAT_TYPE_END;
	  disk_ptr->FAT.buf[no].refer = FAT_REFER_END;
	}
      else
	{
	  disk_ptr->FAT.buf[no].type = FAT_TYPE_LINKING;
	  disk_ptr->FAT.buf[no].refer = FAT_REFER_OK;
	}
      if (is_bad_cluster(disk_ptr, no))
	disk_ptr->FAT.buf[no].badsec = FAT_BADSEC_YES;
    }
}

static void check_FAT_outof(disk *disk_ptr, int first_time)
{
  unsigned short no;
  unsigned short next;

  for (no = 2; no < disk_ptr->cluster.num; ++no)
    {
      if (disk_ptr->FAT.buf[no].badsec == FAT_BADSEC_YES)
	continue;
      if (disk_ptr->FAT.buf[no].type != FAT_TYPE_OTHERS &&
	  disk_ptr->FAT.buf[no].type != FAT_TYPE_LINKING)
	continue;
      next = disk_ptr->FAT.buf[no].value;
      if (next <= 1 || disk_ptr->cluster.num <= next)
	{
	  if (first_time)
	    {
	      end_record();
	      prerr_int("クラスタ %04X は、範囲外クラスタ",
			"cluster %04X refers to an out of cluster", no);
	      prerr_int(" %04X を指しています", " %04X", next);
	    }
	  if (flags.writing)
	    {
	      end_record();
	      prerr_("修正します", "fixing it");
	      disk_ptr->FAT.buf[no].value = 0xffff;
	      write_FAT(disk_ptr, no);
	      disk_ptr->FAT.buf[no].type = FAT_TYPE_END;
	    }
	  else
	    disk_ptr->FAT.buf[no].refer = FAT_REFER_OUTOF;
	  end_line();
	  continue;
	}
      if (disk_ptr->FAT.buf[next].badsec == FAT_BADSEC_YES)
	{
	  if (first_time)
	    {
	      end_record();
	      prerr_int("クラスタ %04X は、使用不能クラスタ",
			"cluster %04X refers to a unusable cluster", no);
	      prerr_int(" %04X を指しています", " %04X", next);
	    }
	  disk_ptr->FAT.buf[no].value = disk_ptr->FAT.buf[next].value;
	  disk_ptr->FAT.buf[no].type = disk_ptr->FAT.buf[next].type;
	  disk_ptr->FAT.buf[no].refer = disk_ptr->FAT.buf[next].refer;
	  if (flags.writing)
	    {
	      end_record();
	      prerr_("修正します", "fixing it");
	      write_FAT(disk_ptr, no);
	    }
	  --no;
	  end_line();
	  continue;
	}
      switch (disk_ptr->FAT.buf[next].type)
	{
	case FAT_TYPE_OTHERS:
	  if (first_time)
	    {
	      end_record();
	      prerr_int("クラスタ %04X は、範囲外クラスタ",
			"cluster %04X refers to an out of cluster", no);
	      prerr_int(" %04X を指しています", " %04X", next);
	    }
	  if (flags.writing)
	    {
	      end_record();
	      prerr_("修正します", "fixing it");
	      disk_ptr->FAT.buf[no].value = 0xffff;
	      write_FAT(disk_ptr, no);
	      disk_ptr->FAT.buf[no].type = FAT_TYPE_END;
	    }
	  else
	    disk_ptr->FAT.buf[no].refer = FAT_REFER_OUTOF;
	  end_line();
	  break;
	case FAT_TYPE_FREE:
	  if (first_time)
	    {
	      end_record();
	      prerr_int("クラスタ %04X は、未使用クラスタ",
			"cluster %04X refers to a free cluster", no);
	      prerr_int(" %04X を指しています", " %04X", next);
	    }
	  if (flags.writing)
	    {
	      end_record();
	      prerr_("修正します", "fixing it");
	      disk_ptr->FAT.buf[no].value = 0xffff;
	      write_FAT(disk_ptr, no);
	      disk_ptr->FAT.buf[no].type = FAT_TYPE_END;
	    }
	  else
	    disk_ptr->FAT.buf[no].refer = FAT_REFER_OUTOF;
	  end_line();
	  break;
	default:
	  break;
	}
      switch (disk_ptr->FAT.buf[next].refered)
	{
	case FAT_REFERED_NO:
	  disk_ptr->FAT.buf[next].refered = FAT_REFERED_ONE;
	  break;
	case FAT_REFERED_ONE:
	  disk_ptr->FAT.buf[next].refered = FAT_REFERED_MANY;
	  if (first_time && flags.verbose)
	    {
	      end_record();
	      prerr_int("クラスタ %04X は、複数ヶ所から指されています",
			"cluster %04X is refered many times", next);
	    }
	  end_line();
	  break;
	default:
	  break;
	}
    }
}

static char loop_cut;

static void check_FAT_loop(disk *disk_ptr, int first_time)
{
  unsigned short no;

  for (no = 2; no < disk_ptr->cluster.num; ++no)
    {
      unsigned short test, last;

      if (disk_ptr->FAT.buf[no].loop != FAT_LOOP_UNDECIDED)
	continue;
      last = test = no;
      while (1)
	{
	  unsigned short fill;

	  switch (disk_ptr->FAT.buf[test].loop)
	    {
	    case FAT_LOOP_IN_SEARCH:
	      fill = FAT_LOOP_YES;
	      if (first_time)
		{
		  prerr_int("クラスタ %04X は、ループになっています",
			    "cluster %04X rushes into a loop", test);
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("これを分断します", "divide it");
		      loop_cut = 1;
		      disk_ptr->FAT.buf[last].value = 0xffff;
		      write_FAT(disk_ptr, last);
		      disk_ptr->FAT.buf[last].type = FAT_TYPE_END;
		      disk_ptr->FAT.buf[last].refer = FAT_REFER_END;
		      fill = FAT_LOOP_NO;
		    }
		  end_line();
		}
	      break;
	    case FAT_LOOP_YES:
	      fill = FAT_LOOP_YES;
	      break;
	    case FAT_LOOP_NO:
	    default:
	      fill = FAT_LOOP_NO;
	      break;
	    case FAT_LOOP_UNDECIDED:
	      disk_ptr->FAT.buf[test].loop = FAT_LOOP_IN_SEARCH;
	      if (disk_ptr->FAT.buf[test].refer == FAT_REFER_OK)
		{
		  last = test;
		  test = disk_ptr->FAT.buf[test].value;
		  continue;
		}
	      fill = FAT_LOOP_NO;
	      break;
	    }
	  for (test = no;
	       disk_ptr->FAT.buf[test].loop == FAT_LOOP_IN_SEARCH;
	       test = disk_ptr->FAT.buf[test].value)
	    {
	      disk_ptr->FAT.buf[test].loop = fill;
	    }
	  break;
	}
    }
}

void check_FAT(disk *disk_ptr)
{
  prhead("ＦＡＴ整合チェック中です", "Checking FAT links");
  loop_cut = 0;
  check_FAT_set(disk_ptr);
  check_FAT_outof(disk_ptr, 1);
  check_FAT_loop(disk_ptr, 1);
  if (loop_cut)
    {
      end_section();
      prhead("二回目のＦＡＴ整合チェック中です", "Checking FAT links again");
      check_FAT_set(disk_ptr);
      check_FAT_outof(disk_ptr, 0);
      check_FAT_loop(disk_ptr, 0);
    }
  end_section();
}

int expected_length(disk *disk_ptr, unsigned short cluster, int length)
{
  unsigned short cluster_num;
  int min_length, max_length;

  cluster_num = 1;
  while (1)
    {
      if (disk_ptr->FAT.buf[cluster].type != FAT_TYPE_LINKING)
	break;
      if (disk_ptr->FAT.buf[cluster].loop != FAT_LOOP_NO)
	break;
      cluster_num++;
      cluster = disk_ptr->FAT.buf[cluster].value;
    }
  min_length = (cluster_num - 1) * disk_ptr->cluster.length;
  max_length = min_length + disk_ptr->cluster.length;
  return (min_length <= length && length <= max_length) ? length : max_length;
}
