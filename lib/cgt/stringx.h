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

#ifndef __STRINGX_H_
#define __STRINGX_H_

#include <stdlibx.h>

char const **strsplit(char const *str, int *argc = NULL,
	char const *sep = " \t", boolean whsp = TRUE);
inline char const **strsplit(char const *str, char const *sep,
	boolean whsp = TRUE, int *argc = NULL)
    { return strsplit(str, argc, sep, whsp); }
char const *strjoin(char const **vec, char const *sep = " ");

unsigned long strhash(char const *);
unsigned long strhashi(char const *);

char const *strbldf(char const *fmt, ...);
char const *strbld(char const *arg1, ...);

char *strapp(char *s1, char const *s2);
char *strnapp(char *s1, char const *s2, int n);

int strvlen(char const **vec);
char **strvdup(char const **vec, int vlen = -1);
void strvfree(char **vec, int vlen = -1);

int strcmpi(char const *, char const *);

inline void
strfree(char const *str)
{
    delete[/*strlen(str) + 1*/] (char*)str;
}

inline char *
strnew(size_t len)
{
    return new char[len + 1];
}

inline char *
strdup(char const *s)
{
    char *nstr = NULL;

    if (s != NULL)
    {
	nstr = strnew(strlen(s));
	strcpy(nstr, s);
    }

    return nstr;
}

inline boolean
streq(char const *s1, char const *s2)
{
    return strcmp(s1, s2) == 0;
}

inline boolean
streqi(char const *s1, char const *s2)
{
	return strcmpi(s1, s2) == 0;
}

#endif // __STRINGX_H_
