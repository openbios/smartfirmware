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

// Copyright (c) 1993,1999-2000 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/cctst.cc,v 1.33 2003/05/01 06:33:13 parag Exp $
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
#undef S_NONE				// for Solaris signal.h
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

// handle pragma directives here
void
do_pragma_directive(Cpp_node_list &toks)
{
	if (toks[0].token != IDENTIFIER)
		return;

	// MPW: "unused(var)"
	if (g_macintosh && streq(toks[0].text, "unused"))
	{
		int v = 1;

		if (toks[1].token == '('/*)*/)
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

int
main(int argc, char **argv)
{
#if defined(__MWERKS__) && defined(macintosh)
	argc = ccommand(&argv);
	
	// This is to turn off character-by-character processing by the MW console for stderr.
	// It dramatically speeds up the program output since we spit out a lot of warnings.
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

	while ((c = getoptx(argc, (const char**)argv,
					"(debug:1000)"
					"(strict_iso:1001)"
					"(warn2error:1002)"
					"(clear_includes:1003)"
					"(no_warnings:1004)"
					"(const_strings:1005)"
					"(macintosh:1006)"
					"(pascal_strings:1007)"
					"(sloppy_args:1012)"
					"(classes:1013)"
					"(verbose:1014)" "v"
					"(init:1015):"
					"I:" "D:" "U:"
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

		case 1006:
			g_macintosh = TRUE;
			break;

		case 1007:
			g_pascal_strings = TRUE;
			break;

		case 1012:
			g_sloppy_args = TRUE;
			break;

		case 1013:
			g_cplusplus = TRUE;
			break;

		case 'v':
		case 1014:
			g_verbose = TRUE;
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
			g_cpp.undef_macro(optarg);		// ignore -U of undefined macros
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
	init_tree();

	int ret = translation_unit();

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

		print_unused_files(stdout, argv[optind]);
		meminfo(allocspc, freespc, allocblks, freeblks, endarena, arenasz);
		fprintf(stdout, "total memory used is %dK; end arena %p\n",
				(int)arenasz / 1024, endarena);
	}

	return !ret + num_errors();
}

// force a different malloc() to be linked in before the one in -lc
static void
never_called()
{
	(void)malloc(32);
}

// all the following is to fake the tree-generating code for testing only

class Noop_node : public Node
{
public:
	Noop_node() : Node() { }
};

static Noop_node *g_noop_node = NULL;

void
init_tree()
{
	if (g_noop_node == NULL)
		g_noop_node = new Noop_node;
}

void
do_tree(Symbol *sym)
{
	if (g_debug || sym == NULL || num_errors() || sym->treedone())
		return;

	sym->treedone(TRUE);

	if (sym->storage() != ENUM && !sym->overloaded() && sym->node() != NULL)
	{
		// do not need the function bodies or initialization lists
		//delete sym->node();
		sym->node(g_noop_node);
	}
}

void
do_tree(Node * /*node*/)
{
	//delete node;
}

TreeNode *
fini_tree()
{
	return NULL;
}

TreeNode *
Scope::mktree()
{
	return NULL;
}

void
Scope::calclocals(StorageAllocator &/*stkalloc*/)
{
}

TreeNode *
Symbol::mktree()
{
	return NULL;
}

TreeNode *
Node::mktree()
{
	return NULL;
}

TreeNode *
Expr_node::mktree()
{
	return NULL;
}

LValueTreeNode *
Expr_node::mklval()
{
	return NULL;
}

TreeNode *
Node::mkinit()
{
	return NULL;
}

TreeNode *
Expr_node::mkinit()
{
	return NULL;
}

TreeNode *
Integer_node::mktree()
{
	return NULL;
}

TreeNode *
Integer_node::mkinit()
{
	return NULL;
}

TreeNode *
Sizeof_node::mktree()
{
	return NULL;
}

TreeNode *
Sizeof_node::mkinit()
{
	return NULL;
}

TreeNode *
Float_node::mktree()
{
	return NULL;
}

TreeNode *
Float_node::mkinit()
{
	return NULL;
}

TreeNode *
String_node::mktree()
{
	return NULL;
}

TreeNode *
String_node::mkinit()
{
	return NULL;
}

LValueTreeNode *
String_node::mklval()
{
	return NULL;
}

LValueTreeNode *
Symbol_node::mklval()
{
	return NULL;
}

TreeNode *
Symbol_node::mktree()
{
	return NULL;
}

TreeNode *
Symbol_node::mkinit()
{
	return NULL;
}

LValueTreeNode *
Array_ref_node::mklval()
{
	return NULL;
}

TreeNode *
Array_ref_node::mktree()
{
	return NULL;
}

LValueTreeNode *
Func_call_node::mklval()
{
	return NULL;
}

TreeNode *
Func_call_node::mktree()
{
	return NULL;
}

LValueTreeNode *
Struct_ref_node::mklval()
{
	return NULL;
}

TreeNode *
Struct_ref_node::mktree()
{
	return NULL;
}

TreeNode *
Struct_ref_node::mkinit()
{
	return NULL;
}

LValueTreeNode *
Operator_node::mklval()
{
	return NULL;
}

TreeNode *
Operator_node::mktree()
{
	return NULL;
}

TreeNode *
Operator_node::mkinit()
{
	return NULL;
}

LValueTreeNode *
Operator2_node::mklval()
{
	return NULL;
}

TreeNode *
Operator2_node::mktree()
{
	return NULL;
}

TreeNode *
Operator2_node::mkinit()
{
	return NULL;
}

LValueTreeNode *
Operator3_node::mklval()
{
	return NULL;
}

TreeNode *
Operator3_node::mktree()
{
	return NULL;
}

TreeNode *
Operator3_node::mkinit()
{
	return NULL;
}

TreeNode *
Cast_node::mktree()
{
	return NULL;
}

LValueTreeNode *
Cast_node::mklval()
{
	return NULL;
}

TreeNode *
Cast_node::mkinit()
{
	return NULL;
}

TreeNode *
Initializer_node::mkinit()
{
	return NULL;
}

TreeNode *
Initializer_node::mktree()
{
	return NULL;
}

TreeNode *
Label_node::mktree()
{
	return NULL;
}

TreeNode *
Case_node::mktree()
{
	return NULL;
}

TreeNode *
Switch_node::mktree()
{
	return NULL;
}

TreeNode *
Compound_node::mktree()
{
	return NULL;
}

TreeNode *
If_node::mktree()
{
	return NULL;
}

TreeNode *
While_node::mktree()
{
	return NULL;
}

TreeNode *
Dowhile_node::mktree()
{
	return NULL;
}

TreeNode *
For_node::mktree()
{
	return NULL;
}

TreeNode *
Goto_node::mktree()
{
	return NULL;
}

TreeNode *
Continue_node::mktree()
{
	return NULL;
}

TreeNode *
Break_node::mktree()
{
	return NULL;
}

TreeNode *
Return_node::mktree()
{
	return NULL;
}

TreeNode *
Asm_node::mktree()
{
	return NULL;
}

TreeNode *
New_node::mktree()
{
	return NULL;
}

TreeNode *
Delete_node::mktree()
{
	return NULL;
}
