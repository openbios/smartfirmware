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
#include "cpp.h"
#include <backtrace.h>
#if defined(__MWERKS__) && defined(macintosh)
#include <console.h>
#endif

static Cpp_node_list lextokbuf;
static int lexloc = 0;
static Cpp_node *g_tok = NULL;

static int
gettok()
{
	if (lexloc >= lextokbuf.size())
	{
		lexloc = 0;
		lextokbuf.resize();
		printf("\n");

		if (g_cpp.scan(&lextokbuf) == END)
			return END;
	}

	g_tok = &lextokbuf.elt(lexloc++);
	return g_tok->token;
}

int
main(int argc, char *argv[])
{
#if defined(__MWERKS__) && defined(macintosh)
	argc = ccommand(&argv);
	printf("Starting...\n");
#endif

	if (argc <= 1)
		return -1;

	g_debug = TRUE;
	programfilename(argv[0]);

	if (!g_cpp.open(argv[1]))
	{
		fprintf(stderr, "cannot open file %s\n", argv[1]);
		return -2;
	}

	g_cpp.add_include_dir("/usr/local/include");

	for (int tok = gettok(); tok != END; tok = gettok())
	{
		if (tok < 255)
			printf(" '%c'", tok);
		else if (tok == IDENTIFIER || tok == STRING || tok == WSTRING)
			printf(" %s", g_tok->text);
		else if (tok == CHARACTER || tok == WCHARACTER)
			printf(" @%s", g_tok->text);
		else if (tok == INTVAL || tok == UINTVAL 
				|| tok == LONGVAL || tok == ULONGVAL)
			printf(" #%s", g_tok->text);
		else if (tok == DOUBLEVAL || tok == FLOATVAL ||
				tok == LONGDOUBLEVAL || tok == SHORTDOUBLEVAL)
			printf(" ##%s", g_tok->text);
		else
			printf(" !%d", tok);
	}

	if (num_errors())
	{
		fprintf(stderr, "completed scan with errors\n");
		return num_errors();
	}

	return 0;
}

// to satisfy link - normally provided by ANSI C parser
const char *
get_current_func()
{
	return "no__FUNC__";
}

// all pragma directives are totally ignored for now
void
do_pragma_directive(Cpp_node_list &/*toks*/)
{
}

// this is just to force our -lcgt/-lcgtdb malloc() to be linked in
static void
never_called()
{
	(void)malloc(32);
}
