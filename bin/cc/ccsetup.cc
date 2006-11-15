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

// Copyright 1994 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/ccsetup.cc,v 1.8 1999/09/26 18:51:22 parag Exp $
#include <stdio.h>
#include <stdlib.h>
#include <stdiox.h>
#include <stringx.h>
#include <srchsort.h>
#include "cpp.h"
#include "ccsetup.h"
#include "globals.h"

struct init_list
{
	int *var;
	const char *name;
};

declare_searchsort(struct init_list);
implement_searchsort(struct init_list);

struct init_list g_init[] =
{
	{ &g_debug, "debug" },
	{ &g_verbose, "verbose" },
	{ &g_cplusplus, "classes" },
	{ &g_strict_iso, "strict_iso" },
	{ &g_macintosh, "macintosh" },
	{ &g_pascal_strings, "pascal_strings" },
	{ &g_warn2error, "warn2error" },
	{ &g_nowarnings, "no_warnings" },
	{ &g_const_strings, "const_strings" },
	{ &g_sloppy_args, "sloppy_args" },
	{ &g_do_tree, "do_tree" },

	{ &g_min_addressable, "min_addressable" },
	{ &g_size_pointer, "size_pointer" },
	{ &g_align_pointer, "align_pointer" },
	{ &g_size_wchar_t, "size_wchar_t" },
	{ &g_size_size_t, "size_size_t" },

	{ &g_size_char, "size_char" },
	{ &g_align_char, "align_char" },
	{ &g_size_short, "size_short" },
	{ &g_align_short, "align_short" },
	{ &g_size_int, "size_int" },
	{ &g_align_int, "align_int" },
	{ &g_size_long, "size_long" },
	{ &g_align_long, "align_long" },
	{ &g_size_longlong, "size_longlong" },
	{ &g_align_longlong, "align_longlong" },

	{ &g_size_float_mantissa, "size_float_mantissa" },
	{ &g_size_float_exponent, "size_float_exponent" },
	{ &g_align_float, "align_float" },
	{ &g_size_shortdouble_mantissa, "size_shortdouble_mantissa" },
	{ &g_size_shortdouble_exponent, "size_shortdouble_exponent" },
	{ &g_align_shortdouble, "align_shortdouble" },
	{ &g_size_double_mantissa, "size_double_mantissa" },
	{ &g_size_double_exponent, "size_double_exponent" },
	{ &g_align_double, "align_double" },
	{ &g_size_longdouble_mantissa, "size_longdouble_mantissa" },
	{ &g_size_longdouble_exponent, "size_longdouble_exponent" },
	{ &g_align_longdouble, "align_longdouble" }
};

static int
init_cmp(const struct init_list *s1, const struct init_list *s2)
{
	return strcmp(s1->name, s2->name);
}

boolean
read_init_file(const char *file)
{
	int i;
	char *line;
	FILE *fp = fopen(file, "r");

	if (g_verbose)
		fprintf(stderr, "init file = \"%s\"\n", file);

	if (fp == NULL)
		fp = fopen(base_name(file), "r");

	if (fp == NULL)
		fp = pathvopen("PATH", file, "r");

	if (fp == NULL)
		fp = pathvopen("PATH", base_name(file), "r");

	if (fp == NULL)
		return FALSE;

	qsort(g_init, sizeof g_init / sizeof g_init[0], init_cmp);

	while ((line = xgets(fp)) != NULL)
	{
		int len = strlen(line);
		char *name = strtok(line, " \t");
		char *value = strtok(NULL," \t");

		if (name == NULL || value == NULL || name[0] == '#')
			continue;

		struct init_list key;
		key.var = NULL;
		key.name = name;
		i = bsearch(key, g_init, sizeof g_init / sizeof g_init[0], init_cmp);

		if (i >= 0)
		{
			*g_init[i].var = atoi(value);
			continue;
		}

		if (streq(name, "define"))
		{
			char *body = value + strlen(value);

			if (body < line + len)
				body++;

			while (*body == ' ' || *body == '\t')
				body++;

			if (body && strpbrk(body, "()"))
				fprintf(stderr, "cannot define macro \"%s\" with arguments",
						value);
			else if (!g_cpp.define_macro(value, body))
				fprintf(stderr, "cannot define macro \"%s\"\n", value);
		}
		else if (streq(name, "undef"))
			g_cpp.undef_macro(value);
		else if (streq(name, "include"))
			g_cpp.add_include_dir(value);
		else
			fprintf(stderr, "unknown init string \"%s\"\n", name);
	}

	return TRUE;
}
