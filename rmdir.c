/* fuse operation: rmdir */

/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <string.h>
#include "fs.h"

static void
delete_tag (sql, tag)
	const char    * sql;
	sqlite3_int64   tag;
{
	sqlite3_stmt * s = db_prepare(sql);
	sqlite3_bind_int64(s, 1, tag);
	db_finish(s);
}

int
fs_rmdir (path)
	const char * path;
{
	char * p;

	char * lhs;
	int    n;

	sqlite3_stmt * s;
	sqlite3_int64 tag;

	p = strrchr(path, '/');

	if (tag_is_valid(&p[1]))
	{
		if (p > path)
		{
			n = ( p - (char *)rmemchr(&p[-1], '/', path) );

			if (
					strncmp(&p[-n], "/.under", n) == 0 ||
					strncmp(&p[-n], "/.above", n) == 0
			){
				s = db_prepare(
						"DELETE FROM `inheritance`"
						"WHERE `parent`=( SELECT ROWID FROM `tags` WHERE `name`=? )"
						"AND   `child`=(  SELECT ROWID FROM `tags` WHERE `name`=? )"
				);

				lhs = rmemchr(&p[-n-1], '/', path) + 1;
				n   = ( &p[-n] - lhs );

				return run_inheritance_statement(s, p, lhs, n, &p[1]);
			}
		}

		s = db_prepare("SELECT ROWID FROM `tags` WHERE `name`=?;");

		sqlite3_bind_string(s, 1, &p[1]);

		if (sqlite3_step(s) == SQLITE_ROW)
		{
			tag = sqlite3_column_int64(s, 0);

			sqlite3_finalize(s);

			delete_tag("DELETE FROM `tags`     WHERE ROWID=?", tag);
			delete_tag("DELETE FROM `mappings` WHERE `tag`=?", tag);

			delete_tag(
					"DELETE FROM `inheritance`"
					"WHERE `parent`=?1 OR `child`=?1",

					tag
			);

			return 0;
		}
		else
		{
			sqlite3_finalize(s);
			return -ENOENT;
		}
	}
	else
	{
		return -EINVAL;
	}
}

