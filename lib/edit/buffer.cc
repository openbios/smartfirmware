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
#include <sys/stat.h>
#include <fcntl.h>


const int BUFSTART = 1024;
const int BUFLIMIT = 64*1024;


void
Lineptr::newid()
{
	static long lineid = 1;

	if ((id = lineid++) <= 0)
		id = lineid = 1;
}

Lineptr::Lineptr(long num)
{
	numchars = num;
	next = prev = this;
	newid();
}

long
Lineptr::add(long num)
{
	if (num == 0)
		return 0;

	newid();
	return numchars += num;
}

long
Lineptr::set(long num)
{
	if (num == numchars)
		return num;

	newid();
	return numchars = num;
}


Buffer::Buffer() : undo(*new Undolist)
{
	dot = colnum = linenum = 0;
	size1 = size2 = 0;
	size = gap = BUFSTART;
	text = new Buftext[size];
	firstline = currline = new Lineptr(0);
	numlines = 1;

	marklist = NULL;

	doundo = FALSE;
	undostate = U_NOUNDO;
	undoloc = 0;
	undo[50].u = NULL;

	setmodified(FALSE);
}

Buffer::~Buffer()
{
	Lineptr *line = firstline->next;

	do
	{
		Lineptr *t = line;
		line = line->next;
		delete t;
	} while (line != firstline);

	for (Mark * mark = marklist; mark != NULL; mark = mark->next)
	{
		mark->loc = -1;
		mark->buffer = NULL;
	}

	delete text;
	delete (Undolist *)&undo;
}

void
Buffer::moverel(long delta)
{
	if (delta == 0)
		return;

	long t;
	dot += delta;

	if (dot < 0)
	{
		delta -= dot;
		dot = 0;
	}
	else if (dot > (t = size1 + size2))
	{
		delta -= dot - t;
		dot = t;
	}

	if (delta > 0)
	{
		delta += colnum;

		for (;;)
		{
			if (delta >= currline->numchars && linenum < numlines - 1)
			{
				delta -= currline->numchars;
				currline = currline->next;
				linenum++;

				if (delta > 0)
					continue;
			}

			colnum = delta;
			break;
		}
	}
	else
	{
		delta = -delta + currline->numchars - colnum;

		for (;;)
		{
			if (delta > currline->numchars && linenum >= 1)
			{
				delta -= currline->numchars;
				currline = currline->prev;
				linenum--;

				if (delta > 0)
					continue;
			}

			colnum = currline->numchars - delta;
			break;
		}
	}
}

void
Buffer::makeroom(long sz)
{
	if (gap > sz)
		return;

	long newsize = size;

	do
		newsize <<= 1;
	while (newsize < BUFLIMIT && newsize < sz);

	if (newsize < sz)
		newsize = sz + BUFLIMIT - sz % BUFLIMIT;

	Buftext *newtext = new Buftext[newsize];

	if (size1 > 0)
		memcpy(newtext, text, (size_t)size1 * sizeof (Buftext));

	long newgap = gap + newsize - size;

	if (size2 > 0)
		memcpy(newtext + size1 + newgap, text + size1 + gap,
				(size_t)size2 * sizeof (Buftext));

	gap = newgap;
	size = newsize;
	delete text;
	text = newtext;
}

void
Buffer::movegap(long loc)
{
	if (size1 == loc)
		return;

	Buftext *p1 = text + size1;
	Buftext *p2 = text + size1 + gap;
	long n;

	if (loc < size1)
	{
		n = size1 - loc;
		size2 += n;
		memmove(p2 - n, p1 - n, n * sizeof *p1);
	}
	else
	{
		n = loc - size1;
		size2 -= n;
		memmove(p1, p2, n * sizeof *p1);
	}

	size1 = loc;
}

long
Buffer::getlocline(long loc)
{
	long old = dot;
	moveto(loc);
	long line = getline();
	moveto(old);
	return line;
}

long
Buffer::getlineloc(long lnum)
{
	if (lnum < 0 || lnum >= numlines)
		return -1;

	Lineptr *line = currline;
	long numchars = 0;

	if (lnum < linenum)
		for (; lnum < linenum; lnum++)
		{
			line = line->prev;
			numchars -= line->numchars;
		}
	else
		for (; lnum > linenum; lnum--)
		{
			numchars += line->numchars;
			line = line->next;
		}

	return dot - colnum + numchars;
}

long
Buffer::getlineid(long lnum)
{
	if (lnum < 0 || lnum >= numlines)
		return -1;

	Lineptr *line = currline;

	if (lnum < linenum)
		for (; lnum < linenum; lnum++)
			line = line->prev;
	else
		for (; lnum > linenum; lnum--)
			line = line->next;

	return line->id;
}

long
Buffer::getlinelen(long lnum)
{
	if (lnum < 0 || lnum >= numlines)
		return -1;

	Lineptr *line = currline;

	if (lnum < linenum)
		for (; lnum < linenum; lnum++)
			line = line->prev;
	else
		for (; lnum > linenum; lnum--)
			line = line->next;

	return line->numchars;
}

void
Buffer::insert(const Buftext *str, long len)
{
	if (len < 0)
		len = strlen(str);

	if (len == 0)
		return;

	if (getundo())
	{
		recundo();
		addundo(U_INSERT, dot, str, len);
		endrecundo();
	}

	for (Mark * mark = marklist; mark != NULL; mark = mark->next)
		if (mark->loc >= dot)
			mark->loc += len;

	makeroom(len);
	movegap(dot);

	Buftext *p = text + size1;
	size1 += len;
	gap -= len;

	long carry = currline->numchars - colnum;
	long oldid = colnum > 0 ? -1 : currline->id;
	const Buftext *s;

	for (s = str--; len-- > 0; *p++ = *s++)
	{
		if (*s == NEWLINE)
		{
			currline->add(s - str - carry);
			colnum = 0;
			linenum++;
			str = s;

			Lineptr *line = currline;
			Lineptr *newline = currline = new Lineptr(carry);

			if (oldid >= 0)
			{
				line->id = newline->id;
				newline->id = oldid;
			}

			newline->prev = line;
			newline->next = line->next;
			line->next = newline->next->prev = newline;
			numlines++;
		}
	}

	long t = s - str - 1;

	if (t > 0)
	{
		currline->add(t);
		colnum += t;
	}

	dot = size1;
	setmodified(TRUE);
}

void
Buffer::erase()
{
	if (getundo())
	{
		recundo();

		if (size1 > 0)
			addundo(U_DELETE, 0, text, size1);

		if (size2 > 0)
			addundo(U_DELETE, size1, text + size1 + gap, size2);

		endrecundo();
	}

	for (Mark * mark = marklist; mark != NULL; mark = mark->next)
		mark->loc = 0;

	gap = size;
	size1 = size2 = 0;
	dot = colnum = linenum = 0;

	for (Lineptr * line = firstline->next; line != firstline;
			line = line->next)
		delete line;

	firstline = currline = new Lineptr(0);
	numlines = 1;
	setmodified(TRUE);
}

void
Buffer::kill(long num)
{
	if (num <= 0)
		return;

	movegap(dot);

	if (num > size2)
		num = size2;

	if (getundo())
	{
		recundo();
		addundo(U_DELETE, dot, text + size1 + gap, num);
		endrecundo();
	}

	for (Mark *mark = marklist; mark != NULL; mark = mark->next)
		if (mark->loc >= dot)
		{
			mark->loc -= num;

			if (mark->loc < dot)
				mark->loc = dot;
		}

	gap += num;
	size2 -= num;

	for (;;)
	{
		Lineptr *line = currline;

		if (num >= line->numchars - colnum && linenum < numlines - 1)
		{
			num -= line->numchars - colnum;
			line->set(colnum + line->next->numchars);

			if (colnum == 0)
				line->id = line->next->id;

			line = line->next;
			line->prev->next = line->next;
			line->next->prev = line->prev;

			if (firstline == line)
				firstline = line->next;

			delete line;
			numlines--;
		}
		else
		{
			line->add(-num);
			break;
		}
	}

	setmodified(TRUE);
}

void
Buffer::killprev(long num)
{
	if (num > dot)
		num = dot;

	if (num <= 0)
		return;

	moverel(-num);
	kill(num);
}

void
Buffer::copy(Buffer &buf, long len)
{
	if (dot < size1)
	{
		long t = size1 - dot;

		buf.insert(text + dot, len < t ? len : t);

		if (len > t)
			buf.insert(text + dot + gap, len - t);
	}
	else
		buf.insert(text + dot + gap, len);
}


boolean
Buffer::readfile(const char *fname)
{
	struct stat sbuf;

	if (stat(fname, &sbuf) < 0)
		return FALSE;

	int fd = open(fname, O_RDONLY);

	if (fd < 0)
		return FALSE;

	erase();

	makeroom(sbuf.st_size);

	if (read(fd, text, (size_t)sbuf.st_size) != sbuf.st_size)
	{
		close(fd);
		return FALSE;
	}

	close(fd);

	size1 = sbuf.st_size;
	gap -= size1;

	if (getundo())
	{
		recundo();
		addundo(U_INSERT, 0, text, size1);
		endrecundo();
	}

	Buftext *p = text;
	long len = size1;
	Buftext *s;

	for (s = text; len-- > 0; s++)
	{
		if (*s == NEWLINE)
		{
			currline->set(s - p + 1);
			Lineptr *newline = new Lineptr(0);
			newline->next = currline->next;
			currline->next = newline;
			newline->prev = currline;
			currline = newline;
			numlines++;
			p = s + 1;
		}
	}

	currline->set(s - p);
	currline = firstline;
	linenum = 0;
	colnum = 0;
	dot = 0;
	setmodified(FALSE);
	return TRUE;
}

boolean
Buffer::writefile(const char *fname)
{
	int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (fd < 0)
		return FALSE;

	if (size1 > 0)
		write(fd, text, (size_t)size1);

	if (size2 > 0)
		write(fd, text + size1 + gap, (size_t)size2);

	close(fd);
	setmodified(FALSE);
	return TRUE;
}


void
Buffer::debugdump()
{
	debug("Buffer:  line=%d linelen=%d col=%d numlines=%d currid=%d"
			" dot=%d len=%d\n", linenum, currline->numchars, colnum,
			numlines, currline->id, dot, length());

	Buftext *p = text;
	long i = size1;
	Lineptr *line = firstline;
	long ln = 0;

	debug("   00:%.2d.%.3d> ", line->numchars, line->id);

	while (i--)
	{
		debug("%c", *p);

		if (*p++ == NEWLINE)
		{
			line = line->next;
			debug("   %.2d:%.2d.%.3d> ", ++ln, line->numchars, line->id);
		}
	}

	p += gap;
	i = size2;

	while (i--)
	{
		debug("%c", *p);
		if (*p++ == NEWLINE)
		{
			line = line->next;
			debug("   %.2d:%.2d.%.3d> ", ++ln, line->numchars, line->id);
		}
	}

	debug("\n");
}
