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


//
//	Copyright (C) 1991,1999  Thomas J. Merritt
//

#ifndef __QUICKSET_H_
#define __QUICKSET_H_

#ifndef __STDLIBX_H_
#include <stdlibx.h>
#endif

class Quickset
{
    short *_index;
    int _indexlen;
    int _indexsz;
    int *_stack;
    int _stacklen;
    int _stacksz;
    void expandindex(int);
    void expandstack(int);
public:
    // constructors
    Quickset();
    Quickset(int s);
    Quickset(Quickset const &);
    ~Quickset();

    // member functions
    void assign(Quickset const &);
    int compare(Quickset const &) const;
    boolean subset(Quickset const &) const;
    boolean has(int i) const;
    int size() const;
    int iter(int) const;
    int max() const;
    int min() const;
    int count() const;
    void clear();
    boolean empty() const;
    void insert(int);
    void remove(int);
    void remove(Quickset const &);
    void b_or(Quickset const &);
    void b_xor(Quickset const &);
    void b_and(Quickset const &);
    void b_not();
    unsigned long hash() const;
    void dump(FILE *fp = stdout) const;

    // overloaded operators
    Quickset &operator=(Quickset const &b) { assign(b); return *this; }
    Quickset &operator+=(int i) { insert(i); return *this; }
    Quickset &operator+=(Quickset const &b) { b_or(b); return *this; }
    Quickset &operator-=(int i) { remove(i); return *this; }
    Quickset &operator-=(Quickset const &b) { remove(b); return *this; }
    Quickset &operator&=(Quickset const &b) { b_and(b); return *this; }
    Quickset &operator|=(Quickset const &b) { b_or(b); return *this; }
    Quickset &operator^=(Quickset const &b) { b_xor(b); return *this; }
    boolean operator==(Quickset const &b) const { return compare(b) == 0; }
    boolean operator!=(Quickset const &b) const { return compare(b) != 0; }
    boolean operator<=(Quickset const &b) const { return subset(b); }
    boolean operator<(Quickset const &b) const
	    { return subset(b) && compare(b) != 0; }
    boolean operator>=(Quickset const &b) const { return b.subset(*this); }
    boolean operator>(Quickset const &b) const
	    { return b.subset(*this) && compare(b) != 0; }
    Quickset operator~() const { Quickset x = *this; x.b_not(); return x; }
    Quickset operator+(Quickset const &b) const
	    { Quickset x = *this; x.b_or(b); return x; }
    Quickset operator-(Quickset const &b) const
	    { Quickset x = *this; x -= b; return x; }
    Quickset operator&(Quickset const &b) const
	    { Quickset x = *this; x.b_and(b); return x; }
    Quickset operator|(Quickset const &b) const
	    { Quickset x = *this; x.b_or(b); return x; }
    Quickset operator^(Quickset const &b) const
	    { Quickset x = *this; x.b_xor(b); return x; }
    boolean operator[](int i) { return has(i); }
};

#endif // __QUICKSET_H_
