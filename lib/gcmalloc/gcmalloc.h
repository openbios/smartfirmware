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

// Copyright (c) 1992 by TJ Merritt and Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/gcmalloc/gcmalloc.h,v 1.10 1992/11/12 18:16:45 parag Exp $

#ifndef __GCMALLOC_H_
#define __GCMALLOC_H_

extern "C"
{
    extern void *malloc(size_t sz);
    extern void free(void *ptr);
    extern void *realloc(void *p, size_t nsz);
    extern void *calloc(size_t elems, size_t elsize);
    extern void cfree(void *p);

    extern void *gcblockalloc(size_t sz, boolean atomic, void *type,
	    const char *file = NULL, int line = 0);
    extern void *gcblockpointer(void *ptr);
    extern size_t gcblocksize(void *ptr);
    extern void *gcblocktype(void *ptr);
    extern boolean gcblockatomic(void *ptr);

    extern void gcaddregion(void *start, void *end);
    extern void gcremoveregion(void *start, void *end);
    extern void garbagecollect();
}

extern int gcafterpages();
extern void gcafterpages(int n);
extern boolean gcignorefree();
extern void gcignorefree(boolean yesno);
extern boolean gcskipinterior();
extern void gcskipinterior(boolean yesno);
extern void gcdump(FILE *fp = stdout);

#define malloc(sz) gcblockalloc((sz), FALSE, NULL, __FILE__, __LINE__)

#endif // __GCMALLOC_H_
