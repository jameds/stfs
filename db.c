/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>

#include <sqlite3.h>

#include "int.h"

sqlite3 * db;
int       db_fd;

#define db_warn( error ) warn ("database error: %s\n", error)
#define db_err(  error ) err  ("database error: %s\n", error)

void
db_link (url)
	const char * url;
{
	atexit(db_close);

	db_fd = open(url, 0, O_RDONLY);

	if (db_fd == -1)
	{
		pexit(url);
	}

	if (sqlite3_open_v2(url, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK)
	{
		err("%s: %s\n", url, sqlite3_errmsg(db));
	}
}

void
db_close ()
{
	sqlite3_close(db);
	close(db_fd);
}

sqlite3_stmt *
db_prepare (sql)
	const char * sql;
{
	sqlite3_stmt * st;

	if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
	{
		db_warn(sqlite3_errmsg(db));
	}

	return st;
}

void
db_exec (sql)
	const char * sql;
{
	char *error;

	sqlite3_exec(db, sql, NULL, NULL, &error);

	if (error != NULL)
	{
		db_warn(error);
		sqlite3_free(error);
	}
}

int
sqlite3_bind_string (s, idx, string)
	sqlite3_stmt * s;
	int            idx;
	const char   * string;
{
	return sqlite3_bind_text(s, idx, string, -1, NULL);
}

int
db_finish (s)
	sqlite3_stmt * s;
{
	int c = sqlite3_step(s);
	sqlite3_finalize(s);
	return c;
}

void
db_advance (s)
	sqlite3_stmt * s;
{
	sqlite3_step(s);
	sqlite3_reset(s);
}
