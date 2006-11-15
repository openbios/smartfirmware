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


typedef unsigned int uInt;

#define MODE(AP, DOM, C, B) \
	((AP << 10) | (DOM << 5) | (C << 3) | (B << 2) | 0x12)

#define CACHE_MODE	MODE(3, 0, 1, 1)
#define NONCACHE_MODE	MODE(3, 0, 0, 0)

void
initmap(uInt *map)
{
	uInt a = 0;

	for (; a < 0xC0000000U; a += 0x100000)
		*map++ = a | NONCACHE_MODE;

	for (; a < 0xC1000000U; a += 0x100000)
		*map++ = a | CACHE_MODE;

	for (; a < 0xF0000000U; a += 0x100000)
		*map++ = a | NONCACHE_MODE;

	for (; a < 0xF1000000U; a += 0x100000)
		*map++ = ((a & ~0x30000000U) + 0x100000) | CACHE_MODE;

	*map++ = 0xC0000000U | CACHE_MODE;
	a += 0x100000;

	for (; a < 0xF7000000U; a += 0x100000)
		*map++ = a | NONCACHE_MODE;

	*map++ = 0xC0000000U | CACHE_MODE;
	a += 0x100000;
	*map++ = 0x00000000U | MODE(3, 0, 1, 0);
	a += 0x100000;

	for (; a <= 0xFFE00000U; a += 0x100000)
		*map++ = a | NONCACHE_MODE;

	*map++ = a | NONCACHE_MODE;
}

#ifdef MAIN
uInt g_map[4096];

int
main()
{
	int i;

	initmap(g_map);

	for (i = 0; i < 4096; i++)
		printf("\t.word\t0x%08X\t\t/* 0x%08X */\n", g_map[i], i * 0x100000);

	exit(0);
}
#endif
