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
#include <math.h>
#include <integer.h>

#include "floatc.h"

#if UNUSED
static Float
poly(Float const &x, Float const *coeff, int n)
{
    Float y(x.floatclass(), coeff[n]);

    for (int i = n - 1; i >= 0; i--)
    	y = y * x + coeff[i];

    return y;
}

static void
coscoeff(Float *coeff, int n)
{
    // calculate coefficients for talyor series expansion of cos(x)
    Float one = coeff[0].floatclass().one();
    Float mult = one;
    Float fact = one;
    Float invfact = one;
    coeff[0] = one;

    for (int i = 1; i <= n; i++)
    {
    	// [(-1)^k/(2k)!]*x^(2k),   c[k] = (-1)^k/(2k)!
	fact *= mult;
	mult += one;
	fact *= mult;
	mult += one;
	coeff[i] = one / fact;

	if (i & 1)
	    coeff[i].neg();
    }
}
#endif

Float
sin(Float const &x)
{
    return dtofloat(x.floatclass(), sin(floattod(x)));
}

Float
cos(Float const &x)
{
    return sin(x - x.floatclass().pi() / x.floatclass().two());
}

Float
tan(Float const &x)
{
    return sin(x) / cos(x);
}

Float
asin(Float const &x)
{
    return x;
}

Float
acos(Float const &x)
{
    return x;
}

Float
atan(Float const &x)
{
    return x;
}

Float
atan(Float const &x, Float const &y)
{
    return x / y;
}

Float
sinh(Float const &x)
{
    return x;
}

Float
cosh(Float const &x)
{
    return x;
}

Float
tanh(Float const &x)
{
    return x;
}

Float
asinh(Float const &x)
{
    return x;
}

Float
acosh(Float const &x)
{
    return x;
}

Float
atanh(Float const &x)
{
    return x;
}
