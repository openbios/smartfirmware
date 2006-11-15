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

// Copyright (c) 1993-2003 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccfcode.cc,v 1.22 2003/04/30 23:26:20 parag Exp $

// main driver for Fcode generator

#include "cpp.h"
#include "ccsym.h"
#include "ccnode.h"
#include "cc.h"
#include "ccsetup.h"
#include "fileuse.h"
#include "globals.h"
#include "error.h"
#include "tokens.h"

#define boolean bogus_bool
#include <limits.h>
#include <float.h>
#undef S_NONE
#include <signal.h>
#undef boolean

#include <backtrace.h>
#include <getoptx.h>
#include <stdiox.h>

#if defined(__MWERKS__) && defined(macintosh)
#include <console.h>
#endif

#ifdef SIGIOT
// dump a stack trace if we catch a signal
static void
interrupt(int sig)
{
	fprintf(stderr, "\r\n\ncaught signal %d\r\n", sig);
	backtrace(TRUE);
	exit(-1);
}
#endif


Symbol_arr *g_dev_list = NULL;
boolean g_expand_inline = FALSE;
boolean g_recursive = FALSE;
boolean g_debug_syms = FALSE;
char *g_sym_alias = NULL;


// handle pragma directives here
void
do_pragma_directive(Cpp_node_list &toks)
{
	extern void p(const char *fmt,...);	// in fcode.cc
	Cpp_node *n, node;
	
	node.token = END;
	toks.end() = node;
	n = toks.getarr();

	if (n->token != IDENTIFIER)
		return;

	// special stuff to make creating devices and properties easier
	if (streq(n->text, "dev"))
	{
		for (n++; n->token == WHITESPACE; n++)
			;

		if (n->token != STRING)
		{
			error(BAD_PRAGMA_FORMAT);
			return;
		}

		p("\n\" dev %s evaluate\n", n->text + 1);
	}
	else if (streq(n->text, "new_device"))
	{
		// if any device has been created it, sorta forget about it
		// - note that this is NOT C behavior but very useful for Fcode
		if (g_dev_list)
		{
			Symbol_arr *init = g_file_scope->init();

			for (int i = g_dev_list->size() - 1; i >= 0; i--)
			{
				g_dev_list->elt(i)->node(NULL);
				g_file_scope->symtab().remove(g_dev_list->elt(i)->name());

				for (int j = 0; j < init->size(); j++)
					if (init->elt(j) == g_dev_list->elt(i))
						init->elt(j) = NULL;
			}

			delete g_dev_list;
			g_dev_list = NULL;
		}

		p("\nnew-device\n");

		for (n++; n->token == WHITESPACE; n++)
			;

		if (n->token == STRING)
		{
			p("\" %s device-name\n", n->text + 1);
			n++;
		}

		for (; n->token == WHITESPACE; n++)
			;

		if (n->token == STRING)
			p("\" %s device-type\n", n->text + 1);

		g_dev_list = new Symbol_arr;
	}
	else if (streq(n->text, "instance"))
	{
		p("instance ");
	}
	else if (streq(n->text, "finish_device"))
	{
		p("finish-device\n\n");
	}
	else if (streq(n->text, "expand_inline"))
	{
		g_expand_inline = TRUE;
	}
	else if (streq(n->text, "recursive"))
	{
		g_recursive = TRUE;
	}
	else if (streq(n->text, "alias"))
	{
		for (n++; n->token == WHITESPACE; n++)
			;

		if (n->token != STRING)
		{
			error(BAD_PRAGMA_FORMAT);
			return;
		}

		g_sym_alias = strdup(n->text + 1);
		g_sym_alias[strlen(g_sym_alias) - 1] = '\0';
	}
}

int
main(int argc, char **argv)
{
#if defined(__MWERKS__) && defined(macintosh)
	argc = ccommand(&argv);

	// This is to turn off character-by-character processing by the MW
	// console for stderr.
	// It dramatically speeds up the program output since we spit out a
	// lot of warnings.
	char errbuf[4096];
	setvbuf(stderr, errbuf, _IOLBF, sizeof errbuf);
	printf("Starting...\n");
#endif

	int c;
	boolean err = FALSE;
	boolean gotinit = FALSE;
	Charvec includes;

	programfilename(argv[0]);

	// catch abnormal signals which typically cause memory faults
#ifdef SIGIOT
	signal(SIGILL, interrupt);
	signal(SIGIOT, interrupt);
	signal(SIGBUS, interrupt);
	signal(SIGSEGV, interrupt);
	signal(SIGQUIT, interrupt);
#endif

	while ((c = getoptx(argc, (const char **)argv,
			/* -options: */
					"I:" "D:" "U:" "v" "g",
			/* +options and --options: */
					"(debug:1000)"
					"(strict_iso:1001)"
					"(warn2error:1002)"
					"(clear_includes:1003)"
					"(no_warnings:1004)"
					"(const_strings:1005)"
					"(syntax_only:1008)"
					"(sloppy_args:1012)"
					// "(classes:1013)"
					"(init:1014):"
					"(verbose:v)"
					"(debug_syms:g)"
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
		includes.reset();
		break;

	case 1004:
		g_nowarnings = TRUE;
		break;

	case 1005:
		g_const_strings = TRUE;
		break;

	case 1008:
		g_do_tree = FALSE;
		break;

	case 1012:
		g_sloppy_args = TRUE;
		break;

	case 1013:
		g_cplusplus = TRUE;
		break;

	case 'v':
		g_verbose = TRUE;
		break;

	case 'g':
		g_debug_syms = TRUE;
		break;

	case 1015:
		gotinit = read_init_file(optarg);

		if (!gotinit)
		{
			fprintf(stderr, "cannot read -init file %s\n", optarg);
			return -3;
		}

		break;

	case 'I':
		includes.end() = optarg;
		break;

	case 'D':
		{
			char *macro = strdup(optarg);
			char *body = strchr(macro, '=');

			if (body == NULL)
				body = "";
			else
				*body++ = '\0';

			if (strpbrk(macro, "()"))
			{
				fprintf(stderr, "cannot -D macro %s with arguments", macro);
				return -4;
			}
			else if (!g_cpp.define_macro(macro, body))
			{
				fprintf(stderr, "cannot -D macro %s - maybe -U is needed\n",
						macro);
				return -5;
			}

			strfree(macro);
		}

		break;

	case 'U':
		g_cpp.undef_macro(optarg);	// ignore -U of undefined macros
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
		fprintf(stderr, "options: --strict_iso --no_warnings"
			" --warn2error\n");
		fprintf(stderr, "\t--const_strings --syntax_only"
			" --sloppy_args\n");
		fprintf(stderr, "\t--verbose -v --debug_syms -g"
			" --clear_includes\n");
		fprintf(stderr, "\t-I include-dir\n");
		fprintf(stderr, "\t-D macro[=value]\n");
		fprintf(stderr, "\t-U macro\n");
		return -1;
	}

	if (includes.size() > 0)
	{
		for (int i = 0; i < includes.size(); i++)
			g_cpp.add_include_dir(includes.elt(i));
	}

	if (!gotinit)
	{
		char *ext = strrchr(argv[optind], '.');

		if (ext == NULL)
			ext = "";

		if (g_cplusplus || streq(ext, ".cc") || streq(ext, ".cxx"))
			gotinit = read_init_file(xsprintf("%sxx.init", argv[0]));

		if (!gotinit)
			gotinit = read_init_file(xsprintf("%s.init", argv[0]));
	}

	if (!g_cpp.open(argv[optind]))
	{
		fprintf(stderr, "cannot open file %s\n", argv[optind]);
		return -2;
	}

	init_keywords();
	init_types();
	init_nodes();
	init_parser();

	if (g_do_tree)
		init_tree();

	int ret = translation_unit();

	if (g_do_tree)
		fini_tree();

	if (g_debug)
		g_file_scope->debug();

	if (g_verbose)
	{
		size_t allocspc;
		size_t freespc;
		size_t allocblks;
		size_t freeblks;
		void *endarena;
		size_t arenasz;

		print_unused_files(stderr, argv[optind]);
		meminfo(allocspc, freespc, allocblks, freeblks, endarena, arenasz);
		fprintf(stderr, "total memory used is %dK; end arena %p\n",
				arenasz / 1024, endarena);
	}

	if (!ret + num_errors())
		printf("\n!!! THIS FILE IS TRASH DUE TO COMPILATION FAILURE !!!\n");

	return !ret + num_errors();
}

// force a different malloc() to be linked in before the one in -lc
static void
never_called()
{
	(void)malloc(32);
}
