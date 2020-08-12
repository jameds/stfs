/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include "int.h"

static sqlite3_stmt * select_tags;
static sqlite3_stmt * delete_tags;

static void
select_inodes (action, part)
	sqlite3_stmt * action;
	struct part  * part;
{
	sqlite3_bind_text(select_tags, 1, part->part, part->length, NULL);
	db_advance(select_tags);
	db_advance(action);
	db_advance(delete_tags);
}

static void
delete_inodes (action, part)
	sqlite3_stmt * action;
	struct part  * part;
{
	sqlite3_bind_text(action, 1, part->part, part->length, NULL);
	db_advance(action);
}

void
select_inodes_from_tags (path)
	const char * path;
{
#if 0
#define PREPARE_FILTER( exists ) \
	filter = db_prepare(\
			"DELETE FROM `select_inodes` WHERE " exists \
			"( SELECT 1 FROM `mappings`" \
			"WHERE `inode`=`select_inodes`.`inode`" \
			"AND `tag` IN ( SELECT `tag` FROM `select_tags` )" \
			");" \
	)
#else
#define NEW_FILTER( in ) \
	db_prepare(\
			"DELETE FROM `select_inodes` WHERE `inode`" in \
			"( SELECT `inode` FROM `mappings` WHERE `tag` IN `select_tags` )" \
	)

#define PREPARE_EXACT_FILTER() \
	filter_exact = db_prepare(\
			"DELETE FROM `select_inodes` WHERE `inode` IN"\
			"( SELECT `inode` FROM `mappings` WHERE `tag`="\
			"( SELECT  ROWID  FROM `tags`     WHERE `name`=? )"\
			")"\
	)
#endif

	enum
	{
		UNKNOWN,
		FILTER,
		INSERT,
		DELETE,
		IGNORE,
	};

	sqlite3_stmt * filter_in    = NULL;
	sqlite3_stmt * filter_out   = NULL;
	sqlite3_stmt * filter_exact = NULL;
	sqlite3_stmt * insert       = NULL;

	struct part part;
	const char * except_token = NULL;

	int initial_action = INSERT;/* need to select the first tag */
	int         action = UNKNOWN;

	int exact;

	create_select_tags_table();

	select_tags = prepare_select_tags();
	delete_tags = db_prepare("DELETE FROM `select_tags`;");

	db_exec(
			"CREATE TEMPORARY TABLE IF NOT EXISTS"
			"`select_inodes` (`inode` INTEGER);"
	);

	insert = db_prepare(
			"INSERT OR IGNORE INTO `select_inodes`"
			"SELECT `inode` FROM `mappings`"
			"WHERE `tag` IN `select_tags`;"
	);

	explode(&part, path, "/");

	while (advance_tag(&part))
	{
		if (action == UNKNOWN)
		{
			/* test for special tokens */
			if (partcmp(&part, "+") == 0)
			{
				action = INSERT;
				continue;
			}
			else if (part.part[0] == '-')
			{
				action = DELETE;
				exact  = ( part.part[1] == '-' );
				continue;
			}
			else if (part.part[0] == '~')
			{
				/* pointer to first exception for little time save */
				except_token = part.part;
				action = IGNORE;
				continue;
			}
			else if (tag_is_valid(part.part))
			{
				action = initial_action;
				initial_action = FILTER;/* filter after this first insert */
			}
		}

		switch (action)
		{
			case FILTER:/* filter already selected inodes by this tag */
				if (filter_in == NULL)
				{
					filter_in = NEW_FILTER ("NOT IN");
				}

				select_inodes(filter_in, &part);
				break;

			case INSERT:/* straight up add inodes matching this tag */
				select_inodes(insert, &part);
				break;

			case DELETE:/* remove inodes matching this tag from the selection */
				if (exact)
				{
					if (filter_exact == NULL)
					{
						PREPARE_EXACT_FILTER();
					}

					delete_inodes(filter_exact, &part);
				}
				else
				{
					if (filter_out == NULL)
					{
						filter_out = NEW_FILTER ("IN");
					}

					select_inodes(filter_out, &part);
				}
				break;
		}

		action = UNKNOWN;
	}

	sqlite3_finalize(insert);
	sqlite3_finalize(filter_in);

	if (except_token != NULL)
	{
		if (filter_out == NULL)
		{
			filter_out = NEW_FILTER ("IN");
		}

		if (filter_exact == NULL)
		{
			PREPARE_EXACT_FILTER();
		}

		explode(&part, except_token, "/");

		while (advance_tag(&part))
		{
			if (part.part[0] == '~')
			{
				exact = ( part.part[1] == '~' );

				if (advance_tag(&part))
				{
					if (tag_is_valid(part.part))
					{
						if (exact)
						{
							delete_inodes(filter_out, &part);
						}
						else
						{
							select_inodes(filter_out, &part);
						}
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	sqlite3_finalize(filter_out);
	sqlite3_finalize(filter_exact);

	sqlite3_finalize(select_tags);
	sqlite3_finalize(delete_tags);

#undef NEW_FILTER
}
