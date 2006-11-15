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

#include "cpp.h"
#include "ccsym.h"
#include "cctype.h"
#include "cc.h"
#include "globals.h"
#include "error.h"
#include "tokens.h"
#include <signal.h>
#include <getoptx.h>
#include <backtrace.h>
#include <frontend.h>
#include <util.h>
#include <tree.h>
#include <ir.h>
#include <codegen.h>

extern long spacefactor;
extern long timefactor;

#ifdef SIGSEGV
// dump a stack trace if we catch a signal
static void
interrupt(int sig)
{
	fprintf(stderr, "\r\n\ncaught signal %d\r\n", sig);
	backtrace(TRUE);
	exit(-1);
}
#endif

// handle pragma directives here
void
do_pragma_directive(Cpp_node_list &toks)
{
	if (toks[0].token != IDENTIFIER)
		return;

	// MPW: "unused(var)"
	if (streq(toks[0].text, "unused"))
	{
		int v = 1;

		if (toks[1].token == '(' /* ) */ )
			v = 2;

		if (toks[v].token == IDENTIFIER)
		{
			Symbol *sym = g_curr_scope->lookup_sym(toks[v].text);

			if (sym)
				sym->unused(TRUE);
		}
	}

	// all other pragmas are completely ignored
}

void
init_type_sizes()
{
	g_min_addressable = g_minaddressableunit;
	g_size_pointer = g_ptrwidth;
	g_align_pointer = g_ptrbitalign;
	g_size_wchar_t = g_wcharwidth;
	g_size_size_t = g_ptrwidth;
	g_size_char = g_charwidth;
	g_align_char = g_charbitalign;
	g_size_short = g_shortwidth;
	g_align_short = g_shortbitalign;
	g_size_int = g_intwidth;
	g_align_int = g_intbitalign;
	g_size_long = g_longwidth;
	g_align_long = g_longbitalign;
	g_size_longlong = g_longlongwidth;
	g_align_longlong = g_longlongbitalign;
	g_size_float_mantissa = g_floatmant + 1;
	g_size_float_exponent = g_floatexp;
	g_align_float = g_floatbitalign;
	g_size_shortdouble_mantissa = g_shortdoublemant + 1;
	g_size_shortdouble_exponent = g_shortdoubleexp;
	g_align_shortdouble = g_shortdoublebitalign;
	g_size_double_mantissa = g_doublemant + 1;
	g_size_double_exponent = g_doubleexp;
	g_align_double = g_doublebitalign;
	g_size_longdouble_mantissa = g_longdoublemant + 1;
	g_size_longdouble_exponent = g_longdoubleexp;
	g_align_longdouble = g_longdoublebitalign;
}

int
main(int argc, const char *argv[])
{
	programfilename(argv[0]);
	irinit();
	frontendinit();
	init_type_sizes();

	// catch abnormal signals which typically cause memory faults
#ifdef SIGSEGV
	signal(SIGILL, interrupt);
	signal(SIGIOT, interrupt);
	signal(SIGBUS, interrupt);
	signal(SIGSEGV, interrupt);
#endif

	int c;
	boolean err = FALSE;
	boolean docheck = TRUE;
	boolean docode = TRUE;

	g_cpp.clear_includes();
	g_cpp.add_include_dir("/usr/include");
	g_cpp.add_include_dir("/usr/local/include");

	while ((c = getoptx(argc, argv,
					"(debug:1000)"
					"(strict_iso:1001)"
					"(warn2error:1002)"
					"(clear_includes:1003)"
					"(no_warnings:1004)"
					"(const_strings:1005)"
					"(debuglevel:1006):"
					"(no_tree:1007)"
					"(no_check:1008)"
					"(no_code:1009)"
					"(templates:1010)"
					"(coverage:1011)"
					"(sloppy_args:1012)"
					"(OS:1013):"
					"(OT:1014):"
					"I:" "D:" "U:" "O"
				)) != EOF)
		switch (c)
		{
		case 1000:
			g_debug = TRUE;
			break;

		case 1001:
			g_strict_iso = TRUE;
			break;

		case 1002:
			g_warn2error = TRUE;
			break;

		case 1003:
			g_cpp.clear_includes();
			break;

		case 1004:
			g_nowarnings = TRUE;
			break;

		case 1005:
			g_const_strings = TRUE;
			break;

		case 1006:
			debuglevel = atol(optarg);
			break;

		case 1007:
			g_do_tree = FALSE;
			break;

		case 1008:
			docheck = FALSE;
			break;

		case 1009:
			docode = FALSE;
			break;

		case 1010:
			g_showtemplates = TRUE;
			break;

		case 1011:
			g_templatecoverage = TRUE;
			break;

		case 1012:
			g_sloppy_args = TRUE;
			break;

		case 1013:
			g_optimize = TRUE;
			spacefactor = atol(optarg);

			if (spacefactor < 1)
				spacefactor = 1;

			break;

		case 1014:
			g_optimize = TRUE;
			timefactor = atol(optarg);

			if (timefactor < 1)
				timefactor = 1;

			break;

		case 'I':
			g_cpp.add_include_dir(optarg);
			break;

		case 'D':
			char *macro = strdup(optarg);
			char *body = strchr(macro, '=');

			if (body == NULL)
				body = "1";
			else
				*body++ = '\0';

			if (strpbrk(macro, "()"))
				fprintf(stderr, "cannot -D macro %s with arguments", macro);
			else if (!g_cpp.define_macro(macro, body))
				fprintf(stderr, "cannot -D macro %s - maybe -U is needed\n",
						macro);

			strfree(macro);
			break;

		case 'U':
			g_cpp.undef_macro(optarg);	// ignore -U of undefined macros
			break;

		case 'O':
			g_optimize = TRUE;
			break;

		default:
			err = TRUE;
			break;
		}

	if (optind >= argc)
		err = TRUE;

	if (err)
	{
		fprintf(stderr, "usage: %s [-options] input-file\n", argv[0]);
		return -1;
	}

	if (!g_cpp.open(argv[optind]))
	{
		fprintf(stderr, "cannot open file %s\n", argv[optind]);
		return -2;
	}

	init_keywords();
	init_types();
	init_parser();

	if (g_do_tree)
		init_tree();

	int ret = translation_unit();
	printf("\n");

	if (g_debug)
		g_file_scope->debug();

	if (g_do_tree && ret && !num_errors())
	{
		TreeNode *tree = fini_tree();

		if (tree != NULL && (!docheck || checktree(tree)))
		{
			if (docode)
			{
				if (g_optimize)
					tree = transform(tree);

				codegen(stdout, tree);
			}
			else
				writetree(tree);
		}
	}

	unsigned allocspc;
	unsigned freespc;
	unsigned allocblks;
	unsigned freeblks;
	void *endarena;
	unsigned arenasz;
	meminfo(allocspc, freespc, allocblks, freeblks, endarena, arenasz);
	fprintf(stderr, "total memory used is %ldK; end arena 0x%X\n",
			arenasz / 1024, endarena);

	return !ret + num_errors();
}

// force a different malloc() to be linked in before the one in -lc
static void
never_called()
{
	(void)malloc(32);
}
