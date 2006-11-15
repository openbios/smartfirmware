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
//	Copyright (C)  1992  Thomas J. Merritt
//

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "srchsort.h"

void
merge(void *dest, int elsz, void const *src1, int len1, void const *src2,
	int len2, int (*cmpfunc)(void const *, void const *))
{
    while (len1 && len2)
    {
    	int d = (*cmpfunc)(src1, src2);

    	if (d <= 0)
	{
	    memcpy(dest, src1, elsz);
	    src1 = (void *)((char *)src1 + elsz);
	    len1--;
	}
	else
	{
	    memcpy(dest, src2, elsz);
	    src2 = (void *)((char *)src2 + elsz);
	    len2--;
	}

	dest = (void *)((char *)dest + elsz);
    }

    while (len1)
    {
    	memcpy(dest, src1, elsz);
	src1 = (void *)((char *)src1 + elsz);
	dest = (void *)((char *)dest + elsz);
	len1--;
    }

    while (len2)
    {
    	memcpy(dest, src2, elsz);
	src2 = (void *)((char *)src2 + elsz);
	dest = (void *)((char *)dest + elsz);
	len2--;
    }
}
