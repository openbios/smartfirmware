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

/* Copyright (c) 1992 by Thomas J. Merritt and Parag Patel.
   All Rights Reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stdlibx.h"
#include "stkback.h"
#include "hash.h"
#include "gcmalloc.h"
#include "gcdefs.h"

static gcstackelt *
init_gcstackelt(gcstackelt *self)
{
	memset(self, '\0', sizeof *self);
	return self;
}

static gcstackelt *
initkey_gcstackelt(gcstackelt *self, gcstackelt *key)
{
	*self = *key;
	return self;
}

static gcstackelt *
copy_gcstackelt(gcstackelt *self, gcstackelt *rhs)
{
	*self = *rhs;
	return self;
}

static gcstackelt *
destroy_gcstackelt(gcstackelt *self)
{
	return self;
}

static boolean
compare_gcstackelt(gcstackelt *self, gcstackelt *e)
{
	return self->file == e->file && self->line == e->line &&
		self->type == e->type &&
		!memcmp(self->stack, e->stack, sizeof self->stack);
}

static unsigned long
hash_gcstackelt(gcstackelt *self)
{
	unsigned long hash = 0;
	uLong *ptr = self->stack;
	int i;

	for (i = 0; i < STACK_SIZE; i++)
		hash += *ptr++;

	hash += (uLong)self->type;
	hash += (uLong)self->file;
	hash += (uLong)self->line;
	return hash >> 4;
}

implement_hashtable(gcstackpool, gcstackiter, gcstackelt, gcstackelt*);

/* must only be called from gcblockalloc or free for stacksnapshot() to work */
void *
gcsnapstack(void *type, long mem, const char *file, int line)
{
	gcstackelt ent, *elt;
	int len;

	gc.inalloc = TRUE;
	ent.type = type;
	ent.file = file;
	ent.line = line;

	/* skip = 1 for us + 1 for our caller = 2 */
	len = stacksnapshot(STACK_SIZE, ent.stack, 2);

	if (gc.stackpool == NULL)
		gc.stackpool = new_gcstackpool();

	insert_gcstackpool(gc.stackpool, &ent);
	elt = cur_gcstackpool(gc.stackpool);
	elt->numcalls++;
	elt->totmem += mem;
	gc.inalloc = FALSE;
	return &elt;
}

void
gcdumpsnapstack(FILE *fp)
{
	gcstackpool *dumpstk;
	gcstackiter *si;
	gcstackelt *e;
	boolean dodumpstk = TRUE;
	int i = 1;

	if (gc.stackpool == NULL)
		return;

	if (fp == NULL)
		fp = stderr;

	fprintf(fp, "\ngc.stackpool...\n");
	gc.inalloc = TRUE;
	dumpstk = new_gcstackpool();
	si = new_gcstackiter();
	table_gcstackiter(si, gc.stackpool);

	for (e = advance_gcstackiter(si); e != NULL;
			i++, e = advance_gcstackiter(si))
	{
		const char *name;
		unsigned long offset;
		gcstackelt *e2, elt;
		int j;

		fprintf(fp, "[%d] type=0x%X file=%s line=%d numcalls=%d totmem=%ld\n",
				i, e->type, e->file ? e->file : "<null>", e->line, e->numcalls,
				e->totmem);

		for (j = 0; j < STACK_SIZE && e->stack[j]; j++)
			if (symbollookup(e->stack[j], &name, &offset))
			{
				if (offset)
					fprintf(fp, "    [%d] %s+0x%X\n", j + 1, name, offset);
				else
					fprintf(fp, "    [%d] %s\n", j + 1, name);
				e->stack[j] = (uLong)name;
			}
			else
			{
				fprintf(fp, "    [%d] 0x%X\n", j + 1, e->stack[j]);
				dodumpstk = FALSE;
			}

		elt = *e;
		elt.numcalls = 0;
		elt.totmem = 0;
		insert_gcstackpool(dumpstk, &elt);
		e2 = cur_gcstackpool(dumpstk);
		e2->numcalls += e->numcalls;
		e2->totmem += e->totmem;
	}

	gc.inalloc = FALSE;

	if (dodumpstk)
	{
		fprintf(fp, "\ndumpstk...\n");
		i = 1;
		table_gcstackiter(si, dumpstk);

		for (e = advance_gcstackiter(si); e != NULL;
				i++, e = advance_gcstackiter(si))
		{
			int j;

			fprintf(fp,
					"[%d] type=0x%X file=%s line=%d numcalls=%d totmem=%ld\n",
					i, e->type, e->file ? e->file : "<null>", e->line,
					e->numcalls, e->totmem);

			for (j = 0; j < STACK_SIZE && e->stack[j]; j++)
				fprintf(fp, "    [%d] %s\n", j + 1, (char *)e->stack[j]);
		}
	}

	delete_gcstackiter(si);
	delete_gcstackpool(dumpstk);
}
