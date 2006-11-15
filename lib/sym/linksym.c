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
/* $Header: /u/cgt/cvs/lib/sym/linksym.c,v 1.2 1998/02/17 19:47:08 parag Exp $ */

#include "defs.h"

/*
 *	Attempts to add the symbol structure sym to the  symbol
 *	table  described by symtab.  If an entry for the symbol
 *	already  exists,  no  action  is  taken  and  NULL   is
 *	returned.   If  the symbol is not already in the table,
 *	sym is entered into symtab and a  pointer  to  the  new
 *	entry is returned.  
 *	
 *	EXAMPLE:
 *		char *name;
 *		name = "fred";
 *		if (NULL == linksym(nametable, (SYMBOL *)(&name)))
 *			printf("already exists.\n");
 *	Note that the "&" is necessary in front of name,  since
 *	linksym  expects  a  pointer  to  a  SYMBOL,  not just a
 *	pointer to a string.  
 */


/* FORWARD */
static SYMBOL  *hashlinksym();

SYMBOL *
linksym(symtab, sym)
register SYMTAB    *symtab;
SYMBOL *sym;
{
    register SYMBOL   **spp,
                      **spp1;
    SYMBOL **goeshere;
    register int    count;

    if (symtab == NULL)
    {
	fprintf(stderr, "linksym:  NULL symbol table pointer\n");
	fflush(stderr);
	abort();
    }

    if (sym == NULL)
    {
	fprintf(stderr, "linksym:  illegal NULL symbol\n");
	fflush(stderr);
	abort();
    }

    if (findsym(symtab, sym) != NULL)	/* symbol already exists */
    {
	return NULL;
    }
    else
    {
	if (symtab->hashsize > 0)
	{
	    return (hashlinksym(symtab, sym));
	}
	if (symtab->nsymbols == symtab->arraysize)
	{
	    /* need to add new space in array */
	    symtab->arraysize += TABINCREMENT;
	    symtab->head = (SYMBOL **)realloc(symtab->head,
		    sizeof (SYMBOL *) * symtab->arraysize);
	}

	goeshere = &symtab->head[symtab->currentsymindex];
	if (symtab->symtest < 0)/*  insert before found symbol */
	{
	}
	else			/* insert  after found symbol */
	{
	    goeshere++;
	}

	/* move entries around */

	spp = &symtab->head[symtab->nsymbols];
	spp1 = spp - 1;
	count = spp - goeshere;
	while (count--)
	{
	    *spp-- = *spp1--;
	}

	*goeshere = sym;
    }

    symtab->nsymbols++;

    /* simulate a findsym(symtab, newsym):  */
    symtab->currentsymindex = goeshere - symtab->head;
    symtab->symtest = 0;

    return sym;
}

static
SYMBOL *
hashlinksym(symtab, sym)
register SYMTAB    *symtab;
SYMBOL *sym;
/*
 *	We know there are symbols in the table at this point AND
 *	that the symbol in question is NOT in the table so link it
 *	in.
 */
{
    register HASHNODE  *ptr,
                       *temp;

    ptr = symtab->hhead[symtab->currentsymindex];

    if (ptr == NULL)
    {
	/* first symbol in this list */
	ptr = _allochashnode();
	ptr->sym = sym;
	symtab->hhead[symtab->currentsymindex] =
	    symtab->currenthashnode = ptr;
    }
    else
    {
	/* Link in at head of list */
	temp = _allochashnode();
	temp->sym = sym;
	temp->next = ptr;
	symtab->hhead[symtab->currentsymindex] = temp;
	symtab->currenthashnode = temp;
    }
    symtab->nsymbols++;
    symtab->symtest = 0;

    if ((symtab->nsymbols) >= symtab->threshold)
    {
	_growtable(symtab);
	findsym(symtab, sym);
    }
    return (sym);
}
