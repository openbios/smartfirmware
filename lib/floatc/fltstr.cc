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

#include "floatc.h"

char const *
floattostr(Float const &x, int width, int prec, boolean sci)
{
    switch (x.floattype())
    {
    case FloatNaN:
    	return "NaN";
    case FloatQuietNaN:
    	return "QNaN";
    case FloatPosInf:
    	return "+Inf";
    case FloatPosZero:
    	return "0";
    case FloatNegZero:
    	return "-0";
    case FloatNegInf:
    	return "-Inf";

    default:
    	break;
    }

    static char buf[BUFSIZ];

    // convert x to 0.1..0.99999... * 10^exp

    if (width)
    	if (prec >= 0)
	    if (sci)
		sprintf(buf, "%*.*E", width, prec, floattod(x));
	    else
		sprintf(buf, "%*.*f", width, prec, floattod(x));
	else
	    if (sci)
		sprintf(buf, "%*E", width, floattod(x));
	    else
		sprintf(buf, "%*f", width, floattod(x));
    else
	if (sci)
	    sprintf(buf, "%E", floattod(x));
	else
	    sprintf(buf, "%f", floattod(x));

    return buf;
}

Float
strtofloat(FloatClass &fc, char const *s, char const **end)
{
    boolean neg = FALSE;
    int maxdigs = ((fc.mantissa() * 146) / 485) + 1;
    Float t(fc);
    int scale = 0;
    int digs = 0;

    if ((*s == '+' || *s == '-') && isdigit(s[1]))
    {
	if (*s == '-')
	    neg = TRUE;

	s++;
    }

    for (; isdigit(*s); s++)
    	if (digs < maxdigs)
	{
	    t *= fc.ten();
	    t += Float(fc, *s - '0');
	    digs++;
	}
	else
	    scale++;

    if (*s == '.')
    {
	s++;

	for (; isdigit(*s); s++)
	    if (digs < maxdigs)
	    {
		t *= fc.ten();
		t += Float(fc, *s - '0');
		digs++;
		scale--;
	    }
    }

    Float mult(fc, 1);

    if ((*s == 'e' || *s == 'E') &&
	    (((s[1] == '+' || s[1] == '-') && isdigit(s[2])) || isdigit(s[1])))
    {
	Float e(fc);
	boolean eneg = 0;
	s++;

	if (*s == '+')
	    s++;
	else if (*s == '-')
	{
	    eneg = 1;
	    s++;
	}

	while (isdigit(*s))
	{
	    e *= fc.ten();
	    e += Float(fc, *s++ - '0');
	}

	if (eneg)
	    e.neg();

	if (scale != 0)
	    e += Float(fc, scale);

	mult = pow(fc.ten(), e);
    }
    else if (scale != 0)
    	mult = pow(fc.ten(), Float(fc, scale));

    t *= mult;

    if (neg)
	t.neg();

    if (end != NULL)
	*end = s;

    return t;
}

Float::Float(FloatClass &fc, char const *s, char const **end)
{
    Init(fc);
    *this = strtofloat(fc, s, end);
}
