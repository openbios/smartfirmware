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
#include <math.h>
#include <integer.h>

#include "floatc.h"

#define LN2      0.69314718056
#define EPSILON  0.00000000001
#define NQ	 8

static Float
exp(Float const &x, Float const &ln2)
{
    FloatClass &fc = x.floatclass();
    enum FloatType xtype = x.floattype();

    switch (xtype)
    {
    case FloatNaN:
    	return fc.nan();
    case FloatQuietNaN:
    case FloatPosInf:
    	return x;
    case FloatNegInf:
    	return fc.zero();
    case FloatNegZero:
    case FloatPosZero:
    	return fc.one();

    default:
    	break;
    }

    Float lim = fc.one();
    lim.mult2n(fc.exponent());

    if (x > lim)
    	return fc.posinf();

    if (-x > lim)
    	return fc.zero();

    Float a = x;
    Float nz = fc.zero();

    {	// more workarounds for braindead cfront
    	Float f1 = fabs(x - dtofloat(fc, LN2));
	Float f2 = dtofloat(fc, EPSILON);

	if (f1 > f2 && !ln2.isZero())
	{
	    // reduce argument
	    nz = floor((x / ln2) + fc.half());
	    a -= nz * ln2;
	}
    }

#ifdef EBUG
    dumpfloat("exp, nz = ", nz);
    dumpfloat("exp, a = ", a);
#endif

    Float z = fc.one();		// exp(a)

    if (!a.isZero())
    {
    	// divide by 2 ^ NQ to speed up convergence
	a.mult2n(-NQ);
	z += a;
	Float num = a * a;
	Float fact = fc.one();

	int iters = fc.mantissa() / NQ;
	int i;

	for (i = 2; i <= iters; i++)
	{
	    fact *= Float(fc, i);
	    z += num / fact;
	    num *= a;
#ifdef EBUG
	    printf("exp, iter %d of %d\n", i, iters);
	    dumpfloat("exp, z = ", z);
	    dumpfloat("exp, num = ", num);
	    dumpfloat("exp, fact = ", fact);
#endif
	}

	// correct for earlier divide
	for (i = 0; i < NQ; i++)
	    z *= z;
    }

#ifdef EBUG
    dumpfloat("exp, z = ", z);
#endif

    int f2i = floattoi(nz);

    if (f2i > INT_MIN && f2i < INT_MAX)
	z.mult2n(f2i);
    else
	z *= pow(fc.two(), nz);

#ifdef EBUG
    dumpfloat("exp, z final = ", z);
#endif

    return z;
}

Float
exp(Float const &x)
{
    return exp(x, x.floatclass().ln2());
}

static Float
log(Float const &x, Float const &ln2)
{
//   The Taylor series for Log converges much more slowly than that of Exp.
//   Thus this routine does not employ Taylor series, but instead computes
//   logarithms by solving Exp (b) = a using the following Newton iteration,
//   which converges to b:
//
//           x_{k+1} = x_k + [a - Exp (x_k)] / Exp (x_k)
//
//   These iterations are performed with a maximum precision level NW that
//   is dynamically changed, approximately doubling with each iteration.
//   See the comment about the parameter NIT in MPDIVX.

    FloatClass &fc = x.floatclass();

    if (x < fc.zero())
	return fc.qnan();

    if (x == fc.zero())
    	return fc.neginf();

    if (x == fc.one())
    	return fc.zero();

    Float z = dtofloat(fc, log(floattod(x)));
    int iters = bits(fc.mantissa() / 15) + 1;

    for (int i = 0; i < iters; i++)
    {
    	Float x0 = exp(z, ln2);
	z += (x - x0) / x0;
    }

    return z;
}

Float
log(Float const &x)
{
    FloatClass &fc = x.floatclass();

//    if (fabs(x - fc.two()) > dtofloat(fc, 0.0000001))
    if (x != fc.two())
	return log(x, fc.ln2());

    return log(x, fc.zero());
}

Float
log10(Float const &x)
{
    return log(x, x.floatclass().ln2()) / x.floatclass().ln10();
}

Float
log2(Float const &x)
{
    return log(x, x.floatclass().ln2()) / x.floatclass().ln2();
}

Float
pow(Float const &y, Float const &x)
{
#ifdef EBUG
    dumpfloat("pow, y = ", y);
    dumpfloat("pow, x = ", x);
#endif

    FloatClass &fc = y.floatclass();
    enum FloatType ytype = y.floattype();
    enum FloatType xtype = x.floattype();

    if (ytype == FloatNaN || ytype == FloatQuietNaN || xtype == FloatNaN ||
	    xtype == FloatQuietNaN)
    {
    	// NaN^x or y^NaN
    	if (ytype == FloatQuietNaN)
	    return y;

	if (xtype == FloatQuietNaN)
	    return x;

	return fc.qnan();
    }

    if (xtype == FloatNegInf || xtype == FloatPosInf)
    {
    	// y^+-Inf
    	if (ytype == FloatNegInf || ytype == FloatPosInf)
	{
	    // +-Inf^+-Inf
	    if (xtype == FloatNegInf)
	    	return fc.zero();

	    return fc.posinf();
	}

	if (ytype == FloatNegZero || ytype == FloatPosZero)
	{
	    // +-0^+-Inf
	    if (xtype == FloatNegInf)
		return fc.posinf();

	    return y;
	}

	if (fabs(x) < fc.one())
	{
	    // |x| < 1.0
	    if (xtype == FloatNegInf)
		return fc.posinf();

	    return fc.zero();
	}

	if (fabs(x) == fc.one())
	    return fc.qnan();

	// |x| > 1.0
	if (xtype == FloatNegInf)
	    return fc.zero();

	return fc.posinf();
    }

    // y^+-0
    if (xtype == FloatNegZero || xtype == FloatPosZero)
    	return fc.one();

    if (ytype == FloatNegInf || ytype == FloatPosInf)
    {
    	// +-Inf^x
	if (xtype == FloatPosDenorm || xtype == FloatPositive)
	    return fc.posinf();

	return fc.zero();
    }

    if (ytype == FloatNegZero || ytype == FloatPosZero)
    {
	// +-0^x
	if (xtype == FloatPosDenorm || xtype == FloatPositive)
	    return fc.zero();

	return fc.posinf();
    }


    // y^x
#ifdef EBUG
    dumpfloat("pow, floor(x) = ", floor(x));
#endif
    long xl = floattol(x);
#ifdef EBUG
    printf("pow, xl = %d\n", xl);
#endif

    {	// more workarounds for braindead cfront
	Float f = floor(x);

	if (f != x || xl == LONG_MAX || xl == LONG_MIN)
	    return exp(x * ln(y));
    }

#ifdef EBUG
    printf("pow, binary method\n");
#endif

    boolean inv = FALSE;

    if (xl < 0)
    {
    	xl = -xl;
	inv = TRUE;
    }

    if (y.isNeg())
    	return fc.qnan();

    Float t = y;
    Float z = fc.one();

    while (xl)
    {
    	if (xl & 1)
	    z *= t;

	t *= t;
	xl >>= 1;
    }

    if (inv)
    	return Float(y.floatclass(), 1) / z;

    return z;
}
