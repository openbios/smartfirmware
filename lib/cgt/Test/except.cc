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
#include <exception.h>


Exception e1 = "exception-1";
Exception e2 = "exception-2";
Exception e3 = "exception-3";


void
f(Exception v)
{
	printf("f(): raising %s...\n", v);
	RAISE(v);
}

void
func(Exception v)
{
	TRY
	{
		printf("func(): calling f()\n", v);
		f(v);
	}
	CATCH(e1)
		printf("func(): caught %s\n", EXCEPTION);
	RECOVER
	{
		printf("func(): caught %s, raising it again...\n", EXCEPTION);
		RAISE(EXCEPTION);
	}
}

void
doit(int c)
{
	printf("Starting\n");
	TRY
	{
		printf("Trying...\n");
		if (c > 2)
			func(e3);
		if (c > 1)
			func(e2);
		if (c > 0)
			func(e1);
	}
	CATCH(e1)
		printf("Caught %s\n", EXCEPTION);
	CATCH(e2)
		printf("Caught %s\n", EXCEPTION);
	printf("Fini!\n");
}

int
main()
{
	TRY
	{
	    doit(1);
	    doit(2);
	    doit(3);
	}
	RECOVER
		printf("faux core dump with %s\n", EXCEPTION);
	return 0;
}

// this is just to force our -lcgt/-lcgtdb malloc() to be linked in
static void
never_called()
{
    (void)malloc(32);
}
