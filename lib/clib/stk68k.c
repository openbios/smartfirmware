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
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <a.out.h>

#include "stdlibx.h"
#include "stdiox.h"
#include "stringx.h"
#include "stkback.h"

#ifdef CPLUS_DEMANGLE
extern const char *cplus_demangle(const char *name);
#endif

typedef struct Symbol
{
	long addr;
	union
	{
		char const *name;
		long offset;
	} u;
} Symbol;

static Symbol *symarray = NULL;
static int symcount = 0;
static char *strings = NULL;

static int
symcmp(void *p1, void *p2)
{
	Symbol *s1 = (Symbol *)p1;
	Symbol *s2 = (Symbol *)p2;
	return s1->addr - s2->addr;
}

boolean
readsymbols(char const *name)
{
	FILE *fp;
	FILHDR filhdr;
	int arrent, i;
	long loc, len;

	if ((fp = pathvopen("PATH", name, "r")) == NULL
			&& (fp = fopen(name, "r")) == NULL)
		return FALSE;

	if (fread(&filhdr, sizeof filhdr, 1, fp) != 1)
	{
		fclose(fp);
		return FALSE;
	}

	if (BADMAG(&filhdr) || fseek(fp, filhdr.f_symptr, SEEK_SET) != 0)
	{
		fclose(fp);
		return FALSE;
	}

	/* this lets us read the symbols of other files, if desired */
	if (symarray)
		free(symarray);

	symcount = filhdr.f_nsyms;
	symarray = (Symbol *)malloc(symcount * sizeof symarray[0]);

	if (symarray == NULL)
	{
		fclose(fp);
		symcount = 0;
		return FALSE;
	}

	arrent = 0;

	for (i = 0; i < symcount; i++)
	{
		SYMENT syment;

		if (fread(&syment, sizeof syment, 1, fp) != 1)
		{
			fclose(fp);
			return FALSE;
		}

		/* we only care about externally visible symbols and static syms 
		*/
		/* that are in the text segment - these should all be functions */
		if ((syment.n_sclass != C_EXT && syment.n_sclass != C_STAT)
				|| (syment.n_scnum != 1 && syment.n_scnum != N_ABS)
				|| syment.n_value == 0 || syment.n_name[0] == '.')
			/* || syment.n_scnum == N_DEBUG */
			continue;

		symarray[arrent].addr = syment.n_value;

		if (syment.n_zeroes)
		{
			char buf[SYMNMLEN + 1];

			memcpy(buf, syment.n_name, SYMNMLEN);
			buf[SYMNMLEN] = '\0';

			if (buf[strlen(buf) - 1] == '.')
				continue;

#ifdef CPLUS_DEMANGLE
			symarray[arrent].u.name = cplus_demangle(buf);

			if (symarray[arrent].u.name == NULL)
#endif
				symarray[arrent].u.name = strdup(buf);
		}
		else
			/* these will be fixed up before we return */
			symarray[arrent].u.offset = syment.n_offset;

		arrent++;
	}

	symcount = arrent;
	/* yes, we are wasting some empty entries in symarray, but this is */
	/* a lot easier than walking the file twice to figure out how many */
	/* functions there really are in there */

	loc = ftell(fp);

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return FALSE;
	}

	len = ftell(fp) - loc - 1;

	if (fseek(fp, loc, SEEK_SET) != 0)
	{
		fclose(fp);
		return FALSE;
	}

	/* again, this is to read symbols of other files */
	strfree(strings);
	strings = strnew(len);

	if (strings == NULL)
	{
		fclose(fp);
		return FALSE;
	}

	if (fread(strings, sizeof strings[0], len, fp) != len)
	{
		fclose(fp);
		return FALSE;
	}

	strings[len] = '\0';
	fclose(fp);

	for (i = 0; i < symcount; i++)
		/* offsets should not point anywhere near the data segment, so
		   this */
		/* trick will work without using an additional boolean per symbol 
		*/
		if (symarray[i].u.offset < len)
		{
			char *s = strings + symarray[i].u.offset;
#ifdef CPLUS_DEMANGLE
			const char *n;
#endif

			symarray[i].u.name = (s[strlen(s) - 1] == '.') ? NULL : s;
#ifdef CPLUS_DEMANGLE
			n = cplus_demangle(symarray[i].u.name);

			if (n != NULL)
				symarray[i].u.name = n;
#endif
		}

	qsort((void *)symarray, symcount, sizeof symarray[0], (void *)symcmp);

#ifdef DEBUG
	for (i = 0; i < symcount; i++)
		if (symarray[i].u.name != NULL)
			fprintf(stderr, "sym[%d] = %s : 0x%0lX\r\n", i, symarray[i].u.name,
					symarray[i].addr);
#endif

	return TRUE;
}

static char const *progfile = NULL;

void
programfilename(char const *name)
{
	progfile = name;
}

boolean
symbollookup(unsigned long addr, char const **name, unsigned long *offset)
{
	int l, r, i;

	if (name != NULL)
		*name = NULL;

	if (offset != NULL)
		*offset = 0;

	if (symcount <= 0 && progfile != NULL)
		readsymbols(progfile);

	if (symarray == NULL)
		return FALSE;

	l = 0;
	r = symcount - 1;

	for (i = (l + r) >> 1; l < i && l < r; i = (l + r) >> 1)
		if (addr < symarray[i].addr)
			r = i;
		else
			l = i;

	if (i >= symcount)
		return FALSE;

	while (symarray[i].u.name == NULL && i > 0)
		i--;

	if (name != NULL)
		*name = symarray[i].u.name;

	if (offset != NULL)
		*offset = addr - symarray[i].addr;

	return TRUE;
}

char const *
addrlabel(unsigned long addr, boolean syms)
{
	char const *name;
	unsigned long offset;
	static char label[BUFSIZ];

	if (syms && symbollookup(addr, &name, &offset))
	{
		if (offset)
			sprintf(label, "%s+0x%lX", name, offset);
		else
			sprintf(label, "%s", name);
	}
	else
		sprintf(label, "0x%08lX", addr);

	return label;
}

void
backtrace(boolean syms)
{
	static boolean inbacktrace = FALSE;
	long *ap = (long *)&ap + 1;

	if (inbacktrace)
		return;

	inbacktrace = TRUE;

	fprintf(stderr, "\r***STACK BACKTRACE***\r\n");

	while (ap != NULL)
	{
		/* print address of caller */
		unsigned short *pc = (unsigned short *)ap[1];

		fprintf(stderr, "%s\t", addrlabel((long)pc, syms));

		/* print address of callee if known */
		/* 0100111010111001 == JSR */
		if (pc[-3] == 0x4EB9)
			fprintf(stderr, "%s",
					addrlabel(((long)pc[-2] << 16) | pc[-1], syms));
		/* 0110000100000000 == BSR.L */
		else if (pc[-3] == 0x61FF)
			fprintf(stderr, "%s",
					addrlabel(((long)pc) + (((long)pc[-2] << 16) | pc[-1]) - 4,
					syms));
		/* 0110000100000000 == BSR.W */
30: Unmatched else
		else if (pc[-2] == 0x6100)
			fprintf(stderr, "%s",
					addrlabel((long)pc + (short)pc[-1] - 2, syms));
		/* 01100001???????? == BSR.B */
34: Unmatched else
		else if ((pc[-1] & 0xFF00) == 0x6100)
			fprintf(stderr, "%s",
					addrlabel(((long)pc) + (char)(pc[-1] & 0xFF), syms));
37: Unmatched else
		else
		fprintf(stderr, "???");

		/* 0101???0??001111 == ADDQ.L #n,%SP */
		/* print paramter of call.  max of five. */
		if ((pc[0] & 0xF13F) == 0x500F)
			switch ((pc[0] & 0x0E00) >> 9)
			{
			case 0:				/* Two int's */
				fprintf(stderr, "(0x%X, 0x%X)\r\n", ap[2], ap[3]);
				break;
			case 4:				/* One int */
				fprintf(stderr, "(0x%X)\r\n", ap[2]);
				break;
			default:			/* Don't know */
				fprintf(stderr, "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X)\r\n", ap[2],
						ap[3], ap[4], ap[5], ap[6]);
				break;
			}
		/* 0100111111101111 == LEA.L #n(%SP),%SP */
		else if (pc[0] == 0x4FEF)
			switch (pc[1])
			{
			case 0:				/* No int's */
				fprintf(stderr, "()\r\n");
				break;
			case 4:				/* One int */
				fprintf(stderr, "(0x%X)\r\n", ap[2]);
				break;
			case 8:				/* Two int's */
				fprintf(stderr, "(0x%X, 0x%X)\r\n", ap[2], ap[3]);
				break;
			case 12:			/* Three int's */
				fprintf(stderr, "(0x%X, 0x%X, 0x%X)\r\n", ap[2], ap[3], ap[4]);
				break;
			case 16:			/* Four int's */
				fprintf(stderr, "(0x%X, 0x%X, 0x%X, 0x%X)\r\n", ap[2],
						ap[3], ap[4], ap[5]);
				break;
			default:			/* Don't know */
				fprintf(stderr, "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X)\r\n", ap[2],
						ap[3], ap[4], ap[5], ap[6]);
				break;
			}
		else					/* Don't know */
			fprintf(stderr, "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X)\r\n", ap[2], ap[3],
					ap[4], ap[5], ap[6]);

		/* next frame point must be on the stack */
		if (ap[0] > 0 || ap[0] <= (long)ap)
		{
			fprintf(stderr, "bad frame pointer 0x%X\r\n", ap[0]);
			break;
		}

	ap = (long *)ap[0];
	}

	inbacktrace = FALSE;
}

int
stacksnapshot(int size, uInt *stack, int skip)
{
	int count = 0;
	long *ap = (long *)&ap + 1;

	while (ap != NULL && count < size)
	{
		unsigned short *pc = (unsigned short *)ap[1];

		/* put caller's address into the return array */
		if (skip == 0)
			stack[count++] = (uInt)pc;
		else
			skip--;

		/* next frame point must be on the stack */
		if (ap[0] > 0 || ap[0] <= (long)ap)
			break;

	ap = (long *)ap[0];
	}

	return count;
}

#ifdef DEBUG
static void
func(int arg)
{
	backtrace(TRUE);
	backtrace(FALSE);
}

main(int argc, const char *argv[])
{
	programfilename(argv[0]);
	func(32);
	return 0;
}
#endif
