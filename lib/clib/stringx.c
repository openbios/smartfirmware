/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */

/* Copyright (c) 1992 by Parag Patel.  All Rights Reserved. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "stringx.h"
#include "expand.h"
#include "stdlibx.h"
#include "stdiox.h"

char *
strapp(char *s1, const char *s2)
{
	int length = strlen(s2);
	char *ptr;

	if (s1 != NULL)
	{
		int len1 = strlen(s1);
		ptr = strnew(length + len1);
		strcpy(ptr, s1);
		strcpy(ptr + len1, s2);
		strfree(s1);
	}
	else
	{
		ptr = strnew(length);
		strcpy(ptr, s2);
	}

	return ptr;
}

char *
strnapp(char *s1, const char *s2, int n)
{
	int length = strlen(s2);
	int len1;
	char *ptr;

	if (length > n)
		length = n;

	len1 = (s1 == NULL) ? 0 : strlen(s1);
	length += len1;
	ptr = strnew(length);
	*ptr = '\0';

	if (s1 != NULL)
	{
		strcpy(ptr, s1);
		strfree(s1);
	}

	strncpy(ptr + len1, s2, n);
	ptr[length] = '\0';
	return ptr;
}

char const *
strbldf(char const *fmt, ...)
{
	static char *buf = NULL;
	static long buflen = 0;
	char *ret;
	va_list ap;

	va_start(ap, fmt);
	ret = xvsprintf(&buf, &buflen, fmt, ap);
	va_end(ap);
	return ret;					/* may be NULL if xvsprintf() fails */
}

const char *
strbld(const char *arg1, ...)
{
	static char *buf = NULL;
	static long buflen = 0;
	int len = 0;
	const char *s;
	va_list ap;

	expand(&buf, &buflen, 1);
	*buf = '\0';
	va_start(ap, arg1);

	for (s = va_arg(ap, const char *); s != NULL; s = va_arg(ap, const char *))
	{
		int end = len;

		len += strlen(s);
		expand(&buf, &buflen, len + 1);
		strcpy(buf + end, s);
	}

	va_end(ap);
	return buf;
}

/* This only works for ASCII strings - it is NOT aNaLSalized. */
int
strcmpi(char const *s1, char const *s2)
{
	while (*s1 && *s2)
	{
		int c1 = tolower(*s1);
		int c2 = tolower(*s2);

		s1++;
		s2++;

		if (c1 != c2)
			return c1 - c2;
	}

	return 0;
}

const char **
strsplit(char const *str, char const *sep, boolean whsp, int *argc)
{
	static const char **strvec = NULL;
	static long strveclen = 0;
	static char *string;
	static long stringlen;
	long args = 0;
	char *sp;

	bufcopy(&string, &stringlen, str);
	sp = string;

	if (!whsp && strchr(sep, *string) != NULL)
	{
		expand(&strvec, &strveclen, args + 1);
		strvec[args] = "";
		strvec[args + 1] = NULL;
		args++;
		sp++;
	}

	while (*sp != '\0')
	{
		char *start;

		if (whsp)
			while (strchr(sep, *sp) != NULL)
				sp++;

		start = sp;

		while (strchr(sep, *sp) == NULL && *sp != '\0')
			sp++;

		if (!whsp || start != sp)
		{
			expand(&strvec, &strveclen, args + 1);
			strvec[args] = start;
			strvec[args + 1] = NULL;
			args++;
		}

		if (*sp == '\0')
			break;

		*sp++ = '\0';
	}

	if (argc != NULL)
		*argc = args;

	return strvec;
}

const char *
strjoin(const char **vec, const char *sep)
{
	static char *string = NULL;
	static long stringlen = 0;
	int seplen = strlen(sep);
	long len = 0;
	const char *s;

	expand(&string, &stringlen, 1);
	*string = '\0';

	for (s = *vec++; s != NULL; s = *vec++)
	{
		int end = len;

		len += strlen(s);
		expand(&string, &stringlen, len + 1);
		strcpy(string + end, s);

		if (*vec != NULL)
		{
			end = len;
			len += seplen;
			expand(&string, &stringlen, len + 1);
			strcpy(string + end, sep);
		}
	}

	return string;
}

#ifdef strfree
#undef strfree
#endif

void
strfree(char *str)
{
	if (str != NULL)
		free(str);
}

#ifdef strnew
#undef strnew
#endif

char *
strnew(int len)
{
	return (char *)malloc(len + 1);
}

#ifdef strdup
#undef strdup
#endif

char *
strdup(const char *str)
{
	char *new;

	if (str == NULL)
		return NULL;

	new = (char *)malloc(strlen(str) + 1);

	if (new !=NULL)
		strcpy(new, str);

	return new;
}
