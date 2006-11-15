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


#include <backtrace.h>

void
foo()
{
    backtrace(TRUE);
}

void
f10(int /*v1*/, int /*v2*/, int /*v3*/, int /*v4*/, int /*v5*/, int /*v6*/,
	int /*v7*/, int /*v8*/, int /*v9*/, int /*v10*/)
{
    foo();
}

void
f9(int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9)
{
    f10(v1, v2, v3, v4, v5, v6, v7, v8, v9, 10);
}

void
f8(int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8)
{
    f9(v1, v2, v3, v4, v5, v6, v7, v8, 9);
}

void
f7(int v1, int v2, int v3, int v4, int v5, int v6, int v7)
{
    f8(v1, v2, v3, v4, v5, v6, v7, 8);
}

void
f6(int v1, int v2, int v3, int v4, int v5, int v6)
{
    f7(v1, v2, v3, v4, v5, v6, 7);
}

void
f5(int v1, int v2, int v3, int v4, int v5)
{
    f6(v1, v2, v3, v4, v5, 6);
}

void
f4(int v1, int v2, int v3, int v4)
{
    f5(v1, v2, v3, v4, 5);
}

void
f3(int v1, int v2, int v3)
{
    f4(v1, v2, v3, 4);
}

void
f2(int v1, int v2)
{
    f3(v1, v2, 3);
}

void
f1(int v1)
{
    f2(v1, 2);
}

void
f0()
{
    f1(1);
}

int
main(int /*argc*/, char **argv)
{
    programfilename(argv[0]);
    f0();
    return 0;
}
