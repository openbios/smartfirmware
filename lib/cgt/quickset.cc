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

#include <stdio.h>

#include "stdlibx.h"
#include "quickset.h"

#define INDEXBUMP 64
#define STACKBUMP 16

void 
Quickset::expandindex(int n)
{
    if (n < _indexsz)
    	return;

    if (n < _indexlen)
    {
    	_indexsz = n + 1;
	return;
    }

    int nindexlen = _indexlen;

    while (n >= nindexlen)
    {
	if (_indexlen >= INDEXBUMP)
	{
	    nindexlen = ((n / INDEXBUMP) * INDEXBUMP) + 1;
	    break;
	}

    	nindexlen <<= 1;
    }

    short *nindex = new short[nindexlen];
    int stacksz = _stacksz;

    for (int i = 0; i < stacksz; i++)
    	nindex[_stack[i]] = i;

    delete[/*_indexlen*/] _index;
    _index = nindex;
    _indexlen = nindexlen;
    _indexsz = n + 1;
}

void 
Quickset::expandstack(int n)
{
    if (n < _stacksz)
    	return;

    if (n < _stacklen)
    {
    	_stacksz = n + 1;
	return;
    }

    int nstacklen = _stacklen;

    while (n >= nstacklen)
    {
	if (_stacklen >= STACKBUMP)
	{
	    nstacklen = ((n / STACKBUMP) * STACKBUMP) + 1;
	    break;
	}

    	nstacklen <<= 1;
    }

    int *nstack = new int[nstacklen];
    int stacksz = _stacksz;

    for (int i = 0; i < stacksz; i++)
    	nstack[i] = _stack[i];

    delete[/*_stacklen*/] _stack;
    _stack = nstack;
    _stacklen = nstacklen;
}

Quickset::Quickset()
{
    _index = NULL;
    _indexlen = 0;
    _indexsz = 0;
    _stack = NULL;
    _stacklen = 0;
    _stacksz = 0;
}

Quickset::Quickset(int s)
{
    _index = new short[s];
    _indexlen = s;
    _indexsz = 0;
    _stack = NULL;
    _stacklen = 0;
    _stacksz = 0;
}

Quickset::Quickset(Quickset const &b)
{
    _index = NULL;
    _indexlen = 0;
    _indexsz = 0;

    _stack = new int[b._stacksz];
    _stacklen = b._stacksz;
    _stacksz = 0;

    int n = b._stacksz;

    for (int i = 0; i < n; i++)
    	insert(b._stack[n]);

    _indexsz = b._indexsz;
}

Quickset::~Quickset()
{
    delete[/*_indexlen*/] _index;
    delete[/*_stacklen*/] _stack;
}

void
Quickset::assign(Quickset const &b)
{
    _stacksz = 0;
    int stacksz = b._stacksz;

    if (_stacklen < stacksz)
    {
    	delete[/*_stacklen*/] _stack;
	_stack = new int[stacksz];
	_stacklen = stacksz;
    }

    for (int i = 0; i < stacksz; i++)
    	insert(b._stack[i]);

    _indexsz = b._indexsz;
}

int
Quickset::compare(Quickset const &/*b*/) const
{
    return FALSE;
}

// is set a subset of s?
boolean
Quickset::subset(const Quickset &s) const
{
    int stacksz = _stacksz;

    for (int i = 0; i < stacksz; i++)
    	if (!s.has(_stack[i]))
	    return FALSE;

    return TRUE;
}

boolean
Quickset::empty() const
{
    return _stacksz == 0;
}

int
Quickset::count() const
{
    return _stacksz;
}

boolean
Quickset::has(int i) const
{
    if (i < 0 || i >= _indexsz)
    	return FALSE;

    int ind = _index[i];
    return ind >= 0 && ind < _stacksz && _stack[ind] == i;
}

int 
Quickset::size() const
{
    return _indexsz;
}

int
Quickset::iter(int i) const
{
    if (i < 0 || i >= _stacksz)
    	return -1;

    return _stack[i];
}

int 
Quickset::max() const
{
    if (_stack == NULL || _stacksz == 0)
	return -1;

    int m = _stack[0];

    for (int i = 1; i < _stacksz; i++)
    	if (m < _stack[i])
	    m = _stack[i];

    return m;
}

int 
Quickset::min() const
{
    if (_stack == NULL || _stacksz == 0)
	return -1;

    int m = _stack[0];

    for (int i = 1; i < _stacksz; i++)
    	if (m > _stack[i])
	    m = _stack[i];

    return m;
}

void
Quickset::insert(int i)
{
    if (i >= 0 && i < _indexsz)
    {
    	int ind = _index[i];

	if (ind >= 0 && ind < _stacksz && _stack[ind] == i)
	    return;
    }

    int e = _stacksz++;
    expandindex(i);
    expandstack(e);
    _stack[e] = i;
    _index[i] = e;
}

void
Quickset::remove(int i)
{
    if (i < 0 || i >= _indexsz)
    	return;

    int ind = _index[i];

    if (ind < 0 || ind >= _stacksz || _stack[ind] != i)
    	return;

    int e = --_stacksz;
    int ei = _stack[e];
    _stack[ind] = ei;
    _index[ei] = ind;
}

void
Quickset::clear()
{
    _indexsz = 0;
    _stacksz = 0;
}

void
Quickset::b_or(Quickset const &b)
{
    int stacksz = _stacksz;

    for (int i = 0; i < stacksz; i++)
	insert(b._stack[i]);
}

void
Quickset::b_and(Quickset const &b)
{
    int stacksz = _stacksz;

    for (int i = 0; i < stacksz; i++)
    	if (!b.has(_stack[i]))
	    remove(_stack[i--]);
}

void
Quickset::b_xor(Quickset const &b)
{
    int stacksz = b._stacksz;

    for (int i = 0; i < stacksz; i++)
    {
    	int n = b._stack[i];

	if (has(n))
	    remove(n);
	else
	    insert(n);
    }
}

void
Quickset::b_not()
{
    int *stack = _stack;
    int stacksz = _stacksz;

    _stack = NULL;
    _stacklen = 0;
    _stacksz = 0;

    for (int i = 0; i < _indexsz; i++)
    {
    	int ind = _index[i];

	if (ind < 0 || ind >= stacksz || stack[ind] != i)
	    insert(i);
    }

    delete[/*_stacklen*/] _stack;
}

void
Quickset::remove(Quickset const &b)
{
    for (int i = 0; i < b._stacksz; i++)
    	remove(b._stack[i]);
}

unsigned long
Quickset::hash() const
{
    unsigned long hash = _stacksz;

    for (int i = 0; i < _stacksz; i++)
    	hash += _stack[i];

    return hash ^ hash >> 16;
}

void
Quickset::dump(FILE */*fp*/) const
{
}
