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
#include <stdio.h>
#define protected public
#include "floatc.h"

char const *
floattraptype(enum FloatTrapType t)
{
    switch (t)
    {
    case FloatFormatMismatch: return "FormatMismatch";
    case FloatInvalidOperation: return "InvalidOperation";
    case FloatDivideByZero: return "DivideByZero";
    case FloatOverflow: return "Overflow";
    case FloatUnderflow: return "Underflow";
    case FloatInexact: return "Inexact";
    default: break;
    }

    return "Unknown";
}

char const *
floatoperationtype(enum FloatOperationType t)
{
    switch (t)
    {
    case FloatNone: return "None";
    case FloatSignalingNaN: return "SignalingNaN";
    case FloatAddition: return "Addition";
    case FloatSubtraction: return "Subtraction";
    case FloatReverseSubtraction: return "ReverseSubtraction";
    case FloatMultiplication: return "Multiplication";
    case FloatDivision: return "Division";
    case FloatModulus: return "Modulus";
    case FloatSquareRoot: return "SquareRoot";
    case FloatConvertion: return "Convertion";
    case FloatComparison: return "Comparison";
    case FloatNextAfter: return "NextAfter";
    }

    return "Unknown";
}

boolean
testtraphandler(enum FloatTrapType t, enum FloatOperationType optype,
	Float const &a, Float const &b, Float &r)
{
    printf("Trap: type = %s, op = %s, status = 0x%02lX\n", floattraptype(t),
	    floatoperationtype(optype), a.floatclass().status());
    dumpfloat("\ta = ", a);
    dumpfloat("\tb = ", b);
    dumpfloat("\tr = ", r);
    a.floatclass().status(0);		// clear status flags
    return TRUE;
}

void
initfloatclass(FloatClass &fc)
{
    fc.trap(FloatFormatMismatch, testtraphandler);
    fc.trap(FloatInvalidOperation, testtraphandler);
    fc.trap(FloatDivideByZero, testtraphandler);
    fc.trap(FloatOverflow, testtraphandler);
    fc.trap(FloatUnderflow, testtraphandler);
    fc.trap(FloatInexact, testtraphandler);
    fc.trapmask(0x003F);
}

Float
mksp(uLong a)
{
    return Float(fcsingle, &a);
}

Float
mkdp(uLong ah, uLong al)
{
    uLong vec[2];
    vec[0] = al;
    vec[1] = ah;
    return Float(fcdouble, vec);
}

char *strings[] =
{
    "",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "123",
    "1234",
    "12345",
    "123456",
    "1234567",
    "12345678",
    "123456789",
    "1234567890",
    "12345678901",
    "123456789012",
    "1234567890123",
    "12345678901234",
    "123456789012345",
    "1234567890123456",
    "12345678901234567",
    "123456789012345678",
    "1234567890123456789",
    "12345678901234567890",
    "123456789012345678901",
    "1234567890123456789012",
    "12345678901234567890123",
    "123456789012345678901234",
    "1234567890123456789012345",
    "12345678901234567890123456",
    "123456789012345678901234567",
    "1234567890123456789012345678",
    "12345678901234567890123456789",
    "123456789012345678901234567890",
    "1.23E+01",
    "1.23E+02",
    "1.23E+04",
    "1.23E+08",
    "1.23E+16",
    "1.23E+32",
    "1.23E+64",
    "1.23E+100",
    "1.23E+128",
    "1.23E+256",
    "1.23E+512",
    "1.23E+1000",
    "1.23E+1024",
    "1.23E+2048",
    "1.23E+4096",
    "1.23E+8192",
    "1.23E+10000",
    "1.23E+100000",
    "1.23E+1000000",
    "1.23E+10000000",
    NULL,
};

main(/*int argc, char **argv*/)
{
    printf("_FLOAT_MINLONGS = %d\n", _FLOAT_MINLONGS);
    printf("BITSPERLONG = %d\n", BITSPERLONG);
    printf("LONGMSB = 0x%08lX\n", LONGMSB);
    printf("Testing string conversions\n");

    int i;

    Float xs = fcsingle.one();

    for (i = 1; i < 16; i++, xs++)
    {
	long l = floattol(xs);

    	dumpfloat("xs = ", xs);
	printf("l = %d\n", (int)l);

	if (l != i)
	    printf("Test failed!\n");
    }

    Float xd = fcdouble.one();

    for (i = 1; i < 16; i++, xd++)
    {
	long l = floattol(xd);

    	dumpfloat("xd = ", xd);
	printf("l = %d\n", (int)l);

	if (l != i)
	    printf("Test failed!\n");
    }

    Float xq = fcquad.one();

    for (i = 1; i < 16; i++, xq++)
    {
	long l = floattol(xq);

    	dumpfloat("xq = ", xq);
	printf("l = %d\n", (int)l);

	if (l != i)
	    printf("Test failed!\n");
    }

    Float xo = fcocta.one();

    for (i = 1; i < 16; i++, xo++)
    {
	long l = floattol(xo);

    	dumpfloat("xo = ", xo);
	printf("l = %d\n", (int)l);

	if (l != i)
	    printf("Test failed!\n");
    }

    for (i = 0; strings[i]; i++)
    {
	char buf[256];
	Float fstr(fcsingle, strings[i]);
	sprintf(buf, "Float(%s) = ", strings[i]);
    	dumpfloat(buf, fstr);
    }

    for (i = 0; strings[i]; i++)
    {
	char buf[256];
	Float fstr(fcdouble, strings[i]);
	sprintf(buf, "Float(%s) = ", strings[i]);
    	dumpfloat(buf, fstr);
    }

    for (i = 0; strings[i]; i++)
    {
	char buf[256];
	Float fstr(fcocta, strings[i]);
	sprintf(buf, "Float(%s) = ", strings[i]);
    	dumpfloat(buf, fstr);
    }

    printf("Tests complete\n");
    return 0;
}
