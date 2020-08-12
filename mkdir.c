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

	p = strrchr(path, '/');/* only insert the last part */

	if (tag_is_valid(&p[1]))
	{
		s = db_prepare("INSERT INTO `tags` (`name`,`time`) VALUES (?,?);");

		sqlite3_bind_string (s, 1, &p[1]);
		sqlite3_bind_int    (s, 2, time(NULL));

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
