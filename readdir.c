/*
fuse operation: readdir
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "fs.h"

static sqlite3_stmt *
prepare_with_previous_tag (sql, path, p)
	const char * sql;
	const char * path;
	const char * p;
{
	sqlite3_stmt * s;

	int n;

	n = ( p - (char *)rmemchr(&p[-1], '/', path) ) - 1;

	s = db_prepare(sql);
	sqlite3_bind_text(s, 1, &p[-n], n, NULL);

	return s;
}

int
fs_readdir (path, fill_buffer, fill, offset, file_info, flags)
	const char                * path;
	void                      * fill_buffer;
	fuse_fill_dir_t             fill;
	off_t                       offset;
	struct fuse_file_info     * file_info;
	enum   fuse_readdir_flags   flags;
{
#define easy_fill( name ) \
	fill(fill_buffer, name, NULL, 0, 0)

	sqlite3_stmt *s;

	int select_inodes = 0;
	int inodes_rows   = 0;

	char *p;
	int   n;

	char * name = NULL;

	if (strcmp(path, "/") == 0)
	{
		easy_fill ("@");/* special dir. of inodes */

		s = db_prepare("SELECT `name` FROM `tags`;");
	}
	else if (strcmp(path, "/@") == 0)
	{
		s = db_prepare("SELECT ROWID, `master`, `page` FROM `inodes`;");

		inodes_rows = 1;
	}
	else if (rcmp(path, "/@") == 0)
	{
		select_inodes_from_tags(path);

		s = db_prepare(
				"SELECT ROWID,   `master`, `page` FROM `inodes`"
				"WHERE  ROWID IN ( SELECT `inode` FROM `select_inodes` );"
		);

		select_inodes = 1;
		inodes_rows   = 1;
	}
	else
	{
		p = strrchr(path, '/');

#define SELECT_INHERITANCE( p, c1, c2 ) \
		s = prepare_with_previous_tag(\
				"SELECT `name`   FROM `tags`        WHERE ROWID IN ("\
				"SELECT `" c1 "` FROM `inheritance` WHERE `" c2 "`=("\
				"SELECT  ROWID   FROM `tags`        WHERE `name`=? )"\
				");"\
				, path, p\
		)

		if (strcmp(p, "/.under") == 0)
		{
			SELECT_INHERITANCE (p, "child", "parent");
		}
		else if (strcmp(p, "/.above") == 0)
		{
			SELECT_INHERITANCE (p, "parent", "child");
		}
		else
		{
			easy_fill ("@");     /* special dir. of inodes */
			easy_fill (".under");/* special dir. of children tags */
			easy_fill (".above");/* special dir. of parent tags */
			easy_fill ("+");/* add inodes with this tag, basically OR */
			easy_fill ("-");/* discard inodes with this tag */
			easy_fill ("~");/* discard with no regard for order of operations */
			/* exact versions of the discard functions only discard the one tag */
			easy_fill ("--");
			easy_fill ("~~");

			s = db_prepare("SELECT `name` FROM `tags`");
		}

#undef SELECT_INHERITANCE
	}

	if (inodes_rows)
	{
		n    = snprintf(NULL, 0, "%lld.%d", INT64_MAX, INT_MAX);
		name = malloc(n + 1);
	}

	while (sqlite3_step(s) == SQLITE_ROW)
	{
		if (inodes_rows)
		{
			if (sqlite3_column_type(s, 2) == SQLITE_INTEGER)
			{
				sprintf(name,
						"%s.%s",
						sqlite3_column_text(s, 1),
						sqlite3_column_text(s, 2)
				);

				easy_fill(name);

				continue;
			}
		}

		easy_fill(sqlite3_column_text(s, 0));
	}

	free(name);

	sqlite3_finalize(s);

	if (select_inodes)
	{
		delete_select_tags_table();
	}

	return 0;
}
