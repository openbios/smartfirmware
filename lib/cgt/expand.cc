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


// Copyright (C) 1991  Thomas J. Merritt.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/cgt/expand.cc,v 1.3 2000/05/31 22:07:31 parag Exp $
#include <memory.h>
#include <string.h>
#include "expand.h"

void
_expand(void **buf, int *buflen, int len, int eltsz, int bump)
{
    int nlen = len + bump - len % bump;
    char *nbuf = new char[nlen * eltsz];

    if (*buf != NULL)
    {
	memcpy((void *)nbuf, *buf, *buflen * eltsz);
	delete[/**buflen*/] *(char**)buf;
    }

//    memset(&nbuf[*buflen * eltsz], '\0', (nlen - *buflen) * eltsz);
    {
    	char *p = &nbuf[*buflen * eltsz];
	int bytes = (nlen - *buflen) * eltsz;

	for (int i = 0; i < bytes; i++)
	    p[i] = '\0';
    }

    *buf = nbuf;
    *buflen = nlen;
}

char *
bufcpy(char *&buf, int &buflen, char const *str)
{
    expand(&buf, &buflen, strlen(str) + 1);
    strcpy(buf, str);
    return buf;
}
