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

/* Copyright (c) 1990,1999 by Parag Patel.  All Rights Reserved. */

#include <stdio.h>
#include "defs.h"
#include "buffer.h"
#include "rex.h"

int foo = 1;

int
main(const int argc, const char *argv[])
{
	auto Buffer buf;

	if (argc < 3)
	{
		fprintf(stderr, "usage: tgrep rex files...\n");
		return 1;
	}

	auto Rex rex;

	if (!rex.compile(argv[1]))
	{
		fprintf(stderr, "syntax error in: \"%s\"\n", argv[1]);
		return 2;
	}

	auto boolean first = TRUE;

	for (auto int i = 2; i < argc; i++)
	{
		if (!buf.readfile(argv[i]))
		{
			fprintf(stderr, "cannot read %s\n", argv[i]);
			continue;
		}

		auto long line = -1;

		for (auto long loc = 0; rex.matchforward(buf, loc);
				loc = rex.subend(0))
		{
			auto long j;

			if (line != buf.getlocline(rex.subloc(0)))
			{
				if (first)
					first = FALSE;
				else
					printf("\n");

				printf("%s: ", argv[i]);

				for (j = buf.getlineloc(buf.getlocline(rex.subloc(0)));
						buf[j] != '\n' && j < buf.length(); j++)
					putchar(buf[j]);

				line = buf.getlocline(rex.subloc(0));
			}

			printf(" [");

			for (j = rex.subloc(0); j < rex.subend(0); j++)
				putchar(buf[j]);

			printf("]");
		}
	}

	if (!first)
		printf("\n");

	return 0;
}
