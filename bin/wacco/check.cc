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

// Copyright (c) 1991,1993,1997 by Parag Patel.  All Rights Reserved.

#include "defs.h"

static void
dostruct(Symbol *sym)
{
	if (sym->rettype == NULL)
		return;

	if (dargstyle)
	{
		sym->mkstruct = (strpbrk(sym->rettype, ",;") != NULL);
		return;
	}

	char *buf = strdup(sym->rettype);

	for (char *s = buf; *s != '\0'; s++)
	{
		if (*s == ',')
		{
			sym->mkstruct = TRUE;
			*s = ';';
		}
	}

	sym->rettype = strget(buf);
	strfree(buf);
}


void
check()
{
	int i;
	Symbol *sym;
	Symnode *n;
	Bitset curr(numsymbols());
	Bitset set(numsymbols());
	Bitset t(numsymbols());

	if (!exportedname && !startsymbol->exportsym)
		startsymbol->exportsym = TRUE;

	for (i = 0; i < numnonterms(); i++)
	{
		sym = getnonterm(i);

		if (sym->type != NONTERMINAL)
			continue;

		dostruct(sym);

		if (sym->mkstruct && sym->exportsym)
			error("Non-simple type for exported non-terminal \"%s\"",
					sym->name);

		curr.clear();

		for (n = sym->node; n != NULL; n = n->orelse)
		{
			set.clear();
			boolean isfirst = TRUE;

			for (Symnode * s = n; s != NULL; s = s->next)
			{
				if (s->sym->type >= CODE)
					continue;

				if (isfirst)
					if (s->sym == sym)
						if (sym->realname != NULL && sym->name != sym->realname)
							error("Head recursion in grouped rules for \"%s\"",
									sym->realname);
						else
							error("Head recursion in rules for \"%s\"",
									sym->name);

				isfirst = FALSE;
				set |= *s->sym->first;

				if (!s->sym->first->has(EMPTY))
					break;
			}

			t = curr;
			t &= set;

			if (t.count() == 0 || (t.count() == 1 && t.has(EMPTY)))
				curr |= set;
			else if (sym->type == NONTERMINAL && sym->realname != NULL
					&& sym->name != sym->realname)
				error("Ambiguous [non-LL(1)] grouped rules in \"%s\"",
						sym->realname);
			else
				error("Ambiguous [non-LL(1)] rules for \"%s\"", sym->name);
		}

		for (Symnode * orelse = sym->node; orelse != NULL; orelse = orelse->orelse)
			for (n = orelse; n != NULL; n = n->next)
				if (n->sym->type == TERMINAL && n->alias != NULL)
					if (sym->realname != NULL && sym->name != sym->realname)
						error("Alias \"%s\" defined for terminal"
								"\"%s\" in grouped rules for \"%s\"",
								n->alias, n->sym->name, sym->realname);
					else
						error("Alias \"%s\" defined for terminal"
								"\"%s\" in rules for \"%s\"",
								n->alias, n->sym->name, sym->name);
	}
}
