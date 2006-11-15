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

// Copyright (c) 1992,1999 by Parag Patel.  All Rights Reserved.

#include <stdio.h>
#include <stdarg.h>
#include "stdiox.h"
#include "stdlibx.h"

char *
xvsprintf(char *&buf, long &buflen, const char *fmt, va_list ap)
{
    static FILE *fp = NULL;

#if defined _DJGPP_SOURCE
    if (fp == NULL)
    	fp = fopen("/tmp/xprintf.tmp", "w+");
#elif defined WINBLOWS
    if (fp == NULL)
    	fp = fopen(".\\xprintf.tmp", "w+");
#else
    if (fp == NULL)
	fp = tmpfile();
#endif

    if (fp == NULL)
	crash("xvsprintf(): cannot open temp file - out of space?");

    if (fmt == NULL)
    {
	if (fp != NULL)
	    fclose(fp);

	fp = NULL;
	return NULL;
    }

    rewind(fp);
    vfprintf(fp, (char*)fmt, ap);
    size_t len = (size_t)ftell(fp);

    if (len >= (size_t)buflen)
    {
	delete buf;
	buf = new char[buflen = len + 1];

	if (buf == NULL)
	    return NULL;
    }

    rewind(fp);
    fread(buf, sizeof (char), len, fp);
    buf[len] = '\0';
    return buf;
}

#ifndef _DJGPP_SOURCE
const char *
xsprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    static char *buf = NULL;
    static long buflen = 0;
    char *ret = xvsprintf(buf, buflen, fmt, ap);
    va_end(ap);
    return ret;	// may be NULL if xvsprintf() fails
}
#else
// With DJGPP lseek is very very slow so don't bother with being overly safe.
const char *
xsprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    static char buf[BUFSIZ];
    vsprintf(buf, fmt, ap);
    va_end(ap);
    return buf;
}
#endif
