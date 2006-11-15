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
/* $Header: /u/cgt/cvs/lib/sym/freesymtab.c,v 1.2 1998/02/17 19:47:08 parag Exp $ */

#include "defs.h"
extern free     ();

/*
 *	Frees the space used by all of the current  entries  in
 *	symtab   (using   the   current  freesym  function  for
 *	symtab).  Using symtab in any commands after  executing
 *	a  freesymtab(symtab)  will probably eventually cause a
 *	core dump [i.e., don't do it].  
 */

/* FORWARD */
static hashfreesymtab();
static funnyfree();

void
freesymtab(symtab)
SYMTAB *symtab;
{
    register int  i;
    register int  nsymbols;
    register SYMBOL   **sp;
    register void (*freefn)();

    if (symtab->hashsize > 0)
    {
	hashfreesymtab(symtab);
	return;
    }
    freefn = symtab->freesym;
    sp = symtab->head;
    nsymbols = symtab->nsymbols;

    for (i = 0; i < nsymbols; i++)
    {
	if (*sp != NULL)
	{
	    (*freefn)(*sp++);
	}
    }

    free((char *)symtab->head);
    free((char *)symtab);
}

static
hashfreesymtab(symtab)
SYMTAB *symtab;
/*
 *	Free a hashed symbol table
 */
{
    register int    i;
    register HASHNODE **stop,
                      **aptr;

    aptr = &symtab->hhead[0];
    stop = aptr + symtab->arraysize;

    for (; aptr < stop; aptr++)
    {
	if (*aptr == NULL)
	    continue;

	funnyfree(symtab, *aptr);
    }

    free((char *)symtab->hhead);
    free((char *)symtab);
}

static
funnyfree(symtab, ptr)
SYMTAB *symtab;
HASHNODE   *ptr;
/*
 *	A recursive routine to walk a link list and free its contents starting
 *	at the end (so no pointers are lost!)
 */
{
    if (ptr->next != NULL)
	funnyfree(symtab, ptr->next);

    (*symtab->freesym)(ptr->sym);
    _freehashnode(ptr);
}
