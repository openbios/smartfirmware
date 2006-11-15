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
#include <ctype.h>
#include <limits.h>

#include <stdlibx.h>
#include <expand.h>

declare_expand(uLong);

#include "floatc.h"

inline boolean
sameformat(Float const &f1, Float const &f2)
{
    FloatClass const &fc1 = f1.floatclass();
    FloatClass const &fc2 = f2.floatclass();
    return fc1.mantissa() == fc2.mantissa() && fc1.exponent() == fc2.exponent();
}

inline char const *
floattypename(enum FloatType t)
{
    switch (t)
    {
    case FloatNaN: return "NaN";
    case FloatQuietNaN: return "QuietNaN";
    case FloatNegInf: return "NegInf";
    case FloatNegative: return "Negative";
    case FloatNegDenorm: return "NegDenorm";
    case FloatNegZero: return "NegZero";
    case FloatPosZero: return "PosZero";
    case FloatPosDenorm: return "PosDenorm";
    case FloatPositive: return "Positive";
    case FloatPosInf: return "PosInf";
    }

    return "Unknown";
}

// Exceptions:
//	FloatInvalidOperation
//	    FloatSignalingNaN			7.1 (1)
//		a == NaN or b == NaN
//	    FloatAddition			7.1 (2)
//		-Inf + +Inf, +Inf + -Inf, +Inf - +Inf, -Inf - -Inf
//	    FloatMultiplication			7.1 (2)
//		0 * Inf, Inf * 0


// handle exceptional conditions, return TRUE if result is in *this
boolean
Float::checkexceptions(enum FloatType thistype, enum FloatType ftype,
	enum FloatOperationType optype, Float const &f)
{
    if (!sameformat(*this, f))
    {
    	Float t(*_fc);
	t.makequietnan();
	_fc->trap(FloatFormatMismatch, optype, *this, f, t);
	*this = t;
	return TRUE;
    }
    
    // try not to waste time handling normal cases
    switch (thistype)
    {
    case FloatNegative:
    case FloatNegDenorm:
    case FloatPosDenorm:
    case FloatPositive:
	switch (ftype)
	{
	case FloatNegative:
	case FloatNegDenorm:
	case FloatPosDenorm:
	case FloatPositive:
	    return FALSE;

	default:
	    break;
	}

    default:
    	break;
    }

    // trap if either is a signalling NaN
    if (thistype == FloatNaN || ftype == FloatNaN)
    {
    	Float t(*_fc);
	t.makequietnan();
	_fc->trap(FloatInvalidOperation, optype, *this, f, t);
	*this = t;
	return TRUE;
    }

    if (thistype == FloatQuietNaN)
    	return TRUE;
	
    if (ftype == FloatQuietNaN)
    {
    	*this = f;
	return TRUE;
    }

    if (optype == FloatReverseSubtraction)
	neg();

    // handle zeros and infinities
    switch (optype)
    {
    case FloatAddition:
    case FloatSubtraction:
    case FloatReverseSubtraction:
	// zero operands are nop's
	// *this + 0 -> *this
	if (ftype == FloatPosZero || ftype == FloatNegZero)
	    return TRUE;

	// 0 + f -> f
	if (thistype == FloatPosZero || thistype == FloatNegZero)
	{
	    *this = f;

	    if (optype == FloatSubtraction)
	    	neg();

	    return TRUE;
	}

	// zeros and NaNs are excluded at this point
	if (thistype == FloatPosInf || thistype == FloatNegInf)
	{
	    if ((ftype == FloatPosInf || ftype == FloatNegInf) &&
	    	((int)(optype == FloatAddition) ^ (int)(thistype == ftype)))
	    {
		Float t(*_fc);
		t.makequietnan();
		_fc->trap(FloatInvalidOperation, optype, *this, f, t);
		*this = t;
	    }

	    return TRUE;
	}

	if (ftype == FloatPosInf || ftype == FloatNegInf)
	{
	    *this = f;

	    if (optype == FloatSubtraction)
	    	neg();

	    return TRUE;
	}

	break;
    case FloatMultiplication:
	// zero operands are nop's
	// 0 * f -> 0
	if (thistype == FloatPosZero || thistype == FloatNegZero)
	{
	    // 0 * Inf -> QNaN
	    if (ftype == FloatPosInf || ftype == FloatNegInf)
	    {
		Float t(*_fc);
		t.makequietnan();
		_fc->trap(FloatInvalidOperation, optype, *this, f, t);
		*this = t;
	    }

	    return TRUE;
	}

	// *this * 0 -> 0
	if (ftype == FloatPosZero || ftype == FloatNegZero)
	{
	    // Inf * 0 -> QNaN
	    if (thistype == FloatPosInf || thistype == FloatNegInf)
	    {
		Float t(*_fc);
		t.makequietnan();
		_fc->trap(FloatInvalidOperation, optype, *this, f, t);
		*this = t;
	    }
	    else
		makezero();

	    return TRUE;
	}

	// zeros and NaNs are excluded at this point
	// +Inf * +f -> +Inf
	// +Inf * -f -> -Inf
	// -Inf * +f -> -Inf
	// -Inf * -f -> +Inf
	if (thistype == FloatPosInf || thistype == FloatNegInf)
	{
	    if (f.sign())
		neg();

	    return TRUE;
	}

	// +f * +Inf -> +Inf
	// +f * -Inf -> -Inf
	// -f * +Inf -> -Inf
	// -f * -Inf -> +Inf
	if (ftype == FloatPosInf || ftype == FloatNegInf)
	{
	    if (sign())
	    {
		*this = f;
		neg();
	    }
	    else
		*this = f;

	    return TRUE;
	}

	break;
    case FloatDivision:
	// zero operands are semi-nop's
	// 0 / f -> 0
	if (thistype == FloatPosZero || thistype == FloatNegZero)
	{
	    // 0 / 0 -> QNaN
	    if (ftype == FloatPosZero || ftype == FloatNegZero)
	    {
		Float t(*_fc);
		t.makequietnan();
		_fc->trap(FloatInvalidOperation, optype, *this, f, t);
		*this = t;
	    }

	    return TRUE;
	}

	// *this / 0 -> Inf
	if (ftype == FloatPosZero || ftype == FloatNegZero)
	{
	    Float t(*_fc);
	    t.makeinf();

	    if ((sign() && !f.sign()) || (!sign() && f.sign()))
		t.neg();

	    _fc->trap(FloatDivideByZero, optype, *this, f, t);
	    *this = t;
	    return TRUE;
	}

	// zeros and NaNs are excluded at this point
	// +Inf / +f -> +Inf
	// +Inf / -f -> -Inf
	// -Inf / +f -> -Inf
	// -Inf / -f -> +Inf
	if (thistype == FloatPosInf || thistype == FloatNegInf)
	{
	    // Inf / Inf -> QNaN
	    if (ftype == FloatPosInf || ftype == FloatNegInf)
	    {
		Float t(*_fc);
		t.makequietnan();
		_fc->trap(FloatInvalidOperation, optype, *this, f, t);
		*this = t;
		return TRUE;
	    }

	    if (f.sign())
		neg();

	    return TRUE;
	}

	// +f / +Inf -> +Inf
	// +f / -Inf -> -Inf
	// -f / +Inf -> -Inf
	// -f / -Inf -> +Inf
	if (ftype == FloatPosInf || ftype == FloatNegInf)
	{
	    if ((!sign() && ftype == FloatPosInf) ||
		    (sign() && ftype == FloatNegInf))
		makezero();
	    else
	    {
		makezero();
		neg();
	    }

	    return TRUE;
	}

    	break;
    case FloatModulus:
    	break;
    case FloatNextAfter:
    	break;

    default:
    	break;
    }

    return FALSE;
}

void
Float::handleinexact(boolean overflow, boolean underflow, Float const &f,
	enum FloatOperationType optype)
{
    // set status bits
    _fc->status(FloatInexact, TRUE);

    // signal overflow if it occured
    if (overflow && _fc->trapenable(FloatOverflow))
    {
    	Float t = *this;

	if (_fc->trap(FloatOverflow, optype, *this, f, t))
	    *this = t;

	return;
    }

    // signal underflow if it occured
    if (underflow && _fc->trapenable(FloatUnderflow))
    {
    	Float t = *this;

	if (_fc->trap(FloatUnderflow, optype, *this, f, t))
	    *this = t;

	return;
    }

    // signal inexact exception
    if (_fc->trapenable(FloatInexact))
    {
    	Float t = *this;

	if (_fc->trap(FloatInexact, optype, *this, f, t))
	    *this = t;
    }
}

// fs * 2^(es + E) = [f1 * 2^(e1+E)] + [f2 * 2^(e2 + E)]
// es = max(e1, e2)
// fs = (f1 * 2^(e1 - es)) + (f2 * 2^(e2 - es))
// if (fs >= 1.0) fs = fs/2, es = es + 1
void
Float::uadd(Float const &f)
{
    // make sure that formats are identical, since thats all we can handle
#ifdef EBUG
    printf("Float::add(this = 0x%08X, f = 0x%08X)\n", this, &f);
    dumpfloat("Float::add, this = ", *this, FALSE);
    dumpfloat("Float::add, f = ", f, FALSE);
#endif

    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

#ifdef EBUG
    printf("Float::add, thistype = %s, ftype = %s\n", floattypename(thistype),
	    floattypename(ftype));
    printf("Float::add, operands are non-zero and finite\n");
#endif

//    // if signs differ then do a subtract instead
//    if (sign() ^ f.sign())
//    {
//    	neg();
//	subr(f);
//	return;
//    }

#ifdef EBUG
    printf("Float::add, signs match\n");
#endif

    int mbits = _fc->mantissa();
    int delta = expdelta(f);		// delta = this->exp - f.exp;
#ifdef EBUG
    printf("Float::add, exponent delta = %d\n", delta);
#endif
    boolean overflow = FALSE;

    // check if either operand is insignificant
    if (delta >= -mbits-1 && delta <= mbits+1)
    {
#ifdef EBUG
	printf("Float::add, about to shift mantissas\n");
#endif

	Float const *b;
	int carry;
	uLong lowbits = 0UL;
	boolean denorm = FALSE;

	if (delta < 0)
	{
	    copyexp(f);
	    lowbits = mrshift(-delta);

	    if (thistype != FloatPosDenorm && thistype != FloatNegDenorm)
		if (mbits + delta < 0)
		    lowbits |= LONGMSB;
		else
		    setmbit(mbits + delta);

	    b = &f;
	    carry = 1;
	}
	else if (delta > 0)
	{
	    Float &t = _fc->tmp(99);
	    t = f;
	    lowbits = t.mrshift(delta);

	    if (ftype != FloatPosDenorm && ftype != FloatNegDenorm)
		if (mbits - delta < 0)
		    lowbits |= LONGMSB;
		else
		    t.setmbit(mbits - delta);

	    b = &t;
	    carry = 1;
	}
	else
	{
	    b = &f;

	    if (thistype == FloatNegDenorm || thistype == FloatPosDenorm)
	    {
		denorm = TRUE;
		carry = 0;
	    }
	    else 
		carry = 2;
	}

#ifdef EBUG
	printf("Float::add, about to add mantissas, carry = %d, lowbits = 0x%08X\n", carry, lowbits);
#endif

	boolean addcarry = madd(*b);		// add the mantissa's
#ifdef EBUG
	printf("Float::add, carry from madd = %d\n", addcarry);
#endif
	carry += addcarry;

#ifdef EBUG
	printf("Float::add, about to normalize, carry = %d\n", carry);
#endif

	uLong highbit;

	// normalize exponent
	switch (carry)
	{
	case 1:
	    if (denorm)
		incexp();

	    break;
	case 2:
	    highbit = mrshift(1);
	    lowbits = (lowbits & 1) ? ((lowbits >> 1) | highbit | 1) :
		    ((lowbits >> 1) | highbit);
	    overflow = incexp();
	    break;
	case 3:
	    highbit = mrshift(1);
	    lowbits = (lowbits & 1) ? ((lowbits >> 1) | highbit | 1) :
		    ((lowbits >> 1) | highbit);
	    overflow = incexp();
	    setmbit(mbits - 1);
	    break;
	}

#ifdef EBUG
	printf("Float::add, normalization complete, overflow = %d, lowbits = 0x%08X\n", overflow, lowbits);
#endif

	if (!overflow && lowbits == 0UL)
	    return;

#ifdef EBUG
	printf("Float::add, about to round result\n");
#endif

	// round result
	overflow = roundup(lowbits) || overflow;
#ifdef EBUG
	printf("Float::add, result calculated, overflow = %d\n", overflow);
#endif
    }
    else
    {
	if (delta < 0)
	    *this = f;

	overflow = roundup(1);
    }

#ifdef EBUG
    printf("Float::add, about to signal exceptions\n");
#endif
    handleinexact(overflow, FALSE, f, FloatAddition);
}

// fd * 2^(ed + E) = [f1 * 2^(e1+E)] - [f2 * 2^(e2 + E)]
// ed = max(e1, e2)
// fd = (f1 * 2^(e1 - ed)) - (f2 * 2^(e2 - ed))
// while (fd < 0.5) fd = fd*2, ed = ed - 1

void
Float::usub(Float const &f)
{
#ifdef EBUG
    printf("Float::sub(this = 0x%08X, f = 0x%08X)\n", this, &f);
    dumpfloat("Float::sub, this = ", *this, FALSE);
    dumpfloat("Float::sub, f = ", f, FALSE);
#endif

    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

#ifdef EBUG
    printf("Float::sub, thistype = %s, ftype = %s\n", floattypename(thistype),
	    floattypename(ftype));
    printf("Float::sub, operands are non-zero and finite\n");
#endif

    // both operands are nonzero and finite at this point
//    if (sign() ^ f.sign())
//    {
//    	neg();
//	add(f);
//	neg();
//	return;
//    }

#ifdef EBUG
    printf("Float::sub, signs match\n");
#endif
    int cmp = ucompare(f);

    if (cmp == 0)
    {
    	makezero();
	return;
    }

    if (cmp < 0)
    {
    	fprintf(stderr, "Float::usub, called with invalid paramters\n");
	return;
    }

//    if ((!sign() && cmp < 0) || (sign() && cmp > 0))
//    {
//    	subr(f);
//	neg();
//	return;
//    }

#ifdef EBUG
    printf("Float::sub, |*this| > |f|\n");
#endif

    int mbits = _fc->mantissa();
    int delta = expdelta(f);		// delta = this->exp - f.exp;
#ifdef EBUG
    printf("Float::sub, exponent delta = %d\n", delta);
#endif
    boolean underflow = FALSE;

    // check if either operand is insignificant
    if (delta <= mbits + 2)
    {
#ifdef EBUG
	printf("Float::sub, about to shift mantissas\n");
#endif

	boolean borrow = FALSE;
	uLong lowbits = 0UL;
	boolean denorm = FALSE;

	if (delta > 0)
	{
	    Float t = f;
	    lowbits = t.mrshift(delta);

	    if (ftype != FloatPosDenorm && ftype != FloatNegDenorm)
		if (mbits - delta == -2)
		    lowbits |= LONGMSB >> 1;
		else if (mbits - delta == -1)
		    lowbits |= LONGMSB;
		else
		    t.setmbit(mbits - delta);

	    borrow = borrowf(-lowbits, 0UL, lowbits);
	    lowbits = -lowbits;
	    borrow = msub(t, borrow);	// subtract the mantissa's
	}
	else
	{
	    if (thistype == FloatNegDenorm || thistype == FloatPosDenorm)
		denorm = TRUE;

	    borrow = msub(f);	// subtract the mantissa's
	}

#ifdef EBUG
	printf("Float::sub, borrow from msub = %d\n", borrow);

	printf("Float::sub, about to normalize, borrow = %d\n", borrow);
#endif

	// normalize exponent
	if (borrow || delta == 0)
	{
	    int sf = mbits - Float::mbits() + 1;
	    int zdelta = expdelta(_fc->zero());

#ifdef EBUG
	    printf("Float::usub, mbits = %d, Float::mbits() = %d, sf = %d, zdelta = %d\n", 
		    mbits, Float::mbits(), sf, zdelta);
#endif

	    if (zdelta > sf)
	    	decexp(sf);
	    else
	    {
	    	decexp(zdelta);
		underflow = TRUE;
		sf = zdelta - 1;
	    }

	    if (sf)
	    {
		mlshift(sf);

		// shift the low bits in
		if (sf == 1)
		{
		    if (lowbits & LONGMSB)
			setmbit(0);

		    lowbits <<= 1;
		}
		else
		{
		    int hind = (sf - 1) / BITSPERLONG;
		    int lind = (sf / BITSPERLONG) - 1;
		    int hshift = (BITSPERLONG - sf) % BITSPERLONG;
		    uLong hmask = (1UL << (BITSPERLONG - hshift)) - 1;
		    setm(hind, getm(hind) | (lowbits >> hshift) & hmask);

		    if (lind >= 0)
		    {
			setm(lind, lowbits << (sf % BITSPERLONG));
			lowbits = 0UL;
		    }
		    else
		    	lowbits <<= sf % BITSPERLONG;
		}
	    }
	}
	else
	    ;	// implicit bit is one so were already normalized

#ifdef EBUG
	printf("Float::sub, normalization complete, underflow = %d, lowbits = 0x%08X\n", underflow, lowbits);
#endif

	if (!underflow && lowbits == 0UL)
	    return;

#ifdef EBUG
	printf("Float::sub, about to round result\n");
#endif

	// round result
	underflow = roundup(lowbits) || underflow;
#ifdef EBUG
	printf("Float::sub, result calculated, underflow = %d\n", underflow);
#endif
    }
    else
	underflow = rounddown(1);

#ifdef EBUG
    printf("Float::sub, about to signal exceptions\n");
#endif
    handleinexact(FALSE, underflow, f, FloatSubtraction);
}

// fd * 2^(ed + E) = [f1 * 2^(e1+E)] - [f2 * 2^(e2 + E)]
// ed = max(e1, e2)
// fd = (f1 * 2^(e1 - es)) - (f2 * 2^(e2 - es))
// while (fs < 0.5) fs = fs*2, es = es - 1
void
Float::usubr(Float const &/*f*/)
{
    // check for exceptional cases
    //enum FloatType thistype = floattype();
    //enum FloatType ftype = f.floattype();

//    if (sign() ^ f.sign())
//    {
//    	neg();
//	add(f);
//	return;
//    }

    // calculate shifts factor
    // subtract the two mantissas applying the shift factors
    // adjust the exponent checking for zero
    // normalize the result
}

void
Float::add(Float const &f)
{
    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatAddition, f))
    	return;

    if (sign() ^ f.sign())
    {
    	int x = ucompare(f);

	if (x >= 0)
	    usub(f);
	else
	{
	    Float t = f;
	    t.usub(*this);
	    *this = t;
	}
    }
    else
    	uadd(f);
}

void
Float::sub(Float const &f)
{
    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatSubtraction, f))
    	return;

    if (sign() ^ f.sign())
    	uadd(f);
    else
    {
    	int x = ucompare(f);

	if (x >= 0)
	    usub(f);
	else
	{
	    Float t = f;
	    t.usub(*this);
	    *this = t;
	    neg();
	}
    }
}

void
Float::subr(Float const &f)
{
    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatReverseSubtraction, f))
    	return;

    if (sign() ^ f.sign())
    {
    	uadd(f);
	neg();
    }
    else
    {
    	int x = ucompare(f);

	if (x >= 0)
	{
	    usub(f);
	    neg();
	}
	else
	{
	    Float t = f;
	    t.usub(*this);
	    *this = t;
	}
    }
}

void
dumpvector(char const *label, int words, uLong *vec)
{
    if (words < 1)
    {
    	if (label == NULL)
	    printf("{ }\n");
	else
	    printf("%s = { }\n", label);

	return;
    }

    if (label == NULL)
	printf("{ 0x%08lX", vec[0]);
    else
	printf("%s = { 0x%08lX", label, vec[0]);

    for (int i = 1; i < words; i++)
    	printf(", 0x%08lX", vec[i]);

    printf(" }\n");
}

void
extractmant(Float const &f, enum FloatType type, uLong *mant)
{
    int words = f.floatclass().msize();

    for (int i = 0; i < words; i++)
    	mant[i] = f.getm(i);

    int b = f.floatclass().mantissa() & (BITSPERLONG - 1);
    
    if (type == FloatPositive || type == FloatNegative)
	if (b)
	    mant[words - 1] |= 1UL << b;
	else
	    mant[words] = 1;
    else if (!b)
	mant[words] = 0;
}

#define HALFSHIFT (BITSPERLONG >> 1)
#define LOWMASK ((1UL << HALFSHIFT) - 1)

// multiple two unsigned integeral of length n and return a product
// of length 2*n.

static void
mult(int n, uLong *a, uLong *b, uLong *p)
{
    int m = n << 1;
    int i;

    for (i = 0; i < m; i++)
    	p[i] = 0UL;

    for (i = 0; i < n; i++)
    	if (a[i])
	    for (int j = 0; j < n; j++)
	    	if (b[j])
		{
		    uLong aa = a[i];
		    uLong bb = b[j];

		    uLong al = aa & LOWMASK;
		    uLong ah = aa >> HALFSHIFT;
		    uLong bl = bb & LOWMASK;
		    uLong bh = bb >> HALFSHIFT;

		    uLong l = al * bl;
		    uLong m0 = al * bh;
		    uLong m1 = ah * bl;
		    uLong h = ah * bh;

		    uLong ll = l & LOWMASK;
		    uLong lh = l >> HALFSHIFT;
		    uLong m0l = m0 & LOWMASK;
		    uLong m0h = m0 >> HALFSHIFT;
		    uLong m1l = m1 & LOWMASK;
		    uLong m1h = m1 >> HALFSHIFT;
		    uLong hl = h & LOWMASK;
		    uLong hh = h >> HALFSHIFT;

		    uLong xlh = lh + m0l + m1l;
		    uLong xhl = (xlh >> HALFSHIFT) + m0h + m1h + hl;
		    hh += xhl >> HALFSHIFT;

		    uLong xl = (xlh << HALFSHIFT) | ll;
		    uLong xh = (hh << HALFSHIFT) | (xhl & LOWMASK);
		    uLong t = p[i + j] + xl;
		    boolean carry = carryf(t, p[i + j], xl);
		    p[i + j] = t;
		    t = p[i + j + 1] + xh + carry;
		    carry = carryf(t, p[i + j + 1], xh);
		    p[i + j + 1] = t;

		    for (int k = i + j + 2; k < m; k++)
		    {
		    	t = p[k] + carry;
			carry = carryf(t, p[k], carry);
			p[k] = t;
		    }
		}
}

boolean
sub(int words, uLong *a, uLong *b, uLong *d)
{
    boolean borrow = FALSE;

    for (int i = 0; i < words; i++)
    {
    	uLong diff = a[i] - b[i];
	boolean borrow2 = borrowf(diff, a[i], b[i]);
	d[i] = diff - borrow;
	borrow = borrowf(d[i], diff, borrow) || borrow2;
    }

    return borrow;
}

uLong
rshift(int words, uLong *vec, int b)
{
    if (b == 0)
    	return 0UL;

    if (b < 0)
    {
    	// left shift
	b = -b;

	int x = b / BITSPERLONG;
	b %= BITSPERLONG;
	int br = BITSPERLONG - b;
	int hind = words - 1;

	if (x == 0)
	{
	    // this is the main shift loop
	    for (int i = hind; i > 0; i--)
		vec[i] = (vec[i] << b) | (vec[i - 1] >> br);

	    vec[0] <<= b;
	    return 0UL;
	}

	if (b == 0)
	{
	    int i;
	    
	    // this is the main shift loop
	    for (i = hind; i >= x; i--)
		vec[i] = vec[i - x];

	    for (; i >= 0; i--)
		vec[i] = 0UL;

	    return 0UL;
	}
	
	int i;

	// this is the main shift loop
	for (i = hind; i > x; i--)
	    vec[i] = (vec[i - x] << b) | (vec[i - x - 1] >> br);

	if (i == x)
	    vec[i--] = vec[0] << b;

	for (; i >= 0; i--)
	    vec[i] = 0UL;

    	return 0UL;
    }

    // right shift
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = BITSPERLONG - b;
    int hind = words - 1;
    uLong out;

    if (x == 0)
    {
	out = vec[0] << br;

	// this is the main shift loop
	for (int i = 0; i < hind; i++)
	    vec[i] = (vec[i] >> b) | (vec[i + 1] << br);

	vec[hind] >>= b;
	return out;
    }

    if (b == 0)
    {
	if (x <= words)
	    out = vec[x - 1];
	else 
	    out = 0UL;

	int i;
    
	for (i = 0; i < x - 1; i++)
	    if (vec[i])
	    {
		out |= 1;
		break;
	    }

	// this is the main shift loop
	for (i = 0; i + x < words; i++)
	    vec[i] = vec[i + x];

	for (; i < words; i++)
	    vec[i] = 0UL;

	return out;
    }

    if (x < words)
	out = (vec[x - 1] >> b) | (vec[x] << br);
    else if (x == words)
	out = vec[hind] >> b;
    else 
    	out = 0UL;

    int i;
    
    for (i = 0; i + 1 < x && i < words; i++)
    	if (vec[i])
	{
	    out |= 1;
	    break;
	}

    if (x <= words && vec[x - 1] << br)
    	out |= 1;

    // this is the main shift loop
    for (i = 0; i + x + 1 < words; i++)
	vec[i] = (vec[i + x] >> b) | (vec[i + x + 1] << br);

    if (i + x == hind)
	vec[i++] = vec[hind] >> b;

    for (; i < words; i++)
    	vec[i] = 0UL;

    return out;
}

int
bits(int words, uLong *x)
{
    int i;
    
    for (i = words - 1; i >= 0; i--)
    	if (x[i])
	    break;

    return i * BITSPERLONG + bits(x[i]);
}

int
normalize(int n, uLong *x)
{
    int bias = n * BITSPERLONG - bits(n, x);
    rshift(n, x, -bias);
    return bias;
}

int
comparevec(int words, uLong *a, uLong *b)
{
    for (int i = words - 1; i >= 0; i--)
    {
    	if (a[i] < b[i])
	    return -1;

	if (a[i] > b[i])
	    return 1;
    }

    return 0;
}

void
div(int words, uLong *n, uLong *d, uLong *div, uLong *rem)
{
    int bits = words * BITSPERLONG;
    int i;
    
    for (i = 0; i < words; i++)
    {
    	div[i] = 0UL;
    	rem[i] = n[i];
    }

    boolean highbit = FALSE;

    for (i = bits - 1; i >= 0; i--)
    {
    	if (highbit || comparevec(words, rem, d) >= 0)
	{
	    div[i / BITSPERLONG] |= 1UL << (i % BITSPERLONG);
	    ::sub(words, rem, d, rem);
	}

	highbit = !!(rem[words - 1] & LONGMSB);
	::rshift(words, rem, -1);
    }
}

// fx = [1.0..2.0)
// ex = 1..(2^n-2)
// E = 2^(n-1)-1
// x = fx * 2^(ex - E)
// p = x * y
// fp * 2^(ep - E) = [fx * 2^(ex - E)] * [fy * 2^(ey - E)]
// fp * 2^(ep - E) = fx * fy * 2^(ex + ey - 2E))
// ip = fx * fy
// es = ex + ey - E
// if (ip >= 2.0)
//     fp = ip / 2, ep = es + 1	// if lost ip bits not zero
// else 
// {
//     while (ip < 1.0 && es > 1)
//         ip *= 2, es--
//     if (ip < 1.0)
//         es--;	// make denormal
//     while (ip != 0.0 && es < 0)
//	   ip /= 2, es++
//     if (ip == 0.0 && es < 0)
//         es = 0	// underflow
// }
//     fp = ip, ep = es - E
 
void
Float::mult(Float const &f)
{
    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatMultiplication, f))
    	return;

    // calculate new sign
    boolean sgn = sign() ^ f.sign();

    // calculate new mantissa
    int mantwords = (_fc->mantissa() / BITSPERLONG) + 1;
    static uLong *xm = NULL;
    static uLong *ym = NULL;
    static uLong *m = NULL;
    static int xmlen = 0;
    static int ymlen = 0;
    static int mlen = 0;
    expand(xm,  xmlen, mantwords);
    expand(ym,  ymlen, mantwords);
    expand(m,  mlen, 2 * mantwords);
    extractmant(*this, thistype, xm);
    extractmant(f, ftype, ym);
    ::mult(mantwords, xm, ym, m);
#ifdef EBUG
    dumpvector("xm", mantwords, xm);
    dumpvector("ym", mantwords, ym);
    dumpvector("m", mantwords * 2, m);
#endif
    int h0bit = _fc->mantissa() << 1;
    int h1bit = h0bit + 1;
    uLong h0mask = 1UL << (h0bit & (BITSPERLONG - 1));
    uLong h1mask = 1UL << (h1bit & (BITSPERLONG - 1));
    int h0ind = h0bit / BITSPERLONG;
    int h1ind = h1bit / BITSPERLONG;
    int shift = h0bit;

    // calculate new exponent
    boolean carry = eadd(f);
    boolean borrow = esub(_fc->one());

    if (carry && borrow)
    {
    	carry = FALSE;
	borrow = FALSE;
    }

    // normalize product mantissa
    if (m[h1ind] & h1mask)	// if h1
    {
    	shift++;
	carry = incexp(1) || carry;
    }
    else if ((m[h0ind] & h0mask) == 0)	// if !h0
    {
#ifdef EBUG
    	printf("m[h0ind] = 0x%08lX, h0mask = 0x%08lX\n", m[h0ind], h0mask);
#endif

	int i;
    
    	// there is at least one 1-bit since zero is handled seperately.
    	for (i = h0ind; i >= 0; i--)
	    if (m[i])
	    	break;

	shift = i * BITSPERLONG + findhighestbit(m[i]);
	borrow = decexp(h0bit - shift) || borrow;
    }

    if (carry && borrow)
    {
    	carry = FALSE;
	borrow = FALSE;
    }

    // normalize exponent
    if (borrow || expdelta(_fc->zero()) == 0)
    {
#ifdef EBUG
    	printf("underflow likely\n");
#endif

    	ecompl();
	int d = expdelta(_fc->zero());

	if (INT_MAX - d < shift + 2)
	    shift = INT_MAX;
	else
	    shift += 2 + d;

	copyexp(_fc->zero());
	incexp(1);
    }
    else if (carry || expdelta(_fc->posinf()) == 0)
    {
	*this = _fc->posinf();
	sign(sgn);
	handleinexact(TRUE, FALSE, f, FloatMultiplication);
	return;
    }

    boolean underflow = FALSE;
    boolean inexact = FALSE;

    int shind = shift / BITSPERLONG;
    uLong shmask = 1UL << (shift & (BITSPERLONG - 1));

#ifdef EBUG
    printf("shift = %d, shind = %d, shmask = 0x%08lX, m[shind] = 0x%08lX\n",
	    shift, shind, shmask, ((shift > h1bit) ? 0UL : m[shind]));
#endif

    // check for underflow
    if (shift > h1bit || (m[shind] & shmask) == 0)
    {
    	underflow = TRUE;
	copyexp(_fc->zero());
    }

    // set mantissa
    uLong lowbits = ::rshift(mantwords * 2, m, shift - _fc->mantissa());
#ifdef EBUG
    dumpvector("rshift m", mantwords * 2, m);
#endif

    if (lowbits)
    	inexact = TRUE;

    for (int i = 0; i < _fc->msize(); i++)
    	setm(i, m[i]);

    // set sign
    sign(sgn);

    // round
    boolean overflow = roundup(lowbits);

    // signal exceptions
    if (overflow || underflow || inexact)
	handleinexact(overflow, underflow, f, FloatMultiplication);

    return;
}

// fd * 2^(ed + E) = [f1 * 2^(e1 + E)] / [f2 * 2^(e2 + E)]
// fd * 2^(ed + E) = (f1 / f2) * [2^(e1 + E) / 2^(e2 + E)]
// fd * 2^(ed + E) = (f1 / f2) * 2^(e1 - e2)
// fd * 2^(ed + E) = (f1 / f2) * 2^(e1 - e2 - E + E)
// ed = e1 - e2 - E
// fd = f1 / f2
// if (fd >= 1.0) fd = fd/2, ed = ed + 1
void
Float::div(Float const &f)
{
    // extract mantissa of this
    // extract mantissa of f
    // normalize mantissa of this, giving exponent bias, thisbias
    // normalize mantissa of f, giving exponent bias, fbias
    // bias = thisbias - fbias
    // divide
    // if div < 1 then div << 1, bias++
    // Ez = Ex - Ey + B - bias
    // if Ez < 0 && -Ez < m, mant >>= -Ez, Ez = 0

    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatDivision, f))
    	return;

    // calculate new sign
    boolean sgn = sign() ^ f.sign();

    // calculate new mantissa
    int mantissa = _fc->mantissa();
    int mantwords = (mantissa / BITSPERLONG) + 1;
    static uLong *n = NULL;
    static uLong *d = NULL;
    static uLong *dv = NULL;
    static uLong *r = NULL;
    static int nlen = 0;
    static int dlen = 0;
    static int dvlen = 0;
    static int rlen = 0;
    expand(n, nlen, mantwords);
    expand(d, dlen, mantwords);
    expand(dv, dvlen, mantwords);
    expand(r, rlen, mantwords);

    extractmant(*this, thistype, n);
    extractmant(f, ftype, d);
    int bias = ::normalize(mantwords, n) - ::normalize(mantwords, d);
    ::div(mantwords, n, d, dv, r);
#ifdef EBUG
    dumpvector("n", mantwords, n);
    dumpvector("d", mantwords, d);
    dumpvector("div", mantwords, dv);
    dumpvector("rem", mantwords, r);
    printf("bias = %d\n", bias);
#endif

    int shift = mantwords * BITSPERLONG - mantissa - 1;

    // normalize result
    if (!(dv[mantwords - 1] & LONGMSB))
    {
    	shift--;
	bias++;
    }
    	
    // calculate new exponent
    boolean borrow = esub(f);
    boolean carry = eadd(_fc->one());

    if (borrow && carry)
    {
	borrow = FALSE;
	carry = FALSE;
    }

    if (bias < 0)
    	carry = incexp(-bias) || carry;
    else
	borrow = decexp(bias) || borrow;

    if (borrow && carry)
    {
    	borrow = FALSE;
	carry = FALSE;
    }

    // normalize exponent
    if (borrow || expdelta(_fc->zero()) == 0)
    {
#ifdef EBUG
    	printf("underflow certain\n");
#endif

    	ecompl();
	int d = expdelta(_fc->zero());

	if (INT_MAX - d < shift + 1)
	    shift = INT_MAX;
	else
	    shift += d + 1;

	copyexp(_fc->zero());
//	incexp(1);
    }
    else if (carry || expdelta(_fc->posinf()) == 0)
    {
	*this = _fc->posinf();
	sign(sgn);
	handleinexact(TRUE, FALSE, f, FloatDivision);
	return;
    }

    boolean underflow = FALSE;
    boolean inexact = FALSE;

    //int shind = shift / BITSPERLONG;
    //uLong shmask = 1UL << (shift & (BITSPERLONG - 1));

#ifdef EBUG
    printf("shift = %d, shind = %d, shmask = 0x%08lX, dv[shind] = 0x%08lX\n",
	    shift, shind, shmask, ((shind >= mantwords) ? 0UL : dv[shind]));
#endif

    // check for underflow
//    if (shind >= mantwords || (dv[shind] & shmask) == 0)
    if ((shift >= mantissa) ||
	    (shift == mantissa - 1 &&
	    (dv[mantwords - 1] & LONGMSB) == 0))
    	underflow = TRUE;

    // set mantissa
    uLong lowbits = ::rshift(mantwords, dv, shift);
#ifdef EBUG
    dumpvector("rshift div", mantwords, dv);
#endif

    int i;
    
    for (i = 0; i < mantwords; i++)
    	if (r[i])
	{
	    lowbits |= 1;
	    break;
	}

    if (lowbits)
    	inexact = TRUE;

    for (i = 0; i < _fc->msize(); i++)
    	setm(i, dv[i]);

    // set sign
    sign(sgn);

    // round
    boolean overflow = roundup(lowbits);

    // signal exceptions
    if (overflow || underflow || inexact)
	handleinexact(overflow, underflow, f, FloatDivision);

    return;
}

// floating point remainder of this divided by f.  rem = *this - int(*this / f) * f
void
Float::rem(Float const &f)
{
    // check for exceptional cases
    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (checkexceptions(thistype, ftype, FloatModulus, f))
    	return;
}

// compare magnitudes of two Float's
//	unordered	-2
//	lessthan	-1
//	equal		 0
//	greaterthan	 1
int
Float::ucompare(Float const &f) const
{
    if (!sameformat(*this, f))
    {
	Float t(*_fc);
	t.makequietnan();
	_fc->trap(FloatFormatMismatch, FloatComparison, *this, f, t);
	return -2;
    }

    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (thistype == FloatNaN || thistype == FloatQuietNaN ||
	    ftype == FloatNaN || ftype == FloatQuietNaN)
    	return -2;

    if ((thistype == FloatPosZero || thistype == FloatNegZero) &&
	    (ftype == FloatPosZero || ftype == FloatNegZero))
	return 0;

    int l = _fc->ehind();
    uLong m = _fc->ehmask();

    if (l == _fc->mhind())
    	m |= _fc->mhmask();

    if ((_vec[l] & m) < (f._vec[l] & m))
	return -1;

    if ((_vec[l] & m) > (f._vec[l] & m))
	return 1;

    for (int i = l - 1; i >= 0; i--)
    {
	if (_vec[i] < f._vec[i])
	    return -1;

	if (_vec[i] > f._vec[i])
	    return 1;
    }

    return 0;
}

// compare two Float's
//	unordered	-2
//	lessthan	-1
//	equal		 0
//	greaterthan	 1
int
Float::compare(Float const &f) const
{
    if (!sameformat(*this, f))
    {
	Float t(*_fc);
	t.makequietnan();
	_fc->trap(FloatFormatMismatch, FloatComparison, *this, f, t);
	return -2;
    }

    enum FloatType thistype = floattype();
    enum FloatType ftype = f.floattype();

    if (thistype == FloatNaN || thistype == FloatQuietNaN ||
	    ftype == FloatNaN || ftype == FloatQuietNaN)
    	return -2;

    if ((thistype == FloatPosZero || thistype == FloatNegZero) &&
	    (ftype == FloatPosZero || ftype == FloatNegZero))
	return 0;

    if (thistype < ftype)
	return -1;
	
    if (thistype > ftype)
	return 1;

    int lessthan = -1;

    if (_vec[_fc->sind()] & _fc->smask())
	lessthan = 1;

    int l = _fc->ehind();
    uLong m = _fc->ehmask();

    if (l == _fc->mhind())
    	m |= _fc->mhmask();

    if ((_vec[l] & m) < (f._vec[l] & m))
	return lessthan;

    if ((_vec[l] & m) > (f._vec[l] & m))
	return -lessthan;

    for (int i = l - 1; i >= 0; i--)
    {
	if (_vec[i] < f._vec[i])
	    return lessthan;

	if (_vec[i] > f._vec[i])
	    return -lessthan;
    }

    return 0;
}

void
Float::scalb(Float const &)
{
}

void
Float::logb()
{
}

void
Float::nextafter(Float const &f)
{
    enum FloatType thistype = floattype();
    enum FloatType ftype = floattype();

    if (checkexceptions(thistype, ftype, FloatNextAfter, f))
	return;

    int overflow = FALSE;
    int zero = FALSE;

    if (thistype == ftype)
    {
    	if (thistype == FloatNegInf || thistype == FloatPosInf ||
		thistype == FloatNegZero || thistype == FloatPosZero)
	    return;

	int c = compare(f);

	if (c < 0)
	    if (sign())
	    	zero = prev();
	    else
	    	overflow = next();
	else if (c > 0)
	    if (sign())
		overflow = next();
	    else
	    	zero = prev();
    }
    else if (thistype < ftype)
    {
    	if (thistype == FloatNegZero)
	    neg();
    	else if (sign())
	    zero = prev();
	else
	    overflow = next();
    }
    else
    {
    	if (thistype == FloatPosZero)
	    neg();
    	else if (sign())
	    overflow = next();
	else
	    zero = prev();
    }

    if (overflow || zero)
	handleinexact(overflow, zero, f, FloatNextAfter);
}

void
Float::mult2n(int n)
{
    if (n == 0)
	return;

    enum FloatType thistype = floattype();
    int m;

    switch (thistype)
    {
    case FloatNaN:
    	makequietnan();
	return;
    case FloatQuietNaN:
    case FloatNegInf:
    case FloatPosInf:
    case FloatNegZero:
    case FloatPosZero:
    	return;
    case FloatNegDenorm:
    case FloatPosDenorm:
    	if (n < 0)
	{
	    uLong lowbits = rshift(-n);
	    roundup(lowbits);
	    return;
	}

	m = _fc->mantissa() - mbits();

	if (m >= n)
	    lshift(n);
	else
	{
	    lshift(m + 1);
	    boolean carry = incexp(n - m);

	    if (carry)
		handleinexact(TRUE, FALSE, _fc->posinf(), FloatMultiplication);
	}

	return;
    case FloatNegative:
    case FloatPositive:
    	if (n < 0)
	{
	    int min = expdelta(_fc->zero()) - 1;

	    if (min < -n)
	    {
	    	// we'll be generating a denormal
	    	decexp(min + 1);
		int sf = -n - min + 1;
		mrshift(sf);

		if (sf > _fc->mantissa())
		    handleinexact(FALSE, TRUE, _fc->zero(),
			    FloatMultiplication);
		else
		    setmbit(_fc->mantissa() - sf);
	    }
	    else
	    	decexp(-n);

	    return;
	}

	boolean carry = incexp(n);

	if (carry)
	    handleinexact(TRUE, FALSE, _fc->posinf(), FloatMultiplication);

    	return;
    }

    return;
}
