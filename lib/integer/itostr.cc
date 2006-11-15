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
#include "integer.h"

char const *
integertostr(Integer const &i, int base)
{
    IntegerClass &ic = i.integerclass();
    int bits = ic.bits();
    int len = bits + 2;

    static char *buf = NULL;
    static int buflen = 0;

    if (len > buflen)
    {
    	if (buf != NULL)
	    delete[/*buflen*/] buf;

	buf = new char[len];
	buflen = len;
    }

    char *s = &buf[buflen];
    *--s = '\0';

    Integer t(ic);
    boolean doneg = FALSE;
    boolean neg = FALSE;

    if (base < 0)
    {
	doneg = TRUE;
	base = -base;
    }

    if (doneg && i.isNeg())
    {
	t = -i;
	neg = TRUE;
    }
    else
	t = i;

    if (bits < BITSPERLONG)
    {
    	uLong tmp = t.get();

	while (tmp)
	{
	    int dig = (int)(tmp % base);
	    tmp /= base;

	    if (dig < 10)
		*--s = '0' + dig;
	    else
		*--s = 'A' + dig - 10;
	}
    }
    else
    {
	int count = 1;
	uLong f = base;

	if (BITSPERLONG == 32 && base == 10)
	{
	    count = 9;
	    f = 0x3B9ACA00;
	}
	else if (BITSPERLONG == 32 && base == 16)
	{
	    count = 7;
	    f = 0x10000000;
	}
	else
	    while (f <= ((~0UL >> 1) / base))
	    {
		f *= base;
		count++;
	    }

	Integer factor(ic, f);
	Integer t2 = t;
	t2.urem(factor);
	t.udiv(factor);
	uLong tmp = t2.get();

	while (!t.isZero())
	{
	    for (int j = 0; j < count; j++)
	    {
		int dig = (int)(tmp % base);
		tmp /= base;

		if (dig < 10)
		    *--s = '0' + dig;
		else
		    *--s = 'A' + dig - 10;
	    }

	    t2 = t;
	    t2.urem(factor);
	    t.udiv(factor);
	    tmp = t2.get();
	}

	while (tmp)
	{
	    int dig = (int)(tmp % base);
	    tmp /= base;

	    if (dig < 10)
		*--s = '0' + dig;
	    else
		*--s = 'A' + dig - 10;
	}
    }

    if (*s == '\0')
	*--s = '0';

    if (neg)
	*--s = '-';

    return s;
}
