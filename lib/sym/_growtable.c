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

/* Copyright (c) 1991 by Parag Patel.  All Rights Reserved. */
/* $Header: /u/cgt/cvs/lib/sym/_growtable.c,v 1.2 1998/02/14 05:17:35 parag Exp $ */

#include "defs.h"

/*
 * The `prime' array is a sequence of primes such that prime[0] = 11 and
 * for all 0 < i < N, prime[i] is the smallest prime number greater
 * than 2 * prime[i - 1].  Requires an N > 1.
 *
 * The new hashsize is the smallest prime[i] which is greater than twice
 * the current hashsize.  If there is no such prime[i], the new hashsize
 * is prime[N - 1].
 */

#define N (sizeof prime)

static int prime[] =
{
	11,
	23,
	47,
	97,
	197,
	397,
	797,
	1597,
	3203,
	6421,
	12853,
	25717,
	51437,
	102877,
	205759,
	411527,
	823117,
	1646237,
	3292489,
	6584983,
	13169977,
	26339969,
	52679969,
	105359939,
	210719881,
	421439783,
	842879579
};

static int
bump(hs)
register int hs;
{
	register int i;

	if (hs < prime[N - 2])
		for (i = 0; i < N; i++)
			if (prime[i] > 2 * hs)
				return prime[i];

	return prime[N - 1];
}


/* FUNCTION */
_growtable(symtab)
register SYMTAB    *symtab;
/*
 *	Grow symbol table by size of original table
 */
{
    register int    i;
    register int    ncopied = 0;
    register HASHNODE  *ptr;
    register HASHNODE **head;
    register SYMBOL    *sym;
    register HASHNODE  *next;
    int     newsize;
    SYMTAB *newtab;

    /* ORIGINALLY: newsize = symtab->arraysize + symtab->hashsize; */
    newsize = bump(symtab->arraysize + symtab->hashsize);

    /* allocate new table */
    newtab = symtaballoc(newsize);
    /* set up new functions */
    symfns(newtab, symtab->symcmp,
	    symtab->symalloc,
	    symtab->freesym,
	    symtab->hashsym);

    /* traverse old table adding symbols to new one and */
    /* freeing hash nodes from old one */
    head = &symtab->hhead[0];
    for (i = 0; i < symtab->arraysize; i++)
    {
	if (*head != NULL)
	{
	    for (ptr = *head; ptr != NULL; ptr = next)
	    {
		ncopied++;
		linksym(newtab, ptr->sym);
		next = ptr->next;
		_freehashnode(ptr);
	    }
	}
	head++;
    }
    if (ncopied != nsyms(symtab))
    {
	fprintf(stderr, "Something's wrong in _grow %d != %d\n",
		ncopied, nsyms(symtab));
    }
    /* free old table's bucket array */
    free((char *)symtab->hhead);

    /* copy new table header over old one */
    symtab->hashsize = symtab->arraysize = newsize;
    symtab->currentsymindex = 0;
    symtab->currenthashnode = NULL;
    symtab->threshold = newtab->threshold;
    symtab->hhead = newtab->hhead;
    symtab->symtest = -1;

    /* free new table's header */
    free((char *)newtab);
}
