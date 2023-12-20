/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/main.c,v $
 * $Author: kawamoto $
 * $Revision: 1.8 $
 * $Date: 1994/04/23 11:04:54 $
 * $Log: main.c,v $
 * Revision 1.8  1994/04/23  11:04:54  kawamoto
 * ドキュメント修正
 *
 * Revision 1.7  1994/01/06  21:12:29  kawamoto
 * エラー処理を変更した
 *
 * Revision 1.6  1993/12/12  14:01:58  kawamoto
 * Ver 1.02 メディアバイト 0xfff0 (IBM format MO) に対応化。
 *
 * Revision 1.5  1993/10/24  17:29:13  kawamoto
 * ヘルプ画面の修正
 *
 * Revision 1.4  1993/10/24  13:19:18  kawamoto
 * lost+found 関係のバグフィックス
 * -isLittleEndian サポート
 *
 * Revision 1.3  1993/05/15  07:50:41  kawamoto
 * ちょっとだけ修正
 *
 * Revision 1.2  1993/04/17  11:28:58  src
 * directory にループがある場合に暴走するのを直した
 * 正式版（Ver 1.00）
 *
 * Revision 1.1  1993/03/26  22:48:46  src
 * version 0.05a
 *
 * Revision 2.5  1993/01/19  21:28:41  kawamoto
 * volume 名を認識しないバグをフィックス
 *
 * Revision 2.4  1993/01/09  09:52:33  kawamoto
 * バージョンナンバー間違い修正
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
 * Revision 1.5  1992/12/12  13:31:56  kawamoto
 * version 0.04
 *
 * Revision 1.4  1992/12/12  11:54:46  kawamoto
 * version 0.04
 *
 * Revision 1.3  1992/11/28  11:46:10  kawamoto
 * version 0.03b
 *
 * Revision 1.2  1992/11/15  17:29:36  kawamoto
 * Ver 0.03 without entering bad sectors.
 *
 * Revision 1.1  1992/11/14  22:26:41  kawamoto
 * Initial revision
 */

/* Copyright (C) 2023 TcbnErik */

#include <iocslib.h>
#include <stdio.h>
#include <string.h>

#include "fsck.h"

#ifdef DEBUG
debug_flags debug;
#endif
normal_flags flags;
static const char progname[] = "fsck";

static void volatile usage(void) {
  if (flags.english) {
    printf("Usage: %s [-options] d:\n", progname);
#ifdef FSCK
    printf(
        "\t-check-reading-sectors\tcheck sector-read (default don't ckeck),\n");
    printf("\t-check-writing-sectors\tcheck sector-write (don't ckeck),\n");
#ifdef DEBUG
    printf("\t-debug-link\t\tdisplay link (don't display),\n");
    printf("\t-debug-DPB\t\tdisplay DPB (don't display),\n");
    printf(
        "\t-debug-force-2bytes\trecognize that FAT has 2bytes size (automatic "
        "decision).\n");
    printf(
        "\t-debug-force-is1.5bytes\t\t  that FAT has 1.5bytes size (automatic "
        "decision).\n");
    printf("\t-debug-path\t\tdisplay path (don't display),\n");
    printf(
        "\t-debug-sector\t\tthe verbose mode of sector reading/writing (don't "
        "display),\n");
#endif
    printf("\t-english\t\tthe english mode (japanese mode),\n");
    printf("\t-force\t\t\tforce to read/write (not force),\n");
    printf(
        "\t-ignore-archive-attrib\tignore the archive attribute (don't "
        "ignore),\n");
#else
    printf("\t-english\t\tthe english mode (default japanese mode),\n");
#endif
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
    printf(
        "\t-isLittleEndian\t\trecognize that FAT is little endian (big "
        "endian),\n");
#endif
    printf(
        "\t-is2bytes\trecognize that FAT has 2bytes size (automatic "
        "decision),\n");
    printf(
        "\t-is1.5bytes\t\t  that FAT has 1.5bytes size (automatic "
        "decision),\n");
#ifdef FSCK
    printf("\t-unix-like\t\tUN*X like (Human original),\n");
    printf("\t-verbose\t\tthe verbose mode (verbose),\n");
    printf("\t-writing\t\twrite really (don't write).\n");
#else
    printf("\t-writing\t\tthe writing mode (the dump mode).\n");
#endif
  } else {
    printf("使用法: %s [-オプション] d:\n", progname);
#ifdef FSCK
    printf(
        "\t-check-reading-"
        "sectors\tセクタリードチェックを行なう（デフォルトはチェックしない）"
        "\n");
    printf(
        "\t-check-writing-"
        "sectors\tセクタライトチェックを行なう（チェックしない）\n");
#ifdef DEBUG
    printf("\t-debug-link\t\tリンクの表示を行なう（表示しない）\n");
    printf("\t-debug-DPB\t\tDPB の表示を行なう（表示しない）\n");
    printf("\t-debug-force2bytes\tFAT ２バイトサイズの強制指定（自動判断）\n");
    printf(
        "\t-debug-force1.5bytes\tFAT 1.5バイトサイズの強制指定（自動判断）\n");
    printf("\t-debug-path\t\tパスの表示を行なう（表示しない）\n");
    printf(
        "\t-debug-sector\t\tセクター読み書きおしゃべりモード（表示しない）\n");
#endif
    printf("\t-english\t\t英語表示にする（日本語表示）\n");
    printf("\t-force\t\t\t無理に読み書きする（無理に読み書きしない）\n");
    printf(
        "\t-ignore-archive-attrib\t属性 A "
        "がなくても通常ファイルとみなす（みなさない）\n");
#else
    printf("\t-english\t\t英語表示にする（デフォルトは日本語表示）\n");
#endif
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
    printf(
        "\t-isLittleEndian\t\tFAT は PC コンパチである (Human オリジナル),\n");
#endif
    printf("\t-is2bytes\t\tFAT は２バイトサイズである（自動判断）\n");
    printf("\t-is1.5bytes\t\tFAT は1.5バイトサイズである（自動判断）\n");
#ifdef FSCK
    printf("\t-unix-like\t\tUN*X 風にする（Human のまま）\n");
    printf("\t-verbose\t\tおしゃべりモード（おしゃべり）\n");
    printf("\t-writing\t\t書き込みを実際に実行する（書き込まない）\n");
#else
    printf("\t-writing\t\t書き込みを実行する（ダンプ）\n");
#endif
  }
  cleanup_exit(1);
}

int main(int argc, char *argv[]) {
  disk *ptr;
  int drive;

#ifdef FSCK
  B_PRINT("X68k File System Checker Ver 1.03");
#endif
#ifdef DUMPFAT
  B_PRINT("X68k FAT Dump and Writer Ver 1.03");
#endif
  B_PRINT(
      " Copyright (C) 1992,93 by Ｅｘｔ(T.Kawamoto)\r\n"
      "                     patchlevel 3 Copyright (C) 2023 by TcbnErik\r\n");

  setup();
  drive = -1;
#ifdef DEBUG
  debug.collect = 0;
  debug.DPB = 0;
  debug.force_2bytes = 0;
  debug.force_15bytes = 0;
  debug.link = 0;
  debug.path = 0;
  debug.sector = 0;
#endif
  flags.check_reading_sectors = 0;
  flags.check_writing_sectors = 0;
  flags.english = 0;
  flags.force = 0;
  flags.ignore_archive_attribute = 0;
  flags.unix_like = 0;
  flags.verbose = 1;
  flags.writing = 0;
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
  flags.little_endian = 0;
#endif
  flags.is_2bytes = 0;
  flags.is_15bytes = 0;
  while (*++argv) {
    if (argv[0][0] == '-') {
      switch (argv[0][1]) {
        case 'e':
        case 'E':
          flags.english++;
          break;
#ifdef FSCK
        case 'c':
        case 'C':
          while (*++*argv && **argv != '-')
            ;
          if (**argv != '-') usage();
          switch (*++*argv) {
            case 'r':
            case 'R':
              flags.check_reading_sectors++;
              break;
            case 'w':
            case 'W':
              flags.check_writing_sectors++;
              break;
            default:
              usage();
          }
          break;
#ifdef DEBUG
        case 'd':
        case 'D':
          while (*++*argv && **argv != '-')
            ;
          if (**argv != '-') usage();
          switch (*++*argv) {
            case 'c':
            case 'C':
              debug.collect++;
              break;
            case 'd':
            case 'D':
              debug.DPB++;
              break;
            case 'f':
            case 'F':
              switch (argv[0][5]) {
                case '2':
                  debug.force_2bytes = 1;
                  break;
                case '1':
                  debug.force_15bytes = 1;
                  break;
                default:
                  usage();
              }
              break;
            case 'l':
            case 'L':
              debug.link++;
              break;
            case 'p':
            case 'P':
              debug.path++;
              break;
            case 's':
            case 'S':
              debug.sector++;
              break;
            default:
              usage();
          }
          break;
#endif
        case 'f':
        case 'F':
          flags.force++;
          break;
#endif
        case 'i':
        case 'I':
          switch (argv[0][3]) {
            case 'n':
            case 'N':
              flags.ignore_archive_attribute++;
              break;
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
            case 'l':
            case 'L':
              flags.little_endian = 1;
              break;
#endif
            case '2':
              flags.is_2bytes = 1;
              break;
            case '1':
              flags.is_15bytes = 1;
              break;
            default:
              usage();
          }
          break;
#ifdef FSCK
        case 'u':
        case 'U':
          flags.unix_like++;
          break;
        case 'v':
        case 'V':
          flags.verbose++;
          break;
#endif
        case 'w':
          if (strcmp(argv[0], "-writing") == 0)
            flags.writing++;
          else
            usage();
          break;
        default:
          usage();
      }
      continue;
    }
    if (argv[0][1] == 0 || argv[0][1] == ':') {
      if ('a' <= argv[0][0] && argv[0][0] <= 'z') drive = argv[0][0] - 'a';
      if ('A' <= argv[0][0] && argv[0][0] <= 'Z') drive = argv[0][0] - 'A';
    }
  }
  if (drive == -1) usage();

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  ptr = init_disk(drive);
  if ((int)ptr < 0) {
    if ((int)ptr == ERROR_BAD_FAT_SIZE)
      print_("FAT サイズが決定出来ませんでした", "We can't decide FAT size");
    else if ((int)ptr == ERROR_VIRTUAL_DEVICE)
      print_("仮想デバイスはサポートしていません",
             "We support of no virtual devices");
    else
      print_("未確認エラーが発生しました", "Unknown error has occured");
    cleanup_exit(1);
  }
#ifdef DEBUG
  B_PRINT("デバッグバージョンは FAT サイズ決定の確認を行ないます.\r\n");
  {
    int c;

    B_PRINT("FAT は ");
    B_PRINT(ptr->FAT.is_2bytes ? "2" : "1.5");
    B_PRINT("バイトに決定しました.\r\n");
    if (flags.writing) {
      B_PRINT("FAT サイズの決定が間違っているのに書き込みを行なう");
      B_PRINT(
          "（-writing をつける）と、\r\nディスクの内容が総て壊れて仕舞います");
      while ((c = keyinp()) != '\r') {
        if (c == 3) {
          B_PRINT("\r\n");
          cleanup_exit(1);
        }
      }
      B_PRINT(
          ".\r\n一度書き込み無し（-writing "
          "をつけない）でチェックすると良いです");
      while ((c = keyinp()) != '\r') {
        if (c == 3) {
          B_PRINT("\r\n");
          cleanup_exit(1);
        }
      }
      B_PRINT(
          ".\r\nチェックしたければ、BREAK キーで止めてからやり直して下さい");
      while ((c = keyinp()) != '\r') {
        if (c == 3) {
          B_PRINT("\r\n");
          cleanup_exit(1);
        }
      }
      B_PRINT(".\r\n");
      B_PRINT(ptr->FAT.is_2bytes ? "2" : "1.5");
      B_PRINT(" バイトで");
    }
    B_PRINT("よろしいですか？ ( y / n ) ");
    c = keyinp();
    B_PRINT("\r\n");
    if (c == 3) cleanup_exit(1);
    if (c != 'y' && c != 'Y') {
      B_PRINT("オプション -debug-force");
      B_PRINT(ptr->FAT.is_2bytes ? "1.5" : "2");
      B_PRINT("bytes で指定して下さい.\r\n");
      cleanup_exit(1);
    }
  }
#endif
#ifdef FSCK
  check_read_sector(ptr);
  check_write_sector(ptr);
  read_FAT(ptr);
  check_FAT(ptr);
  check_root(ptr);
  check_files(ptr);
  find_lost_files(ptr);
  fix_files(ptr);
  remove_bad_cluster(ptr);
#endif
#ifdef DUMPFAT
  read_FAT(ptr);
  if (flags.writing)
    diff_and_write_FAT(ptr);
  else
    dump_FAT(ptr);
#endif
  cleanup_exit(0);
}
