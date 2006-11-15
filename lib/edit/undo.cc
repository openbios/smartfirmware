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

/* Copyright (c) 1990,1999 by Parag Patel.  All Rights Reserved. */

#include "defs.h"
#include "buffer.h"
#include "undo.h"

Undorec::Undorec(long l)
{
	len = l;
	type = Buffer::U_NONE;
	next = NULL;
	text = new Buftext[l];
}

void
Undorec::undo(Buffer &buf)
{
	if (type == Buffer::U_INSERT)
	{
		buf.moveto(dot);
		buf.kill(len);
	}
	else if (type == Buffer::U_DELETE)
	{
		buf.moveto(dot);
		buf.insert(text, len);
	}
}

void
Undorec::redo(Buffer &buf)
{
	if (type == Buffer::U_INSERT)
	{
		buf.moveto(dot);
		buf.insert(text, len);
	}
	else if (type == Buffer::U_DELETE)
	{
		buf.moveto(dot);
		buf.kill(len);
	}
}


void
Undoptr::clear()
{
	while (u != NULL)
	{
		Undorec *p = u;

		u = u->next;
		delete p;
	}
}


long
Buffer::getundosize()
{
	return undo.size();
}

void
Buffer::setundosize(long loc)
{
	if (!doundo || undostate != U_NOUNDO)
		return;

	if (loc <= 0)
		doundo = FALSE;
	else if (loc < undo.size())
		undo.reset(loc);
	else
		undo[loc].u = NULL;
}


static long
prev(long max, long u)
{
	u -= 1;

	if (u < 0)
		u = max - 1;

	return u;
}

static long
next(long max, long u)
{
	u += 1;

	if (u >= max)
		u = 0;

	return u;
}


void
Buffer::recundo()
{
	if (!doundo || undostate < U_NOUNDO)
		return;

	if (undostate++ != U_NOUNDO)
		return;

	undoloc = next(undo.size(), undoloc);
	undo[undoloc].clear();
	undo[undoloc].u = new Undorec(0);
}

void
Buffer::endrecundo()
{
	if (!doundo || undostate <= U_NOUNDO)
		return;

	undostate--;
}

void
Buffer::addundo(Undotype type, long loc, const Buftext *txt, long len)
{
	if (undostate <= U_NOUNDO)
		return;

	Undorec *u = new Undorec(len);

	u->next = undo[undoloc].u;
	undo[undoloc].u = u;
	u->type = type;
	u->dot = loc;
	memcpy((char *)u->text, (char *)txt, (size_t)len * sizeof (Buftext));
}


boolean
Buffer::startundo()
{
	if (!doundo || undostate != U_NOUNDO)
		return ERR;

	undostate = U_UNDOING;
	ul = undoloc = next(undo.size(), undoloc);
	undo[undoloc].clear();
	undo[undoloc].u = new Undorec(0);
	return OK;
}

boolean
Buffer::prevundo()
{
	if (undostate != U_UNDOING)
		return ERR;

	long u = prev(undo.size(), ul);
	Undorec *un = undo[u].u;

	if (u == undoloc || un == NULL)
		return ERR;

	for (; un != NULL; un = un->next)
		un->undo(*this);

	ul = u;
	return OK;
}

void
Buffer::donextundo(Undorec *un)
{
	if (un->next != NULL)
		donextundo(un->next);

	un->redo(*this);
}

boolean
Buffer::nextundo()
{
	if (undostate != U_UNDOING)
		return ERR;

	long u = ul;
	Undorec *un = undo[u].u;

	if (u == undoloc || un == NULL)
		return ERR;

	donextundo(un);
	ul = next(undo.size(), u);
	return OK;
}

void
Buffer::endundo()
{
	if (undostate != U_UNDOING)
		return;

	undostate = U_UNDOREC;
	long lim = undo.size();
	long end = prev(lim, ul);

	for (long u = prev(lim, undoloc); u != end; u = prev(lim, u))
		for (Undorec * un = undo[u].u; un != NULL; un = un->next)
			addundo(un->type == Buffer::U_INSERT
					? Buffer::U_DELETE : Buffer::U_INSERT,
					un->dot, un->text, un->len);

	undostate = U_NOUNDO;
}

implement_array(Undolist, Undoptr, 1024);
