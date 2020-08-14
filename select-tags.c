/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include "int.h"
#include <ctype.h>
#include <string.h>

#define RECURSIVE_SQL \
	"WITH RECURSIVE `family` (`tag`) AS ("\
	"SELECT ROWID FROM `tags` WHERE `name`=?"\
	"UNION SELECT `child` FROM `inheritance`, `family`"\
	"WHERE `parent`=`family`.`tag`"\
	")"

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
			RECURSIVE_SQL
			"INSERT INTO `select_tags` SELECT `tag` FROM `family`"
	);
}

void
select_all_tags (path)
	const char * path;
{
	struct part part;

	sqlite3_stmt * insert;
	sqlite3_stmt * delete     = NULL;
	sqlite3_stmt * delete_one = NULL;

	sqlite3_stmt * s;

	create_select_tags_table();

	insert = prepare_select_tags();

	if (strstr(path, "/-") != NULL)
	{
		delete = db_prepare(
				RECURSIVE_SQL
				"DELETE FROM `select_tags` WHERE `tag` IN `family`"
		);

		delete_one = db_prepare(
				"DELETE FROM `select_tags`"
				"WHERE `tag`=( SELECT ROWID FROM `tags` WHERE `name`=? )"
		);
	}

	s = insert;

	explode(&part, path, "/");

	while (advance_tag(&part))
	{
		if (tag_is_valid(part.part))
		{
			sqlite3_bind_text(s, 1, part.part, part.length, NULL);
			db_advance(s);
		}
		else if (part.part[0] == '-')
		{
			if (part.part[1] != '-')
			{
				s = delete;
			}
			else
			{
				s = delete_one;
			}
			continue;
		}

		s = insert;
	}

	sqlite3_finalize(insert);
	sqlite3_finalize(delete);
	sqlite3_finalize(delete_one);
}

int
tag_is_valid (tag)
	const char * tag;
{
	return isalnum(tag[0]);
}
