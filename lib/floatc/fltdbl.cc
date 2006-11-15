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


#include <float.h>
#include <limits.h>

#include "floatc.h"

#ifndef BYTE_ORDER_LITTLE_ENDIAN
#  if defined(_DJGPP_SOURCE) || defined(MSDOS)
#    define BYTE_ORDER_LITTLE_ENDIAN
#  endif
#endif // LITTLE_ENDIAN

int x = DBL_MANT_DIG;

#define DBL_EXP_BITS (sizeof (double) * CHAR_BIT - DBL_MANT_DIG)
int y = DBL_EXP_BITS;

static FloatClass fcdbl(DBL_MANT_DIG - 1, DBL_EXP_BITS);

Float
dtofloat(FloatClass &fc, double x)
{
    uLong vec[2];
    uLong *xp = (uLong *)&x;

#ifdef BYTE_ORDER_LITTLE_ENDIAN
    vec[0] = xp[0];
    vec[1] = xp[1];
#else
    vec[0] = xp[1];
    vec[1] = xp[0];
#endif
    Float f(fcdbl, vec);

    if (fc.mantissa() == DBL_MANT_DIG - 1 && fc.exponent() == DBL_EXP_BITS)
    	return f;

    return Float(fc, f);
}

double
floattod(Float const &x)
{
    double y;
    uLong *yp = (uLong *)&y;
    Float f(fcdbl, x);

#ifdef BYTE_ORDER_LITTLE_ENDIAN
    yp[0] = f.get(0);
    yp[1] = f.get(1);
#else
    yp[1] = f.get(0);
    yp[0] = f.get(1);
#endif

    return y;
}
