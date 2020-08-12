/* tag [ -d database] target [target2...] */

/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "int.h"

static const char * HELP =
"tag [ -d database] target [target2...]\n"
"Copyright 2020 James R.\n"
;

int     ac;
char ** av;

int
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
	int status = 0;

	const char * database = ".stfs";

	sqlite3_stmt  * s;

	struct
	{
		char          * target;
		sqlite3_int64   rowid;
	} * nodv;
	int nodc;

	int i;

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

		s = db_prepare(
				"INSERT INTO `inodes` (`path`,`ctime`,`mtime`) VALUES (?,?2,?2);");

		sqlite3_bind_int(s, 2, time(NULL));

		nodv = malloc(ac * sizeof *nodv);
		nodc = 0;

		do
		{
			if ((*av)[0] == '/')/* must be an absolute path to target */
			{
				sqlite3_bind_string(s, 1, *av);

				if (sqlite3_step(s) == SQLITE_DONE)
				{
					nodv[nodc].rowid  = sqlite3_last_insert_rowid(db);
					nodv[nodc].target = *av;
					nodc++;
				}
				else
				{
					warn(
							"%s: %s\n",
							*av,
							sqlite3_errmsg(db)
					);
					status    = 2;
				}

				sqlite3_reset(s);
			}
			else
			{
				warn(
						"%s: not an absolute path\n",
						*av
				);
				status    = 2;
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

		sqlite3_finalize(s);
	}
	else
	{
		err("%s", HELP);
	}

	return status;
}
