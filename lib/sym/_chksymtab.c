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
/* $Header: /u/cgt/cvs/lib/sym/_chksymtab.c,v 1.1 1998/02/14 05:16:48 parag Exp $ */

#include "defs.h"

/*
 * Does a series  of  integrity  checks on the  symbol  table.  If any
 * problems are found, a dump of the internal lists is performed.
 */

#define TRUE 1
#define FALSE 0

int
_chksymtab(symtab)
register SYMTAB *symtab;
{
	register ENTRY *entry;
	register ENTRY *prev;
	register int    num = 0;

	entry = &symtab->head;	/* point to start of table */

	do
	{
		prev = entry;
		entry = entry->next;
		if (entry->prev != prev)
		{
			fprintf(stderr, "_chksymtab: corrupt linked list:\n");
			dumplink(prev);
			dumplink(entry);
			dumplink(entry->next);
			return FALSE;
		}
		num++;
	} while (entry->symbol != NULL);

	num--;
	if (symtab->nsymbols != num)
	{
		fprintf(stderr, "_chksymtab: bad symbol count:\n");
		fprintf(stderr, "nsyms() = %d, real count = %d\n",
			symtab->nsymbols, num);
		return FALSE;
	}

	return TRUE;
}

static
int
dumplink(entry)
ENTRY *entry;
{
	fprintf(stderr, "entry 0x%08X:\n", entry);
	fprintf(stderr, "\tprev\t0x%08X\n", entry->prev);
	fprintf(stderr, "\tnext\t0x%08X\n", entry->next);
	fprintf(stderr, "\tsymbol\t0x%08X\n", entry->symbol);
	fprintf(stderr, "\n");
}
