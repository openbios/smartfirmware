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
// $Header: /u/cgt/cvs/bin/cc/fileuse.cc,v 1.10 2003/04/29 21:56:09 parag Exp $

#include <stdlib.h>
#include <stdio.h>
#include <stdlibx.h>
#include <stringx.h>
#include <hashtable.h>
#include <pool.h>
#include "cppdefs.h"
#include "fileuse.h"

class File_use
{
	const char *_name;
	long _lines;
	size_t _size;
	Charvec _includes;
	Charvec _included;
	Charvec _references;
	Charvec _referenced;
	boolean _used;
	boolean _needed;
	boolean _walking;

public:
	File_use()
		{ _name = NULL; _lines = 0; _size = 0; _used = FALSE;
				_needed = FALSE; _walking = FALSE; }
	File_use(File_use &fi) :
			_includes(fi._includes), _included(fi._included),
			_references(fi._references), _referenced(fi._referenced)
		{ _name = fi._name; _lines = fi._lines; _size = fi._size;
				_used = fi._used; _needed = fi._needed;
				_walking = fi._walking; }
	File_use(const char *n)
		{ _name = n; _lines = 0; _size = 0; _used = FALSE;
				_needed = FALSE; _walking = FALSE; }
	void operator=(File_use &fi)
		{ _name = fi._name; _lines = fi._lines; _size = fi._size;
				_used = fi._used; _needed = fi._needed; _walking = fi._walking;
				_includes = fi._includes; _included = fi._included;
				_references = fi._references; _referenced = fi._referenced; }
	boolean operator==(File_use &fi) { return _name == fi._name; }
	boolean operator==(const char *n) { return _name == n; }
	operator const char*() { return _name; }

	const char *name() { return _name; }
	long lines() { return _lines; }
	void lines(long l) { _lines = l; }
	size_t size() { return _size; }
	void size(size_t s) { _size = s; }
	Charvec &includes() { return _includes; }
	Charvec &included() { return _included; }
	Charvec &references() { return _references; }
	Charvec &referenced() { return _referenced; }
	boolean used() { return _used; }
	void used(boolean m) { _used = m; }
	boolean needed() { return _needed; }
	void needed(boolean n) { _needed = n; }
	boolean walking() { return _walking; }
	void walking(boolean w) { _walking = w; }
};

#ifdef macintosh
#  define STRHASH strhashi
#  define STRGET strgeti
#  define STRCMP strcmpi
#else
#  define STRHASH strhash
#  define STRGET strget
#  define STRCMP strcmp
#endif

declare_hashtable(File_use_tab, File_use_iter, File_use, const char*);
implement_hashtable(File_use_tab,File_use_iter, File_use, const char*, STRHASH);

static File_use_tab g_file_use;
static long g_total_lines = 0;
static long g_total_bytes = 0;

static void
add_ref(Charvec &ref, const char *file)
{
	int i;
	
	for (i = 0; i < ref.size(); i++)
		if (ref.elt(i) == file)
			return;

	ref[i] = file;
}

void
new_file_use(const char *file, const char *from_file,
		long numlines, size_t numbytes)
{
	if (!g_verbose || file == NULL)
		return;

	file = STRGET(file);

	if (from_file)
	{
		from_file = STRGET(from_file);
		add_ref(g_file_use[file].included(), from_file);
		add_ref(g_file_use[from_file].includes(), file);
	}

	if (numlines >= 0)
	{
		g_total_lines += numlines;
		g_total_bytes += numbytes;
		g_file_use[file].lines(numlines);
		g_file_use[file].size(numbytes);
	}
}

void
bump_file_use(const char *file, const char *from_file)
{
	if (!g_verbose || file == NULL || from_file == NULL || file == from_file)
		return;

	file = STRGET(file);
	from_file = STRGET(from_file);
	add_ref(g_file_use[file].referenced(), from_file);
	add_ref(g_file_use[from_file].references(), file);
}

static int
ref_cmp(const void *v1, const void *v2)
{
	const char *s1 = *(const char**)v1;
	const char *s2 = *(const char**)v2;

	return STRCMP(s1, s2);
}

static void
sort_ref(Charvec &ref)
{
	if (ref.size() > 1)
		qsort(ref.getarr(), ref.size(), sizeof (const char*), ref_cmp);
}

static int
use_cmp(const void *v1, const void *v2)
{
	File_use *f1 = *(File_use**)v1;
	File_use *f2 = *(File_use**)v2;

	return STRCMP(f1->name(), f2->name());
}

static void
mark_used(const char *fname)
{
	File_use &fi = g_file_use[fname];

	if (fi.used())
		return;

	fi.used(TRUE);

	for (int i = 0; i < fi.references().size(); i++)
		mark_used(fi.references().elt(i));
}

static void
mark_needed(const char *fname)
{
	File_use &fi = g_file_use[fname];
	boolean needed = FALSE;

	if (fi.needed() || fi.walking())
		return;

	fi.walking(TRUE);
	
	int i;

	for (i = 0; i < fi.includes().size(); i++)
		mark_needed(fi.includes().elt(i));

	for (i = 0; i < fi.includes().size(); i++)
	{
		File_use &f = g_file_use[fi.includes().elt(i)];

		if (f.used() || f.needed())
		{
			for (int j = 0; j > fi.included().size(); j++)
			{
				File_use &p = g_file_use[f.included().elt(j)];

				if (p.used() || p.needed())
					return;
			}

			needed = TRUE;
		}
	}

	fi.needed(needed);
	fi.walking(FALSE);
}

void
print_unused_files(FILE *fp, const char *fname)
{
	if (!g_verbose)
		return;

	fname = STRGET(fname);
	mark_used(fname);
	mark_needed(fname);

	if (fp == NULL)
		fp = stdout;

	const long num = g_file_use.size();
	File_use **list = new File_use*[num];
	int l = 0; 

	for (File_use_iter fui = g_file_use; fui; fui++)
		list[l++] = &fui();

	qsort(list, num, sizeof *list, use_cmp);

	for (l = 0; l < num; l++)
	{
		int i;
		File_use &fi = *list[l];		//fui();

		if (!fi.used())
			fprintf(fp, "UNUSED ");

		if (fi.needed())
			fprintf(fp, "NEEDED ");

		fprintf(fp, "file \"%s\" is %ld lines (%d bytes):\n",
				fi.name(), fi.lines(), (int)fi.size());
		fprintf(fp, "\tincludes:\n");
		sort_ref(fi.includes());

		for (i = 0; i < fi.includes().size(); i++)
			fprintf(fp, "\t\tfile \"%s\"\n", fi.includes().elt(i));

		fprintf(fp, "\tincluded by:\n");
		sort_ref(fi.included());

		for (i = 0; i < fi.included().size(); i++)
			fprintf(fp, "\t\tfile \"%s\"\n", fi.included().elt(i));

		fprintf(fp, "\treferences:\n");
		sort_ref(fi.references());

		for (i = 0; i < fi.references().size(); i++)
			fprintf(fp, "\t\tfile \"%s\"\n", fi.references().elt(i));

		fprintf(fp, "\treferenced by:\n");
		sort_ref(fi.referenced());

		for (i = 0; i < fi.referenced().size(); i++)
			fprintf(fp, "\t\tfile \"%s\"\n", fi.referenced().elt(i));
	}

	fprintf(fp, "total scanned = %ld lines (%ld bytes)\n",
			g_total_lines, g_total_bytes);
}
