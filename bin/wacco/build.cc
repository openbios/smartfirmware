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

// Copyright (c) 1991,1993,1995,1997 by Parag Patel.  All Rights Reserved.

#include "defs.h"

static Set *g_noresync = NULL;

static void
markempty()
{
	Symbol *sym;
	Symnode *orelse, *next, *node;
	int i;
	boolean done;
	int num = numsymbols();

	do
	{
		done = TRUE;

		for (i = START; i < num; i++)
		{
			sym = getsymbol(i);

			if (sym->type != NONTERMINAL)
				continue;

			for (orelse = sym->node; orelse != NULL; orelse = orelse->orelse)
			{
				for (node = orelse; node != NULL; node = node->next)
					if (node->sym->type == TERMINAL ||
							node->sym->type == NONTERMINAL ||
							node->sym->type == ZEROPLUS ||
							node->sym->type == ONEZERO)
						break;

				if (node == NULL)
					continue;

				if (node->sym->id == EMPTY && !sym->toempty)
				{
					sym->toempty = TRUE;
					done = FALSE;
					continue;
				}

				for (next = node; next != NULL; next = next->next)
					if (next->sym->type < CODE && !next->sym->toempty)
						break;
					else if (next->sym->type == ZEROPLUS ||
							next->sym->type == ONEZERO)
						// following node is effectively ==> []
						next = next->next;

				if (next == NULL && !sym->toempty)
				{
					sym->toempty = TRUE;
					done = FALSE;
				}
			}
		}
	} while (!done);
}

static void
first()
{
	Symbol *sym, *s;
	Symnode *orelse, *next, *node;
	int i;
	boolean done;
	int num = numsymbols();

	do
	{
		done = TRUE;

		for (i = START; i < num; i++)
		{
			sym = getsymbol(i);

			if (sym->type >= CODE)
				continue;

			if (sym->type == TERMINAL)
			{
				if (sym->first->has(sym->id))
					continue;

				*sym->first += sym->id;
				done = FALSE;
				continue;
			}

			if (sym->toempty && !sym->first->has(EMPTY))
			{
				*sym->first += EMPTY;
				done = FALSE;
			}

			for (orelse = sym->node; orelse != NULL; orelse = orelse->orelse)
			{
				for (node = orelse; node != NULL; node = node->next)
					if (node->sym->type < CODE || node->sym->type == ZEROPLUS)
						break;

				if (node == NULL)
					continue;

				s = node->sym;

				if (s->id == EMPTY)
				{
					if (!sym->first->has(EMPTY))
					{
						*sym->first += EMPTY;
						done = FALSE;
					}

					continue;
				}

				if (node->sym->type == ZEROPLUS)
				{
					node = node->next;

					if (!node->sym->first->subset(*sym->first))
					{
						*sym->first |= *node->sym->first;
						done = FALSE;
					}

					node = node->next;
				}

				for (next = node; next != NULL; next = next->next)
				{
					s = next->sym;

					if (s->type >= CODE)
						continue;

					if (!s->first->subset(*sym->first))
					{
						*sym->first |= *s->first;
						done = FALSE;
					}

					if (!s->toempty)
						break;
				}
			}
		}
	} while (!done);
}

static void
follow()
{
	Symbol *sym, *s;
	Symnode *orelse, *node, *next, *n;
	int i;
	boolean done;
	Bitset set;
	int num = numsymbols();

	if (!exportedname)
		*startsymbol->follow += END;

	do
	{
		done = TRUE;

		for (i = START; i < num; i++)
		{
			sym = getsymbol(i);

			if (sym->type != NONTERMINAL)
				continue;

			if (sym->exportsym)
				*sym->follow += END;

			for (orelse = sym->node; orelse != NULL; orelse = orelse->orelse)
			{
				for (node = orelse; node != NULL; node = node->next)
				{
					s = node->sym;

					if (s->type >= CODE)
						continue;

					for (next = node->next; next != NULL; next = next->next)
						if (next->sym->type < CODE)
							break;

					for (n = next; n != NULL; n = n->next)
						if (n->sym->type < CODE && !n->sym->toempty)
							break;

					if (n == NULL)
					{
						if (sym->follow->subset(*s->follow))
							continue;

						*s->follow |= *sym->follow;
						done = FALSE;
					}

					if (next != NULL)
					{
						set = *next->sym->first;
						set -= EMPTY;

						if (!set.subset(*s->follow))
						{
							*s->follow |= set;
							done = FALSE;
						}
					}
				}
			}
		}
	} while (!done);
}

static void
mksymresync(Symbol *sym)
{
	if (!sym->doresync)
	{
		if (g_noresync == NULL)
		{
			Bitset *set = new Bitset;
			*set += END;
			g_noresync = &settab[set];
		}

		sym->resync = g_noresync;
		return;
	}

	if (sym->list == NULL)
		return;

	Bitset *set = new Bitset;
	Resynclist *x;

	for (Resynclist * r = sym->list; r != NULL; x = r, r = r->next, delete x)
	{
		Symbol *s;

		if (r->name != NULL)
		{
			s = findsymbol(r->name);

			if (s == NULL)
			{
				error("Symbol \"%s\" doesn't exist for \"%s\" resync set",
						r->name, sym->name);
				continue;
			}
		}
		else if (r->paren == 0)
			s = getsymbol(EMPTY);
		else
		{
			error("Cannot handle paren groups in skip-set for \"%s\"",
					sym->name);
			continue;
		}

		if (r->first > 0 || (r->first == 0 && r->follow == 0))
			*set |= *s->first;
		else if (r->first < 0)
			*set -= *s->first;

		if (r->follow > 0)
			*set |= *s->follow;
		else if (r->follow < 0)
			*set -= *s->follow;
	}

//printf("mksymresync %s set = ", sym->name); dumpset(*set);
	sym->resync = &settab[set];

	if (sym->resync->set != set)
		delete set;
//printf("	resync set = "); dumpset(*sym->resync->set);
}

static void
mkresync(Symbol *sym, Symnode *start, Symnode *n)
{
	Bitset *set = new Bitset;
	Symnode *t = n;

	for (int i = 0; t != NULL && i <= 3; t = t->next, i++)
		*set |= *t->sym->first;
//{
//printf("mkresync %s (loop %d) first of %s = ", sym->name, i, t->sym->name);
//dumpset(*t->sym->first);
//set->orelse(*t->sym->first);
//printf("	set = "); dumpset(*set);
//}

	Resynclist *x;

	for (Resynclist * r = n->list; r != NULL; x = r, r = r->next, delete x)
	{
		Symbol *s;

		if (r->name != NULL)
		{
			s = findsymbol(r->name);

			if (s == NULL)
			{
				for (t = start; t != NULL; t = t->next)
					if (t->alias != NULL && strcmp(t->alias, r->name) == 0)
						break;

				if (t != NULL)
					s = t->sym;
			}

			if (s == NULL)
			{
				error("Symbol \"%s\" doesn't exist for \"%s\" resync set",
						r->name, sym->name);
				continue;
			}
		}
		else if (r->paren == 0)
			s = getsymbol(EMPTY);
		else
		{
			for (t = start; t != NULL; t = t->next)
				if (t->sym->realname != NULL
						&& t->sym->realname != t->sym->name)
					if (--(*r).paren <= 0)
						break;

			if (t == NULL)
			{
				error("Paren group %d does not exist in rule for \"%s\"",
						r->paren, sym->name);
				continue;
			}

			s = t->sym;
		}

		if (r->first > 0 || (r->first == 0 && r->follow == 0))
			*set |= *s->first;
		else if (r->first < 0)
			*set -= *s->first;

		if (r->follow > 0)
			*set |= *s->follow;
		else if (r->follow < 0)
			*set -= *s->follow;
	}

//printf("mkresync %s/%s set = ", sym->name, n->sym->name); dumpset(*set);
	n->resync = &settab[set];

	if (n->resync->set != set)
		delete set;
//printf("	resync set = "); dumpset(*n->resync->set);
}

static void
resync()
{
	int i;
	Symbol *sym;
	int num = numsymbols();
	Symnode *orelse;

	for (i = START; i < num; i++)
	{
		sym = getsymbol(i);

		if (sym->type >= CODE)
			continue;

		mksymresync(sym);

		for (orelse = sym->node; orelse != NULL; orelse = orelse->orelse)
			for (Symnode * n = orelse; n != NULL; n = n->next)
				if (n->sym->type < CODE && n->sym->id != EMPTY)
					mkresync(sym, orelse, n);
	}
}

void
buildsets()
{
	markempty();
	first();
	follow();
	resync();
}
