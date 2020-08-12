/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include "int.h"
#include <ctype.h>

void
create_select_tags_table ()
{
	db_exec(
			"CREATE TEMPORARY TABLE IF NOT EXISTS"
			"`select_tags` (`tag` INTEGER);"
	);
}

sqlite3_stmt *
prepare_select_tags ()
{
	return db_prepare(
			"INSERT INTO `select_tags`"
			"WITH RECURSIVE `select_tags` (`tag`) AS ("
			"SELECT ROWID FROM `tags` WHERE `name`=?"
			"UNION SELECT `child` FROM `inheritance`, `select_tags`"
			"WHERE `parent`=`select_tags`.`tag`"
			") SELECT `tag` FROM `select_tags`;"
	);
}

void
select_all_tags (path)
	const char * path;
{
	struct part part;

	sqlite3_stmt * s;

	create_select_tags_table();

	s = prepare_select_tags();

	explode(&part, path, "/");

	while (advance_tag(&part))
	{
		if (tag_is_valid(part.part))
		{
			sqlite3_bind_text(s, 1, part.part, part.length, NULL);
			db_advance(s);
		}
	}

	sqlite3_finalize(s);
}

int
tag_is_valid (tag)
	const char * tag;
{
	return isalnum(tag[0]);
}
