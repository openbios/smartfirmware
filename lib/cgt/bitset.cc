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
#include "bitset.h"

void
Bitset::doexpand(int n)
{
    if (n >= _size)
	_size = n + 1;

    int nlen = (n / BITSPERLONG) + 1;

    if (!_user && nlen <= _len)
    	return;

    unsigned long *nbits;

    if (nlen > __BITSET_VEC_LEN)
	nbits = new unsigned long[nlen];
    else
	nbits = _vec;

    int i = 0;

    if (_bits != NULL)
    {
	for (; i < _len; i++)
	    nbits[i] = _bits[i];

	if (!_user && _bits != _vec)
	    delete[/*_len*/] _bits;
    }

    for (; i < nlen; i++)
	nbits[i] = 0;

    _bits = nbits;
    _len = nlen;
    _user = FALSE;
}

void
Bitset::dowritable()
{
    unsigned long *nbits;

    if (_len > __BITSET_VEC_LEN)
	nbits = new unsigned long[_len];
    else
	nbits = _vec;

    for (int i = 0; i < _len; i++)
    	nbits[i] = _bits[i];

    _bits = nbits;
    _user = FALSE;
}

Bitset::Bitset()
{
    _bits = NULL;
    _user = FALSE;
    _len = 0;
    _size = 0;
}

Bitset::Bitset(int s)
{
    _size = ((s < 0) ? 0 : s);
    _len = (_size + BITSPERLONG - 1) / BITSPERLONG;
    _user = FALSE;

    if (_len > __BITSET_VEC_LEN)
	_bits = new unsigned long[_len];
    else
    	_bits = _vec;

    for (int i = 0; i < _len; i++)
    	_bits[i] = 0UL;
}

Bitset::Bitset(Bitset const &b)
{
    _len = b._len;
    _user = b._user;
    _size = b._size;

    if (!_user)
    {
	if (_len > __BITSET_VEC_LEN)
	    _bits = new unsigned long[_len];
	else
	    _bits = _vec;

	for (int i = 0; i < _len; i++)
	    _bits[i] = b._bits[i];
    }
    else
    	_bits = b._bits;
}

Bitset::Bitset(unsigned long const *bits, int len)
{
    _bits = (unsigned long *)bits;
    _len = ((bits == NULL) ? 0 : len);
//    _size = prev(len * BITSPERLONG) + 1;
    _size = len * BITSPERLONG;
    _user = TRUE;
}

Bitset::~Bitset()
{
    if (!_user && _bits != _vec)
    	delete[/*_len*/] _bits;
}

void
Bitset::assign(Bitset const &b)
{
    if (b._user)
    {
	if (!_user && _bits != _vec)
	    delete[/*_len*/] _bits;

	_bits = b._bits;
	_len = b._len;
	_size = b._size;
	_user = TRUE;
	return;
    }

    if (_len < b._len && !_user && _bits != _vec)
	delete[/*_len*/] _bits;

    if (_user || _len < b._len)
    {
	_len = b._len;

	if (_len > __BITSET_VEC_LEN)
	    _bits = new unsigned long[_len];
	else
	    _bits = _vec;
    }

    _size = b._size;
    _user = FALSE;

    int i;
    
    for (i = 0; i < b._len; i++)
	_bits[i] = b._bits[i];

    for (; i < _len; i++)
	_bits[i] = 0UL;
}

int
Bitset::compare(Bitset const &b) const
{
    if (_bits == b._bits && _len == b._len)
	return 0;

    int i;

    for (i = 0; i < _len && i < b._len; i++)
    	if (_bits[i] != b._bits[i])
	{
	    // compare lowest bits that differs
	    unsigned long x = _bits[i] ^ b._bits[i];
	    return (_bits[i] & (x ^ (x - 1))) ? 1 : -1;
	}

    for (; i < b._len; i++)
	if (b._bits[i] != 0UL)
	    return -1;

    for (; i < _len; i++)
	if (_bits[i] != 0UL)
	    return 1;

    return 0;
}

// is set a subset of s?
boolean
Bitset::subset(const Bitset &s) const
{
    int i = _len;

    if (s._len < _len)
	i = s._len;

    for (i--; i >= 0; i--)
	if ((_bits[i] & s._bits[i]) != _bits[i])
	    return FALSE;

    if (_len <= s._len)
	return TRUE;

    for (i = s._len; i < _len; i++)
	if (_bits[i])
	    return FALSE;

    return TRUE;
}

boolean
Bitset::empty() const
{
    for (int i = 0; i < _len; i++)
    	if (_bits[i])
	    return FALSE;

    return TRUE;
}

int
Bitset::count() const
{
    int n = 0;

    for (int i = 0; i < _len; i++)
    	n += bitcount(_bits[i]);
	
    return n;
}

int
Bitset::next(int x) const
{
    x++;
    int i = x / BITSPERLONG;
    int j = x % BITSPERLONG;

    if (i < 0 || i >= _len)
	return -1;

    if (_bits[i] & (1UL << j))
    	return x;

    uLong b = _bits[i] & ~((1UL << j) - 1);

    if (b)
	return i * BITSPERLONG + findlowestbit(b);

    for (i++; i < _len; i++)
    	if (_bits[i])
	    return i * BITSPERLONG + findlowestbit(_bits[i]);

    return -1;
}

int
Bitset::prev(int x) const
{
    x--;
    int i = x / BITSPERLONG;
    int j = x % BITSPERLONG;

    if (i < 0 || i >= _len)
	return -1;

    if (_bits[i] & (1UL << j))
    	return x;

    uLong b = _bits[i] & ((1UL << j) - 1);

    if (b)
	return i * BITSPERLONG + findhighestbit(b);

    for (i--; i >= 0; i--)
    	if (_bits[i])
	    return i * BITSPERLONG + findhighestbit(_bits[i]);

    return -1;
}

void
Bitset::insert(int i)
{
    expand(i);
    _bits[i / BITSPERLONG] |= 1UL << (i % BITSPERLONG);
}

void
Bitset::remove(int i)
{
    writable();
    int ind = i / BITSPERLONG;
    uLong mask = 1UL << (i  % BITSPERLONG);

    if (i >= 0 && ind < _len)
	_bits[ind] &= ~mask;
}

void
Bitset::clear()
{
    if (_user || _bits == _vec)
    {
	_bits = NULL;
	_len = 0;
	_size = 0;
	return;
    }

    int n = (_size + BITSPERLONG - 1) / BITSPERLONG;

    for (int i = 0; i < n; i++)
    	_bits[i] = 0UL;

    _size = 0;
}

void
Bitset::b_or(Bitset const &b)
{
    expand(b._size - 1);

    for (int i = 0; i < _len && i < b._len; i++)
    	_bits[i] |= b._bits[i];
}

void
Bitset::b_and(Bitset const &b)
{
    writable();
    
    int i;

    for (i = 0; i < _len && i < b._len; i++)
    	_bits[i] &= b._bits[i];

    for (; i < _len; i++)
    	_bits[i] = 0UL;
}

void
Bitset::b_xor(Bitset const &b)
{
    expand(b._size - 1);

    for (int i = 0; i < _len && i < b._len; i++)
    	_bits[i] ^= b._bits[i];
}

void
Bitset::b_not()
{
    writable();
    int sind = (_size - 1) / BITSPERLONG;
    int i;

    for (i = 0; i < sind; i++)
    	_bits[i] = ~_bits[i];

    if (i < _len)
    {
	uLong smask  = (1UL << (_size % BITSPERLONG)) - 1;
	_bits[i] = ~_bits[i] & smask;

	for (i++; i < _len; i++)
	    _bits[i] = 0UL;
    }
}

void
Bitset::remove(Bitset const &b)
{
    int i;
    
    for (i = 0; i < b._len; i++)
	if (b._bits[i])
	    break;

    if (i >= b._len)
	return;

    writable();

    for (i = 0; i < _len && i < b._len; i++)
	_bits[i] &= ~b._bits[i];
}

unsigned long
Bitset::hash() const
{
    int len;
    
    for (len = _len; len > 0; len--)
    	if (_bits[len - 1] != 0UL)
	    break;

    unsigned long hash = len;

    for (int i = 0; i < len; i++)
    	hash = (hash << 3) + _bits[i];

    return hash ^ hash >> 16;
}

void
Bitset::set(unsigned long const *bits, int len)
{
    if (!_user && _bits != _vec)
	delete[/*_len*/] _bits;

    _bits = (unsigned long *)bits;
    _len = ((bits == NULL) ? 0 : len);
//    _size = prev(len * BITSPERLONG) + 1;
    _size = len * BITSPERLONG;
    _user = TRUE;
}

void
Bitset::get(unsigned long const *&bits, int &len) const
{
    bits = _bits;
    len = _len;
}

void
Bitset::dump(FILE *fp) const
{
    fprintf(fp, "Dump of bitset %p\n", this);
    fprintf(fp, "\t_bits = %p\n", _bits);
    fprintf(fp, "\t_len = %d (%d)\n", _len, _len * BITSPERLONG);
    fprintf(fp, "\t_size = %d\n", _size);

    for (int i = 0; i < _len; i++)
	fprintf(fp, "\t_bits[%d] = 0x%08lX\n", i, _bits[i]);
}
