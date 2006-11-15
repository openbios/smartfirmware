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
#include "rex.h"

class Charset
{
	uInt vec[8];	// handles 256 chars (one bit per char) in 32 bytes

public:
	Charset() { vec[0] = vec[1] = vec[2] = vec[3] =
			vec[4] = vec[5] = vec[6] = vec[7] = 0x0; }

	void add(unsigned char c)
		{ vec[c >> 5] |= (1UL << (c & 0x1F)); }

	void del(unsigned char c)
		{ vec[c >> 5] &= ~(1UL << (c & 0x1F)); }

	boolean isin(unsigned char c)
		{ return vec[c >> 5] & (1UL << (c & 0x1F)); }

	void operator+=(unsigned char c) { add(c); }
	void operator-=(unsigned char c) { del(c); }
	boolean operator==(unsigned char c) { return isin(c); }
};

declare_array(Bufchars, Buftext, 1024);
implement_array(Bufchars, Buftext, 1024);

enum Rexcode
{
	R_NONE,	// noop
	R_BOL,	// beginning of line
	R_EOL,	// end of line
	R_BOW,	// beginning of word
	R_EOW,	// end of word
	R_CHAR,	// a single character
	R_ANYVEC,	// any character in vector (for 7/8-bit chars)
	R_NOTVEC,	// any character NOT in vector
	R_ANYARR,	// any character in array (for 16/32-bit chars)
	R_NOTARR,	// any character NOT in array
	R_ANYCHAR,	// any single character
	R_OR,	// pointer to alternative rex
	R_COUNT,	// following thing n to m times - star==0,-1  plus==1,-1
	R_PAREN,	// pointer to parenthesized rex
};

struct Rexnode
{
	Rexcode code;
	union
	{
		Buftext *str;
		Buftext ch;
		short count[2];
		Charset *set;
		Bufchars *arr;
		class Rexlist *rex;
		unsigned long val;
	};

	void clear();
	Rexnode() { code = R_NONE; val = 0; }
	~Rexnode() { clear(); }

	void operator=(Rexnode & n)
			{ code = n.code; val = n.val; n.code = R_NONE; }

	boolean operator==(Rexnode n)
			{ return memcmp(this, &n, sizeof this) == 0; }
};
declare_array(Rexlist, Rexnode, 1024);
implement_array(Rexlist, Rexnode, 1024);

struct Subarea
{
	long start;
	long end;

	boolean operator==(Subarea s)
			{ return start == s.start && end == s.end; }
};
declare_array(Sublist, Subarea, 32);
implement_array(Sublist, Subarea, 32);


void
Rexnode::clear()
{
	switch (code)
	{
	case R_ANYVEC:
		delete set;

	Case R_NOTVEC:
		delete set;

	Case R_ANYARR:
		delete arr;

	Case R_NOTARR:
		delete arr;

	Case R_OR:
		delete rex;

	Case R_PAREN:
		delete rex;

	Default:
		;
	}

	code = R_NONE;
	val = 0;
}


static Charset *
parse_set(const Buftext *&rx, long &len)
{
	Charset *set = new Charset;
	Buftext last = '\0';
	Buftext end;

	while (len-- > 0)
	{
		switch (*rx)
		{
		case '\\':
			len--;
			set->add(*++rx);

		Case '-':
			end = *++rx;
			len--;

			while (last <= end)
				set->add(last++);

		Case ']':
			rx++;
			return set;

		default:
			set->add(*rx);
		}

		last = *rx++;
	}

	delete set;
	return NULL;
}

static Bufchars *
parse_arr(const Buftext *&rx, long &len)
{
	Bufchars *arr = new Bufchars;
	Buftext last = '\0';
	Buftext end;

	while (len-- > 0)
	{
		switch (*rx)
		{
		case '\\':
			len--;
			arr->end() = *++rx;

		Case '-':
			end = *++rx;
			len--;

			while (last <= end)
				arr->end() = last++;

		Case ']':
			rx++;
			return arr;

		default:
			arr->end() = *rx;
		}

		last = *rx++;
	}

	delete arr;
	return NULL;
}

static Rexlist *
parse_rex(const Buftext *&rx, long &len, boolean paren = FALSE)
{
	Rexlist *rex = new Rexlist;
	Rexlist *rex2 = NULL;
	long loc = 0;
	long l;
	short val;

	while (len-- > 0)
	{
		switch (*rx++)
		{
		case '^':
			(*rex)[loc].code = R_BOL;

		Case '$':
			(*rex)[loc].code = R_EOL;

		Case '*':
			if (loc == 0)
			{
				(*rex)[loc].code = R_CHAR;
				(*rex)[loc].ch = '*';
			}
			else
			{
				(*rex)[loc] = (*rex)[l = loc - 1];
				(*rex)[l].code = R_COUNT;
				(*rex)[l].count[0] = 0;
				(*rex)[l].count[1] = -1;
			}

		Case '[':
			if (sizeof (Buftext)== sizeof (char))
			{
				if (*rx == '^')
				{
					(*rex)[loc].code = R_NOTVEC;
					rx++;
					len--;
				}
				else
					(*rex)[loc].code = R_ANYVEC;

				(*rex)[loc].set = parse_set(rx, len);

				if ((*rex)[loc].set == NULL)
				{
					delete rex;
					return NULL;
				}
			}
			else
			{
				if (*rx == '^')
				{
					(*rex)[loc].code = R_NOTARR;
					rx++;
					len--;
				}
				else
					(*rex)[loc].code = R_ANYARR;

				(*rex)[loc].arr = parse_arr(rx, len);

				if ((*rex)[loc].arr == NULL)
				{
					delete rex;
					return NULL;
				}
			}

		Case '\\':
			if (len-- == 0)
			{
				delete rex;
				return NULL;
			}

			switch (*rx++)
			{
			case '<':
				(*rex)[loc].code = R_BOW;

			Case '>':
				(*rex)[loc].code = R_EOW;

			Case '+':
				if (loc == 0)
				{
					delete rex;
					return NULL;
				}

				(*rex)[loc] = (*rex)[l = loc - 1];
				(*rex)[l].code = R_COUNT;
				(*rex)[l].count[0] = 1;
				(*rex)[l].count[1] = -1;

			Case '{':			/* } */
				if (loc == 0)
				{
					delete rex;
					return NULL;
				}

				(*rex)[loc] = (*rex)[l = loc - 1];
				(*rex)[l].code = R_COUNT;

				val = 0;

				for (; len > 0 && isdigit(*rx); rx++, len--)
					val = val * 10 + *rx - '0';

				(*rex)[l].count[0] = val;

				if (*rx == ',')
				{
					rx++, len--;
					val = 0;

					if (*rx == /* { */ '}')
						val = -1;
					else
						for (; len > 0 && isdigit(*rx); rx++, len--)
							val = val * 10 + *rx - '0';

					(*rex)[l].count[1] = val;
				}

				if (*rx == '\\')
					rx++, len--;

				if (*rx != /* { */ '}')
				{
					delete rex;
					return NULL;
				}

				rx++, len--;

			Case '|':
				rex2 = new Rexlist;
				(*rex2)[0].code = R_OR;
				(*rex2)[0].rex = rex;
				rex = parse_rex(rx, len, paren);

				if (rex == NULL)
				{
					delete rex2;
					return NULL;
				}

				(*rex2)[1].code = R_OR;
				(*rex2)[1].rex = rex;
				rex = rex2;
				loc = 1;

			Case '(':
				(*rex)[l = loc].code = R_PAREN;
				(*rex)[l].rex = parse_rex(rx, len, TRUE);

				if ((*rex)[l].rex == NULL)
				{
					delete rex;
					return NULL;
				}

			Case ')':
				if (paren)
					return rex;
				else
				{
					(*rex)[loc].code = R_CHAR;
					(*rex)[loc].ch = ')';
				}

			Default:
				(*rex)[loc].code = R_CHAR;
				(*rex)[loc].ch = *(rx - 1);
			}

		Case '.':
			(*rex)[loc].code = R_ANYCHAR;

		Default:
			(*rex)[loc].code = R_CHAR;
			(*rex)[loc].ch = *(rx - 1);
		}

		loc++;
	}

	return rex;
}


Rex::Rex()
{
	sub = new Sublist;
	rex = NULL;
}

Rex::~Rex()
{
	delete rex;
	delete sub;
}

boolean
Rex::compile(const Buftext *rx, long len)
{
	if (len < 0)
		len = strlen(rx) / sizeof (Buftext);

	delete rex;
	return (rex = parse_rex(rx, len)) != NULL;
}

long
Rex::numsubs()
{
	return sub->size();
}

long
Rex::subloc(int n)
{
	return n < sub->size() ? (*sub)[n].start : -1;
}

long
Rex::subend(int n)
{
	return n < sub->size() ? (*sub)[n].end : -1;
}

#define isword(c) (isalnum(c) || c=='_')

boolean
Rex::tryit(Rexlist &rex, long &orloc, long rlen, Buffer &buf, long &obloc)
{
	long i, j, k;
	long rloc = orloc;
	long bloc = obloc;

	for (; rloc < rlen; rloc++)
	{
		if (bloc >= buf.length())
			return FALSE;

		Rexnode & rn = rex[rloc];
		Buftext ch = buf[bloc];

		switch (rn.code)
		{
		case R_BOL:
			if (bloc != 0 && buf[bloc - 1] != NEWLINE)
				return FALSE;

		Case R_EOL:
			if (bloc != buf.length() - 1 && ch != NEWLINE)
				return FALSE;

			bloc++;

		Case R_BOW:
			if (!isword(ch) || (bloc != 0 && isword(buf[bloc - 1])))
				return FALSE;

		Case R_EOW:
			if (isword(ch) || (bloc != 0 && !isword(buf[bloc - 1])))
				return FALSE;

		Case R_CHAR:
			if (ch != rn.ch)
				return FALSE;

			bloc++;

		Case R_ANYCHAR:
			bloc++;

		Case R_ANYVEC:
			if (!rn.set->isin(ch))
				return FALSE;

			bloc++;

		Case R_NOTVEC:
			if (rn.set->isin(ch))
				return FALSE;

			bloc++;

		Case R_ANYARR:
			for (i = 0; i < rn.arr->size(); i++)
				if ((*rn.arr)[i] == ch)
					break;

			if (i >= rn.arr->size())
				return FALSE;

			bloc++;

		Case R_NOTARR:
			for (i = 0; i < rn.arr->size(); i++)
				if ((*rn.arr)[i] == ch)
					return FALSE;

			bloc++;

		Case R_OR:
			i = 0;
			j = obloc;

			if (tryit(*rn.rex, i, rn.rex->size(), buf, j))
			{
				obloc = j;
				orloc = rloc;
				return TRUE;
			}
			else
			{
				i = 0;
				j = bloc;

				if (tryit(*rex[rloc + 1].rex, i, rex[rloc + 1].rex->size(),
						buf, j))
				{
					obloc = j;
					orloc = rloc;
					return TRUE;
				}
				else
					return FALSE;
			}

		Case R_PAREN:
			i = 0;
			j = bloc;

			if (!tryit(*rn.rex, i, rn.rex->size(), buf, j))
				return FALSE;

			i = sub->size();
			(*sub)[i].start = bloc;
			bloc = j;
			(*sub)[i].end = bloc;

		Case R_COUNT:
			j = bloc;
			k = rloc + 1;

			for (i = 0; tryit(rex, k, k + 1, buf, j); i++, k = rloc + 1)
				if (rn.count[1] != -1 && i > rn.count[1])
					return FALSE;

			if (i < rn.count[0])
				return FALSE;

			bloc = j;
			rloc = k;

		Case R_NONE:
			;

		Default:
			return FALSE;
		}
	}

	obloc = bloc;
	orloc = rloc;
	return TRUE;
}

boolean
Rex::match(Buffer &buf, long bufloc, int inc)
{
	if (rex == NULL)
		return FALSE;

	long rloc;
	long bloc = bufloc;

	do
	{
		rloc = 0;
		sub->reset();
		(*sub)[0].start = bloc;

		if (tryit(*rex, rloc, rex->size(), buf, bloc)
				&& (*sub)[0].start < bloc)
		{
			(*sub)[0].end = bloc;
			return TRUE;
		}

		if (inc == 0)
			return FALSE;

		bloc += inc;
	} while (bloc >= 0 && bloc < buf.length());

	return FALSE;
}
