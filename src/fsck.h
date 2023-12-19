/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/fsck.h,v $
 * $Author: kawamoto $
 * $Revision: 1.4 $
 * $Date: 1993/12/12 14:01:52 $
 * $Log: fsck.h,v $
 *	Revision 1.4  1993/12/12  14:01:52  kawamoto
 *	Ver 1.02 メディアバイト 0xfff0 (IBM format MO) に対応化。
 *
 *	Revision 1.3  1993/10/24  13:19:10  kawamoto
 *	lost+found 関係のバグフィックス
 *	-isLittleEndian サポート
 *
 *	Revision 1.2  1993/04/17  11:28:45  src
 *	directory にループがある場合に暴走するのを直した
 *	正式版（Ver 1.00）
 *
 *	Revision 1.1  1993/03/26  22:48:34  src
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

#include "config.h"

#ifdef DEBUG
typedef struct _DEBUG_FLAGS {
  unsigned int collect : 1;
  unsigned int DPB : 1;
  unsigned int force_2bytes : 1;
  unsigned int force_15bytes : 1;
  unsigned int link : 1;
  unsigned int path : 1;
  unsigned int sector : 1;
} debug_flags;
extern debug_flags debug;
#endif
typedef struct _NORMAL_FLAGS {
  unsigned int check_reading_sectors : 1;
  unsigned int check_writing_sectors : 1;
  unsigned int english : 1;
  unsigned int force : 1;
  unsigned int ignore_archive_attribute : 1;
  unsigned int unix_like : 1;
  unsigned int verbose : 1;
  unsigned int writing : 1;
#ifdef DETERMINE_LITTLE_ENDIAN_BY_OPTION
  unsigned int little_endian : 1;
#endif
  unsigned int is_2bytes : 1;
  unsigned int is_15bytes : 1;
} normal_flags;
extern normal_flags flags;

#define ERROR_OUT_OF_SECTOR -100
#define ERROR_BAD_FAT_SIZE -101
#define ERROR_VIRTUAL_DEVICE -102

typedef struct {
  unsigned int value  :16;
  unsigned int type   : 2;
#define FAT_TYPE_LINKING   0
#define FAT_TYPE_END       1
#define FAT_TYPE_FREE      2
#define FAT_TYPE_OTHERS    3
  unsigned int refer  : 2;
#define FAT_REFER_OK       0
#define FAT_REFER_FIXED    1
#define FAT_REFER_END      2
#define FAT_REFER_OUTOF    3
  unsigned int refered: 2;
#define FAT_REFERED_NO     0
#define FAT_REFERED_ONE    1
#define FAT_REFERED_MANY   2
#define FAT_REFERED_FIXED  3
  unsigned int loop   : 2;
#define FAT_LOOP_UNDECIDED 0
#define FAT_LOOP_YES       1
#define FAT_LOOP_NO        2
#define FAT_LOOP_IN_SEARCH 3
  unsigned int badsec : 1;
#define FAT_BADSEC_OK      0
#define FAT_BADSEC_YES     1
  unsigned int subdir : 1;
#define FAT_SUBDIR_NOTYET  0
#define FAT_SUBDIR_ALREADY 1
} FATbuf;

typedef struct {
  int drive_no;
  struct {
    int length;
    int num;
    unsigned char *bads;
  } sector;
  struct {
    int top_sector;
    int sector_num;
    int little_endian;
    int is_2bytes;
    FATbuf *buf;
  } FAT;
  struct {
    int length;
    int sectors_per;
    unsigned short num;
  } cluster;
  struct {
    int top_sector;
    int sector_num;
  } root;
  struct {
    int top_sector;
    int sector_num;
  } data;
} disk;

/* disk.c */

extern disk *init_disk(int drive);

/* sector.c */

extern void check_read_sector(disk *disk_ptr);
extern void check_write_sector(disk *disk_ptr);
extern int set_bad_sector(disk *disk_ptr, int sector_offset);
extern int is_bad_sector(disk *disk_ptr, int sector_offset);
extern int is_bad_cluster(disk *disk_ptr, int cluster_offset);
extern void read_sector(void *buffer, disk *disk_ptr, int from, int num);
extern void read_cluster(void *buffer, disk *disk_ptr,
			 unsigned short from, unsigned short num);
extern void write_sector(const void *buffer, disk *disk_ptr, int from, int num);
extern void write_cluster(const void *buffer, disk *disk_ptr,
			  unsigned short from, unsigned short num);

/* root.c */

extern void check_root(disk *disk_ptr);

/* fat.c */

extern void remove_bad_cluster(disk *disk_ptr);
extern void read_FAT(disk *disk_ptr);
extern void write_FAT(disk *disk_ptr, unsigned short cluster);
extern void check_FAT(disk *disk_ptr);
extern int expected_length(disk *disk_ptr, unsigned short cluster, int length);

/* dump.c */

extern void dump_FAT(disk *disk_ptr);
extern void diff_and_write_FAT(disk *disk_ptr);

/* dir.c */

extern void check_files(disk *disk_ptr);
extern void find_lost_files(disk *disk_ptr);
extern void fix_files(disk *disk_ptr);

/* malloc.c */

extern void *Malloc(int size);
extern void Free(void *ptr);

/* print.c */

extern int escape(void);
extern int keyinp(void);
extern void prhead(const char *jstring, const char *string);
extern void print_(const char *jformat, const char *format);
extern void print_int(const char *jformat, const char *format, int arg1);
extern void print_str(const char *jformat, const char *format, const char *arg1);
extern void prerr_(const char *jformat, const char *format);
extern void prerr_int(const char *jformat, const char *format, int arg1);
extern void prerr_str(const char *jformat, const char *format, const char *arg1);
extern void prerr_datetime(unsigned long datetime);
extern void prerr_attribute(unsigned char attribute);
extern void end_line(void);
extern void end_record(void);
extern void end_section(void);
#define SAME 0

/* error.c */

extern void setup(void);
extern void volatile cleanup_exit(int code);
extern void set_error_mode(int ignore);
extern int diskred(disk *disk_ptr, unsigned char *adr, int sector, int seclen);
extern int diskwrt(disk *disk_ptr, const unsigned char *adr, int sector, int seclen);
