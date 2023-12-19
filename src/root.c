/*
 * $Project: X68k File System ChecKer $
 * $Copyright: 1992 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/fsck/root.c,v $
 * $Author: src $
 * $Revision: 1.1 $
 * $Date: 1993/03/26 22:49:30 $
 * $Log: root.c,v $
 *	Revision 1.1  1993/03/26  22:49:30  src
 *	version 0.05a
 *
 *	Revision 2.2  1993/01/09  09:46:01  kawamoto
 *	version 0.5
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

#include "fsck.h"
#include "dir.h"

void check_root_sub(disk *disk_ptr, directory *root,
		    unsigned long datetime, int second_char)
{
  direntry *entry;
  int next;

  end_record();
  prerr_("これを作成します", "make it");
  next = nextfreeentry(disk_ptr, root);
  if (next)
    {
      int offset;

      entry = root->entry;
      entry->fileName[0] = '.';
      entry->fileName[1] = second_char;
      for (offset = 2; offset < 8; ++offset)
	entry->fileName[offset] = ' ';
      for (offset = 0; offset < 10; ++offset)
	entry->fileName2[offset] = 0;
      for (offset = 0; offset < 3; ++offset)
	entry->ext[offset] = ' ';
      entry->atr = 0x10;
      put_cluster(entry, 0);
      put_datetime(entry, datetime);
      put_length(entry, 0);
      writeentry(disk_ptr, root);
    }
  else
    {
      end_record();
      prerr_("root にこれ以上領域が無いため、作成出来ませんでした",
	     "cannot make to root directory");
    }
}

void check_root(disk *disk_ptr)
{
  unsigned long datetime;
  int have_period, have_period_twice;
  directory *root;

  prhead("ルートチェック中です", "Checking root directory");
  root = rootdir(disk_ptr);
  have_period = 0;
  have_period_twice = 0;
  while (nextentry(disk_ptr, root))
    {
      direntry *entry;

      entry = root->entry;
      if (entry->fileName[0] != '.')
	continue;
      if (entry->fileName[1] != '.' && entry->fileName[1] != ' ')
	continue;
      if (entry->fileName[2] != ' ')
	continue;
      if (flags.unix_like)
	{
	  if (entry->fileName[1] == ' ')
	    have_period = 1;
	  else
	    have_period_twice = 1;
	  continue;
	}
      prerr_("ルートに不当なエントリ `.", "there exist a illegal entry `.");
      if (entry->fileName[1] == '.')
	prerr_(".", SAME);
      prerr_("' が有ります", "' on the root directory");
      if (flags.writing)
	{
	  end_record();
	  prerr_("これを削除します", "remove it");
	  entry->fileName[0] = 0xe5;
	  writeentry(disk_ptr, root);
	}
      end_line();
    }
  freedir(root);
  if (!flags.unix_like || have_period && have_period_twice)
    {
      end_section();
      return;
    }
  root = rootdir(disk_ptr);
  datetime = current_datetime();
  if (have_period == 0)
    {
      prerr_("ルートにエントリ `.' が有りません",
	     "there exist no entry `.' on the root directory");
      if (flags.writing)
	check_root_sub(disk_ptr, root, datetime, ' ');
      end_line();
    }
  if (have_period == 0)
    {
      prerr_("ルートにエントリ `..' が有りません",
	     "there exist no entry `..' on the root directory");
      if (flags.writing)
	check_root_sub(disk_ptr, root, datetime, '.');
      end_line();
    }
  freedir(root);
  end_section();
}
