/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <unistd.h>

#include "fs.h"

static const char * HELP =
"stfs [FUSE options] [database] mountpoint\n"
"Copyright 2020 James R.\n"
"\n";

static const struct fuse_operations filesystem = {
	.getattr  = fs_getattr,
	.mkdir    = fs_mkdir,
	.readdir  = fs_readdir,
	.readlink = fs_readlink,
	.rename   = fs_rename,
	.rmdir    = fs_rmdir,
	.unlink   = fs_unlink,
};

static void
parse_arguments (ac, av)
	int   * ac;
	char ** av;
{
	const char *database;
	int         database_argument = 0;
	int unknown_database          = 1;

	int i;

	for (i = 1; i < (*ac); ++i)
	{
		if (
				strcmp(av[i], "-?")     == 0 ||
				strcmp(av[i], "-h")     == 0 ||
				strcmp(av[i], "--help") == 0
		){
			warn("%s", HELP);

			strcpy(av[0], "");/* this tells FUSE to not print the usage line */
			av[1] = "--help";
			(*ac) = 2;

			return;/* bail rfn */
		}
		else if (av[i][0] != '-')/* regular argument */
		{
			if (database_argument == 0)/* first regular argument */
			{
				database_argument = i;
			}
			else
			{
				/*
				this probably looks weird; need to confirm the database argument
				was before the mountpoint argument!
				*/
				unknown_database = 0;
			}
		}
	}

	if (unknown_database)
	{
		database = ".stfs";
	}
	else
	{
		database = av[database_argument];

		memmove(
				&av[database_argument],
				&av[database_argument + 1],
				( (*ac) - 1 - database_argument ) * sizeof *av);
		(*ac)--;
	}

	/* now we need to connect that database */
	db_link(database);
}

int
main (ac, av)
	int     ac;
	char ** av;
{
	parse_arguments(&ac, av);
	return fuse_main(ac, av, &filesystem, NULL);
}

int
run_inheritance_statement (s, p, lhs, lhs_length, rhs)
	char * p;
	char * lhs;
	int    lhs_length;
	char * rhs;

	sqlite3_stmt * s;
{
	/* every so often I like to write obfuscation :) */
	int shift = ( p[-1] == 'r' );/* 'under' ends with 'r' */

	sqlite3_bind_text   (s, 2 - shift, lhs, lhs_length, NULL);
	sqlite3_bind_string (s, 1 + shift, rhs);

	db_finish(s);

	return 0;
}

int
get_inode (inode, page, p)
	sqlite3_int64 * inode;
	int           * page;
	char         ** p;
{
	errno  = 0;/* underflow/overflow, though, really? =P */
	*inode = strtoll(*p, p, 10);

	if (**p == '.')
	{
		*page = strtol(&(*p)[1], p, 10);

		if (*page < 1)
		{
			return 1;
		}
	}
	else
	{
		*page = 0;
	}

	if (( **p != '\0' && **p != '/' ) || errno)
	{
		return 1;
	}

	return 0;
}

sqlite3_int64
get_real_inode (inode, page)
	sqlite3_int64 inode;
	int           page;
{
	sqlite3_stmt * s;

	if (page > 0)
	{
		s = db_prepare(
				"SELECT ROWID FROM `inodes`"
				"WHERE `master`=? AND `page`=?"
		);

		sqlite3_bind_int64 (s, 1, inode);
		sqlite3_bind_int   (s, 2, page);

		if (sqlite3_step(s) == SQLITE_ROW)
		{
			inode = sqlite3_column_int64(s, 0);
		}
		else
		{
			inode = 0;
		}

		sqlite3_finalize(s);
	}

	return inode;
}

sqlite3_int64
strtonode (p)
	char ** p;
{
	sqlite3_int64 inode;
	int           page;

	if (get_inode(&inode, &page, p) == 0)
	{
		return get_real_inode(inode, page);
	}
	else
	{
		return 0;
	}
}
