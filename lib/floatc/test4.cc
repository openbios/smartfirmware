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
    default:
    	break;
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

main(/*int argc, char **argv*/)
{
    printf("_FLOAT_MINLONGS = %d\n", _FLOAT_MINLONGS);
    printf("BITSPERLONG = %d\n", BITSPERLONG);
    printf("LONGMSB = 0x%08lX\n", LONGMSB);
    printf("Testing dtofloat function\n");
    dumpfloat("dtofloat(fcdouble, 3) = ", dtofloat(fcdouble, 3));
    dumpfloat("dtofloat(fcdouble, 3.1) = ", dtofloat(fcdouble, 3.1));
    dumpfloat("dtofloat(fcsingle, 3) = ", dtofloat(fcsingle, 3));
    dumpfloat("dtofloat(fcsingle, 3.1) = ", dtofloat(fcsingle, 3.1));

    dumpfloat("Float(fcdouble, mksp(0x00000001) = ",
	    Float(fcdouble, mksp(0x00000001)));
    dumpfloat("Float(fcdouble, mksp(0x00800001) = ",
	    Float(fcdouble, mksp(0x00800001)));
    dumpfloat("Float(fcdouble, mksp(0x80800001) = ",
	    Float(fcdouble, mksp(0x80800001)));
    dumpfloat("Float(fcdouble, mksp(0x3F800000) = ",
	    Float(fcdouble, mksp(0x3F800000)));
    dumpfloat("Float(fcdouble, mksp(0x3F800001) = ",
	    Float(fcdouble, mksp(0x3F800001)));
    dumpfloat("Float(fcdouble, mksp(0x7F800000) = ",
	    Float(fcdouble, mksp(0x7F800000)));
    dumpfloat("Float(fcdouble, mksp(0x7F800001) = ",
	    Float(fcdouble, mksp(0x7F800001)));
    dumpfloat("Float(fcdouble, mksp(0x7F800002) = ",
	    Float(fcdouble, mksp(0x7F800002)));
    dumpfloat("Float(fcdouble, mksp(0xFF800000) = ",
	    Float(fcdouble, mksp(0xFF800000)));

    dumpfloat("Float(fcsingle, mkdp(0x0, 0x00000001) = ",
	    Float(fcsingle, mkdp(0x0, 0x00000001)));
    dumpfloat("Float(fcsingle, mkdp(0x00100000, 0x00000001)) = ",
	    Float(fcsingle, mkdp(0x00100000, 0x00000001)));
    dumpfloat("Float(fcsingle, mkdp(0x80100000, 0x00000001)) = ",
	    Float(fcsingle, mkdp(0x80100000, 0x00000001)));
    dumpfloat("Float(fcsingle, mkdp(0x3FF00000, 0x00000000)) = ",
	    Float(fcsingle, mkdp(0x3FF00000, 0x00000000)));
    dumpfloat("Float(fcsingle, mkdp(0x3FF00000, 0x00000001)) = ",
	    Float(fcsingle, mkdp(0x3FF00000, 0x00000001)));
    dumpfloat("Float(fcsingle, mkdp(0x7FF00000, 0x00000000)) = ",
	    Float(fcsingle, mkdp(0x7FF00000, 0x00000000)));
    dumpfloat("Float(fcsingle, mkdp(0x7FF00000, 0x00000001)) = ",
	    Float(fcsingle, mkdp(0x7FF00000, 0x00000001)));
    dumpfloat("Float(fcsingle, mkdp(0x7FF00000, 0x00000002)) = ",
	    Float(fcsingle, mkdp(0x7FF00000, 0x00000002)));
    dumpfloat("Float(fcsingle, mkdp(0xFFF00000, 0x00000000)) = ",
	    Float(fcsingle, mkdp(0xFFF00000, 0x00000000)));

    FloatClass fcbase(4, 3);

    for (int i = 0; i < 256; i++)
    {
    	uLong vec[1];
	vec[0] = i;
	Float f(fcbase, vec);
	char label[BUFSIZ];
	sprintf(label, "Float(fcdouble, Float(fcbase, 0x%02X)) = ", i);
	dumpfloat(label, Float(fcdouble, f));
    }

    printf("Tests complete\n");
    return 0;
}
