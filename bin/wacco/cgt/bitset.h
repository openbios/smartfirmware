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


/* Copyright (c) 1991,1997 by Thomas J. Merritt and Parag Patel.
   All Rights Reserved.
 */

#ifndef __BITSET_H_
#define __BITSET_H_

#include <stdio.h>

#ifndef __STDLIBX_H_
#include <stdlibx.h>
#endif

#define __BITSET_VEC_LEN 4

class Bitset
{
	unsigned long *_bits;
	int _len : 24;
	int _user : 1;
	int _size;
	unsigned long _vec[__BITSET_VEC_LEN];
	void doexpand(int);
	void expand(int n)
		{ if (_user || n >= _size) doexpand(n); }
	void dowritable();
	void writable() { if (_user) dowritable(); }

public:
	// constructors
	Bitset();
	Bitset(int s);
	Bitset(Bitset const &);
	Bitset(unsigned long const *bits, int len);
	~Bitset();

	// member functions
	void assign(Bitset const &);
	int compare(Bitset const &) const;
	boolean subset(Bitset const &) const;
	boolean has(int i) const
	{
		return i >= 0 && i / BITSPERLONG < _len &&
			(_bits[i / BITSPERLONG] & (1UL << (i % BITSPERLONG)));
	}
	int size() const { return _size; }
	int prev(int) const;
	int next(int) const;
	int max() const { return prev(_size); }
	int min() const { return next(-1); }
	int count() const;
	void clear();
	boolean empty() const;
	void insert(int);
	void remove(int);
	void remove(Bitset const &);
	void b_or(Bitset const &);
	void b_xor(Bitset const &);
	void b_and(Bitset const &);
	void b_not();
	unsigned long hash() const;
	void set(unsigned long const *bits, int len);
	void get(unsigned long const *&bits, int &len) const;
	void dump(FILE *fp = stdout) const;

	// overloaded operators
	Bitset &operator=(Bitset const &b) { assign(b); return *this; }
	Bitset &operator+=(int i) { insert(i); return *this; }
	Bitset &operator+=(Bitset const &b) { b_or(b); return *this; }
	Bitset &operator-=(int i) { remove(i); return *this; }
	Bitset &operator-=(Bitset const &b) { remove(b); return *this; }
	Bitset &operator&=(Bitset const &b) { b_and(b); return *this; }
	Bitset &operator|=(Bitset const &b) { b_or(b); return *this; }
	Bitset &operator^=(Bitset const &b) { b_xor(b); return *this; }
	boolean operator==(Bitset const &b) const { return compare(b) == 0; }
	boolean operator!=(Bitset const &b) const { return compare(b) != 0; }
	boolean operator<=(Bitset const &b) const { return subset(b); }
	boolean operator<(Bitset const &b) const
		{ return subset(b) && compare(b) != 0; }
	boolean operator>=(Bitset const &b) const { return b.subset(*this); }
	boolean operator>(Bitset const &b) const
		{ return b.subset(*this) && compare(b) != 0; }
	Bitset operator~() const { Bitset x = *this; x.b_not(); return x; }
	Bitset operator+(Bitset const &b) const
		{ Bitset x = *this; x.b_or(b); return x; }
	Bitset operator-(Bitset const &b) const
		{ Bitset x = *this; x -= b; return x; }
	Bitset operator&(Bitset const &b) const
		{ Bitset x = *this; x.b_and(b); return x; }
	Bitset operator|(Bitset const &b) const
		{ Bitset x = *this; x.b_or(b); return x; }
	Bitset operator^(Bitset const &b) const
		{ Bitset x = *this; x.b_xor(b); return x; }
	boolean operator[](int i) { return has(i); }
};

#endif // __BITSET_H_
