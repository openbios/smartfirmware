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
/* $Header: /u/cgt/cvs/lib/sym/hashsym.c,v 1.2 1998/02/14 05:17:36 parag Exp $ */

#include "defs.h"
/*#include <values.h>*/
#define BITS(type) (sizeof (type) * 8)

/* FUNCTION */
unsigned
hashsym(sym, hashsize)
SYMBOL    *sym;
unsigned  hashsize;
/*
 *	Returns a hash value for sym in the range 0 .. (hashsize - 1).
 *	This is from the "dragon" Compilers book.
 */
{
	register unsigned h = 0;
	register int g;
	register char *p = sym->string;

	if (p != NULL)
		while (*p != '\0')
		{
			h = (h << 4) + (unsigned)*p++;
			if (g = h & (0xF << BITS(unsigned)-4))
			{
				h ^= g >> BITS(unsigned)-4;
				h ^= g;
			}
		}
	g = h;
	if (g < 0)
	    g = -g;
	return g % hashsize;
}

#ifdef COMMENTED_OUT
    /* original code */
    register int    code;
    register int    c;
    register char  *s;

    code = 0;
    s = sym->string;
    while ((c = *s++) != '\0')
    {
	code <<= 2;
	code ^= c;
    }
    /* make sure code is positive */
    if (code < 0)
	code = -code;
    return (code % hashsize);
#endif /* COMMENTED_OUT */
