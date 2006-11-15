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

/* Copyright 1994 by Parag Patel.  All Rights Reserved. */
/* $Header: /u/cgt/cvs/lib/aoa/gcext.h,v 1.4 1999/02/23 18:21:17 parag Exp $ */

#ifndef __GCEXT_H_
#define __GCEXT_H_

struct gc_interface;

struct gc_type_info
{
	char *type_name;	/* for runtime debugging */
	enum
	{
		NOT_ATOMIC,		/* "offsets" below lists all pointers in object */
		ATOMIC,			/* no pointers in object */
		UNKNOWN_ATOMIC	/* anything could be a pointer - conservative check */
	} atomic;
	enum
	{
		NO_FILL,
		ZERO_FILL,
		ILLEGAL_FILL
	} fill;
	void (*destroy)(void *obj);
	int num_offsets;
	ptrdiff_t *offsets;	/* array of offsets to ptrs in obj */
	void *userdef;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct gc_interface *gc_get_curr_interface(void);
extern void gc_set_curr_interface(struct gc_interface *g);
extern struct gc_interface *gc_find_interface(const char *str);
extern void *gc_alloc(struct gc_type_info *arg, size_t size);
extern void gc_free(void *obj);
extern void gc_del_root(char *ptr);
extern void gc_add_root(char *start, char *end);
extern void gc_collect(void);
extern void gc_collect_all(void);
extern struct gc_type_info *gc_object_type_info(void *obj);

#ifdef __cplusplus
}
#endif

#endif /* __GCEXT_H_ */
