/*
little parser abstraction for cstrtok :)
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <string.h>
#include "int.h"

void
explode (parser, s, d)
	struct part * parser;
	const char  * s;
	const char  * d;
{
	parser->delim  = d;
	parser->part   = s;
	parser->length = 0;
}

const char *
advance (parser)
	struct part * parser;
{
	parser->part = cstrtok(parser->part, parser->delim, &parser->length);
	return parser->part;
}

int
partcmp (part, token)
	struct part * part;
	const char  * token;
{
	if (strlen(token) == part->length)
	{
		return memcmp(part->part, token, part->length);
	}
	else
	{
		return ( part->part[0] - token[0] );
	}
}

int
advance_tag (part)
	struct part * part;
{
	return (
			advance(part) &&
			partcmp(part,  "@") != 0 &&
			partcmp(part, "~@") != 0
	);
}
