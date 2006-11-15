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
/* $Header: /u/cgt/cvs/lib/sym/unlinksym.c,v 1.2 1998/02/17 19:47:09 parag Exp $ */

#include "defs.h"

/*
 *	Removes the entry for sym from symtab.  Returns
 *  a pointer to the symbol that was removed if a matching
 *  entry is found, or
 *	NULL  if  sym  was  not  in  symtab.
 *	The space used
 *	by sym is not deallocated.  
 */

/* FORWARD */
static SYMBOL  *hashunlinksym();

SYMBOL *
unlinksym(symtab, sym)
register SYMTAB    *symtab;
register SYMBOL    *sym;
{
    register SYMBOL   **spp;
    SYMBOL **stopspp;
    SYMBOL **deletehere;

    if (symtab->nsymbols == 0)
	return NULL;

    if (symtab->hashsize > 0)
    {
	return (hashunlinksym(symtab, sym));
    }

    /* see if by chance we're there already:  */
    if (symtab->currentsymindex >= 0)
	spp = &symtab->head[symtab->currentsymindex];
    else
	spp = NULL;

    if (spp==NULL || sym != *spp)
    {
	/* no, find it:  */
	sym = findsym(symtab, sym);
	if (sym == NULL)
	    return NULL;
    }
    deletehere = &symtab->head[symtab->currentsymindex];

    /* move list around */
    stopspp = &symtab->head[symtab->nsymbols - 1];
    for (spp = deletehere; spp < stopspp; spp++)
    {
	*spp = *(spp + 1);
    }

    symtab->nsymbols--;
    symtab->symtest = -1;

    return sym;
}

static
SYMBOL *
hashunlinksym(symtab, sym)
register SYMTAB    *symtab;
register SYMBOL    *sym;
/*
 *	Use findsym() to find the symbol.  Then use current* to
 *	read through the list of symbols on the current hash index
 *	and delete one of them.
 */
{
    register SYMBOL    *tempsym;
    register HASHNODE  *ptr,
                       *oldptr;

    /* see if by chance we're there already:  */
    if ((symtab->currenthashnode == NULL) ||
	    (tempsym = symtab->currenthashnode->sym) != sym)
    {
	/* NOPE - do it the hard way */
	if ((tempsym = findsym(symtab, sym)) == NULL)
	{
	    return (NULL);
	}
    }
    ptr = symtab->hhead[symtab->currentsymindex];
    if (ptr->sym == tempsym)
    {
	/* at head of list */
	symtab->hhead[symtab->currentsymindex] = ptr->next;
	_freehashnode(ptr);
    }
    else
    {
	oldptr = ptr;
	for (ptr = ptr->next; ptr != NULL; ptr = ptr->next)
	{
	    if (ptr->sym == tempsym)
	    {
		oldptr->next = ptr->next;
		_freehashnode(ptr);
		break;
	    }
	    oldptr = ptr;
	}
    }

    symtab->nsymbols--;
    symtab->currenthashnode = NULL;
    symtab->symtest = -1;

    return tempsym;
}
