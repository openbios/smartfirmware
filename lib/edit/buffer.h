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

#ifndef	_BUFFER_H_
#define _BUFFER_H_


typedef char Buftext;	// change this to short or int for 16/32-bit chars


class Buffer;

class Mark
{
	long loc;
	Mark *next;
	Buffer *buffer;

	friend class Buffer;

	void add(Buffer &buf);
	void del();

public:
	Mark(Buffer &buf) { add(buf); }
	~Mark() { del(); }

	long operator=(long dot);
	long operator=(Buffer &buf);
	long operator()() { return loc < 0 ? -1 : loc; }
	operator long() { return loc < 0 ? -1 : loc; }
};

class Undorec;
class Undoptr;
class Undolist;

class Lineptr
{
public:
	long numchars;
	long id;
	Lineptr *prev, *next;

	Lineptr(long num = 0);
	void newid();
	long add(long num);
	long set(long num);
};

class Buffer
{
public:
	enum Undotype
	{
		U_NONE,
		U_INSERT,
		U_DELETE,
		U_MARK,
	};

private:
	long size, size1, gap, size2;
	long dot, colnum, linenum, numlines;
	Lineptr *firstline, *currline;
	Buftext *text;
	Mark *marklist;
	friend class Mark;
	Undolist & undo;
	boolean doundo;
	long undostate, undoloc, ul;
	boolean modified;

	void makeroom(long size);
	void movegap(long loc);

	void donextundo(Undorec * u);

public:
	void debugdump();

	void setundo(boolean state) { doundo = state; }
	boolean getundo() { return doundo; }

	long getundosize();
	void setundosize(long limit);
	void recundo();
	void endrecundo();
	void addundo(Undotype type, long loc, const Buftext *text, long len);
	boolean startundo();
	boolean prevundo();
	boolean nextundo();
	void endundo();

	Buffer();
	~Buffer();

	void moverel(long loc);
	void kill(long num);
	void insert(const Buftext *str, long len = -1);
	void copy(Buffer &buf, long len);
	void erase();

	long getlineloc(long line);
	long getlineid(long line);
	long getlinelen(long line);
	long getlocline(long loc);

	int operator[](long loc)
	{
		if (loc < 0 || loc >= size1 + size2)
			return -1;
		else
			return text[loc >= size1 ? loc + gap : loc];
	}

	long length() { return size1 + size2; }
	long getloc() { return dot; }
	long getcol() { return colnum; }
	long getline() { return linenum; }

	long getlinelen() { return currline->numchars; }
	long getlineid() { return currline->id; }

	long getnumlines() { return numlines; }

	void moveto(long loc) { moverel(loc - dot); }
	void killprev(long num);

	boolean ismodified() { return modified; }
	void setmodified(boolean b) { modified = b; }
	boolean readfile(const char *fname);
	boolean writefile(const char *fname);
};


#endif /* _BUFFER_H_ */
