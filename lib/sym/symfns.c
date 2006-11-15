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
/* $Header: /u/cgt/cvs/lib/sym/symfns.c,v 1.1 1998/02/14 05:16:53 parag Exp $ */

#include "defs.h"

/*
 *	The sym library has default algorithms  for  comparing,
 *	allocating, freeing, and hashing symbols, assuming that 
 *	the symbols consist of a structure whose first field is 
 *	a   pointer  to  a  null-terminated  string.   If  your
 *	application requires more complex  comparisons  than  a
 *	simple  string  compare,  or  if  you  symbol structure
 *	varies from the default, symfns() allows you to specify 
 *	alternate routines to  perform  these  operations.   To
 *	install  an  alternate routine for a particular symtab,
 *	pass  a  pointer  to  the  alternate  routine  in   the
 *	approprites  field(s).   If you wish to continue to use
 *	the  default  version  of  any  routine,  pass  a  NULL
 *	pointer.   Specifications  for  the  routines are given
 *	below.  Note that it is not neccessary to call symfns() 
 *	or to write any alternate routines  if  you  are  happy
 *	with all of the defaults.  
 *	
 *	EXAMPLE:
 *		To install your own versions of symcmp() and hashsym():
 *		symfns(symtab, mysymcmp, NULL, NULL, myhashsym);
 */

void
symfns(symtab, newsymcmp, newsymalloc, newfreesym, newhashsym)
SYMTAB  *symtab;
int      (*newsymcmp)();
SYMBOL  *(*newsymalloc)();
void     (*newfreesym)();
unsigned (*newhashsym)();
{
    if (newsymcmp == NULL)
	symtab->symcmp = symcmp;
    else
	symtab->symcmp = newsymcmp;

    if (newsymalloc == NULL)
	symtab->symalloc = symalloc;
    else
	symtab->symalloc = newsymalloc;

    if (newfreesym == NULL)
	symtab->freesym = freesym;
    else
	symtab->freesym = newfreesym;

    if (newhashsym == NULL)
	symtab->hashsym = hashsym;
    else
	symtab->hashsym = newhashsym;
}
