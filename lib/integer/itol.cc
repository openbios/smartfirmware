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

#include "integer.h"

// checks to make on convertions.  If the dest is larger than the source
// then no error is possible.  If the dest is the same size as the source
// then check the MSB if the sign of the source and dest are different.
// If the source is larger than the dest then ensure that all extra bits
// match the sign of the source.
//
// Cases:
//	sizeof src < sizeof dest
//		sign extend
//	sizeof src == sizeof dest
//		check sign bit
//	sizeof src > sizeof dest
//		check sign bit & extra bits

#define LBITS CHAR_BIT * 4	// CHAR_BIT * sizeof (long)

// mask additional bits in uLong past those in a long.  Includes sign bit
#if LBITS < BITSPERLONG
#  define ULMASK (((1UL << (BITSPERLONG - LBITS)) - 1) << LBITS)
#else
#  define ULMASK 0UL
#endif
#define LMASK (((1UL << (BITSPERLONG - LBITS + 1)) - 1) << (LBITS - 1))

boolean
integertol(Integer const &i, long &res)
{
    IntegerClass &ic = i.integerclass();
    int bits = ic.bits();

    if (bits < LBITS)
    {
    	// sign extend
    	uLong mask = ((1UL << (LBITS - bits)) - 1) << bits;
    	uLong extra = 0UL;

    	if (i.isNeg())
	    extra = mask;

	res = i.get(0) | extra;
	return TRUE;
    }

    if (bits == LBITS)
    {
    	res = i.get(0);
	return TRUE;
    }

    // truncate
    res = i.get(0);

    // ensure that extra bits match the sign bit
    uLong mask = ULMASK;
    uLong extra = 0UL;

    if (i.isNeg())
	extra = ~0UL;

    if ((i.get(0) & mask) != (extra & mask))
	return FALSE;

    int lw = ic.size() - 1;

    for (int j = 0; j < lw; j++)
    	if (i.get(j) != extra)
	    return FALSE;

    return i.get(lw) == (extra & ic.hmask());
}

boolean
integertoul(Integer const &i, unsigned long &res)
{
    res = i.get(0);

    if (i.isNeg())
	return FALSE;

    IntegerClass &ic = i.integerclass();
    int bits = ic.bits();

    if (bits <= LBITS)
    {
    	// zero extend
	res = i.get(0);
	return TRUE;
    }

    // ensure that extra bits are zero
    uLong mask = ULMASK;

    if (i.get(0) & mask)
	return FALSE;

    int sz = ic.size();

    for (int j = 0; j < sz; j++)
    	if (i.get(j))
	    return FALSE;

    return TRUE;
}

boolean
uintegertol(Integer const &i, long &res)
{
    Integer t(iclong, i);
    res = i.get(0);
    return Integer(i.integerclass(), t) == i;
}

boolean
uintegertoul(Integer const &i, unsigned long &res)
{
    Integer t(iclong, i);
    res = i.get(0);

    if (i.isPos())
	return Integer(i.integerclass(), t) == i;

    return FALSE;
}

boolean
ltointeger(long v, Integer &res)
{
    res = Integer(res.integerclass(), v);
    return Integer(iclong, res).get(0) != (unsigned long)v;
}

boolean
ultointeger(unsigned long v, Integer &res)
{
    res = Integer(res.integerclass(), v);
    return Integer(iclong, res).get(0) != v;
}

#ifdef __LONGLONG
#define LLBITS (sizeof (long long) * 8)
#define LLMASK (1UL << (LBITS - 1))

boolean
integertoll(Integer const &i, long long &res)
{
    Integer t(iclonglong, i);
    res = i.get(0);
    return Integer(i.integerclass(), t) == i;
}

boolean
integertoll(Integer const &i, long long &res)
{
    Integer t(iclonglong, i);
    res = i.get(0);
    return i.isPos() && Integer(i.integerclass(), t) == i;
}

#endif // __LONGLONG
