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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "getoptx.h"

#include "darray.h"
/*
#define ARRAY charbuf
#define TYPE char
#define BUMP 32
#define IMPLEMENT
#include "darray.g"
*/

void
test_getoptx(int argc, const char **argv)
{
	int c;
	boolean err = FALSE;

	while ((c = getoptx(argc, argv,
	/* minus options */
					"(#)"
					"ab:c"
					,			/* plus options (may be NULL) */
					"(foo)"
					"(bar):"
					"(FOO:f)"
					"(BAR:b):"
					"(#)"
					"xy:z"
				)) != EOF)
		switch (c)
		{
		case 'a':				/* -a */
			printf("arg 'a'\n");
			break;

		case 'b':				/* -b +bar +BAR */
			printf("arg 'b' = '%s'\n", optarg);
			break;

		case 'c':				/* -c */
			printf("arg 'c'\n");
			break;

		case 'f':				/* +foo +FOO */
			printf("arg 'foo'\n");
			break;

		case '#':				/* +- numeric */
			printf("numeric arg = %c%d\n", optsign, atoi(optarg));
			break;

		default:
			printf("unknown arg '%c'\n", c);
			break;
		}

	if (optind >= argc)
		err = TRUE;

	if (err)
		fprintf(stderr, "usage: %s [-options] stuff\n", argv[0]);
}

int
main(int argc, const char **argv)
{
	charbuf *buf = new_charbuf();
	*end_charbuf(buf) = 'a';
	*end_charbuf(buf) = 'b';
	*end_charbuf(buf) = 'c';
	*end_charbuf(buf) = '\0';
	printf("\"%s\" len=%ld size=%ld\n", buf->array, buf->arrlen, buf->size);
	delete_charbuf(buf);

	test_getoptx(argc, argv);

	return 0;
}
