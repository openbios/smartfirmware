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
/* $Header: /u/cgt/cvs/lib/sym/findsym.c,v 1.2 1998/02/17 19:47:08 parag Exp $ */

#include "defs.h"

/*
 *	Finds an entry for sym in symtab.  If found, returns  a
 *	pointer to the entry, else returns NULL.  Note that sym 
 *	is  a  pointer  to  a  symbol  structure,  not simply a
 *	string.  If the passed sym is NULL, findsym() will  set
 *	the  current  position  [see  nextsym()]  to before the
 *	first entry in the table, and NULL will be returned.  
 *	
 *	EXAMPLE:
 *		SYMBOL tempsym, *symloc;
 *
 *		tempsym.string = "frank";
 *		symloc = findsym(nametable, &tempsym);
 *		if (NULL == symloc)
 *			printf("not found.\n");
 *	
 *	In addition, findsym() also sets the following  variables
 *	in the symtab structure for the table: 
 *	
 *		currentsymindex: index of current symbol
 *	
 *		symtest:   =0 symbol found
 *				   >0 symbol goes after ptr
 *				   <0 symbol goes before ptr
 *
 *		WHEN HASHING currenthashnode is also set.
 *
 *	When hashing, symtest = 0 or != 0 is valid.
 *	
 *	These variables are used by linksym() to determine the
 *	position at which to link a symbol.  linksym() always
 *	calls findsym().  This routine is also used by unlinksym().
 *
 *	findsym() should *ONLY* exit with currentsymindex = -1 when
 *	a findsym(symtab,NULL) has been executed since linksym()
 *	considers currentsymindex to be valid in all other cases.
 */

/* FORWARD */
static SYMBOL  *hashfindsym();

SYMBOL *
findsym(symtab, sym)
register SYMTAB    *symtab;
SYMBOL *sym;
{
    register int    test = -1;
    register SYMBOL   **first,
                      **last,
                      **curr;

    if (symtab == NULL)
    {
	fprintf(stderr, "findsym:  NULL symbol table pointer\n");
	fflush(stderr);
	abort();
    }

    if (sym == NULL)
    {
	symtab->currentsymindex = -1;
	symtab->currenthashnode = NULL;
	return NULL;
    }

    if (symtab->nsymbols == 0 && symtab->hashsize == 0)
    {
	symtab->currentsymindex = 0;
	symtab->symtest = -1;
	return NULL;
    }

    if (symtab->hashsize > 0)
    {
	return (hashfindsym(symtab, sym));
    }

    curr = first = symtab->head;
    last = &symtab->head[symtab->nsymbols - 1];

    /* while not found and table not empty */
    while (first <= last)
    {
	/* There is some tricky stuff going on here!!!! */
	curr = &first[(last - first) >> 1];

	test = (*symtab->symcmp)(sym, *curr);
	if (test == 0)
	{
	    symtab->symtest = test;
	    symtab->currentsymindex = curr - symtab->head;
	    return *curr;

	}
	else if (test < 0)
	{
	    last = curr - 1;
	}
	else
	{
	    first = curr + 1;
	}
    }
    symtab->symtest = test;
    symtab->currentsymindex = curr - symtab->head;
    return NULL;
}

static
SYMBOL *
hashfindsym(symtab, sym)
register SYMTAB    *symtab;
SYMBOL *sym;
/*
 *	If symbol found, set symtest = 0, currenthashnode,
 *	currentsymindex.  If not found, set symtest != 0,
 *	currenthashnode = NULL, currentsymindex = hashvalue
 */
{
    register int   hashindex;
    register HASHNODE  *ptr;

    symtab->currentsymindex = hashindex =
	(*symtab->hashsym)(sym, symtab->arraysize);

    for (ptr = symtab->hhead[hashindex];
	    ptr != NULL && (*symtab->symcmp)(ptr->sym, sym) != 0;
	    ptr = ptr->next)
    {
    }

    /* found it? */
    if (ptr != NULL)
    {
	symtab->symtest = 0;
	symtab->currenthashnode = ptr;
	return (ptr->sym);
    }

    /* not found */
    symtab->symtest = -1;
    symtab->currenthashnode = NULL;
    return (NULL);
}
