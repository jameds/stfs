/*
misc. string functions
*/
/*
Copyright 2020 James R.
All rights reserved.

Read the 'LICENSE' file.
*/

#include <string.h>

char *
strstr3 (s, ss, pt)
	const char *  s;
	const char *  ss;
	char       ** pt;
{
	char * p = strstr(s, ss);
	(*pt) = ( (p != NULL) ? &p[ strlen(ss) ] : NULL );
	return p;
}

char *
strstr2 (s, ss)
	const char * s;
	const char * ss;
{
	char * t;
	strstr3(s, ss, &t);
	return t;
}

int
rcmp (s, ss)
	const char * s;
	const char * ss;
{
	size_t el;
	size_t er;

	el = strlen(s);
	er = strlen(ss);

	if (er <= el)
	{
		return strcmp(&s[ el - er ], ss);
	}
	else
	{
		return ( el - er );
	}
}

/* I am NOT doing feature macros */
char *
stpcpy (p, s)
	char       * p;
	const char * s;
{
	size_t n = strlen(s);
	return memcpy(p, s, n + 1) + n;
}

void *
rmemchr (vp, c, vs)
	const void * vp;
	int           c;
	const void * vs;
{
	const unsigned char *p = vp;
	const unsigned char *s = vs;

	while (p >= s)
	{
		if (*p == (unsigned char)c)
		{
			return memchr(p, *p, 1);
		}
		else
		{
			p--;
		}
	}

	return NULL;
}

int
stropt (s, o)
	const char * s;
	const char * o[];
{
	int i = 0;

	while (o[i] != NULL)
	{
		if (strcmp(s, o[i]) == 0)
		{
			return i;
		}
		else
		{
			i++;
		}
	}

	return -1;
}
