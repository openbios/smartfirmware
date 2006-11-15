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

/* Copyright 1994,1998 by Parag Patel.  All Rights Reserved. */
/* $Header: /u/cgt/cvs/lib/aoa/gcbase.h,v 1.9 2000/12/30 21:08:24 parag Exp $ */

#ifndef __GCBASE_H_
#define __GCBASE_H_

#include "aoa.h"

#define GC_PAGE_OVERHEAD 32

struct gc_type_info;

struct gc_pageinfo
{
	struct Pageinfo p;
	struct gc_interface *interface;
	struct gc_type_info *type;
	/* unsigned long user1, user2 */
};

struct gc_interface
{
/* public: */
	char *name;
	void (*init_page)(struct gc_interface *self, struct gc_pageinfo *page,
		void *arg);
	void (*destroy_page)(struct gc_interface *self, struct gc_pageinfo *page);
	void (*add_root)(struct gc_interface *self, char *start, char *end);
	void (*del_root)(struct gc_interface *self, char *ptr);
	boolean (*need_collect)(struct gc_interface *self);
	void (*collect)(struct gc_interface *self);
	void (*init_block)(struct gc_interface *self, struct gc_pageinfo *page,
		void *obj);
	void (*destroy_block)(struct gc_interface *self, struct gc_pageinfo *page,
		void *obj);

/* private: */
	struct gc_interface *next;
#ifdef MTSAFE
	struct Lockinfo lock;
#endif
};

extern void *gc_base_alloc(struct gc_interface *gc, size_t blocksize,
	void *arg);
extern void *gc_base_realloc(struct gc_interface *gc, void *obj,
	size_t nsize, void *arg);
extern void gc_base_free(struct gc_interface *gc, void *obj);
extern boolean gc_is_gc_page(void *addr);
extern const int *gc_base_blocksizes();
extern const int *gc_base_blockcounts();
extern void gc_abort(char const *fmt, ...);
extern void gc_dumpheap();

#define GETBIT(bmap, bit)	\
	((bmap)[(bit) / BITSPERLONG] & (1UL << ((bit) & (BITSPERLONG - 1))))
#define SETBIT(bmap, bit)	\
	((bmap)[(bit) / BITSPERLONG] |= (1UL << ((bit) & (BITSPERLONG - 1))))
#define CLEARBIT(bmap, bit)	\
	((bmap)[(bit) / BITSPERLONG] &= ~(1UL << ((bit) & (BITSPERLONG - 1))))
#define CLEARBITMAP(bmap, count)	\
	(memset((bmap), 0, (count / BITSPERLONG + 1) * sizeof (uLong)))

#ifdef macintosh
#include <LowMem.h>
#define DATASTART ((void*)LMGetApplZone())
#define DATAEND   ((void *)LMGetApplLimit())
#define STACKSTART ((void *)&stacktop)	/* variable declared on stack */
#define STACKEND   ((void *)LMGetCurStackBase())
#define GCBASEADDR ((void *)&edata)
#define PTRBUMP (sizeof (short))
#endif

#ifdef linux
extern int etext, edata, end;
#define DATASTART ((void*)((((unsigned long)(&etext)) + 0xfff) & ~0xfff))
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)	/* variable declared on stack */
#define STACKEND   ((void*)0xc0000000)
#define GCBASEADDR ((void *)&edata)
#define PTRBUMP (sizeof (char*))
#endif

#if defined bsdi || defined __FreeBSD__ || defined __sparc ||	\
	defined __OpenBSD__
extern int etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)&etext)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)	/* variable declared on stack */
#define STACKEND   ((void *)environ)
#define GCBASEADDR ((void *)&edata)
#define PTRBUMP (sizeof (char*))
#endif

#ifdef _AUX_SOURCE
extern int etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)(((long)&etext + 0x3FFFFF) & ~0x3FFFFF) \
		    + ((long)&etext & 0x1FFF))
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)
#define STACKEND   ((void *)environ)
#define GCBASEADDR ((void *)(((long)&edata + 0x1FFF) & ~0x1FFF))
#define PTRBUMP (sizeof (short))
#endif

#ifdef _DJGPP_SOURCE_
extern int end;
#define GCBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)
#define STACKEND   ((void *)0x80000000)
/*#define PTRMASK (~3U) */
#define PTRBUMP (sizeof (char*))
#endif

#ifndef GCBASEADDR
#define GCBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)0x402000)
#define STACKSTART ((void *)0x400000)
#define STACKEND   ((void *)0x402000)
#define PTRMASK (~3U)
#define PTRBUMP (sizeof (char*))
#endif

#endif /* __GCBASE_H_ */
