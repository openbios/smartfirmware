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

/* (c) Copyright 1997 by CodeGen, Inc.  All Rights Reserved. */

/* Generic program loader to support plug-in binary image loaders. */

#include "defs.h"
#include "exe.h"


Bool
exec_is_exec(Environ *e)
{
	Exec_entry **ex = g_exec_list;

	for (; *ex; ex++)
	{
		e->loadentry = *ex;

		DPRINTF(("checking exec type %s\n", e->loadentry->name));

		if (e->loadentry->is_exec &&
				e->loadentry->is_exec(e, (uByte*)e->load, e->loadlen))
			return TRUE;
	}

	e->loadentry = NULL;
	DPRINTF(("no matching exec type found\n"));
	return FALSE;
}

Retcode
exec_load(Environ *e)
{
	if (e->loadentry == NULL || e->loadentry->load == NULL)
		return E_NO_IMAGE;

	return e->loadentry->load(e, (uByte*)e->load, e->loadlen, &e->entrypoint);
}

Bool
exec_length(Environ *e, uInt *len)
{
	if (!exec_is_exec(e))
		return FALSE;

	if (e->loadentry == NULL || e->loadentry->length == NULL)
		return FALSE;

	*len = e->loadentry->length(e, (uByte*)e->load, e->loadlen);
	return TRUE;
}

Sym_table *
exec_load_symbols(Environ *e)
{
	if (e->loadentry == NULL || e->loadentry->symbols == NULL)
		return NULL;

	return e->loadentry->symbols(e, (uByte*)e->load, e->loadlen);
}

void
exec_free_symbols(Environ *e, Sym_table *tab)
{
	int i;

	if (tab == NULL || tab->list == NULL)
		return;

	for (i = 0; i < tab->num; i++)
		free(tab->list[i].name);

	free(tab->list);
	free(tab);
}

Sym_ent *
exec_sym2addr(Environ *e, Sym_table *tab, Byte *sym, Int slen)
{
    int i;
	Byte name[STR_SIZE];

	if (tab == NULL)
		return NULL;

	setstrlen(&sym, &slen);
	memcpy(name, sym, slen);
	name[slen] = '\0';

	for (i = 0; i < tab->num; i++)
		if (tab->list[i].type &&
				slen == *(uByte*)tab->list[i].name &&
				memcmp(name, tab->list[i].name + 1, slen) == 0)
			return &tab->list[i];

	return NULL;
}

Sym_ent *
exec_addr2sym(Environ *e, Sym_table *tab, uLong addr)
{
	int i;
	Sym_ent *ret = NULL;

	if (tab == NULL)
		return NULL;

	DPRINTF(("exec_addr2sym: tab=%p num=%d\n", tab, tab->num));

	for (i = 0; i < tab->num; i++)
		if (tab->list[i].type &&
				addr >= tab->list[i].addr &&
				(ret == NULL || ret->addr < tab->list[i].addr))
		{
			DPRINTF(("exec_addr2sym: %d: ret=%P(%lx) tab=%P(%lx) addr=%lx\n",
					i, ret ? ret->name : "", ret ? ret->addr : 0,
					tab->list[i].name, tab->list[i].addr,
					addr));
			ret = &tab->list[i];
		}

	return ret;
}
