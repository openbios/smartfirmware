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

// Copyright (c) 1993 by Thomas J. Merritt & Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/cgt/stdlibx.cc,v 1.10 1998/02/14 00:05:47 parag Exp $

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdlibx.h"
#include "backtrace.h"

void
crash(const char *errstr)
{
    fprintf(stderr, "\r\ncrash: %s\r\n",
	    errstr ? errstr : "<no error message>");
    backtrace(TRUE);
#ifdef __MWERKS__
    exit(-1);
#else
    abort();
#endif
}

#ifndef __GNUC__

int
bitcount(uLong v)
{
#if BITSPERLONG > 64
    v = (v & 0x55555555555555555555555555555555) +
	    ((v >> 1) & 0x55555555555555555555555555555555);
    v = (v & 0x33333333333333333333333333333333) +
	    ((v >> 2) & 0x33333333333333333333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F;
    v += v >> 8;
    v += v >> 16;
    v += v >> 32;
    v += v >> 64;
#else
#if BITSPERLONG > 32
    v = (v & 0x5555555555555555) + ((v >> 1) & 0x5555555555555555);
    v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F;
    v += v >> 8;
    v += v >> 16;
    v += v >> 32;
#else
    v = (v & 0x55555555) + ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F;
    v += v >> 8;
    v += v >> 16;
#endif
#endif
    return v & 0xFF;
}

int
bits(uLong v)
{
    int n = 0;

#if BITSPERLONG > 64
    if (v & 0xFFFFFFFFFFFFFFFF0000000000000000)
    {
	n = 64;
	v >>= 64;
    }
#endif

#if BITSPERLONG > 32
    if (v & 0xFFFFFFFF00000000)
    {
	n += 32;
	v >>= 32;
    }
#endif

#if BITSPERLONG > 16
    if (v & 0xFFFF0000)
    {
	n += 16;
	v >>= 16;
    }
#endif

    if (v & 0xFF00)
    {
    	n += 8;
	v >>= 8;
    }

    if (v & 0xF0)
    {
    	n += 4;
	v >>= 4;
    }

    switch (v)
    {
    case 0:
    	break;
    case 1:
    	n++;
	break;
    case 2:
    case 3:
	n += 2;
	break;
    case 4:
    case 5:
    case 6:
    case 7:
	n += 3;
	break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    	n += 4;
	break;
    }

    return n;
}

int
findhighestbit(uLong bits)
{
    int n = 0;

#if BITSPERLONG > 64
    if (bits & 0xFFFFFFFFFFFFFFFF0000000000000000)
    {
    	n = 64;
    	bits >>= 64;
    }
#endif

#if BITSPERLONG > 32
    if (bits & 0xFFFFFFFF00000000)
    {
    	n += 32;
    	bits >>= 32;
    }
#endif

    if (bits & 0xFFFF0000)
    {
    	n += 16;
    	bits >>= 16;
    }

    if (bits & 0xFF00)
    {
    	n += 8;
    	bits >>= 8;
    }

    if (bits & 0xF0)
    {
    	n += 4;
	bits >>= 4;
    }

    switch (bits)
    {
    case 0:
    	return -1;
    case 1:
    	return n;
    case 2:
    case 3:
    	return n + 1;
    case 4:
    case 5:
    case 6:
    case 7:
    	return n + 2;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    	return n + 3;
    }

    return -1;
}

int
findlowestbit(uLong bits)
{
    int n = 0;

#if BITSPERLONG > 64
    if ((bits & 0xFFFFFFFFFFFFFFFF) == 0)
    {
    	n = 64;
    	bits >>= 64;
    }
#endif

#if BITSPERLONG > 32
    if ((bits & 0xFFFFFFFF) == 0)
    {
    	n += 32;
    	bits >>= 32;
    }
#endif

    if ((bits & 0xFFFF) == 0)
    {
    	n += 16;
    	bits >>= 16;
    }

    if ((bits & 0xFF) == 0)
    {
    	n += 8;
    	bits >>= 8;
    }

    if ((bits & 0xF) == 0)
    {
    	n += 4;
	bits >>= 4;
    }

    switch (bits & 0xF)
    {
    case 0:
    	return -1;
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
    case 13:
    case 15:
    	return n;
    case 2:
    case 6:
    case 10:
    case 14:
    	return n + 1;
    case 4:
    case 12:
    	return n + 2;
    case 8:
    	return n + 3;
    }

    return -1;
}

int
carryf(uLong sum, uLong a, uLong /*b*/)
{
#if 1
    return sum < a;
#else
#ifndef BRANCHESEXPENSIVE
    int carry;

    if (a & LONGMSB)
	if (b & LONGMSB)
	    carry = 1;
	else
	    if (sum & LONGMSB)
		carry = 0;
	    else
		carry = 1;
    else
	if (b & LONGMSB)
	    if (sum & LONGMSB)
		carry = 0;
	    else
		carry = 1;
	else
	    carry = 0;

    return carry;
#else
    return (((a & b) | (~sum & a) | (~sum & b)) & LONGMSB) != 0;
#endif
#endif
}

int
borrowf(uLong /*sum*/, uLong a, uLong b)
{
#if 1
    return a < b;
#else
#ifndef BRANCHESEXPENSIVE
    int carry;

    if (a & LONGMSB)
	if (b & LONGMSB)
	    carry = 1;
	else
	    if (sum & LONGMSB)
		carry = 0;
	    else
		carry = 1;
    else
	if (b & LONGMSB)
	    if (sum & LONGMSB)
		carry = 0;
	    else
		carry = 1;
	else
	    carry = 0;

    return carry;
#else
    return (((a & b) | (~sum & a) | (~sum & b)) & LONGMSB) != 0;
#endif
#endif
}

#endif /* !__GNUC__ */
