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
/* $Header: /u/cgt/cvs/lib/sym/nextsym.c,v 1.2 1998/02/17 19:47:09 parag Exp $ */

#include "defs.h"

/*
 *	Advances the current position in symtab [as  set  by  a
 *	previous  linksym() or findsym()] to the next entry, and
 *	returns a pointer this entry.   NULL  is  returned  for
 *	end-of-list.   Calling nextsym() after end-of-list will
 *	return a pointer to the first entry of the  list.   The
 *	incantation  "findsym(symtab, NULL)" may be used to set
 *	the current position to end-of-list in preparation  for
 *	a symbol table dump.  
 */

/* FORWARD */
static SYMBOL  *hashnextsym();

SYMBOL *
nextsym(symtab)
register SYMTAB    *symtab;
{
    if (symtab->hashsize > 0)
    {
	return (hashnextsym(symtab));
    }

    symtab->currentsymindex++;
    symtab->symtest = -1;
    if (symtab->currentsymindex >= symtab->nsymbols)
    {
	symtab->currentsymindex = -1;
	return (NULL);
    }

    return (symtab->head[symtab->currentsymindex]);
}

static
SYMBOL *
hashnextsym(symtab)
register SYMTAB    *symtab;
{
    register HASHNODE  *ptr;

    if ((symtab->currenthashnode == NULL) || (symtab->currenthashnode->next == NULL))
    {
	for (;;)
	{
	    if (++symtab->currentsymindex >= symtab->arraysize)
	    {
		symtab->currentsymindex = -1;
		symtab->symtest = -1;
		symtab->currenthashnode = NULL;
		return (NULL);
	    }
	    if ((ptr = symtab->hhead[symtab->currentsymindex]) != NULL)
	    {
		symtab->currenthashnode = ptr;
		symtab->symtest = 0;
		break;
	    }
	}
    }
    else
    {
	symtab->currenthashnode = symtab->currenthashnode->next;
    }
    return (symtab->currenthashnode->sym);
}
