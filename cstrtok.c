/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

/* strtok for immutable strings */
#include <string.h>

const char *
cstrtok (p, d, token_length)
	const char * p;
	const char * d;
	size_t     * token_length;
{
	p += (*token_length);
	p += strspn(p, d);
	(*token_length) = strcspn(p, d);
	return ( (*token_length) ? p : NULL );
}
