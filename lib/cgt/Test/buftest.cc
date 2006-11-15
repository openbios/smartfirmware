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

//#define ARRAY Charbuf2
//#define TYPE char
//#define BUMP 1024

#include <dynarray.h>

Charbuf buffer;

#define TESTLEN 200000

main()
{
    printf("Testing...\n");		// make sure that printf does its malloc
    char *startaddr = new char[4];
    delete startaddr;

    char *lastarray = NULL;
    int i;

    for (i = 0; i < TESTLEN; i++)
    {
	buffer[i] = (char)(i & 0xFF);

	if (buffer.getarr() != lastarray)
	{
	    printf("i = %d, buffer = 0x%X\n", i, buffer.getarr() - startaddr);
	    lastarray = buffer.getarr();
	}
    }

    for (i = 0; i < TESTLEN; i++)
    	if ((buffer[i] & 0xFF) != (i & 0xFF))
	    break;

    if (i < TESTLEN)
    	printf("Data Lost at %d\n", i);
    else if (buffer.getarr() != lastarray)
    	printf("Buffer incorrectly expanded\n");
    else
    	printf("OK\n");

    return 0;
}

// this is just to force our -lcgt/-lcgtdb malloc() to be linked in
static void
never_called()
{
    (void)malloc(32);
}
