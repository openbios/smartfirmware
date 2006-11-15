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

// Copyright (c) 1991-2002 by Parag Patel.  All Rights Reserved.

#include "defs.h"
#include "toks.h"

#define isvalpha(c) (isalpha(c) || (c) == '_')
#define isodigit(c) (isdigit(c) && (c) < '8')

#define TEXTSIZE 1024
char yytext[TEXTSIZE];
char *text = NULL;

static void
blockcomment()
{
	char c = w_input();

loop:
	while (c != '*' && c != EOI)
		c = w_input();

	if (c != EOI)
		c = w_input();

	if (c != '/' && c != EOI)
		goto loop;
}

static void
linecomment()
{
	for (char c = w_input(); c != '\n' && c != EOI; c = w_input())
		;
}

static void
docomment()
{
	char c = w_input();

	if (c == '/')
		linecomment();
	else if (c == '*')
		blockcomment();
	else
	{
		w_unput(c);
		error("ERROR:  comment expected here");
	}
}

static int
doident(int c)
{
	for (; isvalpha(c) || isdigit(c); c = w_input())
		*text++ = c;

	w_unput(c);
	return ID;
}


static int
donum(int c)
{
	for (; isdigit(c); c = w_input())
		*text++ = c;

	w_unput(c);
	return INT;
}


static int
doquote()
{
	int c = w_input();

	if (isodigit(c))
	{
		int c2 = c;

		for (c = 0; isodigit(c2); c2 = w_input())
			c = (c << 3) | (c2 - '0');

		w_unput(c2);
	}
	else
		switch (c)
		{
		case 'n':
			c = '\n';
			break;

		case 't':
			c = '\t';
			break;

		case 'v':
			c = '\v';
			break;

		case 'b':
			c = '\b';
			break;

		case 'r':
			c = '\r';
			break;

		case 'f':
			c = '\f';
			break;

		case '\\':
			c = '\\';
			break;

		case '\'':
			c = '\'';
			break;

		case '"':
			c = '"';
			break;

		case 'e':
			c = '\033';
			break;

		case 'a':
			c = '\177';
			break;

		case '?':
			c = '\177';
			break;

		case '^':
			c = w_input();

			if (c == '?')
				c = '\177';
			else if (islower(c))
				c -= 'a' + 1;
			else if (c >= '@' && c <= '_')
				c -= '@';
			else
			{
				w_unput(c);
				c = '^';
			}

			break;

		default:
			break;
		}

	return c;
}


static int
dochar()
{
	int c;
	*text++ = '\'';
	*text++ = c = w_input();

	if (c == '\\')
	{
		c = doquote();

		if (c != EOI)
			*text++ = c;
	}

	if (w_input() != '\'')
		error("ERROR:  \' expected here");

	*text++ = '\'';
	return CHARACTER;
}


static int
dostr()
{
	int c;
	*text++ = '"';

	for (c = w_input(); c != '"' && c != EOI; c = w_input())
	{
		*text++ = c;

		if (c == '\\')
		{
			c = w_input();

			if (c != EOI)
				*text++ = c;
		}
	}

	if (c != '"')
		error("ERROR:  \" expected here");

	*text++ = '"';
	return STRING;
}

static int
doopt()
{
	int c = w_input();

	if (c == '{')
		return BLOCKCODE;

	(void)doident(c);
	*text = '\0';

	if (streq(yytext, "export"))
		return EXPORT;
	else if (streq(yytext, "inline"))
		return INLINE;
	else if (streq(yytext, "noinline"))
		return NOINLINE;
	else if (streq(yytext, "noresync"))
		return NORESYNC;
	else if (streq(yytext, "exitcode"))
		return EXITCODE;
	else if (streq(yytext, "opt"))
	{
		*text++ = ' ';
		char c = w_input();

		while (isspace(c))
			c = w_input();

		for (; c != '\n' && c != EOI; c = w_input())
			*text++ = c;

		return DIRECTIVE;
	}
	else
	{
		error("ERROR:  illegal %% char");
		return ID;
	}
}

extern "C" int yylex();

int
yylex()
{
	int c;
	int ret = EOI;

	while ((c = w_input()) != EOI)
	{
		while (isspace(c))
			c = w_input();

		text = yytext;

		if (isvalpha(c))
			ret = doident(c);
		else if (isdigit(c))
			ret = donum(c);
		else
			switch (c)
			{
			case '[':
				c = w_input();

				if (c == ']')
					ret = NULLSYM;
				else
				{
					w_unput(c);
					ret = '[';
				}

				break;

			case '$':
				c = w_input();

				if (c != '$')
				{
					w_unput(c);
					error("ERROR:  expected $$ here");
					continue;
				}

				gotlexstr = TRUE;
				ret = EOI;
				break;

			case ']':
			case '<':
			case '{':
			case '#':
			case '*':
			case '=':
			case '(':
			case ')':
			case ':':
			case ';':
			case '.':
			case '|':
			case ',':
			case '+':
			case '-':
			case '?':
				ret = c;
				break;

			case '/':
				docomment();
				continue;

			case '%':
				ret = doopt();
				break;

			case '\'':
				ret = dochar();
				break;

			case '"':
				ret = dostr();
				break;

			case EOI:
				ret = EOI;
				break;

			default:
				error("%d: Illegal character 0x%X in file (ignored).",
						w_currline(), c);
				break;
			}

		*text = '\0';
		return ret;
	}

	return EOI;
}
