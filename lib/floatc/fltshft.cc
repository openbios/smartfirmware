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

#include "floatc.h"

// must set high bit of v2 if any word above hind - x + 1 is non zero


uLong
lshift(uLong *vec, int n, int lsb, int width, int b)
{
    if (b <= 0)
	return 0UL;

    int msb = n * BITSPERLONG - 1;

    if (lsb > msb)
    	return 0UL;

    int maxwidth = msb - lsb + 1;

    if (width > maxwidth)
    	width = maxwidth;

    if (width < 1)
	return 0UL;

    int msb = lsb + width - 1;
    int lind = lsb / BITSPERLONG;
    int loff = lsb % BITSPERLONG;
    uLong lmask = ~0UL << loff;
    int hind = msb / BITSPERLONG;
    int hoff = msb % BITSPERLONG;
    uLong hmask = ~0UL >> (31 - hoff);
    int x = b / BITSPERLONG;
    b %= BITSPERLONG;
    int br = 31 - b;

    if (lind == hind)
    {
    	lmask &= hmask;
	v = vec[lind] & lmask;

	if (x > 1)
	{
	    vec[lind] &= ~lmask;
	    return v ? LONGMSB : 0UL;
	}

	if (x == 1)
	{
	    vec[lind] &= ~lmask;
	    v <<= 31 - hoff;

	    if (v >> br)
	    	return (v << b) | LONGMSB;

	    return v << b;
	}

	vec[lind] |= (v << b) & lmask;
	return v >> br;
    }

    // lind != hind
    if (b == 0)
    {
	uLong v1;
	uLong v2;

	if (x == 1)
	    v2 = vec[hind] & hmask;
	if (hind - x + 1 > lind)
	    v2 = vec[hind -x + 1];
	else if (hind - x + 1 == lind)
	    v2 = vec[lind] & lmask;
	else
	    v2 = 0UL;

	if (hind - x > lind)
	    v1 = vec[hind - x];
	else if (hind - x == lind)
	    v1 = vec[lind] & lmask;
	else
	    v1 = 0UL;

    	vec[hind] = (vec[hind] & ~hmask) | (v1 & hmask);

	for (int i = hind - 1; i > lind; i--)
	    if (i - x > lind)
	    	vec[i] = vec[i - x];
	    else if (i - x == lind)
	    	vec[i] = vec[lind] & lmask;
	    else
	    	vec[i] = 0UL;

	vec[lind] &= ~lmask;

	if (v2 >> hoff)
	    return (v1 >> hoff) | (v2 << ((BITSPERLONG-1) - hoff)) |
		    (1UL << BITSPERLONG-1));

	return (v1 >> hoff) | (v2 << ((BITSPERLONG-1) - hoff));
    }
    
    uLong v1;
    uLong v2;

    if (x == 0)
	v2 = (vec[hind] & hmask) >> br;
    else if (x == 1)
    {
    	if (hind - lind == 1)
	    v2 = ((vec[hind] & hmask) << b) | ((vec[lind] & lmask) >> br);
	else
	    v2 = ((vec[hind] & hmask) << b) | (vec[hind - 1] >> br);
    }
    else if (hind - x > lind)
	v2 = (vec[hind - x + 1] << b) | (vec[hind - x] >> br);
    else if (hind - x == lind)
	v2 = (vec[lind + 1] << b) | ((vec[lind] & lmask) >> br);
    else if (hind - x + 1 == lind)
	v2 = (vec[lind] & lmask) << b;
    else
	v2 = 0UL;

    if (x == 0)
    {
    	if (hind - lind == 1)
	    v1 = ((vec[hind] & hmask) << b) | ((vec[lind] & lmask) >> br);
	else
	    v1 = ((vec[hind] & hmask) << b) | (vec[hind - 1] >> br);
    }
    else if (hind - x - 1 > lind)
	v1 = (vec[hind - x] << b) | (vec[hind - x - 1] >> br);
    else if (hind - x - 1 == lind)
	v1 = (vec[lind + 1] << b) | ((vec[lind] & lmask) >> br);
    else if (hind - x == lind)
	v1 = (vec[lind] & lmask) << b;
    else
	v1 = 0UL;

    vec[hind] = (vec[hind] & ~hind) | (v1 & hind);

    for (int i = hind - 1; i > lind; i--)
    	if (i - x - 1 > lind)
	    vec[i] = (vec[i - x] << b) | (vec[i - x - 1] >> br);
	else if (i - x - 1 == lind)
	    vec[i] = (vec[lind + 1] << b) | ((vec[lind] & lmask) >> br);
	else if (i - x  == lind)
	    vec[i] = (vec[lind] & lmask) << b;
	else
	    vec[i] = 0UL;

    if (x == 0)
	v0 = vec[lind] << b;
    else
    	v0 = 0UL;

    if (x == 0)
	vec[lind] = (vec[lind] & ~lmask) | ((vec[lind] << b) & lmask);
    else
	vec[lind] &= ~lmask;

    return (v1 >> hoff) | (v2 << ((BITSPERLONG-1) - hoff));
}

uLong
shift(uLong *vec, int n, int lsb, int width, int bits)
{
    if (bits < 0)
    	return rshift(vec, n, lsb, width, -bits);

    return lshift(vec, n, lsb, width, bits);
}
