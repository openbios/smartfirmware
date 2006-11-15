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

// These replace the broken Metrowerks versions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Memory.h>

static void
crash(const char *mesg)
{
    fprintf(stderr, "%s\n", mesg);
    exit(-1);
}

void *
malloc(size_t sz)
{
    void *p = NewPtr(sz);
    
    if (p == NULL)
	crash("malloc(): Out of memory!");

    memset(p, 0, sz);
    return p;
}

void
free(void *p)
{
    if (p == NULL)
	crash("free(): attempt to free NULL pointer!");

    DisposePtr((Ptr)p);
}

void *
realloc(void *ptr, size_t sz)
{
    void *p;
    
    if (ptr != NULL)
    {
    	// resizing to zero means free the original block
    	if (sz == 0)
    	{
    	    DisposePtr((char*)ptr);
    	    return NULL;
    	}
    	
    	// if we have a pointer, try the Mac's resize call first
	SetPtrSize((Ptr)ptr, sz);
	
	if (MemError() == noErr)
	    return ptr;
    }
    
    // we have to allocate another block and copy the original into it
    p = NewPtr(sz);
    
    if (p == NULL)
	crash("realloc(): Out of memory!");

    // original pointer was NULL, so just behave like malloc()
    if (ptr == NULL)
	return p;

    // copy the original, destroy the original pointer, and return
    memcpy(p, ptr, GetPtrSize((Ptr)ptr));
    DisposePtr((Ptr)ptr);
    return p;
}
