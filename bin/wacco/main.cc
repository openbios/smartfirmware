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

// Copyright (c) 1991-2002 by Parag Patel.
// All Rights Reserved.

char whatid[] = "@(#)wacco 3.5b1 (2002 April 27)";

#include "defs.h"
#include "toks.h"
#include "gen.h"
#include "genc.h"
#include "genj.h"
#include "genm.h"

#ifdef unix
#	define boolean bogus_bool
#	include <unistd.h>
#	undef boolean
#else
#	define unlink remove
#endif

#include <stdarg.h>
#include <getoptx.h>

#ifdef BSD
#	include <sys/wait.h>
#else
	extern "C" int wait(int *);
#endif

#if defined(__MWERKS__) && defined(macintosh)
#	include <console.h>
#endif

// These vars are modified from the command line
boolean optimize = TRUE;
boolean dumpcode = TRUE;
boolean dumponlyparser = FALSE;
boolean statonly = FALSE;
#ifdef unix
boolean docompare = TRUE;
#else
boolean docompare = FALSE;
#endif
boolean casesensitive = FALSE;
boolean dargstyle = TRUE;
boolean genlineinfo = TRUE;
boolean doresync = TRUE;
boolean mactabs = TRUE;
const char *headername = NULL;
const char *scannername = NULL;
const char *parsername = NULL;
boolean waccout = FALSE;
const char *lang = "C";
Gencode *gen = NULL;

// just other globals here
boolean exportedname = FALSE;

int
error(const char *fmt, ...)
{
	va_list ap;
	w_numerrors++;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
	va_end(ap);
	return W_RETERR;
}

void
quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "!\n");
	va_end(ap);
	exit(1);
}


#define NUMBUFS 4

// This version of strbldf allows up to NUMBUFS calls to strbldf to be
// used as arguments to a single function call such as printf() or putf().

const char *
strbldf(const char *format, ...)
{
	static char *buf[NUMBUFS];
	static long len[NUMBUFS];
	static int curr = 0;

	if (--curr < 0)
		curr = NUMBUFS - 1;

	va_list ap;
	va_start(ap, format);
	xvsprintf(buf[curr], len[curr], format, ap);
	va_end(ap);
	return buf[curr];
}


void
dumpset(Bitset *set)
{
	if (set == NULL)
	{
		printf("(<null>)\n");
		return;
	}

	printf("(");

	for (int id = START; id < set->size(); id++)
	{
		if (!set->has(id))
			continue;

		Symbol *s = getsymbol(id);
		printf(" %s", s->name);
	}

	if (set->has(EMPTY))
		printf(" []");
	else if (set->has(EOI))
		printf(" $");

	printf(" )\n");
}

void
dump(void)
{
	Symbol *sym;
	Symnode *n1, *n2;
	int i;
	char *s;

	for (i = START; i < numsymbols(); i++)
	{
		sym = getsymbol(i);

		if (sym->type == NONTERMINAL)
		{
			if (sym->lexstr == NULL)
				printf("%s", sym->name);
			else
				printf("%s (%s)", sym->lexstr, sym->name);

			if (sym->rettype != NULL)
				printf("<%s>", sym->rettype);

			if (sym->symcode != NULL)
				printf("\n\t\t{%s}", sym->symcode->name);

			printf(" use=%d usedret=%d inline=%d",
					sym->usecount, sym->usedret, sym->doinline);

			if (sym->toempty)
				printf(" ==> []");

			printf("\n");
			s = "\t:";

			for (n1 = sym->node; n1 != NULL; n1 = n1->orelse)
			{
				const char *post = "";

				for (n2 = n1; n2 != NULL; n2 = n2->next)
				{
					if (n2->sym->type == CODE)
						printf("%s {%s}\n", s, n2->sym->code);
					else if (n2->sym->type == TERMINAL ||
							n2->sym->type == NONTERMINAL)
					{
						printf("%s %s%s", s, n2->sym->name, post);

						if (n2->resync)
						{
							printf("[%d] = ", n2->resync->id);
							dumpset(n2->resync->set);
						}
						else
							printf("\n");
					}
					else if (n2->sym->type == ZEROPLUS)
					{
						post = "*";
						continue;
					}
					else if (n2->sym->type == ONEPLUS)
					{
						post = "+";
						continue;
					}
					else if (n2->sym->type == ONEZERO)
					{
						post = "?";
						continue;
					}

					s = "\t ";
					post = "";
				}

				s = "\t|";
			}

			printf("\t;\n");
		}
		else if (sym->type == TERMINAL)
		{
			printf("[%s]", sym->name);

			if (sym->lexstr != NULL)
				printf(" == %s", sym->lexstr);

			printf("\n");
		}
		else if (sym->type == CODE)
		{
			printf("%s == {%s}\n", sym->name, sym->code);
			continue;
		}
		else if (sym->type == ZEROPLUS)
		{
			printf("<zero-plus>\n");
			continue;
		}
		else if (sym->type == ONEPLUS)
		{
			printf("<one-plus>\n");
			continue;
		}
		else if (sym->type == ONEZERO)
		{
			printf("<one-zero>\n");
			continue;
		}

		printf("FIRST(%s)  = ", sym->name);
		dumpset(sym->first);
		printf("FOLLOW(%s) = ", sym->name);
		dumpset(sym->follow);

		if (sym->resync != NULL)
		{
			printf("SKIP-SET(%s[%d])  = ", sym->name, sym->resync->id);
			dumpset(sym->resync->set);
		}

		printf("\n");
	}
}

void
output_nonterm(Symbol *sym)
{
	Symnode *n1, *n2;
	char *s;

	if (sym->realname && sym->realname != sym->name)
	{
		printf("(");
		s = " ";
	}
	else
	{
		printf("%s", sym->name);
		s = "\n\t: ";
	}

	if (sym->rettype != NULL)
		printf("<%s>", sym->rettype);

	for (n1 = sym->node; n1 != NULL; n1 = n1->orelse)
	{
		const char *post = "";

		for (n2 = n1; n2 != NULL; n2 = n2->next)
		{
			if (n2->sym->type == CODE)
				continue;
			else if (n2->sym->type == TERMINAL)
				printf("%s%s%s", s, n2->sym->name, post);
			else if (n2->sym->type == NONTERMINAL)
			{
				printf("%s", s);

				if (n2->sym->realname &&
						n2->sym->realname != n2->sym->name)
					output_nonterm(n2->sym);
				else
					printf("%s", n2->sym->name);

				printf("%s", post);
			}
			else if (n2->sym->type == ZEROPLUS)
			{
				post = "*";
				continue;
			}
			else if (n2->sym->type == ONEPLUS)
			{
				post = "+";
				continue;
			}

			s = " ";
			post = "";
		}

		s = "\n\t| ";
	}

	if (sym->realname && sym->realname != sym->name)
		printf(" )");
	else
		printf("\n\t;\n");
}

void
output_wacco(void)
{
	for (int i = START; i < numsymbols(); i++)
	{
		Symbol *sym = getsymbol(i);

		if (sym->type != NONTERMINAL ||
				(sym->realname && sym->realname != sym->name))
			continue;

		output_nonterm(sym);
		printf("\n");
	}
}

static char *tmpname = ".wacco.tmp";

FILE *
makefile(const char *fname)
{
	if (docompare)
		fname = tmpname;

	FILE *fp = fopen(fname, "w");

	if (fp == NULL)
		quit("Cannot open %s for writing!", fname);

	return fp;
}

#ifdef unix
void
cmpandmv(const char *file)
{
	if (!docompare)
		return;

#ifdef BSD
	int pid = fork();
#else
	pid_t pid = fork();
#endif

	if (pid == 0)
	{
		execlp("cmp", "cmp", "-s", file, tmpname, NULL);
		exit(-1);
	}

#ifdef BSD
	union wait stat;
	int w;
#else
	int stat;
	pid_t w;
#endif
	do
		w = wait(&stat);
	while (w != pid && w >= 0);

#ifdef BSD
	if (stat.w_termsig == 0 && stat.w_retcode == 0)
		return;
#else
	if (stat == 0)
		return;
#endif

	unlink(file);

	if (link(tmpname, file) < 0)
		quit("Cannot link %s to %s", tmpname, file);

	unlink(tmpname);
}
#else /* !unix */
void
cmpandmv(const char *)
{
}
#endif /* !unix */

int
getoptions(int argc, const char *argv[])
{
	int c;

	while ((c = getoptx(argc, argv, "ciah:s:p:l:DOCLPRWtTV")) != EOF)
		switch (c)
		{
		case 'D':
			statonly = TRUE;
			break;

		case 'c':
			docompare = FALSE;
			break;

		case 'i':
			casesensitive = TRUE;
			break;

		case 'a':
			dargstyle = FALSE;
			break;

		case 'h':
			headername = strget(optarg);
			break;

		case 's':
			scannername = strget(optarg);
			break;

		case 'p':
			parsername = strget(optarg);
			break;

		case 'l':
			lang = strget(optarg);
			break;

		case 'O':
			optimize = FALSE;
			break;

		case 'C':
			dumpcode = FALSE;
			break;

		case 'P':
			dumponlyparser = TRUE;
			break;

		case 'L':
			genlineinfo = FALSE;
			break;

		case 'R':
			doresync = FALSE;
			break;

		case 'W':
			waccout = TRUE;
			break;

		case 't':
			mactabs = TRUE;
			break;

		case 'T':
			mactabs = FALSE;
			break;
		
		case 'V':
			printf("%s\n", whatid + 4);
			exit(0);
			break;

		default:
			quit("Usage: %s [-ciaDOCLRtTV] [-l lang]"
					" [-h header] [-p parser] [-s scanner] [file]",
					argv[0]);
			break;
		}

	return optind;
}


int
main(int argc, char *argv[])
{
#if defined(__MWERKS__) && defined(macintosh)
	argc = ccommand(&argv);
#endif

	int optind = getoptions(argc, (const char **)argv);
	initsym();
	const char *infile = NULL;
	FILE *fp = stdin;

	if (optind < argc)
	{
		fp = fopen(infile = argv[optind], "r");

		if (fp == NULL)
			quit("Cannot open file ``%s'' for reading", argv[optind]);
	}

	w_setfile(fp);

	if (!program())
		return 1;

	if (streqi(lang, "C"))
		gen = new GenC;
	else if (streqi(lang, "Java"))
		gen = new GenJava;
	else if (streqi(lang, "Modula-3"))
		gen = new GenModula3;
	else
	{
		quit("output language ``%s'' unknown or not supported", lang);
	}

	if (startsymbol == NULL && !exportedname)
		quit("No start Symbol defined");

	buildsets();
	check();

	if (w_numerrors > 0)
	{
		if (doresync)
			quit("Not generating any output files");
		else
			error("Ignoring errors and generating output files anyway");
	}

	if (statonly)
	{
		dump();
		return 0;
	}

	if (waccout)
	{
		output_wacco();
		return 0;
	}

	gen->gencode(infile, headername, parsername, scannername);

	if (docompare)
		unlink(tmpname);

	delete gen;
	return 0;
}

/* to force another malloc to be linked in before the standard malloc */
void
never_called()
{
	(void)malloc(32);
}
