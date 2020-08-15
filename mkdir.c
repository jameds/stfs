/*
fuse operation: mkdir
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <time.h>
#include <string.h>
#include "fs.h"

int
fs_mkdir (path, mode)
	const char * path;
	mode_t       mode;
{
	sqlite3_stmt * s;
	const char   * p;
	char         * t;

	sqlite3_int64 inode;

	p = strrchr(path, '/');/* only insert the last part */

	if (( t = strstr2(path, "/.for/") ))
	{
		if (t < p && strchr(t, '/') == p)
		{
			inode = strtonode(&t);
		}
		else
		{
			return -EINVAL;
		}
	}
	else
	{
		inode = 0;
	}

	if (tag_is_valid(&p[1]))
	{
		s = db_prepare("INSERT INTO `tags` (`name`,`time`) VALUES (?,?);");

		sqlite3_bind_string (s, 1, &p[1]);
		sqlite3_bind_int    (s, 2, time(NULL));

		switch (db_finish(s))
		{
			case SQLITE_DONE:
				if (inode)
				{
					s = db_prepare(
							"INSERT INTO `mappings` (`inode,`tag`)"
							"VALUES (?, last_insert_rowid())"
					);
					break;
				}
				else
				{
					return 0;
				}

			case SQLITE_CONSTRAINT:
				if (inode)
				{
					s = db_prepare(
							"INSERT INTO `mappings` (`inode`,`tag`) VALUES (?,"
							"( SELECT ROWID FROM `tags` WHERE `name`=? ))"
					);

					sqlite3_bind_string (s, 2, &p[1]);
					break;
				}
				else
				{
					return -EEXIST;
				}

			default:
				return -EIO;
		}

		sqlite3_bind_int64(s, 1, inode);

		switch (db_finish(s))
		{
			case SQLITE_DONE:
				return 0;
			case SQLITE_CONSTRAINT:
				return -EEXIST;
			default:
				return -EIO;
		}
	}
	else
	{
		return -EIO;
	}
}
