/* fuse operation: readlink */

/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <string.h>
#include "fs.h"

int
fs_readlink (path, into, size)
	const char * path;
	char       * into;
	size_t       size;
{
	char * p;
	char * dot;

	sqlite3_stmt * s;

	p   = strrchr(path, '/');
	dot =  strchr(p,    '.');

	if (dot != NULL)
	{
		s = db_prepare(
				"SELECT `path` FROM `inodes`"
				"WHERE `master`=? AND `page`=?;"
		);

		sqlite3_bind_text   (s, 1, &p[1], ( dot - &p[1] ), NULL);
		sqlite3_bind_string (s, 2, &dot[1]);
	}
	else
	{
		s = db_prepare("SELECT `path` FROM `inodes` WHERE ROWID=?;");

		sqlite3_bind_string(s, 1, &p[1]);
	}

	if (sqlite3_step(s) == SQLITE_ROW)
	{
		strncpy(into, sqlite3_column_text(s, 0), size);

		errno = 0;
	}
	else
	{
		errno = ENOENT;
	}

	sqlite3_finalize(s);

	return -(errno);
}
