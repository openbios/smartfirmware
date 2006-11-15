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

/* Copyright (c) 1999 by CodeGen, Inc.  All Rights Reserved. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef int boolean;
#define FALSE	0
#define TRUE	1


struct symbol
{
	unsigned long addr;
	char *name;
};

static struct symbol *symarray = NULL;
static int numsyms = 0;
static int maxsyms = 0;


void
readsymbols(const char *exe)
{
	FILE *fp;
	char *s;
	unsigned long a;
	char buf[1024];

	snprintf(buf, sizeof buf - 1, "nm -n %s", exe);
	fp = popen(buf, "r");
	
	if (fp == NULL)
	{
		fprintf(stderr, "popen of nm -n %s failed!\n", exe);
		exit(1001);
	}

	while (fgets(buf, sizeof buf - 1, fp))
	{
		buf[sizeof buf - 1] = '\0';
		buf[strlen(buf) - 1] = '\0';

		a = strtol(buf, &s, 16);

		if (!a || !s)
			continue;

		if (numsyms >= maxsyms)
		{
			maxsyms += 1024;
			symarray = realloc(symarray, maxsyms * sizeof *symarray);

			if (symarray == NULL)
			{
				fprintf(stderr, "cannot realloc - out of memory!\n");
				exit(1002);
			}
		}

		while (isspace(*s))
			s++;

		s += 2;

		symarray[numsyms].addr = a;
		symarray[numsyms].name = malloc(strlen(s) + 1);

		if (symarray[numsyms].addr == NULL)
		{
			fprintf(stderr, "cannot strdup - out of memory!\n");
			exit(1003);
		}

		strcpy(symarray[numsyms].name, s);
		numsyms++;
	}

	fclose(fp);
}

void
dumpsymbols(void)
{
	int i;

	for (i = 0; i < numsyms; i++)
		printf("0x%08lX %s\n", symarray[i].addr, symarray[i].name);
}

boolean
symbollookup(unsigned long addr, const char **name, unsigned long *offset)
{
	struct symbol sym;
	int i, l, r;

	sym.addr = addr;
	(*name) = NULL;
	(*offset) = 0;

	if (numsyms <= 0)
		return FALSE;

	l = 0;
	r = numsyms - 1;

	for (i = (l + r) >> 1; l < r; i = (l + r) >> 1)
		if (addr < symarray[i].addr)
			r = i - 1;
		else if (i < numsyms - 1 && addr >= symarray[i + 1].addr)
			l = i + 1;
		else
			break;

	while (symarray[i].name == NULL && i > 0)
		i--;

	*name = symarray[i].name;
	*offset = addr - symarray[i].addr;

	return TRUE;
}

void
processlog(const char *log)
{
	FILE *fp;
	unsigned long a, offset;
	char *s, *addrs;
	const char *name;
	boolean printed;
	char buf[1024];
	
	fp = fopen(log, "r");

	if (fp == NULL)
	{
		fprintf(stderr, "cannot open logfile %s!\n", log);
		exit(2001);
	}

	while (fgets(buf, sizeof buf - 1, fp))
	{
		buf[sizeof buf - 1] = '\0';
		buf[strlen(buf) - 1] = '\0';

		addrs = strrchr(buf, ' ');

		if (!addrs)
			continue;
		
		*addrs++ = '\0';
		printf("%s", buf);
		printed = FALSE;

		for (s = strsep(&addrs, ":"); s; s = strsep(&addrs, ":"))
		{
			a = strtol(s, NULL, 16);

			if (!a)
				continue;
			
			if (symbollookup(a, &name, &offset))
			{
				printf(printed ? ":" : " ");

				printf("%s", name);

				if (offset)
					printf("+0x%lX", offset);
				
				printed = TRUE;
			}
		}

		if (printed)
			printf("\n");
	}

	fclose(fp);
}

int
main(int argc, const char *argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "usage: %s executable logfile [debug]\n", argv[0]);
		return -1;
	}

	readsymbols(argv[1]);

	if (argc > 3)
		dumpsymbols();

	processlog(argv[2]);
	return 0;
}
