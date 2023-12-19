/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/print.c,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:49:08 $
 * $Log: print.c,v $
 *	Revision 1.1  1993/03/26  22:49:08  src
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

#include <iocslib.h>
#include <stdio.h>

#include "fsck.h"

int escape()
{
  return (BITSNS(0) & 2);
}

int keyinp()
{
  int code;

  code = B_KEYINP();
  if ((code & 0xff00) == 0x7100)
    {
      while ((code = B_KEYINP() & 0xff) == 0)
	;
      if (0x20 <= code && code < 0x40)
	code -= 0x20;
    }
  return code & 0xff;
}

#define ON_HEAD    0 /* 行頭である */
#define IN_RECORD  1 /* record の出力中 */
#define END_RECORD 2 /* レコード末であるがまだ , を出力していない */
#define END_LINE   3 /* 行末であるがまだ , を出力していない */
static char in_record = ON_HEAD;

const char *jore(const char *jstring, const char *string)
{
  if (string == SAME)
    return jstring;
  if (jstring == SAME)
    return string;
  return (flags.english ? string : jstring);
}

void prhead(const char *jstring, const char *string)
{
  printf(jore(jstring, string));
  printf("... ");
  in_record = END_LINE;
}

static void print_pre(const char *left_margin)
{
  if (in_record == END_LINE)
    {
      printf(",\n");
      in_record = ON_HEAD;
    }
  if (in_record == ON_HEAD)
    {
      printf(left_margin);
      in_record = IN_RECORD;
    }
  if (in_record == END_RECORD)
    {
      printf(", ");
      in_record = IN_RECORD;
    }
}

void print_(const char *jformat, const char *format)
{
  print_pre("        ");
  printf(jore(jformat, format));
}

void print_int(const char *jformat, const char *format, int arg1)
{
  print_pre("        ");
  printf(jore(jformat, format), arg1);
}

void print_str(const char *jformat, const char *format, const char *arg1)
{
  print_pre("        ");
  printf(jore(jformat, format), arg1);
}

void prerr_(const char *jformat, const char *format)
{
  print_pre("    *** ");
  printf(jore(jformat, format));
}

void prerr_int(const char *jformat, const char *format, int arg1)
{
  print_pre("    *** ");
  printf(jore(jformat, format), arg1);
}

void prerr_str(const char *jformat, const char *format, const char *arg1)
{
  print_pre("    *** ");
  printf(jore(jformat, format), arg1);
}

void prerr_datetime(unsigned long datetime)
{
  int value;

  print_pre("    *** ");
  value = (datetime >> 25) & 0x7f;
  printf("%d-", value + 1980);
  value = (datetime >> 21) & 0x0f;
  printf("%02d-", value);
  value = (datetime >> 16) & 0x1f;
  printf("%02d ", value);
  value = (datetime >> 11) & 0x1f;
  printf("%02d:", value);
  value = (datetime >> 5) & 0x3f;
  printf("%02d:", value);
  value = (datetime << 1) & 0x3e;
  printf("%02d", value);
}

void prerr_attribute(unsigned char attribute)
{
  print_pre("    *** ");
  printf((attribute & 0x40) ? "l" :
	 ((attribute & 0x10) ? "d" :
	  ((attribute & 0x08) ? "v" : "-")));
  printf((attribute & 0x20) ? "a" : "-");
  printf((attribute & 0x04) ? "s" : "-");
  printf((attribute & 0x02) ? "h" : "-");
  printf("r");
  printf((attribute & 0x01) ? "-" : "w");
  printf((attribute & 0x80) ? "x" : "-");
}

void end_line(void)
{
  in_record = END_LINE;
}

void end_record(void)
{
  if (in_record != END_LINE)
    in_record = END_RECORD;
}

void end_section(void)
{
  if (in_record != ON_HEAD)
    {
      in_record = ON_HEAD;
      printf(".\n");
    }
}
