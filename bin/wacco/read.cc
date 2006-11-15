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

// Copyright (c) 1991-2001 by Parag Patel.  All Rights Reserved.

#include "defs.h"
#include "toks.h"

boolean gotlexstr = FALSE;

static Charbuf codebuf;

void
readccomment()
{
	int t;
	int c = w_input();

	while (c != EOI)
		if (c == '*')
		{
			t = w_input();

			if (t == '/')
				break;

			c = t;
		}
		else
			c = w_input();

	if (c == EOI)
		w_scanerr("expected \"*/\"");
}


const char *
readtype()
{
	int len = 0;
	int c, t;
	int count = 0;

	while ((c = w_input()) != EOI)
	{
		switch (c)
		{
		case '"':
		case '\'':
			codebuf[len++] = c;

			while ((t = w_input()) != c && c != EOI)
			{
				codebuf[len++] = t;

				if (t == '\\')
					codebuf[len++] = w_input();
			}

			break;

		case '<':
			count++;
			break;

		case '>':
			count--;
			break;

		default:
			break;
		}

		if (count < 0)
			break;

		codebuf[len++] = c;
	}

	if (c == EOI)
		w_scanerr("expected \">\"");

	codebuf[len] = '\0';
	return strget(codebuf.getarr());
}


const char *
readcode(int &usedret)
{
	int len = 0;
	int c, t;
	int count = 0;

	while ((c = w_input()) != EOI)
	{
		switch (c)
		{
		case '/':
			codebuf[len++] = c;
			c = w_input();

			if (c == '/')
			{
				codebuf[len++] = c;

				while ((c = w_input()) != '\n' && c != EOI)
					codebuf[len++] = c;
			}
			else if (c == '*')
			{
				codebuf[len++] = c;
				t = w_input();

				while (t != EOI)
				{
					codebuf[len++] = t;
					c = w_input();

					if (t == '*' && c == '/')
						break;

					t = c;
				}
			}

			break;

		case '"':
		case '\'':
			codebuf[len++] = c;

			while ((t = w_input()) != c && t != EOI)
			{
				codebuf[len++] = t;

				if (t == '\\')
					codebuf[len++] = w_input();
			}

			break;

		case '$':
			c = w_input();

			if (c == '$')
			{
				usedret |= RET_VALUE;

				if (streqi(lang, "C"))
				{
					codebuf[len++] = '(';
					codebuf[len++] = '*';
				}
			}
			else if (c == '?')
				usedret |= RET_CODE;

			codebuf[len++] = 'w';
			codebuf[len++] = '_';
			codebuf[len++] = 'r';

			if (c == '$')
			{
				codebuf[len++] = 'r';

				if (streqi(lang, "C"))
					codebuf[len++] = ')';

				c = w_input();
			}
			else if (c == '?')
				c = 'c';
			else
				codebuf[len++] = 'v';

			break;

		case '{':
			count++;
			break;

		case '}':
			count--;
			break;

		default:
			break;
		}

		if (count < 0)
			break;

		codebuf[len++] = c;
	}

	if (c == EOI)
		w_scanerr("expected \"}\"");

	codebuf[len] = '\0';
	return strget(codebuf.getarr());
}

const char *
readblockcode(int &usedret)
{
	int len = 0;
	int c;
	boolean done = FALSE;

	while ((c = w_input()) != EOI)
	{
		switch (c)
		{
		case '$':
			c = w_input();

			if (c == '$')
			{
				usedret |= RET_VALUE;

				if (streqi(lang, "C"))
				{
					codebuf[len++] = '(';
					codebuf[len++] = '*';
				}
			}
			else if (c == '?')
				usedret |= RET_CODE;

			codebuf[len++] = 'w';
			codebuf[len++] = '_';
			codebuf[len++] = 'r';

			if (c == '$')
			{
				codebuf[len++] = 'r';

				if (streqi(lang, "C"))
					codebuf[len++] = ')';

				c = w_input();
			}
			else if (c == '?')
				c = 'c';
			else
				codebuf[len++] = 'v';

			break;

		case '%':
			c = w_input();

			if (c == '}')
				done = TRUE;
			else
				codebuf[len++] = '%';

		default:
			break;
		}

		if (done)
			break;

		codebuf[len++] = c;
	}

	if (c == EOI)
		w_scanerr("expected \"%%}\"");

	codebuf[len] = '\0';
	return strget(codebuf.getarr());
}

const char *
getword()
{
	int c;
	int len = 0;

	c = w_input();

	while (isspace(c))
		c = w_input();

	if (c == EOI)
		return NULL;

	for (; c != EOI && !isspace(c); c = w_input())
		codebuf[len++] = c;

	w_unput(c);
	codebuf[len] = '\0';
	return strget(codebuf.getarr());
}
