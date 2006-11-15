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
#include <stdio.h>
#include "floatc.h"

//   The algorithm that is used for computing Pi, which is due to Salamin
//   and Brent, is as follows:
Float
pi(FloatClass &fc)
{
    Float a = fc.one();
    Float b = a;
    b /= fc.sqrttwo();
    Float d = fc.sqrttwo();
    Float const &half = fc.half();
#ifdef EBUG
    dumpfloat("pi, half-a = ", half);
#endif
    d -= half;
#ifdef EBUG
    dumpfloat("pi, half-b = ", half);
#endif
    int n = bits(fc.mantissa() / 8) + 1;

    for (int i = 1; i <= n; i++)
    {
#ifdef EBUG
	printf("pi, iter = %d of %d\n", i, n);
	dumpfloat("a = ", a);
	dumpfloat("b = ", b);
	dumpfloat("d = ", d);
	dumpfloat("pi, half-c = ", half);
#endif
    	Float t1 = a;
#ifdef EBUG
	dumpfloat("pi, t1a = ", t1);
	dumpfloat("pi, half-d = ", half);
#endif
	t1 += b;
#ifdef EBUG
	dumpfloat("pi, t1b = ", t1);
	dumpfloat("pi, half-e = ", half);
#endif
	t1 *= half;
#ifdef EBUG
	dumpfloat("pi, t1c = ", t1);
#endif
	b = sqrt(a * b);
#ifdef EBUG
	dumpfloat("pi, b = ", b);
#endif
	a = t1;
#ifdef EBUG
	dumpfloat("pi, a = ", a);
#endif
	t1 -= b;
#ifdef EBUG
	dumpfloat("pi, t1d = ", t1);
#endif
	t1 *= t1;
#ifdef EBUG
	dumpfloat("pi, t1e = ", t1);
#endif
	t1 *= Float(fc, 1L << i);
#ifdef EBUG
	dumpfloat("pi, t1f = ", t1);
#endif
	d -= t1;
    }

    a += b;
    a *= a;
    a /= d;
    return a;
}

Float
e(FloatClass &fc)
{
    return Float(fc, 1);
}

//   The algorithm that is used for computing Pi, which is due to Salamin
//   and Brent, is as follows:
Float
old_pi(FloatClass &fc)
{
    Float const &one = fc.one();
    Float const &half = fc.half();
    Float const &sqrttwo = fc.sqrttwo();
    Float a = one;
    Float b = one / sqrttwo;
    Float d = sqrttwo - half;
    int n = bits(fc.mantissa() / 8) + 1;

    for (int i = 1; i <= n; i++)
    {
    	Float t1 = a;
	t1 += b;
	t1 *= half;
//    	Float t1 = half * (a + b);
	Float t2 = sqrt(a * b);
	a = t1;
	b = t2;
	Float t3 = a;
	t3 -= b;
//	Float t3 = a - b;
	t3 *= t3;
	t3 *= Float(fc, 1 << i);
	d -= t3;
//	d -= Float(fc, 1 << i) * t3 * t3;
    }

    Float t = a;
    t += b;
//    Float t = a + b;
    t *= t;
    t /= d;
    return t;
//    return t * t / d;
}
