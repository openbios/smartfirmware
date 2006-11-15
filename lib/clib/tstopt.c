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

void
test_getoptx(int argc, const char **argv)
{
	int c;
	int err = 0;

	while ((c = getoptx(argc, argv,
	/* minus options */
					"(#)ab:c(debug)(debugl/evel:1001):(?:1002)",
	/* plus options (may be NULL) */
					"(foo/)(bar):(FOO:f)(BAR:b):(#)xy:z"
				)) != EOF)
		switch (c)
		{
		case 'a':				/* -a */
			printf("arg %ca\n", optsign);
			break;

		case 'b':				/* -b arg; +bar arg; +BAR arg */
			printf("arg %cb = \"%s\"\n", optsign, optarg);
			break;

		case 'c':				/* -c */
			printf("arg %cc\n", optsign);
			break;

		case 'd':				/* -debug */
			printf("arg %cdebug\n", optsign);
			break;

		case 'f':				/* +foo +FOO */
			printf("arg %cfoo\n", optsign);
			break;

		case 'x':				/* +x */
		case 'z':				/* +z */
			printf("arg %c%c\n", optsign, c);
			break;

		case 'y':				/* +y arg */
			printf("arg %cy = \"%s\"\n", optsign, optarg);
			break;

		case '#':				/* +- numeric */
			printf("numeric arg = %c%d\n", optsign, atoi(optarg));
			break;

		case 1001:				/* -debugl/evel 123 */
			printf("arg %cdebuglevel = %d\n", optsign, atoi(optarg));
			break;

		case 1002:				/* -? (but not an error) */
			printf("arg %c?\n", optsign);
			break;

		default:				/* error */
			printf("unknown arg %c%c\n", optsign, c);
			break;
		}

	if (optind >= argc)
		err = 1;

	if (err)
		fprintf(stderr, "usage: %s [-options] stuff\n", argv[0]);

	for (; optind < argc; optind++)
		printf((optind < argc - 1) ? "%s " : "%s\n", argv[optind]);
}

int
main(int argc, const char **argv)
{
	test_getoptx(argc, argv);

	return 0;
}
