/* tag [ -d database] target [target2...] */

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
#include "int.h"

static const char * HELP =
"tag [ -d database] target [target2...]\n"
"tag [ -d database] -INODE [newtarget] ...\n"
"\n"
"Add a new symlink to a STFS database. The filesystem need not be mounted.\n"
"\n"
"Without '-d', the database is assumed to be located at '.stfs' in the\n"
"current directory.\n"
"\n"
"If an argument of '-INODE' is encountered (where 'INODE' is a number, e.g.\n"
"'-2'), the next argument will be the new target for that inode's symlink.\n"
"\n"
"If a next argument does not exist, or is another instance of '-INODE', the\n"
"inode's page number will be reset.\n"
"\n"
"Another target cannot follow 'newtarget'.\n"
"\n"
"For each target in the first usage, or inode in the second, a line is\n"
"printed with the target followed by its inode. If the inode's page number\n"
"is being reset, the line printed will be of the old inode followed the new\n"
"inode instead.\n"
"\n"
"Copyright 2020 James R.\n"
;

#define UPDATE_PATH_SQL "UPDATE `inodes` SET `path` = ?"
#define  RESET_PAGE_SQL "UPDATE `inodes` SET `master` = NULL, `page` = NULL "
#define WHERE_INODE_SQL "WHERE ROWID=?"
#define  WHERE_PAGE_SQL "WHERE `master`=? AND `page`=?"

static int     ac;
static char ** av;

static char * name;

static int
shift ()
{
	if (ac > 1)
	{
		ac--;
		av++;

		return 1;
	}
	else
	{
		return 0;
	}
}

int
main (argc, argv)
	int     argc;
	char ** argv;
{
#define issue( ... ) ( warn (__VA_ARGS__), status = 2 )

	int status = 0;

	const char * database = ".stfs";

	sqlite3_stmt * insert = NULL;
	sqlite3_stmt * select = NULL;

	sqlite3_stmt * retarget      = NULL;
	sqlite3_stmt * retarget_page = NULL;

	sqlite3_stmt * reset         = NULL;
	sqlite3_stmt * reset_page    = NULL;

	sqlite3_stmt * s;

	struct
	{
		char          * target;
		sqlite3_int64   rowid;
	} * nodv;
	int nodc;

	int i;

	sqlite3_int64 master;
	int           page;

	char *p;

	int inserting;
	int resetting;

	int ready;

	ac = argc;
	av = argv;

	if (shift())
	{
		if (strcmp(*av, "-d") == 0)
		{
			if (shift())
			{
				database = *av;

				shift();
			}
			else
			{
				err("-d: missing database argument\n");
			}
		}

		db_link(database);

		inserting = ( (*av)[0] != '-' );

		if (inserting)
		{
			insert = db_prepare(
					"INSERT INTO `inodes` (`path`,`ctime`,`mtime`)"
					"VALUES (?,?2,?2);"
			);

			sqlite3_bind_int(insert, 2, time(NULL));
		}

		nodv = malloc(ac * sizeof *nodv);
		nodc = 0;

		do
		{
			if ((*av)[0] == '-')/* retarget or reset */
			{
				name   = &(*av)[1];

				errno  = 0;
				master = strtoll(name, &p, 10);

				if (*p == '.')
				{
					page = strtol(&p[1], &p, 10);

					if (page < 1)
					{
						issue(
								"%s: page must be greater than zero\n",
								name
						);

						page = -1;
					}
				}
				else
				{
					page = 0;
				}

				if (errno)
				{
					perror(name);
					status = 2;
				}
				else if (*p != '\0')
				{
					issue(
							"%s: inode is malformed\n",
							name
					);
				}
				else if (page >= 0)
				{
					resetting = ( ac < 2 || av[1][0] == '-' );

					if (resetting)
					{
						if (page < 1)
						{
							if (reset == NULL)
							{
								reset = db_prepare(
										RESET_PAGE_SQL
										WHERE_INODE_SQL
								);
							}

							s = reset;
						}
						else
						{
							if (reset_page == NULL)
							{
								reset_page = db_prepare(
										RESET_PAGE_SQL
										WHERE_PAGE_SQL
								);
							}

							s = reset_page;
						}

						if (select == NULL)
						{
							select = db_prepare(
									"SELECT ROWID FROM `inodes`"
									WHERE_PAGE_SQL
							);
						}
					}
					else
					{
						if (page < 1)
						{
							if (retarget == NULL)
							{
								retarget = db_prepare(
										UPDATE_PATH_SQL
										WHERE_INODE_SQL
								);
							}

							s = retarget;
						}
						else
						{
							if (retarget_page == NULL)
							{
								retarget_page = db_prepare(
										UPDATE_PATH_SQL
										WHERE_PAGE_SQL
								);
							}

							s = retarget_page;
						}
					}

					if (resetting)
					{
						sqlite3_bind_int64 (select, 1, master);
						sqlite3_bind_int   (select, 2, page);

						ready = ( sqlite3_step(select) == SQLITE_ROW );

						if (! ready)
						{
							issue(
									"%s: no such inode\n",
									name
							);
						}
					}
					else
					{
						shift();

						sqlite3_bind_string(s, 1, *av);

						ready = 1;
					}

					sqlite3_bind_int64(s, 2 - resetting, master);

					if (page > 0)
					{
						sqlite3_bind_int(s, 3 - resetting, page);
					}

					if (ready)
					{
						sqlite3_step(s);

						if (sqlite3_changes(db))
						{
							if (resetting)
							{
								nodv[nodc].rowid  = sqlite3_column_int64(select, 0);
								nodv[nodc].target = name;
							}
							else
							{
								nodv[nodc].rowid  = master;
								nodv[nodc].target = *av;
							}

							nodc++;
						}
						else
						{
							issue(
									"%s: no such inode\n",
									name
							);
						}
					}

					if (resetting)
					{
						sqlite3_reset(select);
					}

					sqlite3_reset(s);
				}
			}
			else if (inserting)
			{
				if ((*av)[0] == '/')/* must be an absolute path to target */
				{
					name = (*av);

					sqlite3_bind_string(insert, 1, name);

					if (sqlite3_step(insert) == SQLITE_DONE)
					{
						nodv[nodc].rowid  = sqlite3_last_insert_rowid(db);
						nodv[nodc].target = name;
						nodc++;
					}
					else
					{
						issue(
								"%s: %s\n",
								name,
								sqlite3_errmsg(db)
						);
					}

					sqlite3_reset(insert);
				}
				else
				{
					issue(
							"%s: not an absolute path\n",
							*av
					);
				}
			}
		}
		while (shift()) ;

		if (nodc > 0)
		{
			if (status == 2)
			{
				putc('\n', stderr);
			}

			for (i = 0; i < nodc; ++i)
			{
				printf(
						"%s -> %lld\n",
						nodv[i].target,
						nodv[i].rowid
				);
			}
		}

		sqlite3_finalize(insert);
		sqlite3_finalize(select);

		sqlite3_finalize(retarget);
		sqlite3_finalize(retarget_page);

		sqlite3_finalize(reset);
		sqlite3_finalize(reset_page);
	}
	else
	{
		err("%s", HELP);
	}

	return status;
}
