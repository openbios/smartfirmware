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
#include <limits.h>

#include "floatc.h"

#ifndef __GNUC__
void Float::makezero()
{
    for (int i = 0; i < _fc->size(); i++)
	_vec[i] = 0UL;
}
#endif

// *this = int(x*2^exp)*2^-exp
// return 1 for special value, 0 for no change, -1 for change
boolean
Float::fint(int exp)
{
    enum FloatType type = floattype();

    switch (type)
    {
    case FloatNaN:
    case FloatQuietNaN:
    	return -2;
    case FloatPosInf:
    case FloatNegInf:
    	return -1;
    case FloatPosZero:
    case FloatNegZero:
    	return 0;
    case FloatPositive:
    case FloatPosDenorm:
    case FloatNegDenorm:
    case FloatNegative:
	{	// this block necessary for braindead cfront
	    Float t = _fc->one();
	    boolean borrow = t.esub(*this);
	    boolean carry = FALSE;
	    int bias = _fc->mantissa() - exp;

	    if (bias < 0)
		borrow = t.decexp(-bias) || borrow;
	    else
		carry = t.incexp(bias);

	    if (borrow && carry)
	    {
		borrow = FALSE;
		carry = FALSE;
	    }

	    if (borrow)
		return 0;

	    int bitstoclear = t.expdelta(_fc->zero());

	    if (carry || bitstoclear > _fc->mantissa())
	    {
		// all fraction
		makezero();
		return 1;
	    }

	    boolean zero = TRUE;
	    int i;
    
	    for (i = 0; bitstoclear >= BITSPERLONG;
		    i++, bitstoclear -= BITSPERLONG)
	    {
		if (_vec[i])
		    zero = FALSE;

		_vec[i] = 0UL;
	    }

	    if (bitstoclear)
	    {
		uLong mask = (1UL << bitstoclear) - 1;

		if (_vec[i] & mask)
		    zero = FALSE;

		_vec[i] &= ~mask;
	    }

	    return zero ? 0 : 1;
	}
    }

    return -1;
}

// x = frac * 2 ^ exp
void
Float::frexp(Float *frac, Float *exp) const
{
    enum FloatType type = floattype();

    switch (type)
    {
    case FloatNaN:
    case FloatQuietNaN:
    case FloatPosInf:
    case FloatNegInf:
    case FloatPosZero:
    case FloatNegZero:
    	*frac = *this;
	*exp = _fc->zero();
	return;
    case FloatPositive:
    case FloatNegative:
    	*frac = *this;
	frac->copyexp(_fc->one());
	*exp = *this;
	exp->rshift(_fc->mantissa());
	exp->copyexp(_fc->epsilon());
//	exp->normalize();
	return;
    case FloatPosDenorm:
    case FloatNegDenorm:
    	return;
    }
}

Float
frexp(Float const &x, Float *exp)
{
    Float frac(x.floatclass());
    x.frexp(&frac, exp);
    return frac;
}

Float
ceil(Float const &x)
{
    Float t = x;
    int v = t.fint(0);

    if (v > 0 && t.isPos())
    	t.add(x.floatclass().one());

    return t;
}

Float
floor(Float const &x)
{
    Float t = x;
    int v = t.fint(0);

    if (v > 0 && t.isNeg())
    	t.sub(x.floatclass().one());

    return t;
}

extern Float fmod(Float const &, Float const &);
extern Float frexp(Float const &, Float *);
extern Float ldexp(Float const &, Float const &);

Float
modf(Float const &x, Float *ip)
{
    *ip = x;
    int v = ip->fint(0);

    if (v == -2)
    	return x;

    if (v == -1 || v == 0)
    	return x.floatclass().zero();

    return x - *ip;
}
