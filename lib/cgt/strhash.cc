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
#include <ctype.h>

#include <stdlibx.h>
#include <stringx.h>

unsigned long
strhashpjw2(char const *s)
{
    unsigned long h = 0, g;

    for (; *s != '\0'; s++)
    {
    	h <<= 4;
    	h += *s;
	g = h & 0xF000;

	if (g)
	{
	    h ^= g >> 8;
	    h ^= g;
	}
    }

    return h;
}

unsigned long
strhashpjw(char const *s)
{
    unsigned long h = 0, g;

    for (; *s != '\0'; s++)
    {
    	h <<= 4;
    	h += *s;
	g = h & 0xF0000000;

	if (g)
	{
	    h ^= g >> 24;
	    h ^= g;
	}
    }

    return h;
}

unsigned long
strhash(char const *str)
{
    unsigned long hash = 0;

    while (*str != '\0')
	hash = (hash << 2) + *str++;

    return hash ^ (hash >> 16);
}

unsigned long
strhashi(char const *str)
{
    unsigned int hash = 0;

    while (*str != '\0')
    {
	hash = (hash << 2) + tolower(*str);
	str++;
    }

    return hash ^ (hash >> 16);
}
