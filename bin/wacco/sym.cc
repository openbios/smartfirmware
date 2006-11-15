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

// Copyright (c) 1991-2002 by Parag Patel.  All Rights Reserved.

#define IMPLEMENT
#include "defs.h"
#include <limits.h>

inline unsigned sethash(Bitset *key) { return key->hash(); }
implement_hashtable(Settab, Setiter, Set, Bitset*, sethash);
Settab settab;

declare_hashtable(Symtab, Symiter, Symbol, const char*);
implement_hashtable(Symtab, Symiter, Symbol, const char*, strhash);

declare_array(Symptrarr,Symbol*,256);
implement_array(Symptrarr, Symbol*,256);

Symbol *startsymbol = NULL;
Symbol *startcode = NULL;

static Symtab symlist;
static Symptrarr symids, nonterms, terms;


Symbol::Symbol(const char *n)
{
	name = strget(n);
	type = TERMINAL;
	rettype = NULL;
	lexstr = NULL;				// realname
	code = NULL;				// symcode
	endcode = NULL;
	usedret = 0;				// lexdef, line
	toempty = FALSE;
	usecount = 0;
	exportsym = FALSE;
	mkstruct = FALSE;
	doresync = TRUE;
	doinline = FALSE;
	first = new Bitset;
	follow = new Bitset;
	resync = NULL;				// list
	node = NULL;
}


Symnode::Symnode()
{
	sym = NULL;
	alias = NULL;
	next = NULL;
	orelse = NULL;
	resync = NULL;				// list
}

Resynclist::Resynclist(const char *n)
{
	if (n == NULL)
		name = NULL;
	else
		name = strget(n);

	first = follow = 0;
	paren = 0;
	next = NULL;
}


int
numsymbols()
{
	return symlist.size();
}

Symbol *
getsymbol(int id)
{
	if (id < 0 || id >= symids.size())
		return NULL;

	return symids[id];
}

int
numterms()
{
	return terms.size();
}

int
numnonterms()
{
	return nonterms.size();
}

Symbol *
getterm(int id)
{
	if (id < 0 || id >= terms.size())
		return NULL;

	return terms[id];
}

Symbol *
getnonterm(int id)
{
	if (id < 0 || id >= nonterms.size())
		return NULL;

	return nonterms[id];
}


Symbol *
addsymbol(const char *name)
{
	if (symlist.find(name))
		return &symlist();

	Symbol & s = symlist[name];
	static int idnum = 0;
	s.id = idnum;
	symids[idnum++] = &s;
	s.first = new Bitset;
	s.follow = new Bitset;
	return &s;
}

void
addterm(Symbol *s)
{
	if (s == NULL)
		return;

	terms[s->parserid = (int)terms.size()] = s;
}

void
addnonterm(Symbol *s)
{
	if (s == NULL)
		return;

	nonterms[s->parserid = (int)nonterms.size()] = s;
	s->parserid = -1 - s->parserid;

	if (s->type == TERMINAL)
	{
		s->type = NONTERMINAL;
		s->realname = s->name;
	}
}

Symbol *
findsymbol(const char *name)
{
	if (symlist.find(name))
		return &symlist();

	return NULL;
}

void
initsym()
{
	Symbol *s;

	s = addsymbol("$ END");		// id == 0 == END
	s = addsymbol("[]");		// id == 1 == EMPTY
}
