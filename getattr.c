/* fuse operation: getattr */

/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fs.h"

#define COUNT_INHERITANCE_SQL( side ) \
	"SELECT COUNT(*) FROM `inheritance` WHERE `" side "`=(" \
	"SELECT ROWID    FROM `tags`        WHERE `name`=?)"

static int
stat_db (st)
	struct stat * st;
{
	if (fstat(db_fd, st) == 0)
	{
		/* fuck this is ugly!!! */
		if (( st->st_mode & 0700 )) st->st_mode |= 0700;
		if (( st->st_mode & 0070 )) st->st_mode |= 0070;
		if (( st->st_mode & 0007 )) st->st_mode |= 0007;

		st->st_ino   = 0;
		st->st_mode  = ( st->st_mode & ~(S_IFMT) ) | S_IFDIR;

		return 0;
	}
	else
	{
		return -1;
	}
}

static void
backstep_path_part (p, n, path)
	char       ** p;
	int         * n;
	const char  * path;
{
	(*n) = ( *p - (char *)rmemchr(&(*p)[-1], '/', path) );
	(*p) = &(*p)[-(*n)];
}

int
fs_getattr (path, st, file_info)
	const char            * path;
	struct stat           * st;
	struct fuse_file_info * file_info;
{
	const char   * q = NULL;
	sqlite3_stmt * s = NULL;

	char         * p;
	char         * t;
	int n = -1;

	sqlite3_int64 inode;
	int           page;

	int    rows;
	time_t now;
	time_t mtime;

	int ok;

	enum
	{
		NONE,
		TAGS,
		INODES,
	}
	select_table = NONE;

	enum
	{
		NOTPOG,
		AFTER,
		BEFORE,
	}
	pog = NOTPOG;

	struct part part;

	if (strcmp(path, "/") == 0)
	{
		q = "SELECT COUNT(*) FROM `tags`;";
	}
	else if (strcmp(path, "/@") == 0)
	{
		q = "SELECT COUNT(*) FROM `inodes`;";
	}
	else if (( p = strstr2(path, "/@/") ))
	{
		errno = 0;/* underflow/overflow, though, really? =P */
		inode = strtoll(p, &p, 10);

		if (*p == '.')
		{
			page = strtol(&p[1], &p, 10);

			if (page < 1)
			{
				return -EINVAL;
			}
		}
		else
		{
			page = 0;
		}

		if (*p != '\0' || errno)
		{
			return -EINVAL;
		}

		if (fstat(db_fd, st) == 0)
		{
			if (page > 0)
			{
				s = db_prepare(
						"SELECT length(`path`), `ctime`,`mtime`, ROWID FROM"
						"`inodes` WHERE `master`=? AND `page`=?"
				);

				sqlite3_bind_int(s, 2, page);
			}
			else
			{
				s = db_prepare(
						"SELECT length(`path`), `ctime`,`mtime`"
						"FROM `inodes` WHERE ROWID=?"
				);
			}

			sqlite3_bind_int64(s, 1, inode);

			if (sqlite3_step(s) == SQLITE_ROW)
			{
				if (page > 0)
				{
					inode = sqlite3_column_int64(s, 3);
				}

				st->st_ino   = inode;
				st->st_mode  = S_IFLNK|0777;
				st->st_size  = sqlite3_column_int(s, 0);

				st->st_ctim.tv_sec = sqlite3_column_int(s, 1);
				st->st_mtim.tv_sec = sqlite3_column_int(s, 2);

				sqlite3_finalize(s);

				s = db_prepare("SELECT COUNT(*) FROM `mappings` WHERE `inode`=?;");

				sqlite3_bind_int64(s, 1, inode);
				sqlite3_step(s);

				st->st_nlink = sqlite3_column_int(s, 0);

				errno = 0;
			}
			else
			{
				errno = ENOENT;
			}

			sqlite3_finalize(s);

			return -(errno);
		}
	}
	else if (rcmp(path, "/@") == 0)
	{
		q = "SELECT COUNT(*) FROM `select_inodes`;";

		select_table = INODES;
	}
	else
	{
		p = strrchr(path, '/');

		if (strcmp(p, "/.under") == 0)
		{
			q   = COUNT_INHERITANCE_SQL ("child");
			pog = BEFORE;
		}
		else if (strcmp(p, "/.above") == 0)
		{
			q   = COUNT_INHERITANCE_SQL ("parent");
			pog = BEFORE;
		}
		else
		{
			if (p > path)
			{
				explode(&part, rmemchr(&p[-1], '/', path), "/");
				advance(&part);

				if (
						partcmp(&part, ".under") == 0 ||
						partcmp(&part, ".above") == 0
				){
					q   = COUNT_INHERITANCE_SQL ("parent");
					pog = AFTER;
				}
			}

			if (pog == NOTPOG)
			{
				if (
						strcmp(p, "/+")  == 0 ||
						strcmp(p, "/-")  == 0 ||
						strcmp(p, "/~")  == 0 ||
						strcmp(p, "/--") == 0 ||
						strcmp(p, "/~~") == 0
				){
					ok = 1;
				}
				else
				{
					s = db_prepare("SELECT `time` FROM `tags` WHERE `name`=?;");

					sqlite3_bind_string(s, 1, &p[1]);

					ok = ( sqlite3_step(s) == SQLITE_ROW );

					if (ok)
					{
						mtime = sqlite3_column_int(s, 0);
					}

					sqlite3_finalize(s);
				}

				if (ok)
				{
					if (stat_db(st) == 0)
					{
						if (tag_is_valid(&p[1]))
						{
							st->st_ctime = mtime;
							st->st_mtime = mtime;
						}

						select_all_tags(path);
						select_table = TAGS;

						s = db_prepare(
								"SELECT COUNT(*) FROM `tags`"
								"WHERE ROWID NOT IN `select_tags`;"
						);
					}
				}
				else
				{
					return -ENOENT;
				}
			}
		}
	}

	if (q != NULL)
	{
		if (stat_db(st) == 0)
		{
			if (select_table == INODES)
			{
				select_inodes_from_tags(path);
			}

			s = db_prepare(q);

			switch (pog)
			{
				case BEFORE:
					backstep_path_part(&p, &n, path);
				case AFTER:
					/* p is for pog */
					sqlite3_bind_text(s, 1, &p[1], n, NULL);
			}
		}
	}

	if (s != NULL)
	{
#if 0
		{
			p = sqlite3_expanded_sql(s);
			printf("%s: %s\n", path, p);
			sqlite3_free(p);
		}
#endif
		if (sqlite3_step(s) == SQLITE_ROW)
		{
			st->st_nlink = sqlite3_column_int64(s, 0);
		}

		sqlite3_finalize(s);

		if (select_table == TAGS)
		{
			delete_select_tags_table();
		}
		else if (select_table == INODES)
		{
			delete_select_inodes_table();
		}

		return 0;
	}
	else
	{
		return -(errno);/* the stat must have failed */
	}
}
