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


#include <stdlib.h>
#include <stdlibx.h>
#include <ctype.h>
#include <limits.h>
//#include <integer.h>

#include "floatc.h"

#define ALLONES	(~0UL)

void
Float::copyexp(Float const &f)
{
    if (_fc == f._fc)
    {
	int i = _fc->elind();
	_vec[i] = (_vec[i] & ~_fc->elmask()) | (f._vec[i] & _fc->elmask());

	for (i++; i < _fc->ehind(); i++)
	    _vec[i] = f._vec[i];

	if (i == _fc->ehind())
	    _vec[i] = (_vec[i] & ~_fc->ehmask()) | (f._vec[i] & _fc->ehmask());

    	return;
    }

    for (int i = 0; i < _fc->esize(); i++)
    	sete(i, f.gete(i));
}

void
Float::copymant(Float const &f)
{
    if (_fc == f._fc)
    {
	int i = _fc->mlind();
	_vec[i] = (_vec[i] & ~_fc->mlmask()) | (f._vec[i] & _fc->mlmask());

	for (i++; i < _fc->mhind(); i++)
	    _vec[i] = f._vec[i];

	if (i == _fc->mhind())
	    _vec[i] = (_vec[i] & ~_fc->mhmask()) | (f._vec[i] & _fc->mhmask());

	return;
    }

    for (int i = 0; i < _fc->msize(); i++)
    	setm(i, f.getm(i));
}

boolean
Float::next()
{
    enum FloatType t = floattype();

    if (t == FloatNaN || t == FloatQuietNaN ||
	    t == FloatNegInf || t == FloatPosInf)
    	return FALSE;

    boolean carry = TRUE;
    int i;

    for (i = 0; carry && i < _fc->ehind(); i++)
    {
    	uLong sum = _vec[i] + carry;
	carry = carryf(sum, _vec[i], carry);
	_vec[i] = sum;
    }

    if (carry && i == _fc->ehind())
    {
	uLong mask = _fc->ehmask();

	if (i == _fc->mhind())
	    mask |= _fc->mhmask();

	uLong sum = _vec[i] + carry;
	_vec[i] = (_vec[i] & ~mask) | (sum & mask);
    }

    t = floattype();
    return t == FloatPosInf || t == FloatNegInf;
}

boolean
Float::prev()
{
    enum FloatType t = floattype();

    if (t == FloatNaN || t == FloatQuietNaN ||
	    t == FloatNegZero || t == FloatPosZero)
    	return FALSE;

    boolean zero = TRUE;
    boolean borrow = TRUE;
    int i;

    for (i = 0; borrow && i < _fc->ehind(); i++)
    {
    	uLong sum = _vec[i] - borrow;
	borrow = borrowf(sum, _vec[i], borrow);
	_vec[i] = sum;

	if (sum)
	    zero = FALSE;
    }

    for (; zero && i < _fc->ehind(); i++)
    	if (_vec[i])
	    zero = FALSE;

    if (i == _fc->ehind())
    {
	uLong mask = _fc->ehmask();

	if (i == _fc->mhind())
	    mask |= _fc->mhmask();

    	if (borrow)
	{
	    uLong sum = (_vec[i] & mask) - borrow;
	    _vec[i] = (_vec[i] & ~mask) | (sum & mask);

	    if (sum & mask)
		zero = FALSE;
	}
	else if (_vec[i] & mask)
	    zero = FALSE;
    }

    return zero;
}

boolean
Float::incexp(int amount)
{
    if (amount < 0)
    	return decexp(-amount);

    int carry = amount;
    boolean allones = TRUE;
    uLong sum = ALLONES;

    for (int i = 0; i < _fc->esize(); i++)
    {
    	if (sum != ALLONES)
	    allones = FALSE;

    	uLong v = gete(i);
	sum = v + carry;
	carry = carryf(sum, v, carry);
	sete(i, sum);
    }

    // check to see if the exponent has overflowed
    if (sum != _fc->efmask())
	allones = FALSE;

    if (sum & ~_fc->efmask())
	carry = 1;

    return carry || allones;
}

boolean
Float::decexp(int amount)
{
    if (amount < 0)
	return incexp(-amount);

    int borrow = amount;
    boolean allzero = TRUE;

    for (int i = 0; i < _fc->esize(); i++)
    {
    	uLong v = gete(i);
	uLong sum = v - borrow;
	borrow = borrowf(sum, v, borrow);
	sete(i, sum);

	if (sum)
	    allzero = FALSE;
    }

    return borrow || allzero;
}

int
Float::ebits() const
{
    int hind = _fc->ehind();
    uLong hmask = _fc->ehmask();
    int lind = _fc->elind();
    int lbits = BITSPERLONG - _fc->elshift();

    if (_vec[hind] & hmask)
	if (hind == lind)
	    return bits((_vec[hind] & hmask) >> _fc->elshift());
	else
	    return bits(_vec[hind] & hmask) + (hind - lind - 1) * BITSPERLONG +
		    lbits;

    int i;

    for (i = hind - 1; i > lind; i--)
    	if (_vec[i])
	    return bits(_vec[i]) + (i - lind - 1) * BITSPERLONG + lbits;

    if (i == lind)
    	return bits(_vec[lind] >> _fc->elshift());

    return 0;
}

int
Float::mbits() const
{
    int hind = _fc->mhind();
    uLong hmask = _fc->mhmask();

    if (_vec[hind] & hmask)
    	return bits(_vec[hind] & hmask) + hind * BITSPERLONG;

    for (int i = hind - 1; i >= 0; i--)
    	if (_vec[i])
	    return bits(_vec[i]) + i * BITSPERLONG;

    return 0;
}

uLong
Float::rshift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = fc.ehind();
    uLong hmask = fc.ehmask();

    if (hind == fc.mhind())
    	hmask |= fc.mhmask();

    uLong hbits = _vec[hind] & hmask;
    uLong out;

    if (x == 0)
    {
	if (hind)
	    out = _vec[0] << br;
	else
	    out = hbits << br;

	int i;

	// this is the main shift loop
	for (i = 0; i + 1 < hind; i++)
	    _vec[i] = (_vec[i] >> b) | (_vec[i + 1] << br);

	if (i + 1 == hind)
	    _vec[hind - 1] = (_vec[hind - 1] >> b) | (hbits << br);

	_vec[hind] &= ~hmask;
	_vec[hind] |= hbits >> b;
	return out;
    }

    if (b == 0)
    {
	if (x <= hind)
	    out = _vec[x - 1];
	else if (x == hind + 1)
	    out = hbits;
	else 
	    out = 0UL;

	int i;

	for (i = 0; i < x - 1; i++)
	    if (_vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < hind; i++)
	    _vec[i] = _vec[i + x];

	if (i + x == hind)
	    _vec[i++] = hbits >> b;

	for (; i < hind; i++)
	    _vec[i] = 0UL;

	_vec[hind] &= ~hmask;
	return out;
    }

    if (x < hind)
    {
	if (x == 0)
	    out = _vec[x] << br;
	else
	    out = (_vec[x - 1] >> b) | (_vec[x] << br);
    }
    else if (x == hind)
    {
    	if (hind)
	    out = (_vec[hind - 1] >> b) | (hbits << br);
	else
	    out = hbits << br;
    }
    else if (x == hind + 1)
	out = hbits >> b;
    else 
    	out = 0UL;

    int i;

    for (i = 0; i + 1 < x; i++)
    	if (_vec[i])
	{
	    out |= 1;
	    break;
	}

    if (x && _vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < hind; i++)
	_vec[i] = (_vec[i + x] >> b) | (_vec[i + x + 1] << br);

    if (i + x + 1 == hind)
	_vec[i++] = (_vec[hind - 1] >> b) | (hbits << br);

    if (i + x == hind)
	_vec[i++] = hbits >> b;

    for (; i < hind; i++)
    	_vec[i] = 0UL;

    _vec[hind] &= ~hmask;

    if (x == 0)
	_vec[hind] |= hbits >> b;

    return out;
}

uLong
Float::lshift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = fc.ehind();
    int hshift = fc.ehshift();
    uLong hmask = fc.ehmask();

    if (hind == fc.mhind())
    	hmask |= fc.mhmask();

    uLong hbits = _vec[hind] & hmask;
    uLong out;
    uLong v;

    if (x == 0)
    {
	out = hbits >> br;

	if (hind)
	    v = (hbits << b) | (_vec[hind - 1] >> br);
	else
	    v = hbits << b;

	_vec[hind] &= ~hmask;
	_vec[hind] |= v & hmask;

	if (hshift)
	{
	    v = (out << hshift) | (v >> (BITSPERLONG - hshift));

	    if (out >> (BITSPERLONG - hshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	int i;

	// this is the main shift loop
	for (i = hind - 1; i > 0; i--)
	    _vec[i] = (_vec[i] << b) | (_vec[i - 1] >> br);

	if (i == 0)
	    _vec[0] <<= b;

	return out;
    }

    if (b == 0)
    {
	if (hind - x < -1)
	    out = 0UL;
	else if (hind - x == -1)
	{
	    if (hind)
		out = _vec[0];
	    else
		out = hbits;
	}
	else if (x == 1)
	    out = hbits;
	else
	    out = _vec[hind - x + 1];

	int i;

	for (i = hind - x + 2; i < hind; i++)
	    if (_vec[i])
	    {
		out |= LONGMSB;
		break;
	    }

	if (i == hind && hbits)
	    out |= LONGMSB;

	if (hind - x < 0)
	    v = 0UL;
	else if (hind - x == 0)
	{
	    if (hind)
		v = _vec[0];
	    else
		v = hbits;
	}
	else 
	    v = _vec[hind - x];

	_vec[hind] &= ~hmask;
	_vec[hind] |= v & hmask;

	if (hshift)
	{
	    v = (out << hshift) | (v >> (BITSPERLONG - hshift));

	    if (out >> (BITSPERLONG - hshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	// this is the main shift loop
	for (i = hind - 1; i >= x; i--)
	    _vec[i] = _vec[i - x];

	for (; i >= 0; i--)
	    _vec[i] = 0UL;

	return out;
    }

    if (hind - x < -1)
	out = 0UL;
    else if (hind - x == -1)
    {
    	if (hind)
	    out = _vec[0] << b;
	else
	    out = hbits << b;
    }
    else if (x == 1)
	out = (hbits << b) | (_vec[hind - 1] >> br);
    else
	out = (_vec[hind - x + 1] << b) | (_vec[hind - x] >> br);

    if (x > 1)
    {
	if (_vec[hind - x + 1] >> br)
	    out |= LONGMSB;
    }
    if (x == 1)
    {
	if (hbits >> br)
	    out |= LONGMSB;
    }

    int i;

    for (i = hind - x + 2; i < hind; i++)
    	if (_vec[i])
	{
	    out |= LONGMSB;
	    break;
	}

    if (i == hind && hbits)
    	out |= LONGMSB;

    if (hind - x < 0)
	v = 0UL;
    else if (hind - x == 0)
    {
    	if (hind)
	    v = _vec[0] << b;
	else
	    v = hbits << b;
    }
    else 
	v = (_vec[hind - x] << b) | (_vec[hind - x - 1] >> br);

    _vec[hind] &= ~hmask;
    _vec[hind] |= v & hmask;

    if (hshift)
    {
	v = (out << hshift) | (v >> (BITSPERLONG - hshift));

    	if (out >> (BITSPERLONG - hshift))
	    out = v | LONGMSB;
	else
	    out = v;
    }

    // this is the main shift loop
    for (i = hind - 1; i > x; i--)
	_vec[i] = (_vec[i - x] << b) | (_vec[i - x - 1] >> br);

    if (i == x)
	_vec[i--] = _vec[0] << b;

    for (; i >= 0; i--)
	_vec[i] = 0UL;

    return out;
}

uLong
Float::mrshift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int mhind = fc.mhind();
    uLong mhmask = fc.mhmask();
    uLong hbits = _vec[mhind] & mhmask;
    uLong out;

    if (x == 0)
    {
	if (mhind)
	    out = _vec[0] << br;
	else
	    out = hbits << br;

	int i;

	// this is the main shift loop
	for (i = 0; i + 1 < mhind; i++)
	    _vec[i] = (_vec[i] >> b) | (_vec[i + 1] << br);

	if (i + 1 == mhind)
	    _vec[mhind - 1] = (_vec[mhind - 1] >> b) | (hbits << br);

	_vec[mhind] &= ~mhmask;
	_vec[mhind] |= hbits >> b;
	return out;
    }

    if (b == 0)
    {
	if (x <= mhind)
	    out = _vec[x - 1];
	else if (x == mhind + 1)
	    out = hbits;
	else 
	    out = 0UL;

	int i;

	for (i = 0; i < x - 1; i++)
	    if (_vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < mhind; i++)
	    _vec[i] = _vec[i + x];

	if (i + x == mhind)
	    _vec[i++] = hbits >> b;

	for (; i < mhind; i++)
	    _vec[i] = 0UL;

	_vec[mhind] &= ~mhmask;
	return out;
    }

    if (x < mhind)
    {
	if (x == 0)
	    out = _vec[x] << br;
	else
	    out = (_vec[x - 1] >> b) | (_vec[x] << br);
    }
    else if (x == mhind)
    {
    	if (mhind)
	    out = (_vec[mhind - 1] >> b) | (hbits << br);
	else
	    out = hbits << br;
    }
    else if (x == mhind + 1)
	out = hbits >> b;
    else 
    	out = 0UL;

    int i;

    for (i = 0; i + 1 < x; i++)
    	if (_vec[i])
	{
	    out |= 1;
	    break;
	}

    if (x && _vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < mhind; i++)
	_vec[i] = (_vec[i + x] >> b) | (_vec[i + x + 1] << br);

    if (i + x + 1 == mhind)
	_vec[i++] = (_vec[mhind - 1] >> b) | (hbits << br);

    if (i + x == mhind)
	_vec[i++] = hbits >> b;

    for (; i < mhind; i++)
    	_vec[i] = 0UL;

    _vec[mhind] &= ~mhmask;

    if (x == 0)
	_vec[mhind] |= hbits >> b;

    return out;
}

uLong
Float::mlshift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int mhind = fc.mhind();
    int mhshift = fc.mhshift();
    uLong mhmask = fc.mhmask();
    uLong hbits = _vec[mhind] & mhmask;
    uLong out;
    uLong v;

    if (x == 0)
    {
	out = hbits >> br;

	if (mhind)
	    v = (hbits << b) | (_vec[mhind - 1] >> br);
	else
	    v = hbits << b;

	_vec[mhind] &= ~mhmask;
	_vec[mhind] |= v & mhmask;

	if (mhshift)
	{
	    v = (out << mhshift) | (v >> (BITSPERLONG - mhshift));

	    if (out >> (BITSPERLONG - mhshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	int i;

	// this is the main shift loop
	for (i = mhind - 1; i > 0; i--)
	    _vec[i] = (_vec[i] << b) | (_vec[i - 1] >> br);

	if (i == 0)
	    _vec[0] <<= b;

	return out;
    }

    if (b == 0)
    {
	if (mhind - x < -1)
	    out = 0UL;
	else if (mhind - x == -1)
	{
	    if (mhind)
		out = _vec[0];
	    else
		out = hbits;
	}
	else if (x == 1)
	    out = hbits;
	else
	    out = _vec[mhind - x + 1];

	int i;

	for (i = mhind - x + 2; i < mhind; i++)
	    if (_vec[i])
	    {
		out |= LONGMSB;
		break;
	    }

	if (i == mhind && hbits)
	    out |= LONGMSB;

	if (mhind - x < 0)
	    v = 0UL;
	else if (mhind - x == 0)
	{
	    if (mhind)
		v = _vec[0];
	    else
		v = hbits;
	}
	else 
	    v = _vec[mhind - x];

	_vec[mhind] &= ~mhmask;
	_vec[mhind] |= v & mhmask;

	if (mhshift)
	{
	    v = (out << mhshift) | (v >> (BITSPERLONG - mhshift));

	    if (out >> (BITSPERLONG - mhshift))
		out = v | LONGMSB;
	    else
		out = v;
	}

	// this is the main shift loop
	for (i = mhind - 1; i >= x; i--)
	    _vec[i] = _vec[i - x];

	for (; i >= 0; i--)
	    _vec[i] = 0UL;

	return out;
    }

    if (mhind - x < -1)
	out = 0UL;
    else if (mhind - x == -1)
    {
    	if (mhind)
	    out = _vec[0] << b;
	else
	    out = hbits << b;
    }
    else if (x == 1)
	out = (hbits << b) | (_vec[mhind - 1] >> br);
    else
	out = (_vec[mhind - x + 1] << b) | (_vec[mhind - x] >> br);

    if (x > 1)
    {
	if (_vec[mhind - x + 1] >> br)
	    out |= LONGMSB;
    }
    if (x == 1)
    {
	if (hbits >> br)
	    out |= LONGMSB;
    }

    int i;

    for (i = mhind - x + 2; i < mhind; i++)
    	if (_vec[i])
	{
	    out |= LONGMSB;
	    break;
	}

    if (i == mhind && hbits)
    	out |= LONGMSB;

    if (mhind - x < 0)
	v = 0UL;
    else if (mhind - x == 0)
    {
    	if (mhind)
	    v = _vec[0] << b;
	else
	    v = hbits << b;
    }
    else 
	v = (_vec[mhind - x] << b) | (_vec[mhind - x - 1] >> br);

    _vec[mhind] &= ~mhmask;
    _vec[mhind] |= v & mhmask;

    if (mhshift)
    {
	v = (out << mhshift) | (v >> (BITSPERLONG - mhshift));

    	if (out >> (BITSPERLONG - mhshift))
	    out = v | LONGMSB;
	else
	    out = v;
    }

    // this is the main shift loop
    for (i = mhind - 1; i > x; i--)
	_vec[i] = (_vec[i - x] << b) | (_vec[i - x - 1] >> br);

    if (i == x)
	_vec[i--] = _vec[0] << b;

    for (; i >= 0; i--)
	_vec[i] = 0UL;

    return out;
}

uLong
Float::ershift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int elind = fc.elind();
    int ehind = fc.ehind();
    int elshift = fc.elshift();
    uLong elmask = fc.elmask();
    uLong ehmask = fc.ehmask();
    uLong hbits = _vec[ehind] & ehmask;
    uLong out;

    if (elind == ehind)
    {
    	if (x == 1)
	{
	    uLong v = (_vec[elind] & elmask) >> elshift;
	    uLong vx = v >> b;

	    if (b && v << br)
	    	vx |= 1;

	    _vec[elind] &= ~elmask;
	    return vx;
	}
	else if (x)
	{
	    _vec[elind] &= ~elmask;
	    return 1UL;
	}

    	uLong v = (_vec[elind] & elmask) >> elshift;
	uLong vs = v >> b;
    	_vec[elind] &= ~elmask;
	_vec[elind] |= (vs << elshift) & elmask;
	return v << br;
    }

    if (x == 0)
    {
	if (ehind)
	    out = _vec[0] << br;
	else
	    out = hbits << br;

	int i;

	// this is the main shift loop
	for (i = elind; i + 1 < ehind; i++)
	    _vec[i] = (_vec[i] >> b) | (_vec[i + 1] << br);

	if (i + 1 == ehind)
	    _vec[ehind - 1] = (_vec[ehind - 1] >> b) | (hbits << br);

	_vec[ehind] &= ~ehmask;
	_vec[ehind] |= hbits >> b;
	return out;
    }

    if (b == 0)
    {
	if (x <= ehind)
	    out = _vec[x - 1];
	else if (x == ehind + 1)
	    out = hbits;
	else 
	    out = 0UL;

	int i;

	for (i = 0; i < x - 1; i++)
	    if (_vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < ehind; i++)
	    _vec[i] = _vec[i + x];

	if (i + x == ehind)
	    _vec[i++] = hbits >> b;

	for (; i < ehind; i++)
	    _vec[i] = 0UL;

	_vec[ehind] &= ~ehmask;
	return out;
    }

    if (x < ehind)
    {
	if (x == 0)
	    out = _vec[x] << br;
	else
	    out = (_vec[x - 1] >> b) | (_vec[x] << br);
    }
    else if (x == ehind)
    {
    	if (ehind)
	    out = (_vec[ehind - 1] >> b) | (hbits << br);
	else
	    out = hbits << br;
    }
    else if (x == ehind + 1)
	out = hbits >> b;
    else 
    	out = 0UL;

    int i;

    for (i = 0; i + 1 < x; i++)
    	if (_vec[i])
	{
	    out |= 1;
	    break;
	}

    if (x && _vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < ehind; i++)
	_vec[i] = (_vec[i + x] >> b) | (_vec[i + x + 1] << br);

    if (i + x + 1 == ehind)
	_vec[i++] = (_vec[ehind - 1] >> b) | (hbits << br);

    if (i + x == ehind)
	_vec[i++] = hbits >> b;

    for (; i < ehind; i++)
    	_vec[i] = 0UL;

    _vec[ehind] &= ~ehmask;

    if (x == 0)
	_vec[ehind] |= hbits >> b;

    return out;
}

#if 0
uLong
Float::ershift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    uLong lowbits = 0UL;

    if (b >= BITSPERLONG)
    {
    	lowbits = ershift(b - (BITSPERLONG - 1)) ? 1UL : 0UL;
	b = BITSPERLONG - 1;
    }

    int br = BITSPERLONG - b;
    uLong carry;

    int i = fc.ehind();

    carry = (_vec[i] & fc.ehmask()) << br;
    _vec[i] = (_vec[i] & ~fc.ehmask()) | ((_vec[i] & fc.ehmask()) >> b);

    for (i--; i > fc.elind(); i--)
    {
	uLong carryout = _vec[i] << br;
	_vec[i] = (_vec[i] >> b) | carry;
	carry = carryout;
    }

    if (i == fc.elind())
    {
    	uLong carryout = (_vec[i] & fc.elmask()) << br;
	_vec[i] = (_vec[i] & ~fc.elmask()) |
		(((_vec[i] >> b) | carry) & fc.elmask());
	carry = carryout;
    }

    return carry | lowbits;
}
#endif

uLong
Float::elshift(int b)
{
    if (b <= 0)
	return 0UL;

    FloatClass &fc = *_fc;
    uLong highbits = 0UL;

    if (b >= BITSPERLONG)
    {
    	if (elshift(b - (BITSPERLONG - 1)))
	    highbits = LONGMSB;

	b = BITSPERLONG - 1;
    }

    int br = BITSPERLONG - b;
    uLong carry;

    int i = fc.elind();

    carry = (_vec[i] & fc.elmask()) >> br;
    _vec[i] = (_vec[i] & ~fc.elmask()) | ((_vec[i] & fc.elmask()) << b);

    for (i++; i < fc.ehind(); i++)
    {
	uLong carryout = _vec[i] >> br;
	_vec[i] = (_vec[i] << b) | carry;
	carry = carryout;
    }

    if (i == fc.ehind())
    {
    	uLong carryout = (_vec[i] & fc.ehmask()) >> br;
	_vec[i] = (_vec[i] & ~fc.ehmask()) |
		(((_vec[i] << b) | carry) & fc.ehmask());
	carry = carryout;
    }

    return carry | highbits;
}

void
Float::band(Float const &f)
{
    for (int i = 0; i <= _fc->sind(); i++)
    	_vec[i] &= f._vec[i];
}

void
Float::mand(Float const &f)
{
    int i;
    
    for (i = 0; i < _fc->mhind(); i++)
	_vec[i] &= f._vec[i];

    _vec[i] &= (_vec[i] & ~_fc->mhmask()) | (f._vec[i] & _fc->mhmask());
}

void
Float::eand(Float const &f)
{
    int i = _fc->elind();
    _vec[i] &= (_vec[i] & ~_fc->elmask()) | (f._vec[i] & _fc->elmask());

    for (i++; i < _fc->ehind(); i++)
	_vec[i] &= f._vec[i];

    if (i == _fc->mhind())
	_vec[i] &= (_vec[i] & ~_fc->ehmask()) | (f._vec[i] & _fc->ehmask());
}

void
Float::bor(Float const &f)
{
    for (int i = 0; i <= _fc->sind(); i++)
    	_vec[i] |= f._vec[i];
}

void
Float::mor(Float const &f)
{
    int i;
    
    for (i = 0; i < _fc->mhind(); i++)
	_vec[i] |= f._vec[i];

    _vec[i] |= f._vec[i] & _fc->mhmask();
}

void
Float::eor(Float const &f)
{
    int i = _fc->elind();
    _vec[i] |= f._vec[i] & _fc->elmask();

    for (i++; i < _fc->ehind(); i++)
	_vec[i] |= f._vec[i];

    if (i == _fc->mhind())
	_vec[i] |= f._vec[i] & _fc->ehmask();
}

void
Float::bcompl()
{
    for (int i = 0; i <= _fc->sind(); i++)
    	_vec[i] = ~_vec[i];

    _vec[_fc->sind()] &= _fc->smask() | (_fc->smask() - 1);
}

void
Float::mcompl()
{
    int i;
    
    for (i = 0; i < _fc->mhind(); i++)
	_vec[i] = ~_vec[i];

    _vec[i] = (_vec[i] & ~_fc->mhmask()) | (~_vec[i] & _fc->mhmask());
}

void
Float::ecompl()
{
    int i = _fc->elind();
    _vec[i] = (_vec[i] & ~_fc->elmask()) | (~_vec[i] & _fc->elmask());

    for (i++; i < _fc->ehind(); i++)
	_vec[i] = ~_vec[i];

    if (i == _fc->ehind())
	_vec[i] = (_vec[i] & ~_fc->ehmask()) | (~_vec[i] & _fc->ehmask());
}

uLong
Float::getm(int i) const
{
    if (i < 0)
    	return 0UL;

    if (i < _fc->mhind())
    	return _vec[i];

    if (i == _fc->mhind())
    	return _vec[i] & _fc->mhmask();

    return 0UL;
}

void
Float::set(int i, uLong v)
{
    if (i < 0)
    	return;

    if (i < _fc->sind())
    {
    	_vec[i] = v;
	return;
    }

    if (i == _fc->sind())
	_vec[i] = v & (_fc->smask() | (_fc->smask() - 1));
}

void
Float::setm(int i, uLong v)
{
    if (i < 0)
    	return;

    if (i < _fc->mhind())
    {
    	_vec[i] = v;
	return;
    }

    if (i == _fc->mhind())
    {
    	_vec[i] &= ~_fc->mhmask();
	_vec[i] |= v & _fc->mhmask();
    }
}

uLong
Float::gete(int i) const
{
    if (i < 0)
    	return 0UL;

    int hind = _fc->ehind() - _fc->elind();

    if (!_fc->elshift())
    {
	if (i < hind)
	    return _vec[_fc->elind() + i];

	if (i == hind)
	    return _vec[_fc->ehind()] & _fc->ehmask();

    	return 0UL;
    }

    if (_fc->ehshift() >= _fc->mhshift())
	hind--;

    if (i < hind)
	return _vec[_fc->elind() + i] >> _fc->elshift() |
		    _vec[_fc->elind() + i + 1] << _fc->mhshift();

    if (i != hind)
    	return 0UL;

    if (_fc->elind() + i == _fc->ehind())
	return (_vec[_fc->ehind()] & _fc->ehmask()) >> _fc->elshift();

    return _vec[_fc->ehind() - 1] >> _fc->elshift() |
	    (_vec[_fc->ehind()] & _fc->ehmask()) << _fc->mhshift();
}

void
Float::sete(int i, uLong v)
{
    if (i < 0)
    	return;

    int hind = _fc->ehind() - _fc->elind();

    if (!_fc->elshift())
    {
	if (i < hind)
	{
	    _vec[_fc->elind() + i] = v;
	    return;
	}

	if (i == hind)
	{
	    _vec[_fc->ehind()] &= ~_fc->ehmask();
	    _vec[_fc->ehind()] |= v & _fc->ehmask();
	}

	return;
    }

    if (_fc->ehshift() >= _fc->mhshift())
	hind--;

    if (i < hind)
    {
	_vec[_fc->elind() + i] &= ~_fc->elmask();
	_vec[_fc->elind() + i] |= v << _fc->elshift();
	_vec[_fc->elind() + i + 1] &= ~_fc->mhmask();
	_vec[_fc->elind() + i + 1] |= v >> _fc->mhshift();
	return;
    }

    if (i != hind)
    	return;

    if (_fc->elind() + i == _fc->ehind())
    {
	_vec[_fc->ehind()] &= ~_fc->ehmask() | ~_fc->elmask();
	_vec[_fc->ehind()] |= (v << _fc->elshift()) & _fc->ehmask();
    }
    else
    {
	_vec[_fc->elind() + i] &= ~_fc->elmask();
	_vec[_fc->elind() + i] |= v << _fc->elshift();
	_vec[_fc->ehind()] &= ~_fc->ehmask();
	_vec[_fc->ehind()] |= (v >> _fc->mhshift()) & _fc->ehmask();
    }
}

void
Float::setbit(int i, boolean v)
{
    if (i < 0)
	return;

    int ind = i / BITSPERLONG;
    uLong mask = 1UL << (i % BITSPERLONG);

    if (v)
	set(ind, get(ind) | mask);
    else
	set(ind, get(ind) & ~mask);
}

void
Float::setmbit(int i, boolean v)
{
    if (i < 0)
    	return;

    int ind = i / BITSPERLONG;
    uLong mask = 1UL << (i % BITSPERLONG);

    if (v)
	setm(ind, getm(ind) | mask);
    else
	setm(ind, getm(ind) & ~mask);
}

void
Float::setebit(int i, boolean v)
{
    if (i < 0)
	return;

    int ind = i / BITSPERLONG;
    uLong mask = 1UL << (i % BITSPERLONG);

    if (v)
	sete(ind, gete(ind) | mask);
    else
	sete(ind, gete(ind) & ~mask);
}

//boolean
//Float::madd(Float const &f, uLong carryin = 0UL)
//{
//    uLong a = f._vec[0] & _fc->mlmask();
//    uLong b = f._vec[0] & _fc->mlmask();
//    uLong sum = a + b;
//    boolean carry = carryf(sum, a, b);
//    uLong sum2 = sum + carryin;
//    boolean carry2 = carryf(sum2, sum, carryin);
//
//    if (_fc->mantissa() == 32)
//    {
//	_vec[0] = sum2;
//	return carry || carry2;
//    }
//
//    if (_fc->msize() <= 1)
//    {
//	carry = (carry || carry2 || (sum2 & ~_fc->mlmask()));
//	_vec[0] = (_vec[0] & ~_fc->mlmask()) | (sum2 & _fc->mlmask());
//	return carry;
//    }
//
//    _vec[0] = sum2;
//    carryin = carry + carry2;
//
//    for (int i = 1; i < _fc->msize(); i++)
//    {
//	a = f._vec[i] & _fc->mlmask();
//	b = f._vec[i] & _fc->mlmask();
//	sum = a + b;
//	carry = carryf(sum, a, b);
//	_vec[i] = sum + carryin;
//	carryin = carry + carryf(_vec[i], sum, carryin);
//    }
//
//    a = f._vec[i] & _fc->mlmask();
//    b = f._vec[i] & _fc->mlmask();
//    sum = a + b;
//    carry = carryf(sum, a, b);
//    sum2 = sum + carryin;
//    carry2 = carryf(sum2, sum, carryin);
//    carry = (carry || carry2 || (sum2 & ~_fc->mhmask()));
//    _vec[i] = (_vec[i] & ~_fc->mhmask()) | (sum2 & _fc->mhmask());
//    return carry;
//}

boolean
Float::madd(Float const &f, uLong carryin)
{
    uLong sum2 = 0;

    for (int i = 0; i < _fc->msize(); i++)
    {
	uLong a = getm(i);
	uLong b = f.getm(i);
	uLong sum = a + b;
	boolean carry = carryf(sum, a, b);
	sum2 = sum + carryin;
	setm(i, sum2);
	carryin = carry + carryf(sum2, sum, carryin);
    }

    return carryin || (sum2 & ~_fc->mfmask());
}

//boolean
//Float::madd(Float const &f, uLong)
//{
//    uLong sum;
//    boolean carry = FALSE;
//
//    for (int i = 0; i < _fc->msize(); i++)
//    {
//	uLong a = getm(i);
//	uLong b = f.getm(i);
//	sum = a + b + carry;
//	carry = carryf(sum, a, b);
//	setm(i, sum);
//    }
//
//    return carry || (sum & ~_fc->mfmask());
//}

boolean
Float::eadd(Float const &f, uLong carryin)
{
    uLong sum2 = 0;

    for (int i = 0; i < _fc->esize(); i++)
    {
	uLong a = gete(i);
	uLong b = f.gete(i);
	uLong sum = a + b;
	boolean carry = carryf(sum, a, b);
	sum2 = sum + carryin;
	sete(i, sum2);
	carryin = carry + carryf(sum2, sum, carryin);
    }

    return carryin || (sum2 & ~_fc->efmask());
}

boolean
Float::msub(Float const &f, uLong borrowin)
{
    uLong diff2 = 0;

    for (int i = 0; i < _fc->msize(); i++)
    {
	uLong a = getm(i);
	uLong b = f.getm(i);
	uLong diff = a - b;
	boolean borrow = borrowf(diff, a, b);
	diff2 = diff - borrowin;
	setm(i, diff2);
	borrowin = borrow + borrowf(diff2, diff, borrowin);
    }

    return borrowin || (diff2 & ~_fc->mfmask());
}

boolean
Float::esub(Float const &f, uLong borrowin)
{
    uLong diff2 = 0;

    for (int i = 0; i < _fc->esize(); i++)
    {
	uLong a = gete(i);
	uLong b = f.gete(i);
	uLong diff = a - b;
	boolean borrow = borrowf(diff, a, b);
	diff2 = diff - borrowin;
	sete(i, diff2);
	borrowin = borrow + borrowf(diff2, diff, borrowin);
    }

    return borrowin || (diff2 & ~_fc->efmask());
}

int
Float::expdelta(Float const &f) const
{
    boolean borrow = FALSE;
    Long delta = 0;

    for (int i = 0; i < _fc->esize(); i++)
    {
	uLong a = gete(i);
	uLong b = f.gete(i);
	uLong t = a - b;
	boolean borrowout = borrowf(t, a, b);
	uLong diff = t - borrow;
	borrow = borrowout || borrowf(diff, t, borrow);

	if (i == 0)
	    delta = diff;
	else
	{
	    if (borrow && diff != ALLONES)
		return INT_MIN;

	    if (!borrow && diff != 0UL)
	    	return INT_MAX;
	}
    }

#if INT_MAX < LONG_MAX
    if (delta >= INT_MAX)
	return INT_MAX;

    if (delta <= INT_MIN)
	return INT_MIN;
#endif

    return (int)delta;
}

boolean
Float::roundup(uLong lowbits)
{
    if (lowbits == 0UL)
	return FALSE;

    boolean overflow = FALSE;

    switch (_fc->roundmode())
    {
    // lowbits = 0		exact result
    // lowbits < 0x8....	RoundToInf
    // lowbits == 0x8....	RoundToInf or RoundUp if odd
    // lowbits > 0x8...		RoundToInf or RoundUp
    case FloatRoundToNearest:
	if (lowbits < LONGMSB)
	    break;

	if (lowbits != LONGMSB || (_vec[0] & 1))
	    overflow = next();

	break;
    case FloatRoundTowardNegInf:
	if (sign())
	    overflow = next();

	break;
    case FloatRoundTowardPosInf:
	if (!sign())
	    overflow = next();

	break;

    default:
	break;
    }

    return overflow;
}

boolean
Float::rounddown(uLong lowbits)
{
    if (lowbits == 0UL)
	return FALSE;

    boolean underflow = FALSE;

    switch (_fc->roundmode())
    {
    // lowbits = 0		exact result
    // lowbits < 0x8....	RoundToInf
    // lowbits == 0x8....	RoundToInf or RoundUp if odd
    // lowbits > 0x8...		RoundToInf or RoundUp
    case FloatRoundToNearest:
	if (lowbits < LONGMSB)
	    break;

	if (lowbits != LONGMSB || (_vec[0] & 1))
	    underflow = prev();

	break;
    case FloatRoundTowardNegInf:
	if (!sign())
	    underflow = prev();

	break;
    case FloatRoundTowardPosInf:
	if (sign())
	    underflow = prev();

	break;

    default:
	break;
    }

    return underflow;
}

int
Float::normalize()
{
    // convert a denormalized mantissa to the normalized form
    // returning the exponent adjustment that was required
    int sf = _fc->mantissa() - mbits();
    mlshift(sf + 1);
    return sf;
}
