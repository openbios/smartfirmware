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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* Software TLB used to map 64<->32-bit addresses for Fcode32 compatibility
   on systems with 64-bit pointers.
 */

#ifdef SF_64BIT

/* mapping available only if compiler supports them */
#ifndef __LONGLONG
#  error __LONGLONG must be defined and type Long must be a 64-bit integer.
#endif


#include "defs.h"


#define BLOCK_SIZE	4096
#define BLOCK_MASK	(~(BLOCK_SIZE - 1))


struct map
{
	uInt a32;
	uLong a64;
};
typedef struct map Map;


static Map *g_map = NULL;		/* array of Maps - sorted by a32 */
static uInt *g_map64 = NULL;	/* array of indices into g_map */
static uInt g_size = 0;			/* total number of elements in above */
static uInt g_num = 0;			/* currently used number of elements */
static uInt g_alloc32 = BLOCK_SIZE;		/* next allocation address */


static Retcode
growbufs(uInt reqsize)
{
	uInt size;
	Map *map;
	uInt *idx;

	if (reqsize < g_num || reqsize < g_size)
		return NO_ERROR;

	size = reqsize + 64;
	map = (Map*)realloc(g_map, sizeof *g_map * size);

	if (map == NULL)
		return E_OUT_OF_MEMORY;

	g_map = map;
	idx = (uInt*)realloc((char*)g_map64, sizeof *g_map64 * size);

	if (idx == NULL)
		return E_OUT_OF_MEMORY;

	g_map64 = idx;
	g_size = size;
	return NO_ERROR;
}

uInt
lookup32(uInt a32)
{
	uInt l = 0;
	uInt r = g_num - 1;
	uInt i;

	if (g_num == 0)
		return 0;

	/* simple yet dumb binary search - could be faster and less readable */
	for (i = (l + r) / 2; l < i && l < r; i = (l + r) / 2)
		if (a32 < g_map[i].a32)
			r = i;
		else
			l = i;

	return (a32 >= g_map[i].a32 + BLOCK_SIZE) ? i + 1 : i;
}

uInt
lookup64(uLong a64)
{
	uInt l = 0;
	uInt r = g_num - 1;
	uInt i;

	if (g_num == 0)
		return 0;

	/* simple yet dumb binary search - could be faster and less readable */
	for (i = (l + r) / 2; l < i && l < r; i = (l + r) / 2)
	{
		if (a64 < g_map[g_map64[i]].a64)
			r = i;
		else
			l = i;
	}

	return (a64 >= g_map[g_map64[i]].a64 + BLOCK_SIZE) ? i + 1 : i;
}

uLong
map32to64(uInt a32)
{
	Map *map;
	uInt idx;

	if (g_map == NULL)
		growbufs(64);

	idx = lookup32(a32);

	if (idx >= g_num)
		return 0;

	map = &g_map[idx];

	if (a32 < map->a32 || a32 >= map->a32 + BLOCK_SIZE)
		return 0;

	return map->a64 + (a32 - map->a32);
}

uInt
map64to32(uLong a64)
{
	uInt idx;
	Map *map;

	if (g_map == NULL)
		growbufs(64);

	idx = lookup64(a64);
	map = &g_map[g_map64[idx]];

	if (idx >= g_num || a64 < map->a64 || a64 >= map->a64 + BLOCK_SIZE)
	{
		/* create a mapping, allocating a 32-bit address block */
		uInt loc = g_num;
		uLong block = a64 & BLOCK_MASK;
		Retcode ret = growbufs(g_num + 1);

		if (ret != NO_ERROR)
		{
			cprintf(g_e, "map64to32: %s\n", err2str(ret));
			return 0;
		}

		/* allocate the new map the end, which sorts it by 32-bit addr */
		g_map[loc].a32 = g_alloc32;
		g_map[loc].a64 = block;
		g_alloc32 += BLOCK_SIZE;
		g_num++;

		/* move the elements to make room for the new one */
		if (idx < loc && a64 >= map->a64 + BLOCK_SIZE)
			idx++;

		memmove(g_map64 + idx, g_map64 + idx + 1, g_num - idx);
		g_map64[idx] = loc;
		map = &g_map[loc];
	}

	return map->a32 + (uInt)(a64 - map->a64);
}

C(f_32to64)
{
	uInt a32;

	IFCKSP(e, 1, 1);
	POPTYPE(e, a32, uInt);
	PUSH(e, map32to64(a32));
	return NO_ERROR;
}

C(f_64to32)
{
	uLong a64;

	IFCKSP(e, 1, 1);
	POP(e, a64);
	PUSH(e, map64to32(a64));
	return NO_ERROR;
}

const Initentry init_stlb[] =
{
	{ "32->64", f_32to64, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(u32 -- u64) get 64-bit ptr for virtual 32-bit addr") },
	{ "64->32", f_64to32, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(u64 -- u32) map 64-bit ptr to a virtual 32-bit addr") },

	{ NULL, NULL }
};
#endif /* SF_64BIT */
