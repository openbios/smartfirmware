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

// Copyright (c) 2000 by Parag Patel.  All Rights Reserved.

#include "defs.h"
#include "toks.h"
#include "gen.h"
#include <stdio.h>
#include <stdarg.h>

// base-class for all code generators 
// - cannot create an object of this class

Gencode::Gencode()
{
	currfile = NULL;
	fp = NULL;
	currline = 1;
}

Gencode::~Gencode()
{
}

void
Gencode::openfile(const char *fname)
{
	if (fp)
		fclose(fp);

	fp = makefile(fname);
	currfile = fname;
	currline = 1;
}

void
Gencode::closefile()
{
	if (fp)
		fclose(fp);

	fp = NULL;
}

void
Gencode::putindent(int num)
{
	if (fp == NULL)
		quit("Gencode::putindent called with NULL fp");

	if (mactabs)
	{
		while (num--)
			fprintf(fp, "\t");

		return;
	}

	int i = num / 2;

	while (i--)
		fprintf(fp, "\t");

	if (num % 2)
		fprintf(fp, "    ");
}

void
Gencode::doputf(char *fmt, va_list ap)
{
	if (fp == NULL)
		quit("Gencode::doputf called with NULL fp");

	static char *buf = NULL;
	static long buflen = 0;
	int nl = 0;
	xvsprintf(buf, buflen, fmt, ap);

	for (char *s = buf; *s != '\0'; s++)
		if (*s == '\n')
			nl++;

	currline += nl;
	fputs(buf, fp);
}

void
Gencode::putf(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	doputf(fmt, ap);
	va_end(ap);
}

void
Gencode::putf(int ind, char *fmt, ...)
{
	putindent(ind);
	va_list ap;
	va_start(ap, fmt);
	doputf(fmt, ap);
	va_end(ap);
}

void
Gencode::gencode(const char * /*infile*/, const char * /*headername*/,
		const char * /*parsername*/, const char * /*scannername*/)
{
	quit("GenCode::gencode should never be called");
}
