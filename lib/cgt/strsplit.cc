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


//
//	Copyright (C) 1991  Thomas J. Merritt
//

#include <string.h>
#include "stdiox.h"
#include "stdlibx.h"
#include "stringx.h"
#include "expand.h"

declare_expand(char *);
implement_expand(char *);
declare_expand(char);
implement_expand(char);

const char **
strsplit(char const *str, int *argc, char const *sep, boolean whsp)
{
    static char **strvec = NULL;
    static int strveclen = 0;
    static char *string;
    static int stringlen;
    int args = 0;

    bufcpy(string, stringlen, str);
    char *sp = string;

    if (!whsp && strchr(sep, *string) != NULL)
    {
	expand(strvec, strveclen, args + 1);
	strvec[args] = "";
	strvec[args + 1] = NULL;
    	args++;
	sp++;
    }

    while (*sp != '\0')
    {
    	if (whsp)
	    while (strchr(sep, *sp) != NULL)
		sp++;

	char *start = sp;

	while (strchr(sep, *sp) == NULL && *sp != '\0')
	    sp++;

	if (!whsp || start != sp)
	{
	    expand(strvec, strveclen, args + 1);
	    strvec[args] = start;
	    strvec[args + 1] = NULL;
	    args++;
	}

	if (*sp == '\0')
	    break;

	*sp++ = '\0';
    }

    if (argc != NULL)
    	*argc = args;

    return (const char**)strvec;
}

const char *
strjoin(const char **vec, const char *sep)
{
    static char *string = NULL;
    static int stringlen = 0;

    int seplen = strlen(sep);
    int len = 0;
    expand(string, stringlen, 1);
    *string = '\0';

    for (const char *s = *vec++; s != NULL; s = *vec++)
    {
	int end = len;
	len += strlen(s);
	expand(string, stringlen, len + 1);
	strcpy(string + end, s);
	if (*vec != NULL)
	{
	    end = len;
	    len += seplen;
	    expand(string, stringlen, len + 1);
	    strcpy(string + end, sep);
	}
    }
    return string;
}
