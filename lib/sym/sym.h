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
/* $Header: /u/cgt/cvs/lib/sym/sym.h,v 1.2 1998/02/14 05:17:36 parag Exp $ */

#ifndef _SYM_H_
#define _SYM_H_

#include <stddef.h>

/*****
<< the default symbol struct should look like this >>
typedef struct s_symbol
{
    char   *string;
    << other fixed-length info may follow >>
}   SYMBOL;
*****/

/* define SYMTAB as void just to have something to point to */
#ifndef __SYMTAB_
#define __SYMTAB_
typedef void    SYMTAB;
#endif

#ifdef __STDC__
extern SYMBOL*  addsym(SYMTAB* symtab, SYMBOL* sym, size_t symsize);
extern SYMBOL*  findsym(SYMTAB* symtab, SYMBOL* sym);
extern void     freesym(SYMBOL* sym);
extern void     freesymtab(SYMTAB* symtab);
extern unsigned hashsym(SYMBOL* sym, unsigned hashsize);
extern SYMBOL*  linksym(SYMTAB* symtab, SYMBOL *sym);
extern SYMBOL*  nextsym(SYMTAB* symtab);
extern unsigned nsyms(SYMTAB* symtab);
extern int      rmsym(SYMTAB* symtab, SYMBOL* sym);
extern SYMBOL*  symalloc(SYMBOL* sym, size_t symsize);
extern int      symcmp(SYMBOL* s1, SYMBOL* s2);
extern void symfns(SYMTAB* symtab,
		int      (*newsymcmp)(SYMBOL* s1, SYMBOL* s2),
		SYMBOL*  (*newsymalloc)(SYMBOL* sym, size_t size),
		void     (*newfreesym)(SYMBOL* sym),
		unsigned (*newhashsym)(SYMBOL *sym, unsigned hashsize));
extern SYMTAB* symtaballoc(int hsize);
extern SYMBOL* unlinksym(SYMTAB* symtab, SYMBOL* sym);

#else
extern	SYMBOL	 *addsym();
extern	SYMBOL	 *findsym();
extern	void	 freesym();
extern	void	 freesymtab();
extern	unsigned hashsym();
extern	SYMBOL	 *linksym();
extern	SYMBOL	 *nextsym();
extern	unsigned nsyms();
extern	int	 rmsym();
extern	SYMBOL	 *symalloc();
extern	int	 symcmp();
extern	void	 symfns();
extern	SYMTAB	 *symtaballoc();
extern	SYMBOL	 *unlinksym();
#endif

#endif /* _SYM_H_ */
