/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/error.c,v $
 * $Author: kawamoto $
 * $Revision: 1.3 $
 * $Date: 1994/01/06 21:12:23 $
 * $Log: error.c,v $
 *	Revision 1.3  1994/01/06  21:12:23  kawamoto
 *	エラー処理を変更した
 *
 *	Revision 1.2  1993/12/22  07:56:18  kawamoto
 *	Ver 1.02a ... エラー表示に、対象セクタ番号範囲を追加
 *
 *	Revision 1.1  1993/03/26  22:47:58  src
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
 *	Revision 1.1  1992/11/28  13:32:27  kawamoto
 *	Initial revision
 *
 *	Revision 1.4  1992/11/28  12:56:08  kawamoto
 *	version 0.03b
 *
 *	Revision 1.3  1992/11/28  11:46:10  kawamoto
 *	version 0.03b
 *
 *	Revision 1.2  1992/11/15  17:29:36  kawamoto
 *	Ver 0.03 without entering bad sectors.
 *
 *	Revision 1.1  1992/11/15  11:08:00  kawamoto
 *	Initial revision
 */

/* Copyright (C) 2023 TcbnErik */

register unsigned short code asm("d7");

#include <stdlib.h>
#include <stdio.h>
#include <doslib.h>

#include "fsck.h"

static short error_code;
static short ignore_flag = 0;
static void (*old_handler)();
static int error_sector;
static int error_seclen = -1;

int diskred(disk *disk_ptr, unsigned char *adr, int sector, int seclen)
{
  error_code = 0;
  error_sector = sector;
  error_seclen = seclen;
  DISKRED2(adr, disk_ptr->drive_no + 1, sector, seclen);
  error_seclen = -1;
  return error_code;
}

int diskwrt(disk *disk_ptr, const unsigned char *adr, int sector, int seclen)
{
  error_code = 0;
  error_sector = sector;
  error_seclen = seclen;
  DISKWRT2(adr, disk_ptr->drive_no + 1, sector, seclen);
  error_seclen = -1;
  return error_code;
}

static int key_handler()
{
  int ignore;
  char errmsg[100];

  ignore = 0;
  switch (error_code)
    {
    case 0:
      return 2;
    case 2:
    case 14:
      sprintf(errmsg, "ドライブの準備が出来ていません");
      error_code = 2;
      break;
    case 4:
      if (ignore_flag)
	ignore = 2;
      sprintf(errmsg, "ＣＲＣエラーです");
      break;
    case 5:
      sprintf(errmsg, "ディスクの管理領域が壊れています");
      break;
    case 6:
      sprintf(errmsg, "シークエラーです");
      break;
    case 7:
      if (ignore_flag)
	ignore = 2;
      sprintf(errmsg, "無効なメディアです");
      break;
    case 8:
      if (ignore_flag)
	ignore = 2;
      sprintf(errmsg, "セクタが見付かりません");
      break;
    case 10:
      if (ignore_flag)
	ignore = 2;
      sprintf(errmsg, "書き込みエラーです");
      break;
    case 11:
      if (ignore_flag)
	ignore = 2;
      sprintf(errmsg, "読み込みエラーです");
      break;
    case 13:
      sprintf(errmsg, "メディアが書き込みプロテクトされています");
      break;
    default:
      sprintf(errmsg, "未確認エラー 0x%04X です", code);
      break;
    }
  prerr_(errmsg, SAME);
  if (error_seclen == -1)
    prerr_(" (disk 以外)", SAME);
  else
    {
      if (error_seclen != 1)
	sprintf(errmsg, " (Human sector %08X ～ sector %08X)",
		error_sector, error_sector + error_seclen - 1);
      else
	sprintf(errmsg, " (Human sector %08X)", error_sector);
      prerr_(errmsg, SAME);
    }
  if (ignore)
    {
      end_line();
      return ignore;
    }
  end_section();
  cleanup_exit(0);
}

static void __interrupt catchD7()
{
  error_code = code & 0xff;
  if (key_handler())
    code = 2;
  else
    {
      error_code = 0;
      code = 1;
    }
  __builtin_saveregs();
}

static void __interrupt error_handler(void)
{
  if (code < 0x1000)
    __builtin_saveregs(old_handler);
  else
    __builtin_saveregs(catchD7);
}

static void doserrret(void)
{
  cleanup_exit(255);
}

void setup()
{
  old_handler = INTVCG(0x2e);
  INTVCS(0x2e, error_handler);
  INTVCS(0xfff1, doserrret);
}

void volatile cleanup_exit(int exit_code)
{
  end_section();
  INTVCS(0x2e, old_handler);
  exit(exit_code);
}

void set_error_mode(int ignore)
{
  ignore_flag = ignore ? 1 : 0;
}
