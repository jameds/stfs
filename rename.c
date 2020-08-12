/*
fuse operation: rename
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

int
fs_rename (from, to, mode)
	const char   * from;
	const char   * to;
	unsigned int   mode;
{
	char *p;
	char *t;

	char *old_name;

	sqlite3_int64 inode;
	sqlite3_int64 master;
	int           page;

	sqlite3_stmt * s;

	struct part part;

	int n;

	if (
			( old_name = strstr2(from,  "/@/") ) != NULL ||
			( old_name = strstr2(from, "/~@/") ) != NULL
	){
		inode = strtoll(old_name, &p, 10);

		if (*p == '.')
		{
			s = db_prepare(
					"SELECT ROWID FROM `inodes`"
					"WHERE `master`=? AND `page`=?"
			);

			sqlite3_bind_int64  (s, 1, inode);
			sqlite3_bind_string (s, 2, &p[1]);

			if (sqlite3_step(s) == SQLITE_ROW)
			{
				inode = sqlite3_column_int64(s, 0);
			}

			if (sqlite3_finalize(s) != SQLITE_OK)
			{
				return -EIO;
			}
		}

		if (( p = strstr3(to, "/@/", &t) ) != NULL)
		{
			errno = 0;/* underflow/overflow, though, really? =P */
			master = strtoll(t, &t, 10);

			if (*t == '.')
			{
				page = strtol(&t[1], &t, 10);

				if (page < 1)
				{
					return -EINVAL;
				}
			}
			else
			{
				page = 0;
			}

			if (*t != '\0' || errno)
			{
				return -EINVAL;
			}

			if (page < 1 && master != inode)
			{
				return -EINVAL;
			}

			if (strcmp(old_name, &p[3]) != 0)
			{
				s = db_prepare(
						"UPDATE `inodes` SET `master` = ?, `page` = ?"
						"WHERE ROWID=?;"
				);

				if (page > 0)
				{
					sqlite3_bind_int64 (s, 1, master);
					sqlite3_bind_int   (s, 2, page);
				}

				sqlite3_bind_int64(s, 3, inode);

				if (db_finish(s) != SQLITE_DONE)
				{
					return -EEXIST;
				}
			}

			s = db_prepare(
					"INSERT INTO `mappings` (`inode`,`tag`) VALUES (?,"
					"( SELECT ROWID FROM `tags` WHERE `name`=? )"
					");"
			);

			sqlite3_bind_int64(s, 1, inode);

			explode(&part, to, "/");

			while (advance(&part) && part.part < p)
			{
				if (tag_is_valid(part.part))
				{
					sqlite3_bind_text(s, 2, part.part, part.length, NULL);

					if (sqlite3_step(s) != SQLITE_DONE)
					{
						warn(
								"rename %s -> %s, at tag '%.*s', %s\n",
								from,
								to,
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
		else
		{
			return -EINVAL;
		}
	}
	else if (
			( p = strstr3(to, "/.under/", &t) ) != NULL ||
			( p = strstr3(to, "/.above/", &t) ) != NULL
	){
		from = strrchr(from, '/');

		/* the tag names must match! */
		if (strcmp(&from[1], t) != 0)
		{
			return -EINVAL;
		}

		n = ( p - (char *)rmemchr(&p[-1], '/', to) ) - 1;

		s = db_prepare(
				"INSERT INTO `inheritance` (`parent`,`child`) VALUES ("
				"( SELECT ROWID FROM `tags` WHERE `name`=? ),"
				"( SELECT ROWID FROM `tags` WHERE `name`=? ));"
		);

		return run_inheritance_statement(s, &t[-1], &p[-n], n, t);
	}
	else
	{
		if (
				strstr(to,  "/@/") == NULL &&
				strstr(to, "/~@/") == NULL
		){
			from = strrchr(from, '/') + 1;
			to   = strrchr(to,   '/') + 1;

			if (tag_is_valid(from) && tag_is_valid(to))
			{
				s = db_prepare("UPDATE `tags` SET `name` = ? WHERE `name`=?");

				sqlite3_bind_string(s, 2, from);
				sqlite3_bind_string(s, 1, to);

				if (db_finish(s) == SQLITE_DONE)
				{
					if (sqlite3_changes(db) > 0)
					{
						return 0;
					}
					else
					{
						return -ENOENT;
					}
				}
				else
				{
					return -EEXIST;
				}
			}
			else
			{
				return -EINVAL;
			}
		}
		else
		{
			return -EINVAL;
		}
	}
}
