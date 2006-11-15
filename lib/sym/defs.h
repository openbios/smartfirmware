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
/* $Header: /u/cgt/cvs/lib/sym/defs.h,v 1.1 1998/02/14 05:16:49 parag Exp $ */

#include <stdio.h>
#define NEW(x) ((x *)malloc(sizeof (x)))

extern char    *malloc();

/*
 *	This is the default symbol structure.  The user's structure
 *	may be different, as long as he supplies appropriate versions
 *	of symcmp(), symalloc(), and freesym().
 */

typedef struct s_symbol
{
    char   *string;
    /* other fixed-length info may follow */
}   SYMBOL;

typedef struct s_hashnode
{
    SYMBOL *sym;
    struct s_hashnode  *next;
}   HASHNODE;

/* A special routine is used here for efficiency */
extern HASHNODE    *_allochashnode();

struct s_symtab
{
    int      (*symcmp)();	/* SYMBOL comparison fn */
    SYMBOL  *(*symalloc)();	/* SYMBOL allocation fn */
    void     (*freesym)();	/* SYMBOL de-allocation fn */
    unsigned (*hashsym)();	/* SYMBOL hashing fn */
    int      hashsize;		/* number of hashing buckets */
				/* originally requested     */
    int   nsymbols;		/* number of symbols in table */
    int   arraysize;		/* number of symbols possible in array */
				/* or real number of hashing buckets   */

    /* One of the following is used - never both */
    SYMBOL **head;		/* array */
    HASHNODE **hhead;		/* hashing array */

    /* The following are returned by findsym(): */
    int    currentsymindex;	/* current symbol array index */
    HASHNODE *currenthashnode;	/* ptr to current hashing node */
    int     symtest;		/* comparison to entry *ptr */

    int   threshold;		/* # symbols at which to grow hash table */
};
typedef struct s_symtab     SYMTAB;

#define TABINCREMENT 128	/* size of symbol table growth */
#define __SYMTAB_		/* type SYMTAB is already defined */
#include "sym.h"		/* now include the extern declarations */
