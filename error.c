/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "int.h"

void
err (
		const char *format,
		...
){
	va_list ap;
	va_inline (ap, format,

			vfprintf(stderr, format, ap)
	);
	exit(1);
}

void
pexit (prefix)
	const char *prefix;
{
	perror(prefix);
	exit(1);
}
