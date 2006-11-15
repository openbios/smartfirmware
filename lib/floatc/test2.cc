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
#include <stdio.h>
#include <stdiox.h>
#include <string.h>
#include <stringx.h>

#include "floatc.h"

int width = 0;
int prec = 0;
int sci = 0;

char const *
ftos(Float const &x)
{
    return floattostr(x, width, prec, sci);
}

boolean
addtest(FloatClass &fc, const char *sa, const char *sb, const char *sc)
{
    Float a(fc, sa);
    Float b(fc, sb);
    Float c(fc, sc);
    Float d = a + b;
    printf("%s + %s -> %s == %s, %s\n", sa, sb, ftos(d), sc,
	    d == c ? "TRUE" : "FALSE");
    return d == c;
}

boolean
subtest(FloatClass &fc, const char *sa, const char *sb, const char *sc)
{
    Float a(fc, sa);
    Float b(fc, sb);
    Float c(fc, sc);
    Float d = a - b;
    printf("%s - %s -> %s == %s, %s\n", sa, sb, ftos(d), sc,
	    d == c ? "TRUE" : "FALSE");
    return d == c;
}

boolean
negtest(FloatClass &fc, const char *sa, const char *sb)
{
    Float a(fc, sa);
    Float b(fc, sb);
    Float c = -a;
    printf("%s - %s -> %s == %s, %s\n", sa, sb, ftos(c), sb,
	    c == b ? "TRUE" : "FALSE");
    return c == b;
}

boolean
multtest(FloatClass &fc, const char *sa, const char *sb, const char *sc)
{
    Float a(fc, sa);
    Float b(fc, sb);
    Float c(fc, sc);
    Float d = a * b;
    printf("%s * %s -> %s == %s, %s\n", sa, sb, ftos(d), sc,
	    d == c ? "TRUE" : "FALSE");
    return d == c;
}

boolean
divtest(FloatClass &fc, const char *sa, const char *sb, const char *sc)
{
    Float a(fc, sa);
    Float b(fc, sb);
    Float c(fc, sc);
    Float d = a / b;
    printf("%s / %s -> %s == %s, %s\n", sa, sb, ftos(d), sc,
	    d == c ? "TRUE" : "FALSE");
    return d == c;
}

boolean
test(FILE *fp)
{
    char *s = xgets(fp);
    const char **sv;
    int svl;

    if (s == NULL)
    	return TRUE;

    sv = strsplit(s, &svl);

    if (svl != 2)
    {
    	printf("bogus FloatClass description '%s'\n", s);
	return FALSE;
    }

    int exp = atoi(sv[0]);
    int mant = atoi(sv[1]);
    printf("'%s' (%d) '%s' (%d)\n", sv[0], exp, sv[1], mant);

    if (exp < 3 || mant < 3 || (exp + mant) > (1 << 20))
    {
    	printf("bogus FloatClass description, %d %d\n", exp, mant);
	return FALSE;
    }

    FloatClass fc(exp, mant);
    boolean ok = TRUE;

    while ((s = xgets(fp)))
    {
	sv = strsplit(s, &svl);

	if (svl == 0)
	    continue;

	switch (sv[0][0])
	{
	case '+':
	    if (svl == 4)
		ok = addtest(fc, sv[1], sv[2], sv[3]) && ok;
	    else
	    {
		printf("incorrect number of parameters for '+'\n");
		ok = FALSE;
	    }

	    break;
	case '-':
	    if (svl == 4)
		ok = subtest(fc, sv[1], sv[2], sv[3]) && ok;
	    else if (svl == 3)
	    	ok = negtest(fc, sv[1], sv[2]) && ok;
	    else
	    {
		printf("incorrect number of parameters for '-'\n");
		ok = FALSE;
	    }

	    break;
	case '*':
	    if (svl == 4)
		ok = multtest(fc, sv[1], sv[2], sv[3]) && ok;
	    else
	    {
		printf("incorrect number of parameters for '*'\n");
		ok = FALSE;
	    }

	    break;
	case '/':
	    if (svl == 4)
		ok = divtest(fc, sv[1], sv[2], sv[3]) && ok;
	    else
	    {
		printf("incorrect number of parameters for '/'\n");
		ok = FALSE;
	    }

	    break;
	case '#':
	    break;
	case 'w':
	    width = atoi(sv[1]);
	    break;
	case 'p':
	    prec = atoi(sv[1]);
	    break;
	case 's':
	    sci = atoi(sv[1]);
	    break;
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	case '8': case '9':
	    printf("%s --> %s\n", sv[0], ftos(strtofloat(fc, sv[0])));
	    break;
	default:
	    printf("unsupported operator '%s'\n", sv[0]);
	    ok = FALSE;
	    break;
	}
    }

    return ok;
}

int
main(/*int argc, char **argv*/)
{
    test(stdin);
    return 0;
}
