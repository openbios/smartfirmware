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
/* $Header: /u/cgt/cvs/lib/sym/symalloc.c,v 1.1 1998/02/14 05:16:52 parag Exp $ */

#include "defs.h"

extern char    *calloc();
extern char    *malloc();
extern char    *strcpy();
extern int  abs();

/*
 *	symalloc() allocates space for and makes a copy of  the
 *	passed  SYMBOL  and its contents.  If sym is NULL then a
 *	SYMBOL  is  allocated  and  all  of  its   fields   are
 *	initialized  to  zero.   A  pointer  to the new copy is
 *	returned.  If symsize is negative,  abs(symsize)  space
 *	is allocated (including the pointer to string), and all 
 *	of the following bytes are set to 0 instead of the data 
 *	in sym.  If abs(symsize) < sizeof(char *) then a NULL
 *	pointer is returned.
 */

SYMBOL *
symalloc(sym, symsize)
register SYMBOL    *sym;
register size_t    symsize;
{
    register SYMBOL    *oldsym;
    register SYMBOL    *newsym;
    register char  *s;
    register char  *t;
    register int    asize;
    register int    i;

    oldsym = sym;
    asize = abs(symsize);

    if (asize < sizeof (char *))
	return NULL;
    newsym = (SYMBOL *)calloc(1, asize);
    if (sym != NULL)
    {
	t = oldsym->string;
	s = malloc(strlen(t) + 1);
	newsym->string = strcpy(s, t);

	if (symsize > 0)
	    for (i = sizeof (char *); i < asize; i++)
		((char *)newsym)[i] = ((char *)oldsym)[i];
    }

    return newsym;
}
