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

// Copyright (c) 1992 by TJ Merritt and Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/gcmalloc/gcsnap.cc,v 1.7 1999/09/29 04:47:47 parag Exp $

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "stdlibx.h"
#include "backtrace.h"
#include "hashtable.h"
#include "gcmalloc.h"
#include "internals.h"

static unsigned long
stackhash(Stack_elt const &e)
{
    unsigned long hash = 0;
    uLong const *ptr = e.stack;

    for (int i = 0; i < STACK_SIZE; i++)
        hash += *ptr++;

    hash += (uLong)e.type;
    hash += (uLong)e.file;
    hash += (uLong)e.line;

    return hash >> 4;
}

implement_hashtable(Stack_pool, Stack_iter, Stack_elt, Stack_elt, stackhash)
implement_hashtableiter(Stack_pool, Stack_iter, Stack_elt, Stack_elt, stackhash)

Stack_pool *gcstackpool = NULL;

// must only be called from gcblockalloc or free for stacksnapshot() to work
//
void *
gcsnapstack(void *type, long mem, const char *file, int line)
{
    gc.inalloc = TRUE;

    Stack_elt ent;
    ent.type = type;
    ent.file = file;
    ent.line = line;

    // skip = 1 for us + 1 for our caller = 2
    //int len = stacksnapshot(STACK_SIZE, ent.stack, 2);

    if (gcstackpool == NULL)
	gcstackpool = new Stack_pool;

    Stack_elt &elt = (*gcstackpool)[ent];
    elt.numcalls++;
    elt.totmem += mem;
    gc.inalloc = FALSE;
    return &elt;
}

void
gcdumpsnapstack(FILE *fp)
{
    if (gcstackpool == NULL)
	return;

    if (fp == NULL)
	fp = stderr;

    fprintf(fp, "\ngcstackpool...\n");

    gc.inalloc = TRUE;

    Stack_pool dumpstk;
    Stack_iter si;
    boolean dodumpstk = TRUE;
    int i = 1;

    for (si = *gcstackpool; si; si++, i++)
    {
	Stack_elt e = si();
	const char *name;
	unsigned long offset;

	fprintf(fp, "[%d] type=%p file=%s line=%d numcalls=%ld totmem=%ld\n",
		i, e.type, e.file ? e.file : "<null>", e.line, e.numcalls,
		e.totmem);
	e.numcalls = 0;
	e.totmem = 0;

	for (int j = 0; j < STACK_SIZE && e.stack[j]; j++)
	    if (symbollookup(e.stack[j], name, offset))
	    {
		if (offset)
		    fprintf(fp, "    [%d] %s+0x%lX\n", j+1, name, offset);
		else
		    fprintf(fp, "    [%d] %s\n", j+1, name);
		e.stack[j] = (uLong)name;
	    }
	    else
	    {
		fprintf(fp, "    [%d] 0x%lX\n", j+1, e.stack[j]);
		dodumpstk = FALSE;
	    }

	dumpstk[e].numcalls += si().numcalls;
	dumpstk[e].totmem += si().totmem;
    }

    gc.inalloc = FALSE;

    if (!dodumpstk)
	return;

    fprintf(fp, "\ndumpstk...\n");

    i = 1;
    for (si = dumpstk; si; si++, i++)
    {
	Stack_elt &e = si();
	fprintf(fp, "[%d] type=%p file=%s line=%d numcalls=%ld totmem=%ld\n",
		i, e.type, e.file ? e.file : "<null>", e.line, e.numcalls,
		e.totmem);

	for (int j = 0; j < STACK_SIZE && e.stack[j]; j++)
	    fprintf(fp, "    [%d] %s\n", j+1, (char*)e.stack[j]);
    }
}
