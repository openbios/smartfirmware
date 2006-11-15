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

/* Copyright (c) 1993 by TJ Merritt and Parag Patel.  All Rights Reserved.*/
/* $Header: /u/cgt/cvs/lib/aoa/aoa.h,v 1.9 2002/06/30 06:45:31 tjm Exp $ */

#ifndef __AOA_H_
#define __AOA_H_

#include <stdlib.h>
#include <stddef.h>

typedef int boolean;	/* fast */
typedef char bool;	/* small */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef long Long;
typedef unsigned long uLong;

#define BITSPERLONG	32
#define LONGMSB		(1UL << (BITSPERLONG - 1))

struct Freeblock
{
    struct Freeblock *link;
};

#define MULTIPAGE -1

/* size should be in MINALIGN chunks to preserve alignment of following memory*/
struct Pageinfo
{
    int count;	/* # of allocated blocks in page, total # pages */
    int blkclass;  /* index to per blocksize information,
				MULTIPAGE if multi-page */
    struct Pageinfo *link;	/* to link pages together */
    struct Freeblock *block;	/* 1st free block on this page, if any */
};

/* define PAGESIZE, ALIGNMENT, BASESIZE, and PAGEOVERHEAD based upon */
/* architecture of the machine.  All these need to be defined here since */
/* they affect the sizemap */
#ifdef _DJGPP_SOURCE
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
#define NO_VOID_CONST_QSORT_CMP
#endif

#ifdef _AUX_SOURCE
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
#endif

#ifdef OSX_SOURCE
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
#endif

#ifdef i386
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
#endif

#if defined __sparc || defined __sparc__
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
#ifndef __OpenBSD__
#include <thread.h>
#include <synch.h>
#endif
#endif

#ifdef BeOS
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
typedef long mutex_t;
extern void mutex_lock(mutex_t *m);
extern void mutex_unlock(mutex_t *m);
#endif

#if defined __MWERKS__ && !defined BeOS
#define macintosh
#endif

#ifdef macintosh
#define PAGESIZE 4096
#define ALIGNMENT 16
#define BASESIZE 4
#define PAGEOVERHEAD 16
// Allocate memory 1Mb at a whack.  This eats a lot of RAM but should be very fast.
//#define MINPAGES 256
// Allocate 64k at a whack, to keep usage down to reasonable amounts
#define MINPAGES 16
extern size_t getpagesize();
extern void *sbrk(size_t len);
#  ifndef __MWERKS__
#    define NO_VOID_CONST_QSORT_CMP
#  endif
#endif

#ifndef PAGESIZE
#define INSTRUMENT
#endif

#ifndef MINPAGES
#define MINPAGES 4
#endif

#ifdef MTSAFE
/* Locking protocol. g_freepagelock controls g_freepagelist and */
/* g_solopagelist.  g_locks[blkclass] controls a specific g_pageheader and */
/* g_freeheader. It is ok to hold g_freepagelock when acquiring one of the */
/* g_locks[blkclass].  To avoid dead lock, no g_locks should be held prior */
/* to acquiring g_freepagelock though. */

/* lock structure*/
struct Lockinfo
{
    mutex_t lk;
};

#define ACQUIRE_LOCK(l) mutex_lock(&l.lk);
#define RELEASE_LOCK(l) mutex_unlock(&l.lk);
#endif /* MTSAFE */

/* return pointer to beginning of page (descriptor) for a given address*/
#define GETPAGEINFO(p) \
    ((struct Pageinfo *)(((unsigned long)p - PAGEOVERHEAD) & ~(PAGESIZE - 1)))

extern void *aoa_allocpages(int numpgs);
#ifdef MTSAFE
    extern void aoa_freepages(void *ptr, int numpgs, int locked);
#else
    extern void aoa_freepages(void *ptr, int numpgs);
#endif
extern void *aoa_get_heaplow(void);
extern void *aoa_get_heaphigh(void);

/* malloc(), realloc(), and free() are all available for use here */

#endif /* __AOA_H_ */
