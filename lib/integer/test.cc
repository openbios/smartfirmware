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


#include <stdio.h>
#include <stdlib.h>
#include <stdlibx.h>
#include <backtrace.h>

#include "integer.h"

void
dumpinteger(FILE *fp, Integer const &i, char const *comment = "",
	boolean dec = TRUE)
{
    fprintf(fp, "%s0x", comment);

    for (int j = i.size() - 1; j >= 0; j--)
    	fprintf(fp, "%08lX", i.get(j));

    if (dec)
    	fprintf(fp, " (%s)", integertostr(i));

    fprintf(fp, "\n");
}

inline void
dumpinteger(Integer const &i, char const *comment = "", boolean dec = TRUE)
{
    dumpinteger(stdout, i, comment, dec);
}

void
dumpintegerclass(FILE *fp, IntegerClass &ic, char const *comment = "",
	boolean dec = TRUE)
{
    fprintf(fp, "%sbits = %d\n", comment, ic.bits());
    fprintf(fp, "%ssize = %d\n", comment, ic.size());
    fprintf(fp, "%salloc = %s\n", comment,
	    ic.alloc() == TRUE ? "TRUE" : "FALSE");

    fprintf(fp, "%shind = %d\n", comment, ic.hind());
    fprintf(fp, "%shshift = %d\n", comment, ic.hshift());
    fprintf(fp, "%shmask = 0x%lX\n", comment, ic.hmask());
    fprintf(fp, "%slind = %d\n", comment, ic.lind());
    fprintf(fp, "%slshift = %d\n", comment, ic.lshift());
    fprintf(fp, "%slmask = 0x%lX\n", comment, ic.lmask());
    fprintf(fp, "%ssind = %d\n", comment, ic.sind());
    fprintf(fp, "%ssshift = %d\n", comment, ic.sshift());
    fprintf(fp, "%ssmask = 0x%lX\n", comment, ic.smask());
    fprintf(fp, "%ssextmask = 0x%lX\n", comment, ic.sextmask());

    char buf[BUFSIZ];
    sprintf(buf, "%szero = ", comment);
    dumpinteger(fp, ic.zero(), buf, dec);
    sprintf(buf, "%sone = ", comment);
    dumpinteger(fp, ic.one(), buf, dec);
    sprintf(buf, "%sminusone = ", comment);
    dumpinteger(fp, ic.minusone(), buf, dec);
    sprintf(buf, "%sten = ", comment);
    dumpinteger(fp, ic.ten(), buf, dec);
}

inline void
dumpintegerclass(IntegerClass &ic, char const *comment = "", boolean dec = TRUE)
{
    dumpintegerclass(stdout, ic, comment, dec);
}

void
testclass()
{
    IntegerClass ic1(1);
    dumpintegerclass(ic1, "ic1.", FALSE);
    IntegerClass ic2(2);
    dumpintegerclass(ic2, "ic2.", FALSE);
    IntegerClass ic4(4);
    dumpintegerclass(ic4, "ic4.", FALSE);
    IntegerClass ic8(8);
    dumpintegerclass(ic8, "ic8.", FALSE);
    IntegerClass ic16(16);
    dumpintegerclass(ic16, "ic16.", FALSE);
    IntegerClass ic32(32);
    dumpintegerclass(ic32, "ic32.", FALSE);
    IntegerClass ic33(33);
    dumpintegerclass(ic33, "ic33.", FALSE);
    IntegerClass ic34(34);
    dumpintegerclass(ic34, "ic34.", FALSE);
    IntegerClass ic35(35);
    dumpintegerclass(ic35, "ic35.", FALSE);
    IntegerClass ic64(64);
    dumpintegerclass(ic64, "ic64.", FALSE);
    IntegerClass ic66(66);
    dumpintegerclass(ic66, "ic66.", FALSE);
}

void
testmisc(FILE *fp = stdout)
{
    IntegerClass ic32(32);
    Integer a(ic32);
    fprintf(fp, "Constructed a\n");
    Integer b(ic32);
    fprintf(fp, "Constructed b\n");
    Integer c(ic32, 1);
    fprintf(fp, "Constructed c\n");
    Integer d(ic32);
    fprintf(fp, "Constructed d\n");
    int x = 5;
    a = b + c;
    fprintf(fp, "add\n");
    a = b - c;
    fprintf(fp, "sub\n");
    a = b * c;
    fprintf(fp, "mult\n");
    a = b / c;
    fprintf(fp, "div\n");
    a = b % c;
    fprintf(fp, "rem\n");
    a = b & c;
    a = b | c;
    a = b ^ c;
    fprintf(fp, "bitwise\n");
    a = b << x;
    a = b >> x;
    fprintf(fp, "shift\n");
    a = -b;
    a = ~b;
    x = !a;
    fprintf(fp, "unary\n");
    d = c;
    d += c;
    d -= c;
    d *= c;
    d /= c;
    d %= c;
    d &= c;
    d |= c;
    d ^= c;
    d <<= x;
    d >>= x;
    fprintf(fp, "assign\n");
    b.umult(c);
    b.udiv(c);
    b.urem(c);
    b.urshift(x);
    x = b.ucompare(c);
    fprintf(fp, "unsigned\n");
    x = (b == c);
    x = (b != c);
    x = (b < c);
    x = (b <= c);
    x = (b > c);
    x = (b >= c);
    fprintf(fp, "comparison\n");
    x = a.isPos();
    x = a.isNeg();
    x = a.size();
    a = b++;
    a = --b;
    fprintf(fp, "misc\n");
    dumpinteger(fp, a, "a = ");
    fprintf(fp, "a = 0x%s\n", integertostr(a, 16));
    dumpinteger(fp, b, "b = ");
    fprintf(fp, "b = 0x%s\n", integertostr(b, 16));
    dumpinteger(fp, c, "c = ");
    fprintf(fp, "c = 0x%s\n", integertostr(c, 16));
    dumpinteger(fp, d, "d = ");
    fprintf(fp, "d = 0x%s\n", integertostr(d, 16));
    Integer e(ic32, "123456789");
    dumpinteger(fp, e, "e = ");
    fprintf(fp, "e = %s\n", integertostr(e));
    IntegerClass ic16(16);
    Integer f(ic16, e);
    dumpinteger(fp, f, "f = ");
    fprintf(fp, "f = %s\n", integertostr(f));
    Integer g(ic32, f);
    dumpinteger(fp, g, "g = ");
    fprintf(fp, "g = %s\n", integertostr(g));
}

void
add(Integer const &a, Integer const &b)
{
    dumpinteger(a, "a = ");
    dumpinteger(b, "b = ");
    Integer t1 = a + b;
    dumpinteger(t1, "a + b = ");
    Integer t2 = b + a;

    if (t1 != t2)
    {
    	fprintf(stdout, "FAILURE!!! a+b != b+a");
	dumpinteger(t2, "b + a = ");
    }
}

int zero = 0;	// workaround cfront bug - cannot use "0" as it is ambiguous

void
testadd()
{
    IntegerClass ic(66);
    Integer fourx(ic, 0x40000000);
    Integer eightx = fourx;
    eightx <<= 1;
    Integer fx = eightx;
    fx <<= 1;
    fx -= Integer(ic, 1);
    add(Integer(ic, zero), Integer(ic, zero));
    add(Integer(ic, zero), Integer(ic, 1));
    add(Integer(ic, zero), Integer(ic, 2));
    add(Integer(ic, zero), eightx);
    add(Integer(ic, zero), fx);
    add(Integer(ic, 1), Integer(ic, zero));
    add(Integer(ic, 1), Integer(ic, 1));
    add(Integer(ic, 1), Integer(ic, 2));
    add(Integer(ic, 1), eightx);
    add(Integer(ic, 1), fx);
    add(Integer(ic, 2), Integer(ic, zero));
    add(Integer(ic, 2), Integer(ic, 1));
    add(Integer(ic, 2), Integer(ic, 2));
    add(Integer(ic, 2), eightx);
    add(Integer(ic, 2), fx);
    add(eightx, Integer(ic, zero));
    add(eightx, Integer(ic, 1));
    add(eightx, Integer(ic, 2));
    add(eightx, eightx);
    add(eightx, fx);
    add(fx, Integer(ic, zero));
    add(fx, Integer(ic, 1));
    add(fx, Integer(ic, 2));
    add(fx, eightx);
    add(fx, fx);
}

void
sub(Integer const &a, Integer const &b)
{
    dumpinteger(a, "a = ");
    dumpinteger(b, "b = ");
    dumpinteger(a - b, "a - b = ");
}

void
testsub()
{
    IntegerClass ic(66);
    Integer fourx(ic, 0x40000000);
    Integer eightx = fourx;
    eightx <<= 1;
    Integer fx = eightx;
    fx <<= 1;
    fx -= Integer(ic, 1);
    sub(Integer(ic, zero), Integer(ic, zero));
    sub(Integer(ic, zero), Integer(ic, 1));
    sub(Integer(ic, zero), Integer(ic, 2));
    sub(Integer(ic, zero), eightx);
    sub(Integer(ic, zero), fx);
    sub(Integer(ic, 1), Integer(ic, zero));
    sub(Integer(ic, 1), Integer(ic, 1));
    sub(Integer(ic, 1), Integer(ic, 2));
    sub(Integer(ic, 1), eightx);
    sub(Integer(ic, 1), fx);
    sub(Integer(ic, 2), Integer(ic, zero));
    sub(Integer(ic, 2), Integer(ic, 1));
    sub(Integer(ic, 2), Integer(ic, 2));
    sub(Integer(ic, 2), eightx);
    sub(Integer(ic, 2), fx);
    sub(eightx, Integer(ic, zero));
    sub(eightx, Integer(ic, 1));
    sub(eightx, Integer(ic, 2));
    sub(eightx, eightx);
    sub(eightx, fx);
    sub(fx, Integer(ic, zero));
    sub(fx, Integer(ic, 1));
    sub(fx, Integer(ic, 2));
    sub(fx, eightx);
    sub(fx, fx);
}

void
shift(Integer const &a, int b)
{
    Integer t = a;
    dumpinteger(t, "a = ");
    fprintf(stdout, "b = %d\n", b);
    uLong out = t.lshift(b);
    char buf[200];
    sprintf(buf, "a.lshift(b) = 0x%lX, a = ", out);
    dumpinteger(t, buf);
    t = a;
    out = t.rshift(b);
    sprintf(buf, "a.rshift(b) = 0x%lX, a = ", out);
    dumpinteger(t, buf);
    t = a;
    out = t.urshift(b);
    sprintf(buf, "a.urshift(b) = 0x%lX, a = ", out);
    dumpinteger(t, buf);
}

void
testshift()
{
    IntegerClass ic(66);
    uLong vec[3];
    vec[0] = 0x5555AAAA;
    vec[1] = 0xDEADBEEF;
    vec[2] = 0x00000003;
    Integer x(ic, vec);

    for (int i = 0; i <= 66; i++)
    	shift(x, i);
}

void
mult(Integer a, Integer b)
{
    dumpinteger(a, "a = ");
    dumpinteger(b, "b = ");
    Integer t1 = a * b;
    dumpinteger(t1, "a*b = ");
    Integer t2 = b * a;

    if (t1 != t2)
    {
    	fprintf(stdout, "FAILURE!!! a*b != b*a");
	dumpinteger(t2, "b*a = ");
    }
}

void
testmult()
{
    IntegerClass ic(66);
    mult(Integer(ic, 128), Integer(ic, 8));
    mult(Integer(ic, 24), Integer(ic, 8));
    mult(Integer(ic, 1), Integer(ic, 0x5555AAAA));
    mult(Integer(ic, 0x5555AAAA), Integer(ic, 1));
    mult(Integer(ic, 0x10001000), Integer(ic, 0x10001000));
}

void
divide(Integer a, Integer b)
{
    dumpinteger(a, "a = ");
    dumpinteger(b, "b = ");
    dumpinteger(a / b, "a/b = ");
    dumpinteger(a % b, "a%b = ");
}

void
testdivide()
{
    IntegerClass ic(66);
    divide(Integer(ic, 128), Integer(ic, 8));
    divide(Integer(ic, 129), Integer(ic, 8));
    divide(Integer(ic, 130), Integer(ic, 8));
    divide(Integer(ic, 32), Integer(ic, 32));
}

void
bitop(Integer const a, Integer const b)
{
    dumpinteger(a, "a = ");
    dumpinteger(b, "b = ");
    dumpinteger(a | b, "a|b = ");
    dumpinteger(a & b, "a&b = ");
    dumpinteger(a ^ b, "a^b = ");
}

void
testbitops()
{
    IntegerClass ic(66);
    bitop(Integer(ic, zero), Integer(ic, zero));
    bitop(Integer(ic, zero), Integer(ic, 1));
    bitop(Integer(ic, zero), Integer(ic, 0x55555555));
    bitop(Integer(ic, zero), Integer(ic, 0xAAAAAAAA));
    bitop(Integer(ic, zero), Integer(ic, 0xFFFFFFFF));
    bitop(Integer(ic, 1), Integer(ic, zero));
    bitop(Integer(ic, 1), Integer(ic, 1));
    bitop(Integer(ic, 1), Integer(ic, 0x55555555));
    bitop(Integer(ic, 1), Integer(ic, 0xAAAAAAAA));
    bitop(Integer(ic, 1), Integer(ic, 0xFFFFFFFF));
    bitop(Integer(ic, 0x55555555), Integer(ic, zero));
    bitop(Integer(ic, 0x55555555), Integer(ic, 1));
    bitop(Integer(ic, 0x55555555), Integer(ic, 0x55555555));
    bitop(Integer(ic, 0x55555555), Integer(ic, 0xAAAAAAAA));
    bitop(Integer(ic, 0x55555555), Integer(ic, 0xFFFFFFFF));
    bitop(Integer(ic, 0xAAAAAAAA), Integer(ic, zero));
    bitop(Integer(ic, 0xAAAAAAAA), Integer(ic, 1));
    bitop(Integer(ic, 0xAAAAAAAA), Integer(ic, 0x55555555));
    bitop(Integer(ic, 0xAAAAAAAA), Integer(ic, 0xAAAAAAAA));
    bitop(Integer(ic, 0xAAAAAAAA), Integer(ic, 0xFFFFFFFF));
    bitop(Integer(ic, 0xFFFFFFFF), Integer(ic, zero));
    bitop(Integer(ic, 0xFFFFFFFF), Integer(ic, 1));
    bitop(Integer(ic, 0xFFFFFFFF), Integer(ic, 0x55555555));
    bitop(Integer(ic, 0xFFFFFFFF), Integer(ic, 0xAAAAAAAA));
    bitop(Integer(ic, 0xFFFFFFFF), Integer(ic, 0xFFFFFFFF));
}

void
bitfuncs(Integer const i)
{
    fprintf(stdout, "bitcount(0x%s) = %d\n", integertostr(i, 16), bitcount(i));
    fprintf(stdout, "bits(0x%s) = %d\n", integertostr(i, 16), bits(i));
    fprintf(stdout, "findhighestbit(0x%s) = %d\n", integertostr(i, 16),
	    findhighestbit(i));
    fprintf(stdout, "findlowestbit(0x%s) = %d\n", integertostr(i, 16),
	    findlowestbit(i));
}

void
testbitfuncs()
{
    IntegerClass ic(68);
    uLong vec[3];
    vec[0] = 0x5555AAAA;
    vec[1] = 0xDEADBEEF;
    vec[2] = 0x0000000A;
    Integer deadbeef(ic, vec);
    Integer ooo(ic);
    Integer oox(ic, 0x5555);
    Integer oxo = oox << BITSPERLONG;
    Integer xoo = oxo << BITSPERLONG;
    bitfuncs(deadbeef);
    bitfuncs(ooo);
    bitfuncs(oox);
    bitfuncs(oxo);
    bitfuncs(oxo | oox);
    bitfuncs(xoo);
    bitfuncs(xoo | oox);
    bitfuncs(xoo | oxo);
    bitfuncs(xoo | oxo | oox);
}

void
testconvfuncs()
{
//    extern boolean integertol(Integer const &i, long &res);
//    extern boolean integertoul(Integer const &i, unsigned long &res);
//    extern boolean ltointeger(long v, Integer &res);
//    extern boolean ultointeger(unsigned long v, Integer &res);
}

void
testdefects()
{
    IntegerClass ic8(8);
    Integer x(ic8, 'x');
    dumpinteger(x, "Integer(IntegerClass(8), 'x') = ");
    IntegerClass ic66(66);
    uLong vec[3];
    vec[0] = 0x00000000;
    vec[1] = 0xAAAAD555;
    vec[2] = 0x00000003;
    Integer x2(ic66, vec);
    x2.neg();
    dumpinteger(x2, "- -6148867780172054528 = ");
}

main(int /*argc*/, char **argv)
{
    programfilename(argv[0]);
    testclass();
    testmisc();
    testadd();
    testsub();
    testshift();
    testmult();
    testdivide();
    testbitops();
    testbitfuncs();
    testconvfuncs();
    testdefects();
    return 0;
}
