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

/* Copyright (c) 1990,1999 by Parag Patel.  All Rights Reserved. */

#include "defs.h"


static Buffer *buf;
static long bufloc;

#define INIT		long loc = bufloc;
#define GETC()		buf[loc++]
#define PEEKC()		buf[loc]
#define UNGETC(c)	(--loc)
#define RETURN(r)	return r;
#define ERROR(e)	{ errval = e; return NULL; }

static int errval = 0;

#include <regexp.h>



boolean
Buffer::search(const char *str, incr dir, boolean end)
{
	int ln, col, c, gotit, len;
	char *s, *s2, expbuf[1024];
	char *line = NULL;

	if (compile(str, expbuf, expbuf + sizeof expbuf, '\0') == NULL)
		return ERR;

	gotit = FALSE;
	ln = buf->line;
	len = lnlen(buf->lines[ln]);
	s = line = text2str(lnstr(buf->lines[ln]), len);

	if (dir && buf->col < len)
	{
		col = buf->col + incr;

		if (step(s + col, expbuf))
		{
			buf->col = (end ? loc2 : loc1) - s;
			free(line);
			return OK;
		}
	}
	else if (buf->col > 0)
	{
		loc2 = s;
		s2 = NULL;

		while (step(loc2, expbuf))
		{
			col = (end ? loc2 : loc1) - s;

			if (col >= buf->col)
				break;

			s2 = end ? loc2 : loc1;
		}

		if (s2 != NULL)
		{
			buf->col = s2 - s;
			free(line);
			return OK;
		}
	}

	for (ln = buf->line + incr; ln >= 0 && ln < buf->numlines; ln += incr)
	{
		free(line);

		if (step(s = line = text2str(lnstr(buf->lines[ln]),
						lnlen(buf->lines[ln])), expbuf))
		{
			gotit = TRUE;
			break;
		}
	}

	if (!gotit)
	{
		free(line);
		return ERR;
	}

	if (dir)
		col = (end ? loc2 : loc1) - s;
	else
	{
		s2 = end ? loc2 : loc1;

		while (step(loc2, expbuf))
			s2 = end ? loc2 : loc1;

		col = s2 - s;
	}

	buf->line = ln;
	buf->col = col;
	free(line);
	return OK;
}



lookingat(buf, str, end)
BUFFER *buf;
char *str;
int end;
{
	static char *line = NULL;
	int ln;
	char *s, expbuf[100];

	if (compile(str, expbuf, expbuf + sizeof expbuf, '\0') == NULL)
		return ERR;

	if (line != NULL)
		free(line);

	ln = buf->line;

	if (!advance(s = line = text2str(lnstr(buf->lines[ln]),
					lnlen(buf->lines[ln])), expbuf))
		return ERR;

	if (end && loc2 == s + buf->col)
		return OK;

	if (!end && loc1 == s + buf->col)
		return OK;

	return ERR;
}



lookfor(buf, ch, dir)
BUFFER *buf;
int ch;
int dir;
{
	int col, len;
	TEXT *str;

	dir = dir ? 1 : -1;
	len = lnlen(buf->lines[buf->line]);
	str = lnstr(buf->lines[buf->line]);

	for (col = buf->col + dir; col >= 0 && col < len; col += dir)
		if (str[col] == ch)
		{
			buf->col = col;
			return OK;
		}

	return ERR;
}
