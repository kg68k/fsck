/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/disk.c,v $
 * $Author: kawamoto $
 * $Revision: 1.3 $
 * $Date: 1993/12/12 14:01:40 $
 * $Log: disk.c,v $
 *	Revision 1.3  1993/12/12  14:01:40  kawamoto
 *	Ver 1.02 メディアバイト 0xfff0 (IBM format MO) に対応化。
 *
 *	Revision 1.2  1993/10/24  13:18:54  kawamoto
 *	lost+found 関係のバグフィックス
 *	-isLittleEndian サポート
 *
 *	Revision 1.1  1993/03/26  22:47:36  src
 *	version 0.05a
 *
 *	Revision 2.2  1993/01/09  09:46:01  kawamoto
 *	version 0.5
 *
 *	Revision 2.1  1992/12/19  11:42:08  kawamoto
 *	公表バージョン (ver 0.04, rev 2.1)
 *
 *	Revision 1.2  1992/11/15  17:29:36  kawamoto
 *	Ver 0.03 without entering bad sectors.
 *
 *	Revision 1.1  1992/11/14  22:26:41  kawamoto
 *	Initial revision
 */

/* Copyright (C) 2023 TcbnErik */

#include <doslib.h>

#include "fsck.h"

disk *init_disk(int drive)
{
  int table_num;
  int offset;
  int FAT_bytes, area_bytes;
  disk *work;
  struct DPBPTR dpbbuf;

  work = (disk *)Malloc(sizeof(disk));
  work->drive_no = drive;
  if (GETDPB(drive + 1, &dpbbuf) < 0)
    return (disk *)ERROR_VIRTUAL_DEVICE;

  work->sector.length = dpbbuf.byte;
  work->FAT.top_sector = dpbbuf.fatsec;
  work->FAT.sector_num = dpbbuf.fatlen;
  work->root.top_sector = work->FAT.top_sector + dpbbuf.fatcount * work->FAT.sector_num;
  work->data.top_sector = dpbbuf.datasec;
  work->root.sector_num = work->data.top_sector - work->root.top_sector;
  work->cluster.sectors_per = dpbbuf.sec + 1;
  work->cluster.length = work->sector.length * work->cluster.sectors_per;
  work->cluster.num = dpbbuf.maxfat - 1;
  work->data.sector_num = work->cluster.sectors_per * (work->cluster.num - 2);
  work->sector.num = work->data.top_sector + work->data.sector_num;
  table_num = (work->sector.num + 7) >> 3;
  work->sector.bads = (unsigned char *)Malloc(sizeof(unsigned char) * table_num);
  for (offset = 0; offset < table_num; ++offset)
    work->sector.bads[offset] = 0;
  work->FAT.buf = (FATbuf *)Malloc(sizeof(FATbuf) * work->cluster.num);
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
  work->FAT.little_endian = flags.little_endian;
#else
  work->FAT.little_endian = (dpbbuf.shift & 0x80) ? 1 : 0;
#endif
#ifdef DEBUG
  if (debug.DPB)
    {
      print_int("DPB->drive %d", SAME, dpbbuf.drive);
      end_line();
      print_int("DPB->unit %d", SAME, dpbbuf.unit);
      end_line();
      print_int("DPB->byte %d", SAME, dpbbuf.byte);
      end_line();
      print_int("DPB->sec %d", SAME, dpbbuf.sec);
      end_line();
      print_int("DPB->shift %d", SAME, dpbbuf.shift);
      end_line();
      print_int("DPB->fatsec %d", SAME, dpbbuf.fatsec);
      end_line();
      print_int("DPB->fatcount %d", SAME, dpbbuf.fatcount);
      end_line();
      print_int("DPB->fatlen %d", SAME, dpbbuf.fatlen);
      end_line();
      print_int("DPB->dircount %d", SAME, dpbbuf.dircount);
      end_line();
      print_int("DPB->datasec %d", SAME, dpbbuf.datasec);
      end_line();
      print_int("DPB->maxfat %d", SAME, dpbbuf.maxfat);
      end_line();

      print_int("drive_no %d", SAME, work->drive_no);
      end_line();
      print_int("sector.length %d", SAME, work->sector.length);
      end_line();
      print_int("sector.num %d", SAME, work->sector.num);
      end_line();
      print_int("fat.top_sector %d", SAME, work->FAT.top_sector);
      end_line();
      print_int("fat.sector_num %d", SAME, work->FAT.sector_num);
      end_line();
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
      print_int("fat.little_endian %d", SAME, work->FAT.little_endian);
      end_line();
#endif
      print_int("fat.is_2bytes %d", SAME, work->FAT.is_2bytes);
      end_line();
      print_int("cluster.length %d", SAME, work->cluster.length);
      end_line();
      print_int("cluster.sectors_per %d", SAME, work->cluster.sectors_per);
      end_line();
      print_int("cluster.num %d", SAME, work->cluster.num);
      end_line();
      print_int("root.top_sector %d", SAME, work->root.top_sector);
      end_line();
      print_int("root.sector_num %d", SAME, work->root.sector_num);
      end_line();
      print_int("data.top_sector %d", SAME, work->data.top_sector);
      end_line();
      print_int("data.sector_num %d", SAME, work->data.sector_num);
      end_section();
    }
  if (debug.force_15bytes)
    {
      work->FAT.is_2bytes = 0;
      return work;
    }
  if (debug.force_2bytes)
    {
      work->FAT.is_2bytes = 1;
      return work;
    }
#endif
  if (work->cluster.num > 0xff6) /* クラスタ数は 1.5 bytes に収まらない */
    {
      /* 間違いなく 2 bytes FAT である */
      if (flags.is_15bytes) /* 無茶な事は言わないで */
	{
	  print_("2 bytes FAT だと思われるので -is1.5bytes は無視します",
		 "I think the FAT size is 2 bytes length,"
		 " so I shall ignore -is1.5bytes option");
	  end_section();
	}
      FAT_bytes = work->cluster.num << 1;
      area_bytes = work->FAT.sector_num * work->sector.length;
      if (work->cluster.num <= 0xfff7 && FAT_bytes <= area_bytes)
      /* クラスタ数も正しいし、FAT の領域にもちゃんと収まる */
	{
	  if (flags.is_2bytes) /* オプションがついていればそれに従う */
	    {
	      print_("-is2bytes は正しいと思います",
		     "I think the option -is2bytes is correct");
	      end_section();
	      work->FAT.is_2bytes = 1;
	      return work;
	    }
	  /* オプションがついていなくても、2 bytes だと思う */
	  print_("2 bytes FAT です", "2 bytes FAT");
	  end_section();
	  work->FAT.is_2bytes = 1;
	  return work;
	}
      /* FAT の領域に収まらないから、間違っていると思う */
      print_("FAT があまりにも小さすぎます", SAME);
      end_section();
      return (disk *)ERROR_BAD_FAT_SIZE;
    }
  /* クラスタ数は 1.5 bytes に収まるけれども、依然、2 bytes の可能性はある */
  FAT_bytes = work->cluster.num << 1;
  area_bytes = work->FAT.sector_num * work->sector.length;
  if (FAT_bytes <= area_bytes)
  /* 2 bytes FAT としても収まって仕舞う（FAT 領域が十分大きい）*/
    {
      if (flags.is_2bytes) /* オプションがついていればそれに従う */
	{
	  work->FAT.is_2bytes = 1;
	  return work;
	}
      /* オプションがついていなければ、やはり 1.5 bytes */
    }
  /* 2 bytes FAT 程は大きくない、、じゃ、1.5 bytes ？ */
  FAT_bytes = work->cluster.num + ((work->cluster.num + 1) >> 1);
  area_bytes = work->FAT.sector_num * work->sector.length;
  if (FAT_bytes <= area_bytes)
  /* 1.5 bytes FAT になら収まる（FAT 領域がそれなりに大きい）*/
    {
      if (flags.is_2bytes) /* 無茶な事は言わないで */
	{
	  print_("1.5 bytes FAT だと思われるので -is2bytes は無視します",
		 "I think the FAT size is 1.5 bytes length,"
		 " so I shall ignore -is2bytes option");
	  end_section();
	}
      if (flags.is_15bytes) /* オプションがついていればそれに従う */
	{
	  print_("-is1.5bytes は正しいと思います",
		 "I think the option -is2bytes is correct");
	  end_section();
	  work->FAT.is_2bytes = 0;
	  return work;
	}
      /* オプションがついていなくても、1.5 bytes だと思う */
      work->FAT.is_2bytes = 0;
      return work;
    }
  /* 1.5 bytes FAT にすら収まり切らない */
  print_("FAT があまりにも小さすぎます", SAME);
  end_section();
  return (disk *)ERROR_BAD_FAT_SIZE;
}
