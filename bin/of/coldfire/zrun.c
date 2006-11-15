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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Load a gzipped executable image and run it.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include "zlib.h"

#if defined(ROM) && !defined(BSS)
#error BSS must also be defined if ROM is defined.
#endif

/* default entry point from gcc - located at the beginning of .text */
void
entry(void)
{
	extern void real_main(void);

	/* initialize stack pointer */
	asm("	move.l	#0xFFFF0,%sp");

	real_main();

	/* kick back to the debugger */
	asm("	move.l	#0,%d0");
	asm("	trap	#15");
}


/* debugging hook to call the builtin dBUG monitor on m5206 eval board
 */
void
debug_write(int ch)
{
	asm("	move.l	8(%a6),%d1");
	asm("	move.l	#0x13,%d0");
	asm("	trap	#15");
}

void
dprintf(const char *fmt, ...)
{
	va_list args;
	Byte *s;
	Byte buf[1024];
	extern int vbprintf(char *, const char *, va_list);

	va_start(args, fmt);
	vbprintf(buf, fmt, args);
	va_end(args);

	/* convert all newlines to carriage-return-newline pairs */
	s = buf;

	while (*s)
	{
		if (*s == '\n')
			debug_write('\r');
		
		debug_write(*s++);
	}
}

#include "zarr.h"

extern void meminfo(size_t *allocspc, size_t *freespc, size_t *allocblks,
	size_t *freeblks, void **endarena, size_t *arenasz);

void
real_main(void)
{
	extern void init_malloc(void *memory, size_t max);
	int ret;
#ifdef ROM
	Byte *dest = (Byte*)0x10000 - 0xD0;
	uLong destlen = gz_orig_length;
	const Byte *source = gz_image;
	uLong sourcelen = gz_length;
	int (*func)(void) = (int (*)(void))(Byte*)0x10000;
	Byte *mem = (Byte*)(((uLong)(dest + destlen) + 16) & ~7);

	memset((Byte*)BSS, 0, 0xFF000 - BSS);		/* clear BSS */
	init_malloc(mem, BSS - (uLong)mem);
#else
	Byte *dest = (Byte*)BSS - gz_orig_length;
	uLong destlen = gz_orig_length;
	const Byte *source = gz_image;
	uLong sourcelen = gz_length;
	int (*func)(void) = (int (*)(void))0x40000;
	extern char end[];

	init_malloc(end, (uLong)dest - (uLong)end);
	dprintf("init_malloc(%p, %p)\n", mem, BSS - (uLong)mem);
#endif

	//dprintf("dest=%#x destlen=%#x source=%#x sourcelen=%#x"
	//		end=%#x\n func=%#x",
	//		dest, destlen, source, sourcelen, end, func);
	dprintf("uncompressing %d bytes into...", sourcelen);
	ret = uncompress(dest, &destlen, source, sourcelen);

	switch (ret)
	{
	case Z_OK:
		break;

	case Z_MEM_ERROR:
		dprintf("not enough memory\n");
		return;

	case Z_BUF_ERROR:
		dprintf("output buffer too small\n");
		return;

	case Z_DATA_ERROR:
		dprintf("input appears to be corrupt or not gzipped\n");
		return;

	default:
		dprintf("unknown error %d\n", ret);
		return;
	}

	/* check COFF magic numbers */
	if (dest[0] != 0x01 || dest[1] != 0x50)
	{
		dprintf("bad magic numbers: %#x,%#x\n", dest[0], dest[1]);
		return;
	}

#ifdef ROM
	memset((char*)func + destlen, 0, BSS - (uLong)func - destlen);
#else
	/* committed to launching now - this steps on malloc memory */
	destlen -= 0x2000;		/* skip COFF header */
	dest += 0x2000;
	memcpy((char*)func, dest, destlen);
	memset((char*)func + destlen, 0, 0xF0000 - (uLong)func - destlen);
#endif

	dprintf("%d bytes\n", destlen);
	(*func)();
}

void
error(int e)
{
	dprintf("fatal error: %d\n", e);

	/* kick back to the debugger */
	asm("	move.l	#0,%d0");
	asm("	trap	#15");
}

asm("	nop");
