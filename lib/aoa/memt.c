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

extern size_t blocksize(void *p);

void
myalloc(size_t len)
{
    void *p = malloc(len);
    printf("malloc(%d) = %p blocksize=%d\n", len, p, blocksize(p));
}

int
main()
{
    extern void *sbrk(int);
    void *p = sbrk(0);
    printf("sbrk(0) = %p\n", p);
    myalloc(4);
    myalloc(8);
    myalloc(12);
    myalloc(16);
    myalloc(20);
    myalloc(24);
    myalloc(28);
    myalloc(32);
    myalloc(36);
    myalloc(40);
    myalloc(44);
    myalloc(48);
    myalloc(52);
    myalloc(56);
    myalloc(60);
    myalloc(64);
    myalloc(68);
    myalloc(72);
    myalloc(76);
    myalloc(80);
    myalloc(84);
    myalloc(88);
    myalloc(92);
    myalloc(96);
    myalloc(100);
    myalloc(104);
    myalloc(108);
    myalloc(112);
    myalloc(116);
    myalloc(120);
    myalloc(124);
    myalloc(128);
    myalloc(136);
    myalloc(140);
    myalloc(144);
    myalloc(148);
    myalloc(156);
    myalloc(160);
    myalloc(168);
    myalloc(176);
    myalloc(184);
    myalloc(192);
    myalloc(204);
    myalloc(212);
    myalloc(224);
    myalloc(240);
    myalloc(252);
    myalloc(272);
    myalloc(288);
    myalloc(312);
    myalloc(340);
    myalloc(368);
    myalloc(408);
    myalloc(452);
    myalloc(508);
    myalloc(580);
    myalloc(680);
    myalloc(816);
    myalloc(1020);
    myalloc(1360);
    myalloc(2040);
    printf("sbrk(0) = %p, %dK\n", sbrk(0),
	    ((char *)sbrk(0) - (char *)p) / 1024);
    return 0;
}
