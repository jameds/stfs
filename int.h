/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stddef.h>
#include <sqlite3.h>

/* my magnum opus */
#define va_inline( __ap, __last, ... ) \
	(\
			va_start (__ap, __last),\
			__VA_ARGS__,\
			va_end   (__ap)\
	)

#define warn( ... ) fprintf(stderr, __VA_ARGS__)
void err (const char *format, ...);
void pexit (const char *prefix);

const char * cstrtok (
		const char * string,
		const char * delimiters,
		size_t     * token_length
);

char * strstr2 (const char *string, const char *substr);
char * strstr3 (const char *string, const char *substr, char **return_tail);
int    rcmp    (const char *,       const char *);
char * stpcpy        (char *to,     const char *from);
void * rmemchr (const void *before, int c, const void *start);
int    stropt  (const char *string, const char **list);

int tag_is_valid (const char *tag);

/* database */
void db_link (const char *path);
void db_close (void);

sqlite3_stmt * db_prepare (const char *sql);
void           db_exec    (const char *sql);

extern sqlite3 * db;
extern int       db_fd;

int  sqlite3_bind_string (sqlite3_stmt *, int index, const char *string);
int  db_finish           (sqlite3_stmt *);
void db_advance          (sqlite3_stmt *);

/* part parser */

struct part
{
	const char * part;
	size_t       length;

	const char * delim;
};

void explode (struct part *parser, const char *string, const char *delimiters);
const char * advance (struct part *parser);
int partcmp (struct part *part, const char *token);

int advance_tag (struct part *part);

/* sql helper */

void            create_select_tags_table ();
sqlite3_stmt * prepare_select_tags ();

void select_all_tags (const char *path);
void select_inodes_from_tags (const char *path);

#define delete_select_tags_table()   db_exec("DELETE FROM `select_tags`")
#define delete_select_inodes_table() db_exec("DELETE FROM `select_inodes`")

int run_inheritance_statement (
		sqlite3_stmt *,
		char * right_separator,
		char *  left_tag,
		int     left_length,
		char * right_tag
);
