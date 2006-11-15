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

/* Copyright (c) 1992,1997 by Parag Patel and Thomas J. Merritt.
   All Rights Reserved. */

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#ifdef macintosh
  #include <Files.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
#endif
#include "expand.h"
#include "stdlibx.h"
#include "stdiox.h"

char *
xgets(FILE *fp, boolean nl)
{
	static char *buf = NULL;
	static long buflen = 0;
	long len = 0;
	int ch;

	while ((ch = getc(fp)) != EOF)
	{
		expand(&buf, &buflen, len + 1);

		if (ch == '\n' && !nl)
		{
			buf[len] = '\0';
			return buf;
		}

		buf[len++] = ch;

		if (ch == '\n')
		{
			expand(&buf, &buflen, len + 1);
			buf[len] = '\0';
			return buf;
		}
	}

	if (len == 0)
		return NULL;

	expand(&buf, &buflen, len + 1);
	buf[len] = '\0';
	return buf;
}

FILE *
fileopen(char const *dir, char const *file, char const *mode)
{
	static char *buf = NULL;
	static long buflen = 0;

	if (dir == NULL || dir[0] == '\0')
		dir = CURRENT_DIR;

	expand(&buf, &buflen, strlen(dir) + strlen(file) + 2);
	sprintf(buf, "%s/%s", dir, file);
	return fopen(buf, mode);
}

FILE *
pathopen(char *path, char const *file, char const *mode)
{
	char *colon;
	FILE *fp;

	if (path == NULL)
		path = CURRENT_DIR;

	if (file == NULL || file == '\0')
		return NULL;

	if (mode == NULL)
		mode = "r";

#ifdef macintosh
	if (file[0] != PATH_SEP)	/* Mac abs paths do not start with
								   PATH_SEP */
		return fopen(file, mode);
#else
	if (file[0] == PATH_SEP)	/* assume PATH_SEP also marks the root */
		return fopen(file, mode);
#endif

	for (colon = strchr(path, PATH_LIST_SEP); colon != NULL;
			path = colon + 1, colon = strchr(path, PATH_LIST_SEP))
	{
		*colon = '\0';
		fp = fileopen(path, file, mode);
		*colon = PATH_LIST_SEP;

		if (fp != NULL)
			return fp;
	}

	return fileopen(path, file, mode);
}

FILE *
pathvopen(char const *pathvar, char const *file, char const *mode)
{
	char *var = getenv(pathvar);

	if (var == NULL)
		return NULL;

	return pathopen(var, file, mode);
}

char *
xvsprintf(char **buf, long *buflen, const char *fmt, va_list ap)
{
	static FILE *fp = NULL;
	long len;

#ifdef _DJGPP_SOURCE
	if (fp == NULL)
		fp = fopen("/tmp/xprintf.tmp", "w+");
#else
	if (fp == NULL)
		fp = tmpfile();
#endif

	if (fp == NULL)
		crash("xvsprintf(): cannot open temp file - out of space?");

	if (fmt == NULL)
	{
		if (fp != NULL)
			fclose(fp);

		fp = NULL;
		return NULL;
	}

	rewind(fp);
	vfprintf(fp, (char *)fmt, ap);
	len = ftell(fp);

	if (len >= *buflen)
	{
		if (*buf)
			free(*buf);

		*buf = (char *)malloc(*buflen = len + 1);

		if (*buf == NULL)
			return NULL;
	}

	rewind(fp);
	fread(*buf, sizeof (char), len, fp);
	*buf[len] = '\0';
	return *buf;
}

#ifndef _DJGPP_SOURCE
const char *
xsprintf(const char *fmt, ...)
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
#else
/* With DJGPP lseek is very very slow so don't bother with being overly safe. */
const char *
xsprintf(const char *fmt, ...)
{
	static char buf[BUFSIZ];

	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	return buf;
}
#endif

const char *
dir_name(const char *name)
{
	static char *buf = NULL;
	static long buflen = 0;
	char *ptr;

	expand(&buf, &buflen, strlen(name) + 3);
	strcpy(buf, name);

	ptr = strrchr(buf, PATH_SEP);

	if (ptr == NULL)
	{
		ptr = buf;
		*ptr++ = CURRENT_DIR[0];
	}
	else if (ptr == buf)
		ptr++;

	*ptr = '\0';
	return buf;
}

const char *
base_name(const char *name)
{
	char *ptr = strrchr(name, PATH_SEP);
	return (ptr == NULL) ? name : ptr + 1;
}

boolean
filelength(const char *name, size_t *size)
{
#ifdef macintosh
	short file;
	Str255 path;
	boolean ret = TRUE;

	strcpy((char *)&path[1], name);
	path[0] = (unsigned char)strlen(name);

	if (FSOpen(path, 0, &file) != noErr)
		return FALSE;

	if (GetEOF(file, (long *)size) != noErr)
		ret = FALSE;

	FSClose(file);
	return ret;
#else
	struct stat sbuf;

	if (stat(name, &sbuf) < 0)
		return FALSE;

#ifdef S_ISDIR
	if (S_ISDIR(sbuf.st_mode))
		return FALSE;
#else
	if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
		return FALSE;
#endif

	*size = (size_t)sbuf.st_size;
	return TRUE;
#endif
}


static FILE *debugfp;

void
set_debug_fp(FILE *fp)
{
	debugfp = fp;
}

void
dprintf(const char *fmt, ...)
{
	va_list args;

	if (debugfp == NULL)
		debugfp = stderr;

	va_start(args, fmt);
	vfprintf(debugfp, fmt, args);
	va_end(args);
}
