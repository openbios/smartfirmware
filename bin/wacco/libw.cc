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

/* Copyright (c) 1991,1993-1994,1997 by Parag Patel.  All Rights Reserved. */

/* This file may be compiled with C++, ANSI C, or K&R C as necessary. */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(__STDC__) || defined(__cplusplus)
#include <stdarg.h>
#endif
#include "toks.h"

#define ERRSTACK 16
#define ERRBUF 192
#define UNGET 64

struct Errelt
{
	int col;
	int skip;					/* boolean */
	char err[ERRBUF];
};


#ifdef __cplusplus
extern "C" {
extern void w_closefile(void);
extern int w_openfile(char *);
extern FILE *w_getfile(void);
extern void w_setfile(FILE *);
extern int w_currcol(void);
extern int w_currline(void);
extern char *w_getcurrline(void);
extern int w_input(void);
extern int w_unput(int);
extern void w_output(int);
extern void w_scanerr(const char *fmt, ...);
}
#endif

static FILE *fp = stdin;

static int line = 0;
static int col = 0;
static char linebuf[512];
static int lineloc = 0;
static int ubuf[UNGET];
static int top = 0;

static int tokcol = 0;
static struct Errelt errstk[ERRSTACK];
static int numerrs = 0;


#if defined(__STDC__) || defined(__cplusplus)
void
w_flusherrs(void)
#else
w_flusherrs()
#endif
{
	int i;
	int pcol = 0;

	if (numerrs <= 0)
		return;

	fprintf(stderr, "%d:\t%s", line, linebuf);

	if (linebuf[strlen(linebuf) - 1] != '\n')
		fprintf(stderr, "\n");

	fprintf(stderr, "\t");

	for (i = 0; i < numerrs; i++)
	{
		for (; pcol < errstk[i].col - 1; pcol++)
			fprintf(stderr, linebuf[pcol] == '\t' ?
					"\t" : (errstk[i].skip ? "*" : " "));

		if (i > 0 && errstk[i].col == errstk[i - 1].col)
			continue;

		if (linebuf[pcol] == '\t')
			fprintf(stderr, "\t");

		pcol++;
		fprintf(stderr, errstk[i].skip ? "*" : "^");
	}

	fprintf(stderr, "\n");

	for (i = 0; i < numerrs; i++)
	{
		int j;

		if (errstk[i].skip)
			continue;

		fprintf(stderr, "\t");

		for (j = 0; j < errstk[i].col - 1; j++)
			fprintf(stderr, linebuf[j] == '\t' ? "\t" : " ");

		if (linebuf[j] == '\t')
			fprintf(stderr, "\t");

		fprintf(stderr, "%s\n", errstk[i].err);
	}

	numerrs = 0;
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_scanerr(const char *fmt, ...)
#else
w_scanerr(fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9)
char *fmt;
long a1, a2, a3, a4, a5, a6, a7, a8, a9;
#endif
{
#if defined(__STDC__) || defined(__cplusplus)
	va_list ap;
	va_start(ap, fmt);
#endif

	if (numerrs >= ERRSTACK)
		w_flusherrs();

	w_numerrors++;

	if (fmt == NULL)
		errstk[numerrs].skip = 1;
	else
	{
		errstk[numerrs].skip = 0;
#if defined(__STDC__) || defined(__cplusplus)
		vsprintf(errstk[numerrs].err, (char *)fmt, ap);
#else
		sprintf(errstk[numerrs].err, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
	}

	errstk[numerrs].col = tokcol;
	numerrs++;
#if defined(__STDC__) || defined(__cplusplus)
	va_end(ap);
#endif
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_err_expected(const char *token)
#else
w_err_expected(token)
char *token;
#endif
{
	w_scanerr("expected \"%s\"", token);
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_err_illegal(const char *rule)
#else
w_err_illegal(token)
char *token;
#endif
{
	w_scanerr("illegal (badly formed) %s", rule);
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_err_skip(void)
#else
w_err_skip()
#endif
{
	w_scanerr(NULL);
}

int
#if defined(__STDC__) || defined(__cplusplus)
w_input(void)
#else
w_input()
#endif
{
	int c;

	if (top > 0)
		return ubuf[--top];

	if (line > 0)
		c = linebuf[lineloc++];
	else
		c = '\0';

	while (c == '\0')
	{
		w_flusherrs();
		line++;
		lineloc = 0;
		tokcol = col = 0;
		linebuf[0] = '\0';

		if ((fgets(linebuf, sizeof linebuf - 1, fp)) == NULL)
		{
			line = 0;
			return W_EOI;
		}

		c = linebuf[lineloc++];
	}

	col++;
	return c;
}

int
#if defined(__STDC__) || defined(__cplusplus)
w_unput(int c)
#else
w_unput(c)
int c;
#endif
{
	if (top >= UNGET)
		return W_RETERR;

	ubuf[top++] = c;
	return W_RETOK;
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_output(int c)
#else
w_output(c)
int c;
#endif
{
	putchar(c);
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_closefile(void)
#else
w_closefile()
#endif
{
	if (fp == NULL)
		return;

	fclose(fp);
}

int
#if defined(__STDC__) || defined(__cplusplus)
w_openfile(char *fname)
#else
w_openfile(fname)
char *fname;
#endif
{
	if (fp != NULL)
		w_closefile();

	if ((fp = fopen(fname, "r")) == NULL)
		return W_RETERR;

	return W_RETOK;
}

FILE *
#if defined(__STDC__) || defined(__cplusplus)
w_getfile(void)
#else
w_getfile()
#endif
{
	return fp;
}

#if defined(__STDC__) || defined(__cplusplus)
void
w_setfile(FILE *f)
#else
w_setfile(f)
FILE *f;
#endif
{
	fp = f;
}

int
#if defined(__STDC__) || defined(__cplusplus)
w_currcol(void)
#else
w_currcol()
#endif
{
	return col;
}

int
#if defined(__STDC__) || defined(__cplusplus)
w_currline(void)
#else
w_currline()
#endif
{
	return line;
}

char *
#if defined(__STDC__) || defined(__cplusplus)
w_getcurrline(void)
#else
w_getcurrline()
#endif
{
	return linebuf;
}

#if defined(__cplusplus)
extern "C" int yylex(void);
#elif defined(__STDC__)
extern int yylex(void);
#else
extern int yylex();
#endif

int
#if defined(__STDC__) || defined(__cplusplus)
w_gettoken(void)
#else
w_gettoken()
#endif
{
	tokcol = col;
	return yylex();
}
