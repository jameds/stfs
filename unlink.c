/*
fuse operation: unlink
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <string.h>
#include "fs.h"

static sqlite3_stmt *
select_page (name, dot, sql)
	const char * sql;
	const char * name;
	const char * dot;
{
	sqlite3_stmt * s = db_prepare(sql);
	sqlite3_bind_text   (s, 1, name, ( dot - name ), NULL);
	sqlite3_bind_string (s, 2, &dot[1]);
	return s;
}

int
fs_unlink (path)
	const char * path;
{
	char * name;
	char * dot;

	sqlite3_stmt * s;
	sqlite3_int64 inode;

	struct part part;

	name = strrchr(path, '/') + 1;
	dot  =  strchr(name, '.');

	if (
			strncmp(path,  "/@/", 3) == 0 ||
			strncmp(path, "/~@/", 4) == 0
	){
		if (dot != NULL)
		{
			s = select_page(name, dot,
					"DELETE FROM `inodes`"
					"WHERE `master`=? AND `page`=?;"
			);
		}
		else
		{
			s = db_prepare("DELETE FROM `inodes` WHERE ROWID=?");

			sqlite3_bind_string(s, 1, name);
		}

		switch (db_finish(s))
		{
			case SQLITE_DONE:
				return 0;
			default:
				return -EIO;
		}
	}
	else
	{
		if (dot != NULL)
		{
			s = select_page(name, dot,
					"SELECT ROWID FROM `inodes`"
					"WHERE `master`=? AND `page`=?;"
			);

			if (sqlite3_step(s) == SQLITE_ROW)
			{
				inode = sqlite3_column_int64(s, 0);

				sqlite3_finalize(s);
			}
			else
			{
				sqlite3_finalize(s);
				return -EIO;
			}
		}

		s = db_prepare(
				"DELETE FROM `mappings` WHERE `inode`=? AND `tag`="
				"( SELECT ROWID FROM `tags` WHERE `name`=? );"
		);

		if (dot != NULL)
		{
			sqlite3_bind_int64(s, 1, inode);
		}
		else
		{
			sqlite3_bind_string(s, 1, name);
		}

		explode(&part, path, "/");

		while (advance_tag(&part))
		{
			if (tag_is_valid(part.part))
			{
				sqlite3_bind_text(s, 2, part.part, part.length, NULL);

				if (sqlite3_step(s) != SQLITE_DONE)
				{
					warn(
							"error removing inode %s from tag '%.*s': %s\n",
							&name[1],
							part.length,
							part.part,
							sqlite3_errmsg(db)
					);
				}

				sqlite3_reset(s);
			}
		}

		sqlite3_finalize(s);

		return 0;
	}
}
