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
dumptrap(char const *name, FloatClass &fc, enum FloatTrapType t,
	FILE *file = stdout)
{
    char const *trapname = floattraptype(t);
    fprintf(file, "%s.trapenable(%s) = %s\n", name, trapname,
	    fc.trapenable(t) ? "enabled" : "disabled");

    if (fc.trap(t) == NULL)
	fprintf(file, "%s.trapenable(%s) = NULL\n", name, trapname);
    else if (fc.trap(t) == testtraphandler)
	fprintf(file, "%s.trap(%s) = testtraphandler\n", name, trapname);
    else
	fprintf(file, "%s.trap(%s) = %p\n", name, trapname, fc.trap(t));
}

void
dumpfloatclass(char const *name, FloatClass &fc, FILE *file = stdout,
	boolean fp = TRUE)
{
    fprintf(file, "%s.bits() = %d\n", name, fc.bits());
    fprintf(file, "%s.mantissa() = %d\n", name, fc.mantissa());
    fprintf(file, "%s.exponent() = %d\n", name, fc.exponent());
    fprintf(file, "%s.size() = %d\n", name, fc.size());
    fprintf(file, "%s.msize() = %d\n", name, fc.msize());
    fprintf(file, "%s.esize() = %d\n", name, fc.esize());
    fprintf(file, "%s.alloc() = %s\n", name, fc.alloc() ? "TRUE" : "FALSE");
    fprintf(file, "%s.mhind() = %d\n", name, fc.mhind());
    fprintf(file, "%s.mhshift() = %d\n", name, fc.mhshift());
    fprintf(file, "%s.mhmask() = 0x%08lX\n", name, fc.mhmask());
    fprintf(file, "%s.mlind() = %d\n", name, fc.mlind());
    fprintf(file, "%s.mlshift() = %d\n", name, fc.mlshift());
    fprintf(file, "%s.mlmask() = 0x%08lX\n", name, fc.mlmask());
    fprintf(file, "%s.ehind() = %d\n", name, fc.ehind());
    fprintf(file, "%s.ehshift() = %d\n", name, fc.ehshift());
    fprintf(file, "%s.ehmask() = 0x%08lX\n", name, fc.ehmask());
    fprintf(file, "%s.elind() = %d\n", name, fc.elind());
    fprintf(file, "%s.elshift() = %d\n", name, fc.elshift());
    fprintf(file, "%s.elmask() = 0x%08lX\n", name, fc.elmask());
    fprintf(file, "%s.efmask() = 0x%08lX\n", name, fc.efmask());
    fprintf(file, "%s.sind() = %d\n", name, fc.sind());
    fprintf(file, "%s.sshift() = %d\n", name, fc.sshift());
    fprintf(file, "%s.smask() = 0x%08lX\n", name, fc.smask());
    char label[BUFSIZ];
    sprintf(label, "%s.zero() = ", name);
    dumpfloat(label, fc.zero(), file, fp);
    sprintf(label, "%s.one() = ", name);
    dumpfloat(label, fc.one(), file, fp);
    sprintf(label, "%s.two() = ", name);
    dumpfloat(label, fc.two(), file, fp);
    sprintf(label, "%s.four() = ", name);
    dumpfloat(label, fc.four(), file, fp);
    sprintf(label, "%s.minusone() = ", name);
    dumpfloat(label, fc.minusone(), file, fp);
    sprintf(label, "%s.ten() = ", name);
    dumpfloat(label, fc.ten(), file, fp);
    sprintf(label, "%s.half() = ", name);
    dumpfloat(label, fc.half(), file, fp);
    sprintf(label, "%s.fourth() = ", name);
    dumpfloat(label, fc.fourth(), file, fp);
    sprintf(label, "%s.tenth() = ", name);
    dumpfloat(label, fc.tenth(), file, fp);

    if (fc.constantset(FloatPi))
    {
	sprintf(label, "%s.pi() = ", name);
	dumpfloat(label, fc.pi(), file, fp);
    }

    if (fc.constantset(FloatE))
    {
	sprintf(label, "%s.e() = ", name);
	dumpfloat(label, fc.e(), file, fp);
    }

    if (fc.constantset(FloatSqrtTwo))
    {
	sprintf(label, "%s.sqrttwo() = ", name);
	dumpfloat(label, fc.sqrttwo(), file, fp);
    }

    sprintf(label, "%s.posinf() = ", name);
    dumpfloat(label, fc.posinf(), file, fp);
    sprintf(label, "%s.neginf() = ", name);
    dumpfloat(label, fc.neginf(), file, fp);
    sprintf(label, "%s.nan() = ", name);
    dumpfloat(label, fc.nan(), file, fp);
    sprintf(label, "%s.qnan() = ", name);
    dumpfloat(label, fc.qnan(), file, fp);
    dumptrap(name, fc, FloatFormatMismatch, file);
    dumptrap(name, fc, FloatInvalidOperation, file);
    dumptrap(name, fc, FloatDivideByZero, file);
    dumptrap(name, fc, FloatOverflow, file);
    dumptrap(name, fc, FloatUnderflow, file);
    dumptrap(name, fc, FloatInexact, file);
}

inline void
dumpfloatclass(char const *name, FloatClass &fc, boolean fp,
	FILE *file = stdout)
{
    dumpfloatclass(name, fc, file, fp);
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

void
floatclasstest()
{
    FloatClass fc_mini(2, 2);
    dumpfloatclass("fc_mini", fc_mini, FALSE);
    FloatClass fc_single(23, 8);
    dumpfloatclass("fc_single", fc_single);
    FloatClass fc_double(52, 11);
    dumpfloatclass("fc_double", fc_double);
    FloatClass fc_extended(64, 15);
    dumpfloatclass("fc_extended", fc_extended, FALSE);
    FloatClass fc_quad1(112, 15);
    dumpfloatclass("fc_quad1", fc_quad1, FALSE);
    FloatClass fc_quad2(96, 31);
    dumpfloatclass("fc_quad2", fc_quad2, FALSE);
    FloatClass fc_t1(32, 32);
    dumpfloatclass("fc_t1", fc_t1, FALSE);
    FloatClass fc_t2(48, 70);
    dumpfloatclass("fc_t2", fc_t2, FALSE);
    FloatClass fc_t3(48, 60);
    dumpfloatclass("fc_t3", fc_t3, FALSE);
}

void
floatconstructtest()
{
    FloatClass fc(52, 11);
    dumpfloat("Float() = ", Float(fc));
    dumpfloat("Float(20) = ", Float(fc, 20));
    dumpfloat("Float(200) = ", Float(fc, 200));
    Float f0(fc);
    Float f20(fc, 20);
    Float f200(fc, 200);
    Float f200x = f200;
    dumpfloat("f0 = ", f0);
    dumpfloat("f20 = ", f20);
    dumpfloat("f200 = ", f200);
    dumpfloat("f200x = ", f200x);
//    Float fstr(fc, "-123456789.0123456789e+20");
    Float fstr(fc, "-1234567890123456789");
//    Float fstr(fc, -123456789);
    dumpfloat("fstr = ", fstr);
    Float fx(fc);
    dumpfloat("(fx = fstr) = ", fx = fstr);
}

void
floatgetsettest(char const *name, FloatClass &fc)
{
    int exp = fc.exponent();
    int mant = fc.mantissa();

    Float x(fc);
    int i;

    for (i = -1; i <= mant; i++)
    {
    	x.setmbit(i);
	char label[BUFSIZ];
	sprintf(label, "%s.setmbit(%d) = ", name, i);
	dumpfloat(label, x, FALSE);
    }

    x.makezero();

    for (i = -1; i <= exp; i++)
    {
    	x.setebit(i);
	char label[BUFSIZ];
	sprintf(label, "%s.setebit(%d) = ", name, i);
	dumpfloat(label, x, FALSE);
    }

    x.makezero();

    for (i = -1; i <= exp + mant + 1; i++)
    {
    	x.setbit(i);
	char label[BUFSIZ];
	sprintf(label, "%s.setbit(%d) = ", name, i);
	dumpfloat(label, x, FALSE);
    }
}

void
floatgetsettest()
{
    FloatClass fc_t1(32, 32);
    floatgetsettest("fc_t1", fc_t1);
    FloatClass fc_t2(48, 70);
    floatgetsettest("fc_t2", fc_t2);
    FloatClass fc_t3(48, 60);
    floatgetsettest("fc_t3", fc_t3);
}

void
floatshifttest(char const *name, FloatClass &fc)
{
    int exp = fc.exponent();
    int mant = fc.mantissa();
    int bits = exp + mant + 1;
    int words = fc.size();
    int hbits = bits % BITSPERLONG;
    uLong hmask;

    if (hbits)
    	hmask = (1UL << hbits) - 1;
    else
	hmask = ~0UL;

    uLong *vec = new uLong[words];
    int i;

    for (i = 0; i < words; i++)
    	if (i & 1)
	    vec[i] = 0xDEADBEEF;
	else
	    vec[i] = 0x5555AAAA;

    vec[words - 1] &= hmask;

    for (i = 0; i <= mant + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];

	if (i == 0)
	{
	    sprintf(label, "%s, fx = ", name);
	    dumpfloat(label, fx, FALSE);
	}

	uLong out = fx.mlshift(i);
	sprintf(label, "%s, fx.mlshift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    for (i = 0; i <= mant + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];
	uLong out = fx.mrshift(i);
	sprintf(label, "%s, fx.mrshift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    for (i = 0; i <= exp + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];
	uLong out = fx.elshift(i);
	sprintf(label, "%s, fx.elshift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    for (i = 0; i <= exp + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];
	uLong out = fx.ershift(i);
	sprintf(label, "%s, fx.ershift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    for (i = 0; i <= exp + mant + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];
	uLong out = fx.lshift(i);
	sprintf(label, "%s, fx.lshift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    for (i = 0; i <= exp + mant + BITSPERLONG; i++)
    {
	Float fx(fc, vec);
	char label[BUFSIZ];
	uLong out = fx.rshift(i);
	sprintf(label, "%s, fx.rshift(%d) = (out = 0x%08lX), ", name, i, out);
	dumpfloat(label, fx, FALSE);
    }

    delete[/*words*/] vec;
}

void
floatprottest()
{
    floatgetsettest();
    FloatClass fc(52, 11);
    Float f0(fc);
    Float f1(fc, 1);
    Float f16(fc, 16);
    Float f256(fc, 256);
    Float f257(fc, 257);
    Float fx(fc);
	
    dumpfloat("f1 = ", f1);
    dumpfloat("f16 = ", f16);
    f1.incexp(4);
    dumpfloat("f1.incexp(4) = ", f1);
    f16.decexp(4);
    dumpfloat("f16.decexp(4) = ", f16);

    Float f3(fc, 3);
    floatshifttest("fc", fc);
    FloatClass fc_t1(32, 32);
    floatshifttest("fc_t1", fc_t1);
    FloatClass fc_t2(48, 70);
    floatshifttest("fc_t2", fc_t2);
    FloatClass fc_t3(48, 60);
    floatshifttest("fc_t3", fc_t3);

//    void and(Float const &f);
//    void mand(Float const &f);
//    void eand(Float const &f);
//    void or(Float const &f);
//    void mor(Float const &f);
//    void eor(Float const &f);
//    void compl();
//    void mcompl();
//    void ecompl();
}

void
floatmisctest()
{
    FloatClass fc(52, 11);
    // test fint
    Float x(fc);
    int i;

    for (i = 0; i <= 40; i++)
    {
    	Float y = x;
	dumpfloat("y = ", y);
	int v = y.fint(0);
	dumpfloat("fint(y, 0) -> ", y);
	printf("fint(y, 0) = %d\n", v);
	x.add(fc.fourth());
    }

    x.makezero();

    for (i = 0; i <= 40; i++)
    {
    	Float y = x;
	dumpfloat("y = ", y);
	int v = y.fint(-1);
	dumpfloat("fint(y, -1) -> ", y);
	printf("fint(y, -1) = %d\n", v);
	x.add(fc.fourth());
    }

    x.makezero();

    for (i = 0; i <= 40; i++)
    {
    	Float y = x;
	dumpfloat("y = ", y);
	int v = y.fint(1);
	dumpfloat("fint(y, 1) -> ", y);
	printf("fint(y, 1) = %d\n", v);
	x.add(fc.fourth());
    }

    // test nextafter
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

Float
addsingle(uLong a, uLong b)
{
    return mksp(a) + mksp(b);
}

Float
adddouble(uLong ah, uLong al, uLong bh, uLong bl)
{
    return mkdp(ah, al) + mkdp(bh, bl);
}

void
floataddtest()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f0(fc);
    Float f1(fc, 1);
    Float f20(fc, 20);
    Float f200(fc, 200);
    Float fpinf(fc);
    fpinf.makeinf();
    Float fninf = fpinf;
    fninf.neg();
    Float fnan(fc);
    fnan.makenan();
    Float fqnan(fc);
    fqnan.makequietnan();
    Float fn0 = f0;
    fn0.neg();

    // misc. tests
    Float fa(fc);
    dumpfloat("fa = ", fa);
    fa.add(f200);
    dumpfloat("fa.add(f200); fa = ", fa);
    fa = Float(fc, 1);
    dumpfloat("fa = ", fa);
    fa.add(Float(fc, 1));
    dumpfloat("fa.add(Float(1)) = ", fa);
    fa = Float(fc, 2);
    dumpfloat("Float(2) = ", fa);
    fa.add(Float(fc, 3));
    dumpfloat("Float(2).add(Float(3)) = ", fa);
    fa = Float(fc, 13);
    dumpfloat("Float(13) = ", fa);
    fa.add(Float(fc, 13));
    dumpfloat("Float(13).add(Float(13)) = ", fa);

    Float one(fc, 1);
    Float n = one;

    dumpfloat("Float(1) = ", one);

    for (int i = 0; i < 60; i++)
    {
    	Float t = n + n;
	n = t + one;
	dumpfloat("n <- n + n + 1 = ", n);
    }

    // 		Value		Name
    //		+0		f0
    //		-0		fn0
    //		+1		f1
    //		+20		f20
    //		+200		f200
    //		+Inf		fpinf
    //		-Inf		fninf
    //		NaN		fnan
    //		QNaN		fqnan

    // White box tests
    //	1.  Different formats
    FloatClass fcsingle(23, 8);
    initfloatclass(fcsingle);
    dumpfloat("Float(fcsingle, 2) + Float(fc, 3) = ",
	    Float(fcsingle, 2) + Float(fc, 3));

    //	2.  Exceptional cases
    //	    a.  A is NaN
    Float fn(fc);
    fn = fnan;
    fn.add(f20);
    dumpfloat("NaN + 20 = ", fn);

    //	    b.  A is QNaN
    fn = fqnan;
    fn.add(f20);
    dumpfloat("QNaN + 20 = ", fn);

    //	    c.  A is PInf
    fn = fpinf;
    fn.add(f20);
    dumpfloat("PInf + 20 = ", fn);

    //	    d.  A is NInf
    fn = fninf;
    fn.add(f20);
    dumpfloat("NInf + 20 = ", fn);

    //	    e.  A is PZero
    fn = f0;
    fn.add(f20);
    dumpfloat("+0 + 20 = ", fn);

    //	    f.  A is NZero
    fn = fn0;
    fn.add(f20);
    dumpfloat("-0 + 20 = ", fn);

    //	    g.  B is NaN
    fn = f20;
    fn.add(fnan);
    dumpfloat("20 + NaN = ", fn);

    //	    h.  B is QNaN
    fn = f20;
    fn.add(fqnan);
    dumpfloat("20 + QNaN = ", fn);

    //	    i.  B is PInf
    fn = f20;
    fn.add(fpinf);
    dumpfloat("20 + PInf = ", fn);

    //	    j.  B is NInf
    fn = f20;
    fn.add(fninf);
    dumpfloat("20 + NInf = ", fn);

    //	    k.  B is PZero
    fn = f20;
    fn.add(f0);
    dumpfloat("20 + 0 = ", fn);

    //	    l.  B is NZero
    fn = f20;
    fn.add(fn0);
    dumpfloat("20 + -0 = ", fn);

    //	3.  Different signs of A and B
    fn = f20;
    fn.neg();
    fn.add(f200);
    dumpfloat("-20 + 200 = ", fn);

    //	4.  2*A < B
    fn = f20;
    fn.add(f200);
    dumpfloat("20 + 200 = ", fn);

    //	5.  A > 2*B
    fn = f200;
    fn.add(f20);
    dumpfloat("200 + 20 = ", fn);

    //	6.  A ~= B
    //	    a.  A and B are Positive
    dumpfloat("0x0F000002 + 0x0F000004 = ", addsingle(0x0F000002, 0x0F000004));

    //	    b.  A and B are PosDenorm
    dumpfloat("0x00000001 + 0x00000002 = ", addsingle(0x00000001, 0x00000002));

    //	7.  A + B carry is 0
    // (covered above)
    //	8.  A + B carry is 1
    //	    a.  neither A or B are denormals
    // (covered above)
    //	    b.  either A or B is denormal
    dumpfloat("0x00401000 + 0x00400000 = ", addsingle(0x00401000, 0x00400000));

    //	9.  A + B carry is 2
    // (covered above)
    //	10. A + B carry is 3
    dumpfloat("0x10401000 + 0x10400000 = ", addsingle(0x10401000, 0x10400000));

    //	11. A + B lowbits = 0
    //	    a.  overflow
    dumpfloat("0x7F7FFFFE + 0x74000000 = ", addsingle(0x7F7FFFFE, 0x74000000));
    dumpfloat("0x7F7FFFFF + 0x73800000 = ", addsingle(0x7F7FFFFF, 0x73800000));

    //	    b.  no overflow
    //	12. A + B lowbits > 0
    //	    a.  rm = RoundToNearest
    //		i.  lowbits < 0x8000000
    //		ii. lowbits == 0x8000000
    //		iii.lowbits > 0x8000000
    dumpfloat("0x7F7FFFFF + 0x73FFFFFF = ", addsingle(0x7F7FFFFF, 0x73FFFFFF));
    //	    b.  rm = RoundTowardPosInf
    //		i.  A + B is pos
    //		ii. A + B is neg
    //	    c.  rm = RoundTowardNegInf
    //		i.  A + B is pos
    //		ii. A + B is neg
    //	    d.  rm = RoundTowardZero
    //	13. A <<< B
    //	    a.  rm = ToNearest
    //	    b.  rm = TowardPosInf
    //		i.  A is pos
    //		ii. A is neg
    //	    c.  rm = TowardNegInf
    //		i.  A is pos
    //		ii. A is neg
    //	14. A >>> B
    //	15. overflow occurs
}

Float
subsingle(uLong a, uLong b)
{
    return mksp(a) - mksp(b);
}

Float
subdouble(uLong ah, uLong al, uLong bh, uLong bl)
{
    return mkdp(ah, al) - mkdp(bh, bl);
}

void
floatsub1test()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f0(fc);
    Float f1(fc, 1);
    Float f20(fc, 20);
    Float f200(fc, 200);
    Float fpinf(fc);
    fpinf.makeinf();
    Float fninf = fpinf;
    fninf.neg();
    Float fnan(fc);
    fnan.makenan();
    Float fqnan(fc);
    fqnan.makequietnan();
    Float fn0 = f0;
    fn0.neg();

    // misc. tests
    Float fa(fc);
    dumpfloat("fa = ", fa);
    fa.sub(f200);
    dumpfloat("fa.sub(f200); fa = ", fa);
    fa = Float(fc, 1);
    dumpfloat("fa = ", fa);
    fa.sub(Float(fc, 1));
    dumpfloat("fa.sub(Float(1)) = ", fa);
    fa = Float(fc, 2);
    dumpfloat("Float(2) = ", fa);
    fa.sub(Float(fc, 3));
    dumpfloat("Float(2).sub(Float(3)) = ", fa);
    fa = Float(fc, 13);
    dumpfloat("Float(13) = ", fa);
    fa.sub(Float(fc, 13));
    dumpfloat("Float(13).sub(Float(13)) = ", fa);
    Float one(fc, 1);

    // 		Value		Name
    //		+0		f0
    //		-0		fn0
    //		+1		f1
    //		+20		f20
    //		+200		f200
    //		+Inf		fpinf
    //		-Inf		fninf
    //		NaN		fnan
    //		QNaN		fqnan

    // White box tests
    //	1.  Different formats
    FloatClass fcsingle(23, 8);
    initfloatclass(fcsingle);
    dumpfloat("Float(fcsingle, 2) - Float(fc, 3) = ",
	    Float(fcsingle, 2) - Float(fc, 3));

    //	2.  Exceptional cases
    //	    a.  A is NaN
    Float fn(fc);
    fn = fnan;
    fn.sub(f20);
    dumpfloat("NaN - 20 = ", fn);

    //	    b.  A is QNaN
    fn = fqnan;
    fn.sub(f20);
    dumpfloat("QNaN - 20 = ", fn);

    //	    c.  A is PInf
    fn = fpinf;
    fn.sub(f20);
    dumpfloat("PInf - 20 = ", fn);

    //	    d.  A is NInf
    fn = fninf;
    fn.sub(f20);
    dumpfloat("NInf - 20 = ", fn);

    //	    e.  A is PZero
    fn = f0;
    fn.sub(f20);
    dumpfloat("+0 - 20 = ", fn);

    //	    f.  A is NZero
    fn = fn0;
    fn.sub(f20);
    dumpfloat("-0 - 20 = ", fn);

    //	    g.  B is NaN
    fn = f20;
    fn.sub(fnan);
    dumpfloat("20 - NaN = ", fn);

    //	    h.  B is QNaN
    fn = f20;
    fn.sub(fqnan);
    dumpfloat("20 - QNaN = ", fn);

    //	    i.  B is PInf
    fn = f20;
    fn.sub(fpinf);
    dumpfloat("20 - PInf = ", fn);

    //	    j.  B is NInf
    fn = f20;
    fn.sub(fninf);
    dumpfloat("20 - NInf = ", fn);

    //	    k.  B is PZero
    fn = f20;
    fn.sub(f0);
    dumpfloat("20 - 0 = ", fn);

    //	    l.  B is NZero
    fn = f20;
    fn.sub(fn0);
    dumpfloat("20 - -0 = ", fn);

    //	3.  Different signs of A and B
    fn = f20;
    fn.neg();
    fn.sub(f200);
    dumpfloat("-20 - 200 = ", fn);

    //	4.  2*A < B
    fn = f20;
    fn.sub(f200);
    dumpfloat("20 - 200 = ", fn);

    //	5.  A > 2*B
    fn = f200;
    fn.sub(f20);
    dumpfloat("200 - 20 = ", fn);

    //	6.  A ~= B
    //	    a.  A and B are Positive
    dumpfloat("0x0F000002 - 0x0F000004 = ", subsingle(0x0F000002, 0x0F000004));
    dumpfloat("0x0F000004 - 0x0F000002 = ", subsingle(0x0F000004, 0x0F000002));

    //	    b.  A and B are PosDenorm
    dumpfloat("0x00000001 - 0x00000002 = ", subsingle(0x00000001, 0x00000002));
    dumpfloat("0x00000002 - 0x00000001 = ", subsingle(0x00000002, 0x00000001));

    //	7.  A - B carry is 0
    // (covered above)
    //	8.  A - B carry is 1
    //	    a.  neither A or B are denormals
    // (covered above)
    //	    b.  either A or B is denormal
    dumpfloat("0x00401000 - 0x00400000 = ", subsingle(0x00401000, 0x00400000));
    dumpfloat("0x00400000 - 0x00401000 = ", subsingle(0x00400000, 0x00401000));

    //	9.  A - B carry is 2
    // (covered above)
    //	10. A - B carry is 3
    dumpfloat("0x10401000 - 0x10400000 = ", subsingle(0x10401000, 0x10400000));
    dumpfloat("0x10400000 - 0x10401000 = ", subsingle(0x10400000, 0x10401000));

    //	11. A - B lowbits = 0
    //	    a.  overflow
    dumpfloat("0x7F7FFFFE - 0x74000000 = ", subsingle(0x7F7FFFFE, 0x74000000));
    dumpfloat("0x74000000 - 0x7F7FFFFE = ", subsingle(0x74000000, 0x7F7FFFFE));
    dumpfloat("0x7F7FFFFF - 0x73800000 = ", subsingle(0x7F7FFFFF, 0x73800000));
    dumpfloat("0x73800000 - 0x7F7FFFFF = ", subsingle(0x73800000, 0x7F7FFFFF));

    //	    b.  no overflow
    //	12. A - B lowbits > 0
    //	    a.  rm = RoundToNearest
    //		i.  lowbits < 0x8000000
    //		ii. lowbits == 0x8000000
    //		iii.lowbits > 0x8000000
    dumpfloat("0x7F7FFFFF - 0x73FFFFFF = ", subsingle(0x7F7FFFFF, 0x73FFFFFF));
    dumpfloat("0x73FFFFFF - 0x7F7FFFFF = ", subsingle(0x73FFFFFF, 0x7F7FFFFF));
}

void
floatsub2test()
{
    //	    b.  rm = RoundTowardPosInf
    //		i.  A - B is pos
    //		ii. A - B is neg
    //	    c.  rm = RoundTowardNegInf
    //		i.  A - B is pos
    //		ii. A - B is neg
    //	    d.  rm = RoundTowardZero
    //	13. A <<< B
    //	    a.  rm = ToNearest
    //	    b.  rm = TowardPosInf
    //		i.  A is pos
    //		ii. A is neg
    //	    c.  rm = TowardNegInf
    //		i.  A is pos
    //		ii. A is neg
    //	14. A >>> B
    //	15. overflow occurs
    //  16. misc.
    dumpfloat("0x3FF00000 0x0 - 0x3FF00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3FF00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3FE00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3FE00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3FD00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3FD00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3FC00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3FC00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3FB00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3FB00000, 0x0));	// one hex digit
    dumpfloat("0x3FF00000 0x0 - 0x3F700000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3F700000, 0x0));	// 2 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3F300000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3F300000, 0x0));	// 3 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3EF00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3EF00000, 0x0));	// 4 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3EB00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3EB00000, 0x0));	// 5 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3EA00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3EA00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3E700000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3E700000, 0x0));	// 6 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3E300000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3E300000, 0x0));	// 7 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3DF00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3DF00000, 0x0));	// 8 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3DB00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3DB00000, 0x0));	// 9 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3D700000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3D700000, 0x0));	// 10 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3D300000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3D300000, 0x0));	// 11 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3CF00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3CF00000, 0x0));	// 12 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3CB00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3CB00000, 0x0));	// 13 hex digits
    dumpfloat("0x3FF00000 0x0 - 0x3CA00000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3CA00000, 0x0));
    dumpfloat("0x3FF00000 0x0 - 0x3C900000 0x0 = ",
	    subdouble(0x3FF00000, 0x0, 0x3C900000, 0x0));
    dumpfloat("0x3FF00000 0x0 + 0x3CA00000 0x00000002 = ",
	    adddouble(0x3FF00000, 0x0, 0x3CA00000, 0x00000002));	// round up
    dumpfloat("0x3FF00000 0x0 + 0x3C9FFFFF 0xFFFFFFFC = ",
	    adddouble(0x3FF00000, 0x0, 0x3C9FFFFF, 0xFFFFFFFC));	// chop up
    dumpfloat("0x3FF00000 0x0 - 0x3C900000 0x00000002 = ",
	    subdouble(0x3FF00000, 0x0, 0x3C900000, 0x00000002));	// chop up
    dumpfloat("0x3FF00000 0x0 - 0x3C8FFFFF 0xFFFFFFFC = ",
	    subdouble(0x3FF00000, 0x0, 0x3C8FFFFF, 0xFFFFFFFC));	// round up
}

void
multtest(char const *aname, char const *bname, Float const &a, Float const &b)
{
    char label[BUFSIZ];
    Float t = a;
    sprintf(label, "%s = ", aname);
    dumpfloat(label, t);
    t.mult(b);
    sprintf(label, "%s.mult(%s) = ", aname, bname);
    dumpfloat(label, t);
}

void
floatmulttest()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f0(fc);
    Float f1(fc, 1);
    Float f20(fc, 20);
    Float f200(fc, 200);
    Float fpinf(fc);
    fpinf.makeinf();
    Float fninf = fpinf;
    fninf.neg();
    Float fnan(fc);
    fnan.makenan();
    Float fqnan(fc);
    fqnan.makequietnan();
    Float fn0 = f0;
    fn0.neg();
    Float fa(fc);

    // misc. tests
    fa = f200;
    dumpfloat("fa = ", fa);
    fa.mult(f20);
    dumpfloat("fa.mult(f200); fa = ", fa);
    multtest("f200", "f20", f200, f20);

    Float three(fc, 3);
    Float n(fc, 1);

    dumpfloat("Float(3) = ", three);

    for (int i = 0; i < 60; i++)
    {
    	Float t = n * three;
	n = t;
	dumpfloat("n <- n * 3 = ", n);
    }

    // 		Value		Name
    //		+0		f0
    //		-0		fn0
    //		+3		three
    //		+1		f1
    //		+20		f20
    //		+200		f200
    //		+Inf		fpinf
    //		-Inf		fninf
    //		NaN		fnan
    //		QNaN		fqnan


// IV.	Float operations
// 	G.  Multiply

//	NaN * ?
    multtest("fnan", "f20", fnan, f20);
//	? * NaN
    multtest("f20", "fnan", f20, fnan);
//	QNaN * ?
    multtest("fqnan", "f20", fqnan, f20);
//	? * QNaN
    multtest("f20", "fqnan", f20, fqnan);
//	PInf * PInf
    multtest("fpinf", "fpinf", fpinf, fpinf);
//	PInf * NInf
    multtest("fpinf", "fninf", fpinf, fninf);
//	NInf * PInf
    multtest("fninf", "fpinf", fninf, fpinf);
//	NInf * NInf
    multtest("fninf", "fninf", fninf, fninf);
//	PInf * 1.0
    multtest("fpinf", "f1", fpinf, f1);
//	1.0 * PInf
    multtest("f1", "fpinf", f1, fpinf);
//	PInf * 0.0
    multtest("fpinf", "f0", fpinf, f0);
//	0.0 * PInf
    multtest("f0", "fpinf", f0, fpinf);
//	1.0 * 0.0
    multtest("f1", "f0", f1, f0);
//	0.0 * 1.0
    multtest("f0", "f1", f0, f1);
//	0.0 * 0.0
    multtest("f0", "f0", f0, f0);
}

void
floatoptest()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f0(fc);
    Float f20(fc, 20);
    Float f200(fc, 200);

    floataddtest();
    floatsub1test();
    floatsub2test();
    Float fa(fc);
    dumpfloat("fa = ", fa);
    fa.sub(f200);
    dumpfloat("fa.sub(f200); fa = ", fa);
    fa = f200;
    dumpfloat("fa = ", fa);
    fa.neg();
    dumpfloat("fa.neg(); fa = ", fa);
    floatmulttest();
    fa = f200;
    dumpfloat("fa = ", fa);
    fa.div(f20);
    dumpfloat("fa.div(f20); fa = ", fa);
    fa = f200;
    dumpfloat("fa = ", fa);
    fa.rem(f20);
    dumpfloat("fa.rem(f20); fa = ", fa);

    fa = f0;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa += f20) = ", fa += f20);
    fa = f0;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa -= f20) = ", fa -= f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa *= f20) = ", fa *= f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa /= f20) = ", fa /= f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa %%= f20) = ", fa %= f20);

    fa = f0;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa + f20) = ", fa + f20);
    fa = f0;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa - f20) = ", fa - f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("-fa = ", -fa);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa * f20) = ", fa * f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa / f20) = ", fa / f20);
    fa = f200;
    dumpfloat("fa = ", fa);
    dumpfloat("(fa %% f20) = ", fa % f20);

    fa = f20;
    dumpfloat("fa = ", fa);
    dumpfloat("fa++ = ", fa++);
    dumpfloat("fa = ", fa);
    fa = f20;
    dumpfloat("fa = ", fa);
    dumpfloat("--fa = ", --fa);
    dumpfloat("fa = ", fa);

    printf("f20.equal(f20) = %d\n", f20.equal(f20));
    printf("f20.equal(f200) = %d\n", f20.equal(f200));
    printf("(f20 == f20) = %d\n", f20 == f20);
    printf("(f20 == f200) = %d\n", f20 == f200);
    printf("(f20 != f20) = %d\n", f20 != f20);
    printf("(f20 != f200) = %d\n", f20 != f200);
    printf("f20.lessthan(f20) = %d\n", f20.lessthan(f20));
    printf("f20.lessthan(f200) = %d\n", f20.lessthan(f200));
    printf("f200.lessthan(f20) = %d\n", f200.lessthan(f20));
    printf("(f20 < f20) = %d\n", f20 < f20);
    printf("(f20 < f200) = %d\n", f20 < f200);
    printf("(f200 < f20) = %d\n", f200 < f20);
    printf("(f20 <= f20) = %d\n", f20 <= f20);
    printf("(f20 <= f200) = %d\n", f20 <= f200);
    printf("(f200 <= f20) = %d\n", f200 <= f20);
    printf("(f20 > f20) = %d\n", f20 > f20);
    printf("(f20 > f200) = %d\n", f20 > f200);
    printf("(f200 > f20) = %d\n", f200 > f20);
    printf("(f20 >= f20) = %d\n", f20 >= f20);
    printf("(f20 >= f200) = %d\n", f20 >= f200);
    printf("(f200 >= f20) = %d\n", f200 >= f20);

    printf("f20.isPos() = %d\n", f20.isPos());
    printf("f20.isNeg() = %d\n", f20.isNeg());
    printf("f20.isZero() = %d\n", f20.isZero());
    printf("f20.isPZero() = %d\n", f20.isPZero());
    printf("f20.isNZero() = %d\n", f20.isNZero());
    printf("f20.isInf() = %d\n", f20.isInf());
    printf("f20.isPInf() = %d\n", f20.isPInf());
    printf("f20.isNInf() = %d\n", f20.isNInf());
    printf("f20.isNan() = %d\n", f20.isNan());
    printf("f20.isSNan() = %d\n", f20.isSNan());
    printf("f20.isQNan() = %d\n", f20.isQNan());
    printf("!f0 = %d\n", !f0);
    printf("!f20 = %d\n", !f20);
    printf("f20.finite() = %d\n", f20.finite());
    printf("f20.unordered(f200) = %d\n", f20.unordered(f200));
    printf("f200.unordered(f20) = %d\n", f200.unordered(f20));
    printf("f0.floattype() = %d\n", f0.floattype());
    printf("f20.floattype() = %d\n", f20.floattype());

    fa = f200;
    fa.copysign(f20);
    dumpfloat("f200.copysign(f20) = ", fa);
    fa = f200;
    fa.scalb(f20);
    dumpfloat("f200.scalb(f20) = ", fa);
    fa = f200;
    fa.logb();
    dumpfloat("f200.logb() = ", fa);
    fa = f200;
    fa.nextafter(f20);
    dumpfloat("f200.nextafter(f20) = ", fa);
    dumpfloat("sqr(f20) = ", sqr(f20));
    dumpfloat("sqrt(f20) = ", sqrt(f20));
}

void
floatlogtest()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f20(fc, 20);
    Float f200(fc, 200);

    exp(f200);
    pow(f200, f20);
    log(f200);
    ln(f200);
    exp(f200);
}

void
floattrigtest()
{
    FloatClass fc(52, 11);
    initfloatclass(fc);
    Float f20(fc, 20);
    Float f200(fc, 200);

    sin(f20);
    cos(f20);
    tan(f20);
    asin(f20);
    acos(f20);
    atan(f20);
    atan(f200, f20);
    sinh(f20);
    cosh(f20);
    tanh(f20);
    asinh(f20);
    acosh(f20);
    atanh(f20);
}

main(/*int argc, char **argv*/)
{
//    errorinfo(argv[0], NULL, 0, &v, &m);
    printf("_FLOAT_MINLONGS = %d\n", _FLOAT_MINLONGS);
    printf("BITSPERLONG = %d\n", BITSPERLONG);
    printf("LONGMSB = 0x%08lX\n", LONGMSB);
    printf("Testing FloatClass construction\n");
    floatclasstest();
    printf("Testing Float construction\n");
    floatconstructtest();
    printf("Testing protected member functions\n");
    floatprottest();
    printf("Testing semi-numerical operations\n");
    floatmisctest();
    printf("Testing basic math operations\n");
    floatoptest();
    printf("Testing logrithmic functions\n");
    floatlogtest();
    printf("Testing trigonometric functions\n");
//    floattrigtest();
    printf("Tests complete\n");
    return 0;
}
