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


#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#ifdef __DJGPP_SOURCE
#include <aout.h>
#else
#include <a.out.h>
#endif
#include <stdio.h>
#include <unistd.h>

#include <stdlibx.h>
#include <stdiox.h>
#include <stringx.h>
#include <srchsort.h>

#include "backtrace.h"

extern char const *demangle(char const *);

struct symbol
{
    unsigned long addr;
    char const *name;
};

declare_searchsort(struct symbol)
implement_searchsort(struct symbol)

static struct symbol *symarray = NULL;
static int symcount = 0;
static char *strings = NULL;
static int stringsize = 0;
static unsigned long start_addr = 0;

static int
symcmp(const struct symbol *s1, const struct symbol *s2)
{
    return s1->addr - s2->addr;
}

boolean
readsymbols(char const *name)
{
    FILE *fsym;
    struct exec ebuf;
    off_t strings_offset;
    off_t symbol_offset;
    off_t symbol_size;

    if ((fsym = pathvopen("PATH", name, "rb")) == NULL)
    {
	char *base = (char *)base_name(name);
	int len = strlen(base);
	int killexe = FALSE;

	if (streq(&base[len - 4], ".exe"))
	{
	    killexe = TRUE;
	    base[len - 4] = '\0';
	}

	fsym = pathvopen("PATH", base, "rb");

	if (killexe)
	    base[len - 4] = '.';

	if (fsym == NULL)
	{
//	    fprintf(stderr, "readsymbols: can't open program file.\n");
	    return FALSE;
	}
    }


    if (fread((char *)&ebuf, sizeof(struct exec), 1, fsym) != 1 ||
	    N_BADMAG(ebuf))
    {
//	fprintf(stderr, "readsymbols: can't read program header.\n");
	fclose(fsym);
	return FALSE;
    }

    start_addr = N_TXTADDR(ebuf);
    symbol_offset = N_SYMOFF(ebuf);
    symbol_size = ebuf.a_syms;
    strings_offset = symbol_offset + symbol_size;

    if (fseek(fsym, symbol_offset, SEEK_SET))
    {
//	fprintf(stderr, "readsymbols: can't seek to symbol table.\n");
	fclose(fsym);
	return FALSE;
    }

    // this is so we can call readsymbols() for other executables
    if (symarray != NULL)
    {
	delete[/*symcount*/] symarray;
	delete[/*stringsize*/] strings;
	symarray = NULL;
	strings = NULL;
    }

    symcount = symbol_size / sizeof (struct nlist);
    symarray = new struct symbol[symcount];
    struct nlist *nbuf = new struct nlist[symcount];

    if ((int)fread((char *)nbuf, sizeof(struct nlist), symcount, fsym) !=
	    symcount)
    {
	delete[/*symcount*/] nbuf;
    	delete[/*symcount*/] symarray;
	symarray = NULL;
//	fprintf(stderr, "readsymbols: can't read symbol table.\n");
	fclose(fsym);
	return FALSE;
    }

    int j = 0;
    unsigned maxstrx = 0;
    int i;

    for (i = 0; i < symcount; i++)
    {
	if (nbuf[i].n_un.n_strx == 0 || (nbuf[i].n_type & N_STAB))
	    continue;

//	if (!(nbuf[i].n_type & N_EXT))
//	    continue;

	if ((unsigned)nbuf[i].n_un.n_strx > maxstrx)
	    maxstrx = nbuf[i].n_un.n_strx;

	if (j != i)
	    nbuf[j++] = nbuf[i];
    }

    symcount = j;

    if (fseek(fsym, strings_offset, SEEK_SET))
    {
//	fprintf(stderr, "readsymbols: can't seek to string table.\n");
	delete[/*symcount*/] nbuf;
    	delete[/*symcount*/] symarray;
	symarray = NULL;
	fclose(fsym);
	return FALSE;
    }

    stringsize = maxstrx + BUFSIZ + 1;
    strings = new char[stringsize + 1];
    int chars = fread(strings, 1, maxstrx + BUFSIZ, fsym);
    fclose(fsym);

    if ((unsigned)chars <= maxstrx)
    {
//	fprintf(stderr, "readsymbols: can't read string table.\n");
	delete[/*symcount*/] nbuf;
    	delete[/*symcount*/] symarray;
    	delete[/*stringsize*/] strings;
	symarray = NULL;
	strings = NULL;
	fclose(fsym);
    	return FALSE;
    }

    strings[chars] = '\0';

    for (i = 0; i < symcount; i++)
    {
#ifdef DEMANGLE
	symarray[i].name = strdup(demangle(&strings[nbuf[i].n_un.n_strx]));
#else
	symarray[i].name = &strings[nbuf[i].n_un.n_strx];
#endif
	symarray[i].addr = nbuf[i].n_value;
    }

#ifdef DEMANGLE
    delete[/*stringsize*/] strings;
    strings = NULL;
#endif

    // sort by address
    qsort(symarray, symcount, symcmp);
    delete[/*symcount*/] nbuf;
    return TRUE;
}

#ifdef EBUG
void
dumpsymbols(FILE *fp)
{
    for (int i = 0; i < symcount; i++)
    	fprintf(fp, "0x%08X %s\n", symarray[i].addr, symarray[i].name);
}
#endif

static char const *progfile = NULL;

void
programfilename(char const *name)
{
    progfile = name;
}

boolean
symbollookup(unsigned long addr, char const *&name, unsigned long &offset)
{
    struct symbol sym;
    sym.addr = addr;
    name = NULL;
    offset = 0;

    if (symcount <= 0)
    {
    	if (progfile != NULL)
	    readsymbols(progfile);

	if (symcount <= 0)
	{
	    char *name = getenv("PROGRAMFILENAME");

	    if (name != NULL)
		readsymbols(name);
	}
    }

    if (symcount <= 0)
	return FALSE;

    int l = 0;
    int r = symcount - 1;
	int i;

    for (i = (l + r) >> 1; l < r; i = (l + r) >> 1)
	if (addr < symarray[i].addr)
	    r = i - 1;
	else if (i < symcount - 1 && addr >= symarray[i + 1].addr )
	    l = i + 1;
	else
	    break;

    while (symarray[i].name == NULL && i > 0)
	i--;

    name = symarray[i].name;
    offset = addr - symarray[i].addr;
    return TRUE;
}

char const *
addrlabel(unsigned long addr, boolean syms)
{
    char const *name;
    unsigned long offset;
    static char label[BUFSIZ];

    if (syms && symbollookup(addr, name, offset))
    	if (offset == 0)
	    sprintf(label, "%s", name);
	else
	    sprintf(label, "%s+0x%lX", name, offset);
    else
	sprintf(label, "0x%08lX", addr);

    return label;
}

void
backtrace(boolean syms)
{
    long *bp = &((long *)&syms)[-2];
    static boolean inbacktrace = FALSE;

    if (inbacktrace)
	return;

    inbacktrace = TRUE;
    fprintf(stderr, "\r***STACK BACKTRACE***\r\n");

    while (bp != NULL && (unsigned long)bp[1] >= start_addr)
    {
	/* print address of caller */
	unsigned char *pc = (unsigned char *)bp[1];
	char const *label = addrlabel((long)pc, syms);
	int labellen = strlen(label);

	if (labellen < 8)
	    fprintf(stderr, "%s\t\t", label);
	else if (labellen < 16)
	    fprintf(stderr, "%s\t", label);
	else
	    fprintf(stderr, "%s\n\t\t", label);

	/* print address of callee if known */
	/* 11101000 == CALL disp32 */
	if (pc[-5] == 0xE8)
	    fprintf(stderr, "%s",
		    addrlabel((long)pc + (((long)pc[-1] << 24) |
		    ((long)pc[-2] << 16) | ((long)pc[-3] << 8) | pc[-4]),
		    syms));
	else
	    fprintf(stderr, "???");
		
	/* print paramter of call.  max of five. */
	/* 10000011 11000100 == ADDL $n,%esp */
	if (pc[0] == 0x83 && pc[1] == 0xC4)
	    switch (pc[2])
	    {
	    case 0: 		/* No int's */
		fprintf(stderr, "()\r\n");
		break;
	    case 4: 		/* One int */
		fprintf(stderr, "(0x%lX)\r\n", bp[2]);
		break;
	    case 8: 		/* Two int's */
		fprintf(stderr, "(0x%lX, 0x%lX)\r\n", bp[2], bp[3]);
		break;
	    case 12: 		/* Three int's */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4]);
		break;
	    case 16: 		/* Four int's */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4], bp[5]);
		break;
	    default:		/* Don't know */
		fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
			bp[2], bp[3], bp[4], bp[5], bp[6]);
		break;
	    }
	else			/* Don't know */
	    fprintf(stderr, "(0x%lX, 0x%lX, 0x%lX, 0x%lX, 0x%lX)\r\n",
		    bp[2], bp[3], bp[4], bp[5], bp[6]);

	if (bp[0] == 0)
	    break;

	/* next frame point must be on the stack */
	if (bp[0] <= (long)bp)
	{
	    fprintf(stderr, "bad frame pointer 0x%lX\r\n", bp[0]);
	    break;
	}

	bp = (long *)bp[0];
    }

    inbacktrace = FALSE;
}

int
stacksnapshot(int size, uLong *stack, int skip)
{
    unsigned *bp = &((unsigned *)&size)[-2];
    int count = 0;

    //while (count < size && bp[1] > 1)
    while (count < size && bp != NULL && (unsigned long)bp[1] >= start_addr)
    {
	if (!skip)
	    stack[count++] = (uLong)bp[1];
	else
	    skip--;

	if (bp[0] == 0)
	    break;

	if (bp[0] <= (unsigned)bp)
	{
	    fprintf(stderr, "bad frame pointer 0x%X\r\n", bp[0]);
	    break;
	}

	bp = (unsigned *)(bp[0]);
    }

    for (int i = count; i < size; i++)
    	stack[i] = 0UL;

    return count;
}

#ifdef EBUG
static void
func(int arg)
{
    backtrace(TRUE);
    backtrace();
    printf("\nSymbols:\n");
    dumpsymbols(stdout);
}

main(int argc, const char *argv[])
{
    programfilename(argv[0]);
    func(32);
    return 0;
}
#endif
