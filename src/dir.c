/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/dir.c,v $
 * $Author: kawamoto $
 * $Revision: 1.3 $
 * $Date: 1993/10/24 13:18:35 $
 * $Log: dir.c,v $
 *	Revision 1.3  1993/10/24  13:18:35  kawamoto
 *	lost+found 関係のバグフィックス
 *	-isLittleEndian サポート
 *
 *	Revision 1.2  1993/04/17  11:27:53  src
 *	directory にループがある場合に暴走するのを直した
 *	正式版（Ver 1.00）
 *
 *	Revision 1.1  1993/03/26  22:46:57  src
 *	version 0.05a
 *
 *	Revision 2.4  1993/01/09  21:44:08  kawamoto
 *	archive attribute が立っていなくても通常ファイルだとみなす
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
 *	Revision 1.6  1992/12/19  11:33:35  kawamoto
 *	ver 0.04
 *
 *	Revision 1.5  1992/12/12  13:31:56  kawamoto
 *	version 0.04
 *
 *	Revision 1.4  1992/11/28  13:30:48  kawamoto
 *	ディレクトリエントリが未使用クラスタを指していた場合のチェックの強化
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

#include "fsck.h"
#include "dir.h"
#include "collect.h"

static void check_subdir(disk *disk_ptr, directory *dir)
{
  while (nextentry(disk_ptr, dir))
    {
      direntry *entry;

      entry = dir->entry;
      if (entry->fileName[0] == '.')
	{
	  if (entry->fileName[1] == ' ')
	    continue;
	  if (entry->fileName[1] == '.' && entry->fileName[2] == ' ')
	    continue;
	}

      if (is_file_or_directory(entry->atr))
	{
	  unsigned short file_cluster;

	  if (flags.ignore_archive_attribute == 0 && (entry->atr & 0x70) == 0)
	    {
	      prerr_str("ファイル %s にはアーカイブアトリビュートが立っていません",
			"file %s has no archive attribute", dir->file_path);
	      if (flags.writing)
		{
		  end_record();
		  prerr_("これを立てます", "give it");
		  entry->atr = 0x20;
		  writeentry(disk_ptr, dir);
		}
	      end_line();
	    }
	  file_cluster = get_cluster(entry);
	  while (1)
	    {
	      if (file_cluster < 2 || disk_ptr->cluster.num <= file_cluster ||
		  disk_ptr->FAT.buf[file_cluster].type == FAT_TYPE_OTHERS ||
		  disk_ptr->FAT.buf[file_cluster].type == FAT_TYPE_FREE)
		{
		  prerr_str("ファイル %s には実体が有りません",
			    "file %s has no body", dir->file_path);
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("これを削除します", "remove it");
		      entry->fileName[0] = 0xe5;
		      writeentry(disk_ptr, dir);
		    }
		  end_line();
		  break;
		}
	      if (disk_ptr->FAT.buf[file_cluster].badsec == FAT_BADSEC_YES)
		{
		  file_cluster = disk_ptr->FAT.buf[file_cluster].value;
		  continue;
		}
	      if (file_cluster != get_cluster(entry))
		{
		  end_record();
		  prerr_str("ファイル %s は、使用不能クラスタを指しています",
			    "file %s refers to a unusable cluster", dir->file_path);
		  put_cluster(entry, file_cluster);
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("修正します", "fixing it");
		      writeentry(disk_ptr, dir);
		    }
		}
	      switch (disk_ptr->FAT.buf[file_cluster].refered)
		{
		case FAT_REFERED_NO:
		  disk_ptr->FAT.buf[file_cluster].refered = FAT_REFERED_ONE;
		  break;
		case FAT_REFERED_ONE:
		  disk_ptr->FAT.buf[file_cluster].refered = FAT_REFERED_MANY;
		  break;
		default:
		  break;
		}
	      break;
	    }
	}

      if (is_directory(entry->atr))
	{
	  unsigned short child_cluster;
	  directory *child_dir;

	  child_cluster = get_cluster(entry);
	  if (disk_ptr->FAT.buf[child_cluster].subdir == FAT_SUBDIR_NOTYET)
	    {
	      disk_ptr->FAT.buf[child_cluster].subdir = FAT_SUBDIR_ALREADY;
	      child_dir = subdir(disk_ptr, dir->file_path, child_cluster);
	      check_subdir(disk_ptr, child_dir);
	      freedir(child_dir);
	    }
	  else
	    {
	      prerr_str("ディレクトリ %s はループしています",
			"directory %s is a loop", dir->file_path);
	      if (flags.writing)
		{
		  end_record();
		  prerr_("これを削除します", "remove it");
		  entry->fileName[0] = 0xe5;
		  writeentry(disk_ptr, dir);
		}
	      end_line();
	    }
	}
    }
}

void check_files(disk *disk_ptr)
{
  directory *root;
  unsigned short no;

  prhead("ファイルチェック中です", "Checking files");
  for (no = 2; no < disk_ptr->cluster.num; ++no)
    disk_ptr->FAT.buf[no].subdir = FAT_SUBDIR_NOTYET;
  root = rootdir(disk_ptr);
  check_subdir(disk_ptr, root);
  freedir(root);
  end_section();
}


static char *num_to_hex = "0123456789ABCDEF";

void find_lost_files(disk *disk_ptr)
{
  unsigned short no;
  directory *root, *lost_found = NULL;
  direntry *entry;
  unsigned long datetime = 0;
  int found = 0;

  prhead("紛失ファイルサーチ中です", "Finding lost files");
  if (flags.writing)
    {
      datetime = current_datetime();
      root = rootdir(disk_ptr);
      found = 0;
      entry = NULL;
      while (nextentry(disk_ptr, root))
	{
	  entry = root->entry;
	  if (strncmp(entry->fileName, "lost+fou", 8))
	    continue;
	  if (strcmp(entry->fileName2, "nd"))
	    continue;
	  lost_found = subdir(disk_ptr, root->file_path, get_cluster(entry));
	  freedir(root);
	  found = 1;
	}
      if (!found)
	{
	  freedir(root);
	  lost_found = rootdir(disk_ptr);
	}
    }
  for (no = 2; no < disk_ptr->cluster.num; ++no)
    {
      switch (disk_ptr->FAT.buf[no].type)
	{
	case FAT_TYPE_LINKING:
	case FAT_TYPE_END:
	  if (disk_ptr->FAT.buf[no].refered == FAT_REFERED_NO &&
	      disk_ptr->FAT.buf[no].badsec == FAT_BADSEC_OK)
	    {
	      prerr_int("クラスタ %04X 以降は、紛失ファイルです",
			"cluster %04X has a lost file", no);
	      if (flags.writing)
		{
		  int next;

		  end_record();
		  prerr_("これを復元します", "save it");
		  next = nextfreeentry(disk_ptr, lost_found);
		  if (!next && found)
		    {
		      found = 0;
		      freedir(lost_found);
		      lost_found = rootdir(disk_ptr);
		      next = nextfreeentry(disk_ptr, lost_found);
		    }
		  if (next)
		    {
		      int offset;

		      entry = lost_found->entry;
		      entry->fileName[0] = num_to_hex[(no >> 12) & 15];
		      entry->fileName[1] = num_to_hex[(no >>  8) & 15];
		      entry->fileName[2] = num_to_hex[(no >>  4) & 15];
		      entry->fileName[3] = num_to_hex[(no      ) & 15];
		      for (offset = 4; offset < 8; ++offset)
			entry->fileName[offset] = ' ';
		      for (offset = 0; offset < 10; ++offset)
			entry->fileName2[offset] = 0;
		      for (offset = 0; offset < 3; ++offset)
			entry->ext[offset] = ' ';
		      entry->atr = 0x20;
		      put_cluster(entry, no);
		      put_datetime(entry, datetime);
		      put_length(entry, expected_length(disk_ptr, no, -1));
		      writeentry(disk_ptr, lost_found);
		      disk_ptr->FAT.buf[no].refered = FAT_REFERED_ONE;
		      /* write_FAT(disk_ptr, no); */
		    }
		  else
		    {
		      end_record();
		      prerr_("lost+found 及び root にこれ以上領域が無いため、"
			     "復元出来ませんでした",
			     "cannot save to lost+found and/or root directory");
		      no = disk_ptr->cluster.num;
		    }
		}
	      end_line();
	    }
	  break;
	default:
	  break;
	}
    }
  if (flags.writing)
    freedir(lost_found);
  end_section();
}

static void collect_bad_direntry(disk *disk_ptr, directory *dir,
				 unsigned long my_datetime,
				 unsigned short my_cluster,
				 unsigned char my_attribute,
				 unsigned long parent_datetime,
				 unsigned short parent_cluster,
				 unsigned char parent_attribute)
{
  while (nextentry(disk_ptr, dir))
    {
      direntry *entry;

      entry = dir->entry;
      if (entry->fileName[0] == '.')
	{
	  if (entry->fileName[1] == ' ')
	    {
	      unsigned short now_cluster;
	      unsigned long now_datetime;
	      unsigned char now_attribute;

	      now_cluster = get_cluster(entry);
	      now_datetime = get_datetime(entry);
	      now_attribute = entry->atr;
	      if (now_cluster != my_cluster ||
		  (flags.unix_like && (now_datetime != my_datetime ||
				       now_attribute != my_attribute)))
		{
		  prerr_str("ディレクトリ %s は、", "directory %s", dir->file_path);
		  if (now_cluster != my_cluster)
		    prerr_int("不正クラスタ %04X を指して",
			      " points bad cluster No %04X", now_cluster);
		  if (now_cluster != my_cluster && flags.unix_like &&
		      (now_datetime != my_datetime || now_attribute != my_attribute))
		    prerr_("いて更に、", ", and");
		  if (flags.unix_like && now_datetime != my_datetime)
		    {
		      prerr_("日時 ", " has wrong datetime ");
		      prerr_datetime(now_datetime);
		    }
		  if (flags.unix_like && now_datetime != my_datetime &&
		      now_attribute != my_attribute)
		    prerr_(", ", SAME);
		  if (flags.unix_like && now_attribute != my_attribute)
		    {
		      prerr_("属性 ", " has wrong attribute ");
		      prerr_attribute(now_attribute);
		    }
		  if (flags.unix_like && (now_datetime != my_datetime ||
					  now_attribute != my_attribute))
		    prerr_(" が間違って", "");
		  prerr_("います", "");
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("これを修正します", "fix it");
		      put_cluster(entry, my_cluster);
		      if (flags.unix_like)
			{
			  put_datetime(entry, my_datetime);
			  entry->atr = my_attribute;
			}
		      writeentry(disk_ptr, dir);
		    }
		  end_line();
		}
#ifdef DEBUG
	      if (debug.link)
		end_line();
#endif
	      continue;
	    }
	  if (entry->fileName[1] == '.' && entry->fileName[2] == ' ')
	    {
	      unsigned short now_cluster;
	      unsigned long now_datetime;
	      unsigned char now_attribute;

	      now_cluster = get_cluster(entry);
	      now_datetime = get_datetime(entry);
	      now_attribute = entry->atr;
	      if (now_cluster != parent_cluster ||
		  (flags.unix_like && (now_datetime != parent_datetime ||
				       now_attribute != parent_attribute)))
		{
		  prerr_str("ディレクトリ %s は、", "directory %s", dir->file_path);
		  if (now_cluster != parent_cluster)
		    prerr_int("不正クラスタ %04X を指して",
			      " points bad cluster No %04X", now_cluster);
		  if (now_cluster != parent_cluster && flags.unix_like &&
		      (now_datetime != parent_datetime ||
		       now_attribute != parent_attribute))
		    prerr_("いて更に、", ", and");
		  if (flags.unix_like && now_datetime != parent_datetime)
		    {
		      prerr_("日時 ", " has wrong datetime ");
		      prerr_datetime(now_datetime);
		    }
		  if (flags.unix_like && now_datetime != parent_datetime &&
					 now_attribute != parent_attribute)
		    prerr_(", ", SAME);
		  if (flags.unix_like && now_attribute != parent_attribute)
		    {
		      prerr_("属性 ", " has wrong datetime ");
		      prerr_attribute(now_attribute);
		    }
		  if (flags.unix_like && (now_datetime != parent_datetime ||
					  now_attribute != parent_attribute))
		    prerr_(" が間違って", "");
		  prerr_("います", "");
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("これを修正します", "fix it");
		      put_cluster(entry, parent_cluster);
		      if (flags.unix_like)
			{
			  put_datetime(entry, parent_datetime);
			  entry->atr = parent_attribute;
			}
		      writeentry(disk_ptr, dir);
		    }
		  end_line();
		}
#ifdef DEBUG
	      if (debug.link)
		end_line();
#endif
	      continue;
	    }
	}

      {
	unsigned short file_cluster;

	file_cluster = get_cluster(entry);
	if (file_cluster < 2 || disk_ptr->cluster.num <= file_cluster ||
	    disk_ptr->FAT.buf[file_cluster].type == FAT_TYPE_OTHERS ||
	    disk_ptr->FAT.buf[file_cluster].type == FAT_TYPE_FREE)
	  continue;
      }
      if (is_file_or_directory(entry->atr))
	{
	  unsigned short file_cluster;

	  file_cluster = get_cluster(entry);
#ifdef DEBUG
	  if (debug.link)
	    print_(" (", SAME);
#endif
	  while (1)
	    {
#ifdef DEBUG
	      if (debug.link)
		{
		  if (disk_ptr->FAT.buf[file_cluster].refered == FAT_REFERED_MANY)
		    print_("*", SAME);
		  print_int("%04X", SAME, file_cluster);
		  end_record();
		}
#endif
	      if (disk_ptr->FAT.buf[file_cluster].refered == FAT_REFERED_MANY)
		{
		  collect(dir);
#ifdef DEBUG
		  if (debug.link)
		    {
		      while (disk_ptr->FAT.buf[file_cluster].refer == FAT_REFER_OK)
			{
			  file_cluster = disk_ptr->FAT.buf[file_cluster].value;
			  print_int("%04X", SAME, file_cluster);
			  end_record();
			  if (escape())
			    break;
			}
		    }
#endif
		  break;
		}
	      if (disk_ptr->FAT.buf[file_cluster].refer != FAT_REFER_OK)
		break;
	      file_cluster = disk_ptr->FAT.buf[file_cluster].value;
	    }
#ifdef DEBUG
	  if (debug.link)
	    {
	      file_cluster = disk_ptr->FAT.buf[file_cluster].value;
	      print_int("%04X)", SAME, file_cluster);
	      end_line();
	    }
#endif
        }

      if (is_directory(entry->atr))
	{
	  unsigned long child_datetime;
	  unsigned short child_cluster;
	  unsigned char child_attribute;
	  directory *child_dir;

	  child_cluster = get_cluster(entry);
	  if (disk_ptr->FAT.buf[child_cluster].subdir == FAT_SUBDIR_NOTYET)
	    {
	      disk_ptr->FAT.buf[child_cluster].subdir = FAT_SUBDIR_ALREADY;
	      child_datetime = get_datetime(entry);
	      child_attribute = entry->atr;
	      child_dir = subdir(disk_ptr, dir->file_path, child_cluster);
	      if (get_length(entry) != 0)
		{
		  prerr_str("ディレクトリ %s のサイズが間違っています",
			    "directory %s has non zero length", dir->file_path);
		  if (flags.writing)
		    {
		      end_record();
		      prerr_("これを修正します", "fix it");
		      put_length(entry, 0);
		      writeentry(disk_ptr, dir);
		    }
		  end_line();
		}
	      collect_bad_direntry(disk_ptr, child_dir,
				   child_datetime, child_cluster, child_attribute,
				   my_datetime, my_cluster, my_attribute);
	      freedir(child_dir);
	    }
	  else
	    {
	      prerr_str("ディレクトリ %s はループしています",
			"directory %s is a loop", dir->file_path);
	      if (flags.writing)
		{
		  end_record();
		  prerr_("これを削除します", "remove it");
		  entry->fileName[0] = 0xe5;
		  writeentry(disk_ptr, dir);
		}
	      end_line();
	    }
	}
      if (is_file_or_symlink(entry->atr))
	{
	  unsigned short file_cluster;
	  int file_length, real_length;

	  file_cluster = get_cluster(entry);
	  file_length = get_length(entry);
	  real_length = expected_length(disk_ptr, file_cluster, file_length);
	  if (real_length != file_length)
	    collect(dir);
	}
    }
}

static void warning_duplication(disk *disk_ptr, directory *dir)
{
  direntry *entry;
  unsigned short file_cluster;
  unsigned short duplicated_cluster;

  duplicated_cluster = dir->batting_cluster_max;
  readentry(disk_ptr, dir);
  entry = dir->entry;
  file_cluster = get_cluster(entry);
  if (file_cluster == 0)
    {
      freeentrybuffer(disk_ptr, dir);
      return;
    }
  if (flags.verbose)
    {
      if (is_directory(entry->atr))
	prerr_("ディレクトリ", "directory");
      else if (is_symlink(entry->atr))
	prerr_("シンボリックリンク", "symbolic link");
      else
	prerr_("ファイル", "file");
      prerr_str(" %s は、以下のものと連結しています",
		" %s is connected with the follows", dir->file_path);
      end_line();
    }
  if (file_cluster == duplicated_cluster)
    {
      disk_ptr->FAT.buf[file_cluster].refered = FAT_REFERED_FIXED;
      freeentrybuffer(disk_ptr, dir);
      return;
    }
  while (disk_ptr->FAT.buf[file_cluster].value != duplicated_cluster)
    file_cluster = disk_ptr->FAT.buf[file_cluster].value;
  disk_ptr->FAT.buf[file_cluster].refer = FAT_REFER_FIXED;
  file_cluster = disk_ptr->FAT.buf[file_cluster].value;
  disk_ptr->FAT.buf[file_cluster].refered = FAT_REFERED_FIXED;
  write_FAT(disk_ptr, file_cluster);
  freeentrybuffer(disk_ptr, dir);
}

static void fix_duplication(disk *disk_ptr, directory *dir)
{
  direntry *entry;
  unsigned short file_cluster;
  unsigned short duplicated_cluster;

  duplicated_cluster = dir->batting_cluster_max;
  readentry(disk_ptr, dir);
  entry = dir->entry;
  file_cluster = get_cluster(entry);
  if (file_cluster == 0)
    {
      freeentrybuffer(disk_ptr, dir);
      return;
    }
  if (flags.verbose)
    {
      if (is_directory(entry->atr))
	print_("ディレクトリ", "directory");
      else if (is_symlink(entry->atr))
	print_("シンボリックリンク", "symbolic link");
      else
	print_("ファイル", "file");
      print_str(" %s", SAME, dir->file_path);
    }
  if (file_cluster == duplicated_cluster)
    {
      if (flags.writing)
	{
	  end_record();
	  prerr_("これを削除します", "remove it");
	  entry->fileName[0] = 0xe5;
	  writeentry(disk_ptr, dir);
	  adjust(disk_ptr, dir);
	}
      end_line();
      freeentrybuffer(disk_ptr, dir);
      return;
    }
  if (flags.writing)
    {
      while (disk_ptr->FAT.buf[file_cluster].value != duplicated_cluster)
        file_cluster = disk_ptr->FAT.buf[file_cluster].value;
      if (disk_ptr->FAT.buf[file_cluster].refer != FAT_REFER_FIXED)
	{
	  end_record();
	  prerr_("これを切り詰めます", "truncate it");
	  disk_ptr->FAT.buf[file_cluster].value = 0xffff;
	  write_FAT(disk_ptr, file_cluster);
	  disk_ptr->FAT.buf[file_cluster].type = FAT_TYPE_END;
	  put_length(entry, expected_length(disk_ptr, get_cluster(entry), -1));
	  writeentry(disk_ptr, dir);
	  adjust(disk_ptr, dir);
	}
    }
  end_line();
  disk_ptr->FAT.buf[file_cluster].refer = FAT_REFER_FIXED;
  freeentrybuffer(disk_ptr, dir);
}

static void fix_file_length(disk *disk_ptr, directory *dir)
{
  direntry *entry;
  unsigned short file_cluster;
  int file_length, real_length;

  readentry(disk_ptr, dir);
  entry = dir->entry;
  if (is_directory(entry->atr))
    return;
  file_cluster = get_cluster(entry);
  file_length = get_length(entry);
  real_length = expected_length(disk_ptr, file_cluster, file_length);
  if (real_length < file_length)
    {
      prerr_str("ファイル %s は表示より実体の方が短いです",
		"file %s has really shorter length", dir->file_path);
      if (flags.writing)
	{
	  end_record();
	  prerr_("これを切り詰めます", "truncate it");
	  put_length(entry, real_length);
	  writeentry(disk_ptr, dir);
	  adjust(disk_ptr, dir);
	}
      end_line();
    }
  else if (real_length > file_length)
    {
      prerr_str("ファイル %s は表示より実体の方が長いです",
		"file %s has really longer length", dir->file_path);
      if (flags.writing)
	{
	  end_record();
	  prerr_("これを拡大します", "stretch it");
	  put_length(entry, real_length);
	  writeentry(disk_ptr, dir);
	  adjust(disk_ptr, dir);
	}
      end_line();
    }
  freeentrybuffer(disk_ptr, dir);
}

void fix_files(disk *disk_ptr)
{
  unsigned long datetime;
  unsigned char attribute;
  directory *dir;
  directory *root;

  prhead("パスチェック中です", "Checking path names");
  datetime = 0;
  attribute = 0x10;
  root = rootdir(disk_ptr);
  while (nextentry(disk_ptr, root))
    {
      direntry *entry;

      entry = root->entry;
      if (entry->fileName[0] == '.' && entry->fileName[1] == ' ')
	{
	  datetime = get_datetime(entry);
	  attribute = entry->atr;
	  break;
	}
    }
  freedir(root);
#ifdef DEBUG
  end_line();
  if (debug.link)
    debug.path = 1;
#endif
  {
    unsigned short no;

    root = rootdir(disk_ptr);
    for (no = 2; no < disk_ptr->cluster.num; ++no)
	disk_ptr->FAT.buf[no].subdir = FAT_SUBDIR_NOTYET;
    collect_bad_direntry(disk_ptr, root, datetime, 0, attribute,
			 datetime, 0, attribute);
    freedir(root);
    while (is_there_more_duplication(disk_ptr, &dir))
      {
	warning_duplication(disk_ptr, dir);
	while (its_duplicators(&dir))
	  fix_duplication(disk_ptr, dir);
      }
    while (next_collection(&dir))
      fix_file_length(disk_ptr, dir);
  }
  end_section();
}
