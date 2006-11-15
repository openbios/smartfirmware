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
#include <integer.h>

#include "floatc.h"

Float
sqr(Float const &x)
{
    FloatClass &fc = x.floatclass();
    boolean inexact = fc.trapenable(FloatInexact, FALSE);

    Float t = x;
    t *= x;

    fc.trapenable(FloatInexact, inexact);
    return t;
}

// fy * 2^(ey - E) = [fx * 2^(ex - E)] ^ (1/2)
// fy * 2^(ey - E) = [fx/2 * 2^(ex + 1 - E)] ^ (1/2)
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[(ex + 1 - E) * (1/2)]
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[(ex + 1 - E) / 2]
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[(ex + 1 - E) / 2 + E - E]
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[ex/2 + 1/2 - E/2 + E - E]
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[ex/2 + (E + 1)/2 - E]
// fy * 2^(ey - E) = (fx/2)^(1/2) * 2^[(ex + E + 1)/2 - E]
// if (ex & 1)
// 	fy = fx^(1/2), ey = (ex + E) >> 1
// else
// 	fy = (fx/2)^(1/2), ey = (ex + E + 1) >> 1

#if 0
Float
sqrt(Float const &x)
{
    FloatClass &fc = x.floatclass();

#ifdef EBUG
    dumpfloat("sqrt(x), x = ", x);
#endif

    switch (x.floattype())
    {
    case FloatNaN:
    case FloatQuietNaN:
    case FloatNegInf:
    case FloatNegative:
    case FloatNegDenorm:
    	return fc.qnan();
    case FloatPosInf:
    case FloatNegZero:
    case FloatPosZero:
    	return x;
    }

    // disable the inexact trap since this operation is guaranteed to
    // be inexact
    boolean inexact = fc.trapenable(FloatInexact, FALSE);

    Float t1 = x;
    t1.copyexp(fc.half());
#ifdef EBUG
    dumpfloat("sqrt(x), normalized value, t1 = ", t1);
#endif

    // use a polynomial to get a first guess at sqrt(t1)
    Float t2 = (dtofloat(fc, -0.1984742) * t1 + dtofloat(fc, 0.8804894)) * t1 +
	    dtofloat(fc, 0.3176687);

#ifdef EBUG
    dumpfloat("sqrt(x), first guess, t2 = ", t2);
#endif

    int iters = bits(fc.mantissa() / 6);

#ifdef EBUG
    printf("sqrt(x), iters = %d\n", iters);
#endif

    // iterate via newtons methods to converge on a solution
    for (int i = 0; i < iters; i++)
    {
    	// t2 = (t2 + t1/t2)/2
    	Float t3 = t1;
	t3 /= t2;
#ifdef EBUG
	dumpfloat("sqrt(x), alternate root, t3 = ", t3);
#endif
	t3 += t2;
	t3.mult2n(-1);
	t2 = t3;
#ifdef EBUG
	dumpfloat("sqrt(x), average root, t2 = ", t2);
#endif
    }

    // compute exponent
    Float t3(fc);
    t3.copyexp(x);
    boolean borrow = t3.esub(fc.half());
    boolean odd = t3.gete(0) & 1;
    t3.ershift(1);

    if (borrow)
	t3.setebit(fc.exponent() - 1);

#ifdef EBUG
    printf("sqrt(x), borrow = %d, odd = %d\n", borrow, odd);
    dumpfloat("sqrt(x), exponent bias, t3 = ", t3);
#endif

    if (odd)
    {
	t2 *= fc.sqrttwo();
#ifdef EBUG
	dumpfloat("sqrt(x), root * sqrt(2) = ", t2);
#endif
    }

    t2.eadd(t3);

#ifdef EBUG
    dumpfloat("sqrt(x), final root value, t2 = ", t2);
#endif

    fc.trapenable(FloatInexact, inexact);
    return t2;
}
#endif

Float
sqrt(Float const &x)
{
    FloatClass &fc = x.floatclass();

#ifdef EBUG
    dumpfloat("sqrt(x), x = ", x);
#endif

    switch (x.floattype())
    {
    case FloatNaN:
    case FloatQuietNaN:
    case FloatNegInf:
    case FloatNegative:
    case FloatNegDenorm:
    	return fc.qnan();
    case FloatPosInf:
    case FloatNegZero:
    case FloatPosZero:
    	return x;
    default:
	break;
    }

    // disable the inexact trap since this operation is guaranteed to
    // be inexact
    boolean inexact = fc.trapenable(FloatInexact, FALSE);


    Float const *scale = &fc.one();

    if (x == fc.two())
    	scale = &fc.half();

    Float t1 = x;
    t1.copyexp(*scale);
#ifdef EBUG
    dumpfloat("sqrt(x), normalized value, t1 = ", t1);
#endif

    // use a polynomial to get a first guess at sqrt(t1)
    Float t2 = (dtofloat(fc, -0.1984742) * t1 + dtofloat(fc, 0.8804894)) * t1 +
	    dtofloat(fc, 0.3176687);

#ifdef EBUG
    dumpfloat("sqrt(x), first guess, t2 = ", t2);
#endif

    int iters = bits(fc.mantissa() / 6);

#ifdef EBUG
    printf("sqrt(x), iters = %d\n", iters);
#endif

    // iterate via newtons methods to converge on a solution
    for (int i = 0; i < iters; i++)
    {
    	// t2 = (t2 + t1/t2)/2
    	Float t3 = t1;
	t3 /= t2;
#ifdef EBUG
	dumpfloat("sqrt(x), alternate root, t3 = ", t3);
#endif
	t3 += t2;
	t3.mult2n(-1);
	t2 = t3;
#ifdef EBUG
	dumpfloat("sqrt(x), average root, t2 = ", t2);
#endif
    }

    // compute exponent
    Float t3(fc);
    t3.copyexp(x);
    boolean borrow = t3.esub(*scale);
    boolean odd = (boolean)(t3.gete(0) & 1);
    t3.ershift(1);

    if (borrow)
	t3.setebit(fc.exponent() - 1);

#ifdef EBUG
    printf("sqrt(x), borrow = %d, odd = %d\n", borrow, odd);
    dumpfloat("sqrt(x), exponent bias, t3 = ", t3);
#endif

    if (odd)
    {
	t2 *= fc.sqrttwo();
#ifdef EBUG
	dumpfloat("sqrt(x), root * sqrt(2) = ", t2);
#endif
    }

    t2.eadd(t3);

#ifdef EBUG
    dumpfloat("sqrt(x), final root value, t2 = ", t2);
#endif

    fc.trapenable(FloatInexact, inexact);
    return t2;
}
