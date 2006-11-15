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

/* Copyright (c) 1993-2005 by TJ Merritt and Parag Patel.  All Rights Reserved.
 */

/* NOTE: g_freeheadercount is not properly syncronized */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

#ifdef MTSAFE
#include <thread.h>
#endif

#include "aoa.h"

#ifdef malloc
#  undef malloc
#endif

#ifdef calloc
#  undef calloc
#endif

#ifdef BACKTRACE
extern void backtrace(boolean syms);
#endif

#ifdef macintosh
#include <Memory.h>

size_t
getpagesize()
{
    return PAGESIZE;
}

void *
sbrk(size_t len)
{
    return (void*)NewPtr(len);
}

#endif /* macintosh */

#ifdef BeOS
void
mutex_lock(mutex_t *m)
{
    while (atomic_add(m, 1) != 0)
	atomic_add(m, -1);
}

void
mutex_unlock(mutex_t *m)
{
    atomic_add(m, -1);
}
#endif /* BeOS */


/* A page size has not been specified.  So we will allow it to vary at run*/
/* time and further instrument the code so that performance can be analyzed.*/
#ifdef INSTRUMENT

/* global parameters */
static size_t g_pagesize = 4096;
static size_t g_alignment = 16;
static size_t g_basesize = 4;
static size_t g_numclasses;
static size_t g_pageoverhead;
static size_t g_threshhold;

/* per block class, data structures */
static struct Pageinfo **g_pageheaders = NULL;
static struct Pageinfo **g_freeheaders = NULL;

#ifdef MTSAFE
static struct Lockinfo *g_locks = NULL;
#endif /* MTSAFE*/

static int *g_blocksizes = NULL;
static int *g_blockcount = NULL;

/* the size map */
static short *g_sizemap = NULL;

/* we're going to define all of these macros so make sure that they are */
/* not previously defined. */

#ifdef PAGESIZE
#undef PAGESIZE
#endif

#ifdef ALIGNMENT
#undef ALIGNMENT
#endif

#ifdef BASESIZE
#undef BASESIZE
#endif

#ifdef PAGEOVERHEAD
#undef PAGEOVERHEAD
#endif

#define PAGESIZE g_pagesize
#define ALIGNMENT g_alignment
#define BASESIZE g_basesize
#define NUMCLASSES g_numclasses
#define PAGEOVERHEAD g_pageoverhead
#define THRESHHOLD g_threshhold

#else /* def INSTRUMENT*/

#if PAGESIZE == 4096 && ALIGNMENT == 16 && BASESIZE == 4 && PAGEOVERHEAD == 16

#define NUMCLASSES 31
#define THRESHHOLD 2032

static const int g_blocksizes[NUMCLASSES] =
{
    4, 8, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
    272, 288, 304, 336, 368, 400, 448, 496, 576, 672, 816, 1008, 1360,
    2032,
};

static const int g_blockcount[NUMCLASSES] =
{
    1020, 510, 255, 127, 85, 63, 51, 42, 36, 31, 28, 25, 23, 21, 19, 18, 17,
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,
};

static const unsigned char g_sizemap[THRESHHOLD + 1] =
{
    0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30,
};

#endif /* PAGESIZE == 4096 && ALIGNMENT == 16 && BASESIZE == 4 && PAGEOVERHEAD == 16 */

/* place additional configuration information here */

/* per block class, data structures */
static struct Pageinfo *g_pageheaders[NUMCLASSES];
static struct Pageinfo *g_freeheaders[NUMCLASSES];

#ifdef MTSAFE
static struct Lockinfo g_locks[NUMCLASSES];
#endif /* MTSAFE*/

#endif /* ndef INSTRUMENT*/

#ifndef MINPAGES
#define MINPAGES 4
#endif /* MINPAGES*/

static struct Pageinfo *g_freepagelist = NULL;
static struct Pageinfo *g_solopagelist = NULL;
static int g_freeheadercount = 0;
static void *heaplow = NULL;
static void *heaphigh = NULL;
static int g_minpages = MINPAGES;
#ifdef BLOCKKERN
static size_t g_kern = 0;
#endif

#ifdef MTSAFE
struct Lockinfo g_freepagelock;
#endif

#ifdef INSTRUMENT
static FILE *g_malloclog = NULL;
static FILE *g_statfile = NULL;
static FILE *g_debugfile = NULL;
#endif

static void
abortf(char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#ifdef macintosh
    fprintf(stderr, "\n");
#else
    fprintf(stderr, "\r\n");
#endif
#ifdef BACKTRACE
    backtrace(TRUE);
#endif
#ifndef macintosh
    abort();
#endif
    exit(-1);
}

static struct Pageinfo *
mergepages(struct Pageinfo *pi1, struct Pageinfo *pi2)
{
    /* it is assumed that pi1 and pi2 are already sorted by address */
    struct Pageinfo *ret = NULL;
    struct Pageinfo *last = NULL;

    if (pi1 == NULL)
	return pi2;

    if (pi2 == NULL)
	return pi1;

    while (pi1 != NULL && pi2 != NULL)
    {
	struct Pageinfo *pi;

	/* select the next page */
	if (pi1 < pi2)
	{
	    pi = pi1;
	    pi1 = pi1->link;
	}
	else
	{
	    pi = pi2;
	    pi2 = pi2->link;
	}

	/* add it to the merged list */
	if (last == NULL)
	{
	    ret = pi;
	    last = pi;
	}
	else if ((char *)last + last->count * PAGESIZE == (char *)pi)
	    last->count += pi->count;
	else
	{
	    last->link = pi;
	    last = pi;
	}
    }

    if (pi1 == NULL)
	pi1 = pi2;

    /* add remainder of pi1 to ret. */
    for (; pi1 != NULL; pi1 = pi1->link)
    {
	if ((char *)last + last->count * PAGESIZE == (char *)pi1)
	    last->count += pi1->count;
	else
	{
	    last->link = pi1;
	    last = pi1;
	}
    }

    last->link = NULL;
    return ret;
}

void *
aoa_get_heaplow(void)
{
    return heaplow;
}

void *
aoa_get_heaphigh(void)
{
    return heaphigh;
}

void
aoa_freepages(void *ptr, int numpgs
#ifdef MTSAFE
    , int locked
#endif
    )
{
    /* insert pages into the free list in sorted order by address. */
    struct Pageinfo *fpi = (struct Pageinfo *)((unsigned long)ptr & ~(PAGESIZE - 1));

    void *ptrend;
    struct Pageinfo *prev;
    struct Pageinfo *freepage;

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "aoa_freepages(ptr=%p, numpgs=%d)\n", ptr, numpgs);
#endif

    if (fpi != ptr)
	abortf("aoa_freepages(): fpi=0x%X != ptr=0x%X", fpi, ptr);

    ptrend = ((char *)ptr) + numpgs * PAGESIZE;
    prev = NULL;
#ifdef MTSAFE
    if (!locked)
	ACQUIRE_LOCK(g_freepagelock);
#endif

    if (numpgs == 1)
    {
	fpi->link = g_solopagelist;
	fpi->blkclass = MULTIPAGE;
	fpi->count = 1;
	g_solopagelist = fpi;
#ifdef MTSAFE
	if (!locked)
	    RELEASE_LOCK(g_freepagelock);
#endif
	return;
    }

    freepage = g_freepagelist;

    for (; freepage != NULL; prev = freepage, freepage = freepage->link)
    {
	if ((char *)freepage + freepage->count * PAGESIZE == (char *)ptr)
	{
	    struct Pageinfo *nextpage = freepage->link;
	    freepage->count += numpgs;

	    if (nextpage && ((char *)freepage + freepage->count * PAGESIZE ==
			(char *)nextpage))
	    {
		freepage->count += nextpage->count;
		freepage->link = nextpage->link;
	    }

#ifdef MTSAFE
	    if (!locked)
		RELEASE_LOCK(g_freepagelock);
#endif
	    return;
	}

	if (ptrend == freepage)
	{
	    fpi->link = freepage->link;
	    fpi->count = numpgs + freepage->count;
	    fpi->blkclass = MULTIPAGE;

	    if (prev == NULL)
		g_freepagelist = fpi;
	    else
		prev->link = fpi;

#ifdef MTSAFE
	    if (!locked)
		RELEASE_LOCK(g_freepagelock);
#endif
	    return;
	}

	if ((char *)ptr < (char *)freepage)
	    break;
    }

    fpi->link = freepage;
    fpi->count = numpgs;
    fpi->blkclass = MULTIPAGE;

    if (prev == NULL)
	g_freepagelist = fpi;
    else
	prev->link = fpi;

#ifdef MTSAFE
    if (!locked)
	RELEASE_LOCK(g_freepagelock);
#endif
}

static int
#if defined NO_VOID_CONST_QSORT_CMP
ptrcmp(void *p1, void *p2)
#else
ptrcmp(void const *p1, void const *p2)
#endif
{
    struct Pageinfo *pi1 = *(struct Pageinfo **)p1;
    struct Pageinfo *pi2 = *(struct Pageinfo **)p2;

    if (pi1 < pi2)
	return -1;

    if (pi1 == pi2)
	return 0;

    return 1;
}

static void
releasepages()
{
    /* caller has already acquired locks so we don't need to do it here */
    struct Pageinfo *pi;
    int numblocks;

    if (g_solopagelist == NULL)
	return;

    /* find the length of the pending free page list */
    numblocks = 0;

    for (pi = g_solopagelist; pi != NULL; pi = pi->link)
	numblocks++;

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "releasepages: numblocks = %d\n", numblocks);
#endif

    if (numblocks == 1)
    {
	pi = g_solopagelist;
	g_solopagelist = NULL;
#ifdef MTSAFE
	aoa_freepages(pi, 1, TRUE);    /* will otherwise hang */
#else
	aoa_freepages(pi, 1);    /* will otherwise hang */
#endif
	return;
    }

    if (numblocks == 2)
    {
	struct Pageinfo *pi2;
	pi = g_solopagelist;
	pi2 = pi->link;
	g_solopagelist = NULL;

	if (pi > pi2)
	{
	    pi->link = NULL;
	    pi2->link = pi;
	    pi = pi2;
	}

	g_freepagelist = mergepages(g_freepagelist, pi);
	return;
    }

    pi = g_solopagelist;
    g_solopagelist = NULL;

    while (numblocks)
    {
	/* sort a page full of pointers. */
	int maxptrs = (PAGESIZE - PAGEOVERHEAD) / sizeof (struct Pageinfo *);
	int i = 0;
	int j;
	struct Pageinfo **pp = (struct Pageinfo **)((char *)pi + PAGEOVERHEAD);
	struct Pageinfo *npl;

	for (; pi != NULL; pi = pi->link)
	{
	    if (i == maxptrs)
		break;

	    pp[i++] = pi;
	}

	qsort(pp, i, sizeof (struct Pageinfo *), ptrcmp);

	/* build a new free list */
	npl = pp[i - 1];
	npl->link = NULL;

	for (j = i - 2; j >= 0; j--)
	{
	    struct Pageinfo *pi = pp[j];

	    if ((char *)pi + PAGESIZE == (char *)npl)
	    {
		pi->count = npl->count + 1;
		pi->link = npl->link;
	    }
	    else
		pi->link = npl;

	    npl = pi;
	}

	/* merge into existing free list */
	g_freepagelist = mergepages(g_freepagelist, npl);

	/* update pending free list */
	numblocks -= i;
    }
}

static int
expandarena(int numpgs)
{
    int offset;
    void *page;
    void *high;

    if (numpgs < g_minpages)
	numpgs = g_minpages;
#ifdef macintosh
    else
	numpgs++;		/* pages will not be aligned, so leave
				   room for slop */
#endif

#ifdef EXPIRETIME
    {
	static int checkdone = 0;
	time_t curtime;
	struct tm *timeinfo;

	if (!checkdone)
	{
	    curtime = time(&curtime);

	    if (curtime >= EXPIRETIME)
	    {
		fprintf(stderr, "SmartAlloc expired\n");
		exit(7);
	    }
	}
    }
#endif

    /* allocate the pages */
    page = (void *)sbrk(numpgs * PAGESIZE);

    if (page == NULL || (int)page == -1)
    {
#ifdef INSTRUMENT
	if (g_debugfile)
	    fprintf(g_debugfile, "expandarena(%d), sbrk(%d) failed\n", numpgs,
		numpgs *PAGESIZE);
#endif
	return FALSE;
    }

/* determine offset from start of the page sbrk returned */
offset = (unsigned long)page % PAGESIZE;

    if (offset)
    {
#ifdef macintosh
	/* we do not have a real sbrk(), so just drop one page to align
	   things */
	if (numpgs == 1)
	    return FALSE;

	numpgs--;
#else			    /* !macintosh */
	/* if not zero then allocate more space at the end to page align
	   things */
	void *base = (void *)sbrk(PAGESIZE - offset);

	if (base == NULL || (int)base == -1)
	{
#ifdef INSTRUMENT
	    if (g_debugfile)
		fprintf(g_debugfile,
			"expandarena(%d), offset sbrk(%d) failed\n", numpgs,
			PAGESIZE - offset);
#endif
	    return FALSE;
	}

	if ((char *)page + numpgs * PAGESIZE != (char *)base)
	{
#ifdef INSTRUMENT
	    if (g_debugfile)
		fprintf(g_debugfile,
			"expandarena(%d), offset sbrk(%d) was not contiguous\n", numpgs,
			PAGESIZE - offset);
#endif
	    if (numpgs == 1)
		return FALSE;

	    numpgs--;
	}
#endif			    /* !macintosh */

	/* revised page pointer */
	page = (void *)((char *)page + (PAGESIZE - offset));
    }

    /* update heap low and high water marks */
    if (heaplow == NULL || (char *)page < (char *)heaplow)
	heaplow = (char *)page;

    high = (void *)((char *)page + numpgs * PAGESIZE);

    if (heaphigh == NULL || (char *)high > (char *)heaphigh)
	heaphigh = high;

/* add pages to free list */
#ifdef INSTRUMENT
    if (g_statfile)
	fprintf(g_statfile, "expandarena: adding %dK to arena\n",
		(numpgs * PAGESIZE) / 1024);
#endif
#ifdef MTSAFE
    aoa_freepages(page, numpgs, TRUE);    /* will otherwise hang */
#else
    aoa_freepages(page, numpgs);/* will otherwise hang */
#endif
    return TRUE;
}

static void
releasefreeheaders(int numpgs)
{
    /* caller has already acquired freepagelist lock so we */
    /* don't need to do it here. */
    int blkclass;
#ifdef INSTRUMENT
    int pgsfreed = 0;
#endif

    for (blkclass = NUMCLASSES - 1; blkclass >= 0; blkclass--)
    {
	struct Pageinfo *pi;
#ifdef MTSAFE
	ACQUIRE_LOCK(g_locks[blkclass]);
#endif
	pi = g_freeheaders[blkclass];

	if (pi != NULL)
	{
	    struct Pageinfo *lp;
	    int pgs = 1;
	    g_freeheaders[blkclass] = NULL;

	    for (lp = pi; lp->link != NULL; lp = lp->link)
	    {
		lp->blkclass = MULTIPAGE;
		lp->count = 1;
		pgs++;
	    }

	    g_freeheadercount -= pgs;
#ifdef MTSAFE
	    RELEASE_LOCK(g_locks[blkclass]);
#endif
	    lp->link = g_solopagelist;
	    lp->blkclass = MULTIPAGE;
	    lp->count = 1;
	    g_solopagelist = pi;
#ifdef INSTRUMENT
	    pgsfreed += pgs;
#endif

	    /* if largest block of pages is at least numpgs return */

	    ;		    /* NOTE: not yet implemented */
	    numpgs = 0;
	}
#ifdef MTSAFE
	else
	    RELEASE_LOCK(g_locks[blkclass]);
#endif
    }

#ifdef INSTRUMENT
    if (g_statfile)
	fprintf(g_statfile, "releasefreeheaders: released %d pages (%dK)\n",
		pgsfreed, (pgsfreed * PAGESIZE) / 1024);
#endif
}

void *
aoa_allocpages(int numpgs)
{
    struct Pageinfo *prev = NULL;
    struct Pageinfo *freepage;
    void *ptr;

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "aoa_allocpages(%d)\n", numpgs);
#endif
#ifdef MTSAFE
    ACQUIRE_LOCK(g_freepagelock);
#endif

    if (numpgs == 1)
    {
	freepage = g_solopagelist;

	if (freepage)
	{
	    g_solopagelist = freepage->link;
#ifdef MTSAFE
	    RELEASE_LOCK(g_freepagelock);
#endif
	    return (char *)freepage;
	}
    }

    /* search for pages */
    freepage = g_freepagelist;

    for (; freepage != NULL; prev = freepage, freepage = freepage->link)
	if (freepage->count >= numpgs)
	    break;

    /* check if any pages were found */
    if (freepage == NULL)
    {
	if (g_solopagelist)
	{
	    releasepages();

	    for (prev = NULL, freepage = g_freepagelist; freepage != NULL;
		    prev = freepage, freepage = freepage->link)
		if (freepage->count >= numpgs)
		    break;
	}

	if (freepage == NULL)
	{
	    /* release pages in the g_freeheaders lists.  We are finally 
	    */
	    /* reclaiming this memory.  This is about as long as we can
	       delay. */
	    if (g_freeheadercount)
	    {
		releasefreeheaders(numpgs);
		releasepages();

		for (prev = NULL, freepage = g_freepagelist; freepage != NULL;
			prev = freepage, freepage = freepage->link)
		    if (freepage->count >= numpgs)
			break;
	    }

	    if (freepage == NULL)
	    {
		if (!expandarena(numpgs))
		{
#ifdef MTSAFE
		    RELEASE_LOCK(g_freepagelock);
#endif
#ifdef INSTRUMENT
		    if (g_debugfile)
			fprintf(g_debugfile,
				"aoa_allocpages(%d), cannot expand data area\n",
				numpgs);
#endif
		    return NULL;
		}

		for (prev = NULL, freepage = g_freepagelist; freepage != NULL;
			prev = freepage, freepage = freepage->link)
		    if (freepage->count >= numpgs)
			break;

		/* everything failed! */
		if (freepage == NULL)
		{
#ifdef MTSAFE
		    RELEASE_LOCK(g_freepagelock);
#endif
#ifdef INSTRUMENT
		    if (g_debugfile)
			fprintf(g_debugfile,
				"aoa_allocpages(%d), no block large enough after expand\n",
				numpgs);
#endif
		    return NULL;
		}
	    }
	}
    }

    /* remove pages from freepage list */
    if (freepage->count == numpgs)
    {
	if (prev == NULL)
	    g_freepagelist = freepage->link;
	else
	    prev->link = freepage->link;

	ptr = freepage;
    }
    else
    {
#ifdef ALLOCENDPAGES
	freepage->count -= numpgs;
	ptr = (char *)freepage + freepage->count * PAGESIZE;
	freepage = (struct Pageinfo *)ptr;
	freepage->count = numpgs;
	freepage->link = NULL;
	freepage->blkclass = MULTIPAGE
#else
	    void *npage = (char *)freepage + numpgs * PAGESIZE;
	int count = freepage->count - numpgs;
	struct Pageinfo *link = freepage->link;
	ptr = freepage;
	freepage = (struct Pageinfo *)npage;
	freepage->count = count;
	freepage->link = link;
	freepage->blkclass = MULTIPAGE;

	if (prev == NULL)
	    g_freepagelist = freepage;
	else
	    prev->link = freepage;
#endif
    }

#ifdef MTSAFE
    RELEASE_LOCK(g_freepagelock);
#endif
#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "aoa_allocpages(%d), page = %p\n", numpgs, ptr);
#endif
    return ptr;
}

static int
getmoreblocks(int blkclass)
{
    struct Pageinfo *pi;
    void *page;
    int sz;
    int bpp;
    struct Freeblock *e;
    int i;
#ifdef BLOCKKERN
    int excess;
#endif

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "getmoreblocks(%d)\n", blkclass);
#endif

    if (blkclass == MULTIPAGE)
	return FALSE;

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "getmoreblocks(%d), size is valid\n", blkclass);
#endif

    pi = g_freeheaders[blkclass];

    if (pi != NULL)
    {
	/* This bypasses building the linked lists within the page if we 
	*/
	/* have a free page that already has a linked list built. */
	g_freeheaders[blkclass] = pi->link;
	pi->link = g_pageheaders[blkclass];
	g_pageheaders[blkclass] = pi;
	g_freeheadercount--;
	return TRUE;
    }

#ifdef MTSAFE
    /* we can't hold the block class lock since it may be reacquired by */
    /* aoa_allocpages. */
    RELEASE_LOCK(g_locks[blkclass]);
#endif

    page = aoa_allocpages(1);    /* get a page */

#ifdef MTSAFE
    ACQUIRE_LOCK(g_locks[blkclass]);
#endif

    if (page == NULL)
	return FALSE;

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "getmoreblocks(%d), got new page\n", blkclass);
#endif

    /* fill out pageinfo for this page */
    sz = g_blocksizes[blkclass];
    bpp = g_blockcount[blkclass];
#ifdef BLOCKKERN
    excess = PAGESIZE - (sz * bpp) - PAGEOVERHEAD;

    if (excess)
    {
#ifdef macintosh
	excess = (g_kern ^ (((long)page >> 15) & ~0xF)) %
	    (excess + ALIGNMENT - (excess % ALIGNMENT));
#else
	excess = (g_kern ^ (((long)page >> 14) & ~0xF)) %
	    (excess + ALIGNMENT - (excess % ALIGNMENT));
#endif
	g_kern += ALIGNMENT;

	if (excess % ALIGNMENT != 0)
	    abortf("getmoreblocks(): g_kern was trashed\n");
    }

    e = (struct Freeblock *)((char *)page + PAGEOVERHEAD + excess);
#else
    e = (struct Freeblock *)((char *)page + PAGEOVERHEAD);
#endif
    pi = (struct Pageinfo *)page;
    pi->count = 0;
    pi->blkclass = blkclass;
    pi->block = e;
    pi->link = g_pageheaders[blkclass];
    g_pageheaders[blkclass] = pi;
    bpp--;

    for (i = 0; i < bpp; i++)
    {
	struct Freeblock *ne = (struct Freeblock *)((char *)e + sz);
	e->link = ne;
	e = ne;
    }

    e->link = NULL;
    return TRUE;
}

#ifdef INSTRUMENT
long 
power2(long x)
{
    long i = 1;

    while (i < x)
	i += i;

    return i;
}

void
dump_tables(char const *filename)
{
    int col;
    int i;
    char *type;
    int ind;
    long sz;

    FILE *fp = fopen(filename, "w");

    if (fp == NULL)
	return;

    fprintf(fp, "\n#if PAGESIZE == %d && ALIGNMENT == %d && BASESIZE == %d && PAGEOVERHEAD == %d\n\n",
	    g_pagesize, g_alignment, g_basesize, g_pageoverhead);
    fprintf(fp, "#define NUMCLASSES %d\n", g_numclasses);
    fprintf(fp, "#define THRESHHOLD %d\n\n", g_threshhold);
    fprintf(fp, "static const int g_blocksizes[NUMCLASSES] =\n{\n   ");
    col = 3;

    for (i = 0; i < g_numclasses; i++)
    {
	int blocksize = g_blocksizes[i];
	int w;

	if (blocksize < 10)
	    w = 3;
	else if (blocksize < 100)
	    w = 4;
	else if (blocksize < 1000)
	    w = 5;
	else if (blocksize < 10000)
	    w = 6;
	else
	    w = 7;

	if (col + w >= 75)
	{
	    fprintf(fp, " %d,\n   ", blocksize);
	    col = w + 3;
	}
	else
	{
	    fprintf(fp, " %d,", blocksize);
	    col += w;
	}
    }

    fprintf(fp, "\n};\n\n");
    fprintf(fp, "static const int g_blockcount[NUMCLASSES] =\n{\n   ");
    col = 3;

    for (i = 0; i < g_numclasses; i++)
    {
	int blockcount = g_blockcount[i];
	int w;

	if (blockcount < 10)
	    w = 3;
	else if (blockcount < 100)
	    w = 4;
	else if (blockcount < 1000)
	    w = 5;
	else if (blockcount < 10000)
	    w = 6;
	else
	    w = 7;

	if (col + w >= 75)
	{
	    fprintf(fp, " %d,\n   ", blockcount);
	    col = w + 3;
	}
	else
	{
	    fprintf(fp, " %d,", blockcount);
	    col += w;
	}
    }

    fprintf(fp, "\n};\n\n");

    if (g_numclasses <= 256)
	type = "char";
    else if (g_numclasses <= 65536)
	type = "short";
    else
	type = "int";

    fprintf(fp, "static const unsigned %s g_sizemap[THRESHHOLD + 1] =\n{\n   ", type);
    ind = 0;
    col = 3;

    for (sz = 0; sz <= g_threshhold; sz++)
    {
	int t = ind;
	int w;

	if (t < 10)
	    w = 3;
	else if (t < 100)
	    w = 4;
	else if (t < 1000)
	    w = 5;
	else if (t < 10000)
	    w = 6;
	else
	    w = 7;

	if (col + w >= 75)
	{
	    fprintf(fp, " %d,\n   ", t);
	    col = w + 3;
	}
	else
	{
	    fprintf(fp, " %d,", t);
	    col += w;
	}

	if (g_blocksizes[ind] == sz)
	    ind++;
    }

    fprintf(fp, "\n};\n\n");
    fprintf(fp, "#endif /* PAGESIZE == %d && ALIGNMENT == %d && BASESIZE == %d && PAGEOVERHEAD == %d */\n\n",
	    g_pagesize, g_alignment, g_basesize, g_pageoverhead);
    fclose(fp);
}

static void *
getmem(size_t sz)
{
    void *ptr = (void *)sbrk(sz + ALIGNMENT - 1);
    unsigned long offset;

    if (ptr == NULL || (int)ptr == -1)
	return NULL;

    offset = (unsigned long)ptr % ALIGNMENT;

    if (offset)
	ptr = (void *)((char *)ptr + (ALIGNMENT - offset));

    return ptr;
}


static int initialized = FALSE;
#ifdef MTSAFE
struct Lockinfo initlock;
#endif

static void
init()
{
    char *e;
    long blocksize;
    long nblocksize;
    int bpp;
    int blkclass;
    long sz;

#ifdef MTSAFE
    ACQUIRE_LOCK(initlock);
#endif

    if (initialized)
    {
#ifdef MTSAFE
	RELEASE_LOCK(initlock);
#endif
	return;
    }

    g_freepagelist = NULL;
    heaplow = NULL;
    heaphigh = NULL;

    e = getenv("SMARTALLOC_PAGE_SIZE");

    if (e != NULL && *e != '\0')
	g_pagesize = power2(atoi(e));

    e = getenv("SMARTALLOC_ALIGNMENT");

    if (e != NULL && *e != '\0')
	g_alignment = power2(atoi(e));

    e = getenv("SMARTALLOC_BASE_SIZE");

    if (e != NULL && *e != '\0')
	g_basesize = power2(atoi(e));

    e = getenv("SMARTALLOC_MIN_PAGES");

    if (e != NULL && *e != '\0')
    {
	g_minpages = atoi(e);

	if (g_minpages < 1)
	    g_minpages = MINPAGES;
    }

    g_pageoverhead = ((sizeof (struct Pageinfo)+g_alignment - 1) /
	    g_alignment) * g_alignment;

    g_threshhold = (g_pagesize - g_pageoverhead) / 2;
    g_threshhold -= g_threshhold % g_alignment;    /* round down */

    /* calculate the number of block classes */
    g_numclasses = 1;
    blocksize = g_threshhold;

    for (bpp = 3;; bpp++)
    {
	nblocksize = (g_pagesize - g_pageoverhead) / bpp;
	nblocksize -= nblocksize % g_alignment;

	if (nblocksize == blocksize)
	    break;

	g_numclasses++;
	blocksize = nblocksize;
    }

    for (blocksize -= g_alignment; blocksize > g_alignment;
	    blocksize -= g_alignment)
	g_numclasses++;

    for (; blocksize >= g_basesize; blocksize >>= 1)
	g_numclasses++;

    /* initialize g_blocksizes */
    g_blocksizes = (int *)getmem(sizeof (int)*g_numclasses);
    g_blockcount = (int *)getmem(sizeof (int)*g_numclasses);
    g_pageheaders = (struct Pageinfo **)getmem(sizeof (struct Pageinfo *)*
	    g_numclasses);
    g_freeheaders = (struct Pageinfo **)getmem(sizeof (struct Pageinfo *)*
	    g_numclasses);
#ifdef MTSAFE
    g_locks = (struct Lockinfo *)getmem(sizeof (struct Lockinfo)*
	    g_numclasses);
    memset(g_locks, '\0', sizeof (struct Lockinfo)*g_numclasses);
#endif
    blkclass = 0;

    for (blocksize <<= 1; blocksize < g_alignment; blocksize <<= 1)
    {
	g_blocksizes[blkclass] = blocksize;
	g_blockcount[blkclass] = (g_pagesize - g_pageoverhead) / blocksize;
	g_pageheaders[blkclass] = NULL;
	g_freeheaders[blkclass] = NULL;
	blkclass++;
    }

    for (; blocksize <= nblocksize; blocksize += g_alignment)
    {
	g_blocksizes[blkclass] = blocksize;
	g_blockcount[blkclass] = (g_pagesize - g_pageoverhead) / blocksize;
	g_pageheaders[blkclass] = NULL;
	g_freeheaders[blkclass] = NULL;
	blkclass++;
    }

    for (bpp -= 2; bpp >= 2; bpp--)
    {
	blocksize = (g_pagesize - g_pageoverhead) / bpp;
	blocksize -= blocksize % g_alignment;
	g_blocksizes[blkclass] = blocksize;
	g_blockcount[blkclass] = bpp;
	g_pageheaders[blkclass] = NULL;
	g_freeheaders[blkclass] = NULL;
	blkclass++;
    }

    if (blkclass != g_numclasses)
	abortf("init(): block class mismatch, %d, %d", blkclass, g_numclasses);

    /* initialize g_sizemap */
    g_sizemap = (short *)getmem(sizeof (short)*(g_threshhold + 1));
    blkclass = 0;

    for (sz = 0; sz <= g_threshhold; sz++)
    {
	g_sizemap[sz] = blkclass;

	if (g_blocksizes[blkclass] == sz)
	    blkclass++;
    }

    if (blkclass != g_numclasses)
	abortf("init(): block class mismatch after size map init, %d, %d", blkclass, g_numclasses);

    /* this is for things calling malloc in the following statements... */
    initialized = TRUE;
#ifdef MTSAFE
    RELEASE_LOCK(initlock);
#endif

    e = getenv("SMARTALLOC_TABLES_FILE");

    if (e != NULL && *e != '\0')
	dump_tables(e);

    e = getenv("SMARTALLOC_LOG_FILE");

    if (e != NULL && *e != '\0')
	g_malloclog = fopen(e, "w");

    e = getenv("SMARTALLOC_STAT_FILE");

    if (e != NULL)
    {
	if (*e != '\0')
	    g_statfile = fopen(e, "w");
	else
	    g_statfile = stderr;
    }

    e = getenv("SMARTALLOC_DEBUG_FILE");

    if (e != NULL)
    {
	if (*e != '\0')
	    g_debugfile = fopen(e, "w");
	else
	    g_debugfile = stderr;
    }
}
#endif

void *
#ifdef INSTRUMENT
base_malloc(size_t sz)
#else
malloc(size_t sz)
#endif
{
    int pgs;
    void *base;
    struct Pageinfo *pi;

#ifdef INSTRUMENT
    if (!initialized)
	init();
#endif

    if (sz <= THRESHHOLD)
    {
	int blkclass = g_sizemap[sz];
	struct Pageinfo *ppi = NULL;
	struct Freeblock *ptr;

#ifdef INSTRUMENT
	if (g_debugfile)
	    fprintf(g_debugfile, "malloc(%d), small block, class = %d\n", sz,
		    blkclass);
#endif
#ifdef MTSAFE
	ACQUIRE_LOCK(g_locks[blkclass]);
#endif

	pi = g_pageheaders[blkclass];

	if (pi != NULL)
	{
	    struct Freeblock *block = pi->block;

	    if (block != NULL)
	    {
		struct Freeblock *link = block->link;
		pi->block = link;
		pi->count++;

		if (link == NULL)
		    /* remove completely full block off of the list */
		    g_pageheaders[blkclass] = pi->link;

#ifdef MTSAFE
		RELEASE_LOCK(g_locks[blkclass]);
#endif
		return (void *)block;
	    }
	    else
		abortf("malloc(): corrupt arena, full block on page list\n");

	    ppi = pi;
	    pi = pi->link;
	}

	/* search pages with blocks of the given size */
	for (; pi != NULL && pi->block == NULL; ppi = pi, pi = pi->link)
	    ;

	if (pi == NULL)
	{
	    if (!getmoreblocks(blkclass))
	    {
#ifdef MTSAFE
		RELEASE_LOCK(g_locks[blkclass]);
#endif
#ifdef INSTRUMENT
		if (g_debugfile)
		    fprintf(g_debugfile, "malloc(%d), no more small blocks\n",
			    sz);
#endif
		return NULL;
	    }

	    /* this works since getmoreblocks always sticks new pages */
	    /* at the front of the list */
	    pi = g_pageheaders[blkclass];
	    ppi = NULL;
	}

	if (ppi != NULL)
	{
	    /* move page with free blocks to the head of the list so we
	       save */
	    /* time on the next call for this size. */
	    ppi->link = pi->link;
	    pi->link = g_pageheaders[blkclass];
	    g_pageheaders[blkclass] = pi;
	}

	ptr = pi->block;
	pi->block = ptr->link;
	pi->count++;
#ifdef MTSAFE
	RELEASE_LOCK(g_locks[blkclass]);
#endif

#ifdef INSTRUMENT
	if (g_debugfile)
	    fprintf(g_debugfile, "malloc(%d), class=%d small block=%p\n", sz,
		    blkclass, ptr);
#endif
	return (void *)ptr;
    }

#ifdef INSTRUMENT
    if (g_debugfile)
	fprintf(g_debugfile, "malloc(%d), large block\n", sz);
#endif

    /* block is bigger than threshhold so allocate whole pages */
    pgs = (sz + PAGEOVERHEAD + PAGESIZE - 1) / PAGESIZE;
    base = aoa_allocpages(pgs);

    if (base == NULL)
	return NULL;

    pi = (struct Pageinfo *)base;
    pi->count = pgs;
    pi->link = NULL;
    return (char *)base + PAGEOVERHEAD;
}

void
#ifdef INSTRUMENT
base_free(void *ptr)
#else
free(void *ptr)
#endif
{
    struct Pageinfo *pi;
    int blkclass;

#ifdef SMART_FREE_PTR_CHECK
  #if defined macintosh || defined BeOS
    if ((char*)ptr >= (char*)NULL && (char *)ptr <= (char *)256)
  #else			    /* !macintosh */
    extern char end[];
    if ((char*)ptr >= (char*)NULL && (char *)ptr <= end)
  #endif			    /* !macintosh */
    {
#endif	/* SMART_FREE_PTR_CHECK */
	if (ptr == NULL)
	{
	    static char *e = NULL;

	    if (e == NULL)
	    {
		e = getenv("SMARTALLOC_PROHIBIT_NULL_FREE");

		if (e == NULL)
		    e = "";
	    }

	    if (*e && atoi(e))
		abortf("free(): attempt to free NULL pointer");

	    return;
	}

#ifdef SMART_FREE_PTR_CHECK
	abortf("free(): attempt to free a pointer to code or static data");
    }
#endif	/* SMART_FREE_PTR_CHECK */

#ifdef INSTRUMENT
    if ((char *)ptr < (char *)heaplow || (char *)ptr > (char *)heaphigh)
	abortf("free(): attempt to free a pointer outside of the heap");
#endif

    pi = GETPAGEINFO(ptr);
    blkclass = pi->blkclass;

    if (blkclass >= 0 && blkclass != MULTIPAGE)
    {
	struct Freeblock *e;
	struct Freeblock *l;
	int count;
	struct Pageinfo *ppi;
	struct Pageinfo *p;

	if (blkclass >= NUMCLASSES)
	    abortf("free(0x%X): bad block class %d in page 0x%X", ptr,
		    blkclass, pi);

#ifdef INSTRUMENT
	if (g_debugfile)
	    fprintf(g_debugfile,
		    "free(%p) pi=%p blocksize=%d numblocks=%d\n",
		    ptr, pi, g_blocksizes[blkclass], g_blockcount[blkclass]);
#endif
#ifdef MTSAFE
	ACQUIRE_LOCK(g_locks[blkclass]);
#endif

	/* place the block on the pages free block list */
	e = (struct Freeblock *)ptr;
	l = pi->block;
	e->link = l;
	pi->block = e;

	if (l == NULL)
	{
	    /* full page, now has a free block so put it on the pagelist 
	    */
	    pi->link = g_pageheaders[blkclass];
	    g_pageheaders[blkclass] = pi;
	}

	/* decrement the use count for this page */
	count = pi->count - 1;
	pi->count = count;

	if (count > 0)
	{
	    /* some blocks are in use so just return */
#ifdef MTSAFE
	    RELEASE_LOCK(g_locks[blkclass]);
#endif
	    return;
	}

	if (count != 0)
	    abortf("free(0x%X): bad free block count %d in page 0x%X", ptr,
		    count, pi);

	/* all blocks in this page are free, so put it back in the */
	/* freepage pool */
	ppi = NULL;
	p = g_pageheaders[blkclass];

	for (; p != NULL && p != pi; ppi = p, p = p->link)
	    ;

	if (p == NULL)
	    abortf("free(0x%X): cannot find block 0x%X in pageheaders", ptr, pi);

	if (ppi == NULL)
	    g_pageheaders[blkclass] = pi->link;
	else
	    ppi->link = pi->link;

	/* place the page on the block class indexed free list */
	pi->link = g_freeheaders[blkclass];
	g_freeheaders[blkclass] = pi;
	g_freeheadercount++;
#ifdef MTSAFE
	RELEASE_LOCK(g_locks[blkclass]);
#endif
	return;
    }

    if (blkclass != MULTIPAGE)
	abortf("free(0x%X): invalid pointer - class=0x%X", ptr, blkclass);

#ifdef MTSAFE
    aoa_freepages(pi, pi->count, FALSE);
#else
    aoa_freepages(pi, pi->count);
#endif
}

void *
#ifdef INSTRUMENT
base_realloc(void *p, size_t nsz)
#else
realloc(void *p, size_t nsz)
#endif
{
    struct Pageinfo *pi;
    int blkclass;
    size_t sz;
    void *np;

    if (p == NULL)
#ifdef INSTRUMENT
	return base_malloc(nsz);
#else
    return malloc(nsz);
#endif

    if (nsz == 0)
    {
#ifdef INSTRUMENT
	base_free(p);
#else
	free(p);
#endif
	return NULL;
    }

    pi = GETPAGEINFO(p);
    blkclass = pi->blkclass;
    sz = (blkclass >= 0 && blkclass != MULTIPAGE) ? g_blocksizes[blkclass] :
	pi->count * PAGESIZE - PAGEOVERHEAD;

    if (nsz > sz)
    {
	/* make sure that we grow by at least ~twenty percent */
	int minnsz = (sz * 77) / 64;
	void *np;

	if (nsz < minnsz)
	    nsz = minnsz;

#ifdef INSTRUMENT
	np = base_malloc(nsz);
#else
	np = malloc(nsz);
#endif

	if (np == NULL)
	    return NULL;

	memcpy(np, p, sz);
#ifdef INSTRUMENT
	base_free(p);
#else
	free(p);
#endif
	return np;
    }

    /* make sure that we shrink by at least 75% */
    if (nsz > sz / 4)
	return p;

#ifdef INSTRUMENT
    np = base_malloc(nsz);
#else
    np = malloc(nsz);
#endif

    if (np == NULL)
	return NULL;

    memcpy(np, p, nsz);
#ifdef INSTRUMENT
    base_free(p);
#else
    free(p);
#endif
    return np;
}

void *
calloc(size_t elems, size_t elsize)
{
    size_t size = elems * elsize;
    void *p = malloc(size);
    struct Pageinfo *pi;

    if (p == NULL)
	return NULL;

    pi = GETPAGEINFO(p);

    if (pi->blkclass >= 0)
	memset(p, '\0', g_blocksizes[pi->blkclass]);
    else if (pi->blkclass == MULTIPAGE)
	memset(p, '\0', pi->count * PAGESIZE - PAGEOVERHEAD);
    else
	abortf("calloc: malloc returned a bogus block");

    return p;
}

void
cfree(void *p)
{
    free(p);
}

void *
#ifdef INSTRUMENT
base_memalign(size_t align, size_t sz)
#else
memalign(size_t align, size_t sz)
#endif
{
    size_t npgs;
    size_t rpgs;
    void *p;
    void *p2;
    unsigned long offset;
    struct Pageinfo *pi;
    size_t waste;

    /* force alignment to be a power of two */
    size_t algn = 1;
    size_t mask;

    while (algn < align)
	algn += algn;

    mask = algn - 1;

#ifdef INSTRUMENT
    if (!initialized)
	init();
#endif

    /* see if we can find a block that is properly aligned */
    if (sz <= THRESHHOLD)
    {
	int blkclass = g_sizemap[sz];
	struct Pageinfo *pi;
	struct Pageinfo *ppi = NULL;
	int emptypage = 0;

	/* it might be a nice idea to see if it is even possible to find a
	   */
	/* block before we go searching */

#ifdef MTSAFE
	ACQUIRE_LOCK(g_locks[blkclass]);
#endif

	if (g_pageheaders[blkclass] == NULL)
	{
	    getmoreblocks(blkclass);
	    emptypage = 1;
	}

	for (pi = g_pageheaders[blkclass]; pi != NULL; ppi = pi, pi = pi->link)
	{
	    struct Freeblock *b;
	    struct Freeblock *pb = NULL;

	    for (b = pi->block; b != NULL; pb = b, b = b->link)
		if (((unsigned long)b & mask) == 0)
		{
		    struct Freeblock *nb = b->link;

		    if (pb == NULL)
			pi->block = nb;
		    else
			pb->link = nb;

		    pi->count++;

		    if (pb == NULL && nb == NULL)
		    {
			if (ppi == NULL)
			    g_pageheaders[blkclass] = pi->link;
			else
			    ppi->link = pi->link;
		    }

#ifdef MTSAFE
		    RELEASE_LOCK(g_locks[blkclass]);
#endif
		    return (void *)b;
		}
	}

	if (emptypage)
	{
	    /* move it the the freeheaders list */
	    pi = g_pageheaders[blkclass];
	    g_pageheaders[blkclass] = NULL;
	    pi->link = g_freeheaders[blkclass];
	    g_freeheaders[blkclass] = pi;
	    g_freeheadercount++;
	}

	/* at this point we should really see if we can create a page */
	/* that would have a block that we could use. */

#ifdef MTSAFE
	RELEASE_LOCK(g_locks[blkclass]);
#endif
    }

    npgs = (sz + algn + PAGEOVERHEAD + PAGESIZE - 1) / PAGESIZE;
    p = aoa_allocpages(npgs);
    pi = (struct Pageinfo *)p;
    pi->count = npgs;
    pi->link = NULL;
    p2 = (char *)p + PAGEOVERHEAD;
    offset = algn - (unsigned long)p2 % algn;

    if (offset != algn)
	p2 = (char *)p2 + offset;

    pi = GETPAGEINFO(p2);
    pi->blkclass = -1;
    pi->link = NULL;
    waste = ((char *)pi - (char *)p) / PAGESIZE;

    /* free lead in pages that are not used */
    if (waste)
    {
#ifdef MTSAFE
	aoa_freepages(p, waste, FALSE);
#else
	aoa_freepages(p, waste);
#endif
	npgs -= waste;
    }

    rpgs = (((char *)p2 - (char *)pi) + sz + PAGESIZE - 1) / PAGESIZE;
    pi->count = rpgs;

#ifdef DEBUG
    fprintf(stderr, "memalign: sz = %d, align = %d, npgs = %d, waste1 = %d, waste2 = %d\n",
	    sz, align, npgs, waste, npgs - rpgs);
#endif

    /* free unused trailer pages */
    if (rpgs < npgs)
    {
#ifdef MTSAFE
	aoa_freepages((char *)pi + rpgs * PAGESIZE, npgs - rpgs, FALSE);
#else
	aoa_freepages((char *)pi + rpgs * PAGESIZE, npgs - rpgs);
#endif
    }

    return p2;
}

#ifdef INSTRUMENT
void *
malloc(size_t sz)
{
    static int inmalloc = 0;
    void *p = base_malloc(sz);

    if (!inmalloc)
    {
	inmalloc = 1;

	if (g_malloclog != NULL)
	    fprintf(g_malloclog, "m %u %p\n", sz, p);

	inmalloc = 0;
    }

    return p;
}

void
free(void *ptr)
{
    if (g_malloclog != NULL)
	fprintf(g_malloclog, "f %p\n", ptr);

    base_free(ptr);
}

void *
realloc(void *p, size_t nsz)
{
    void *np = base_realloc(p, nsz);

    if (g_malloclog != NULL)
	fprintf(g_malloclog, "r %p %u %p\n", p, nsz, np);

    return np;
}

void *
memalign(size_t align, size_t sz)
{
    void *p = base_memalign(align, sz);

    if (g_malloclog != NULL)
	fprintf(g_malloclog, "a %u %u %p\n", align, sz, p);

    return p;
}
#endif

void *
valloc(size_t sz)
{
    return memalign(PAGESIZE, sz);
}

size_t
blocksize(void *ptr)
{
    struct Pageinfo *pi;
    int blkclass;
    int blksize;

#ifdef SMART_FREE_PTR_CHECK
  #if defined macintosh || defined BeOS
    if ((char *)ptr <= (char *)256)
  #else			    /* !macintosh */
    extern char end[];

    if ((char *)ptr <= end)
  #endif			    /* !macintosh */
	return 0;
#endif	/* SMART_FREE_PTR_CHECK */

    pi = GETPAGEINFO(ptr);
    blkclass = pi->blkclass;

    if (blkclass == MULTIPAGE)
	return pi->count * PAGESIZE - PAGEOVERHEAD;

    blksize = g_blocksizes[blkclass];

    if (blksize > 0)
	return blksize;

    return 0;
}

void
meminfo(size_t *allocspc, size_t *freespc, size_t *allocblks,
    size_t *freeblks, void **endarena, size_t *arenasz)
{
    struct Pageinfo *fp;
    int pg;

    *allocspc = 0;
    *freespc = 0;
    *allocblks = 0;
    *freeblks = 0;
    *endarena = heaphigh;
    *arenasz = (char *)heaphigh - (char *)heaplow;

    for (fp = g_freepagelist; fp != NULL; fp = fp->link)
    {
	freeblks++;
	freespc += fp->count * PAGESIZE;
    }

    for (pg = 0; pg < NUMCLASSES; pg++)
	if (g_pageheaders[pg] != NULL)
	{
	    struct Pageinfo *pi = g_pageheaders[pg];

	    if (g_blocksizes[pi->blkclass] > 0)
	    {
		freeblks += g_blockcount[pi->blkclass] - pi->count;
		allocblks += pi->count;
		allocspc += pi->count * g_blocksizes[pi->blkclass];
	    }
	    else if (pi->blkclass == MULTIPAGE)
	    {
		allocblks++;
		allocspc += pi->count * PAGESIZE - PAGEOVERHEAD;
	    }
	}
}

void
dumpheap()
{
    int i;
    struct Pageinfo *pi;
    int numpgs;
#ifndef INSTRUMENT
    FILE *g_debugfile = stderr;
#endif

    if (g_debugfile == NULL)
	return;

    fprintf(g_debugfile, "PAGESIZE = %d\n", PAGESIZE);
    fprintf(g_debugfile, "ALIGNMENT = %d\n", ALIGNMENT);
    fprintf(g_debugfile, "BASESIZE = %d\n", BASESIZE);
    fprintf(g_debugfile, "NUMCLASSES = %d\n", NUMCLASSES);
    fprintf(g_debugfile, "PAGEOVERHEAD = %d\n", PAGEOVERHEAD);
    fprintf(g_debugfile, "THRESHHOLD = %d\n", THRESHHOLD);

    for (i = 0; i < NUMCLASSES; i++)
    {
	int count = g_blockcount[i];
	int used = 0;
	int avail = 0;

	fprintf(g_debugfile, "block class %d[%d,%d], pages=%p, free=%p, used = ",
		i, g_blocksizes[i], g_blockcount[i], g_pageheaders[i],
		g_freeheaders[i]);

	for (pi = g_pageheaders[i]; pi; pi = pi->link)
	{
	    used += pi->count;
	    avail += count - pi->count;
	}

	fprintf(g_debugfile, "%d, avail = %d\n", used, avail);
    }

    for (pi = (struct Pageinfo *)heaplow;
	    pi != NULL && (char *)pi < (char *)heaphigh;
	    pi = (struct Pageinfo *)((char *)pi + numpgs * PAGESIZE))
    {
	int size;

	if (pi->blkclass == MULTIPAGE)
	{
	    size = pi->count * PAGESIZE;
	    numpgs = pi->count;
	}
	else
	{
	    size = g_blocksizes[pi->blkclass];
	    numpgs = 1;
	}

	fprintf(g_debugfile, "Page %p, class=%d, size=%d, count=%d, blks=%d, link=%p, block=%p\n",
		(char *)pi, pi->blkclass, size, pi->count,
		g_blockcount[pi->blkclass], pi->link, pi->block);
    }


    for (pi = g_solopagelist; pi; pi = pi->link)
	fprintf(g_debugfile, "Solo page %p, link = %p\n", pi, pi->link);

    for (pi = g_freepagelist; pi; pi = pi->link)
	fprintf(g_debugfile, "Free page %p, count = %d, link = %p\n", pi,
		pi->count, pi->link);
}
