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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

/*#define DEBUG*/
#define DEBUG

#include "defs.h"

struct self
{
	Environ *e;
	Instance *us;
	uInt phys;
	uInt size;
	uPtr virt;
	uInt seekpos;
	volatile uByte *va;
};

typedef struct self Self;

C(f_sram_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Retcode ret;
	Self *s;
	Entry *ent;
	Byte *paddr;
	Int plen;
	Cell c;

	DPRINTF(("f_sram_open: pkg=%p\n", pkg));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	/* allocate and clear a self object */
	s = (Self *)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->us = inst;
	s->e = e;

	/* decode the reg property to find the address and size
	 * of the CSR registers.
	 */
	ent = find_table(pkg->props, "reg", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("f_sram_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	ret = prop_decode_int(&paddr, &plen, &s->phys);

	if (ret == NO_ERROR)
		ret = prop_decode_int(&paddr, &plen, &s->size);
	
	DPRINTF(("f_sram_open: paddr=%p plen=%d\n", paddr, plen));
	DPRINTF(("f_sram_open: phys=%#x size=%#x\n", s->phys, s->size));

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	PUSH(e, s->phys);
	PUSH(e, s->size);
	ret = execute_method_name(e, inst->parent, "map-in", CSTR);

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POP(e, c);

	/* point to CSRs for various byte/word/lword accesses */
	s->virt = c;
	s->va = (volatile uByte *)(uPtr)c;
	DPRINTF(("f_sram_open: va=%p\n", s->va));

	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_sram_close)			/* close (--) */
{
	Instance *inst = (Instance *)(uPtr)e->currinst;
	Retcode ret;
	Self *s = (Self *)inst->self;

	IFCKSP(e, 0, 2);
	PUSH(e, s->virt);
	PUSH(e, s->size);
	ret = execute_method_name(e, inst->parent, "map-out", CSTR);

	free(s);
	return ret;
}

static int
read_sram(Environ *e, Self *s, int offset, uByte *data, int len)
{
	int actual = 0;
	uByte volatile *sram = &s->va[offset];

	if (offset < 0 || offset >= s->size)
		return 0;

	DPRINTF(("read_sram: offset %#x, data %#x, len %d\n", offset, data, len));

	while (len > 0 && offset < s->size)
	{
		*data++ = *sram++;
		len--;
		actual++;
		offset++;
	}

	DPRINTF(("read_sram: actual = %d\n", actual));
	return actual;
}

static int
write_sram(Environ *e, Self *s, int offset, uByte *data, int len)
{
	int actual = 0;
	uByte volatile *sram = &s->va[offset];

	if (offset < 0 || offset >= s->size)
		return 0;

	DPRINTF(("write_sram: offset %#x, data %#x, len %d\n", offset, data, len));

	while (len > 0 && offset < s->size)
	{
		*sram++ = *data++;
		len--;
		actual++;
		offset++;
	}

	DPRINTF(("write_sram: actual = %d\n", actual));
	return actual;
}

C(f_sram_read_bytes)			/* read-bytes (addr len off -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Int actual = 0;
	Byte *addr;

	DPRINTF(("sram read-bytes\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	IFCKSP(e, 3, 1);
	POP(e, off);
	POP(e, len);
	POPT(e, addr, Byte*);

	DPRINTF(("sram read-bytes: off %#x, addr %#x, len %d\n", off, addr, len));
	actual = read_sram(e, inst->self, off, addr, len);
	DPRINTF(("sram read-bytes: actual = %d\n", actual));

	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
}

C(f_sram_write_bytes)			/* write-bytes (addr len off -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Byte *addr;
	Self *s;
	DPRINTF(("sram write-bytes\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 3, 1);
	POP(e, off);
	POP(e, len);
	POPT(e, addr, Byte*);

	if (!write_sram(e, s, off, addr, len))
	    len = -2;

	PUSH(e, len);
	return NO_ERROR;
}

C(f_sram_seek)			/* seek (pos.hi pos.lo -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt poshi, poslo;

	DPRINTF(("sram seek\n"));
	IFCKSP(e, 2, 1);

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	s = inst->self;
	POP(e, poshi);
	POP(e, poslo);

	DPRINTF(("sram seek: hi=%u lo=%u\n", poshi, poslo));

	if (poshi != 0U || poslo > s->size)
	{
		PUSH(e, -1);		/* seek failure */
		return NO_ERROR;
	}

	s->seekpos = poslo;

	PUSH(e, 0);			/* 0 or 1 are seek success */
	return NO_ERROR;
}

C(f_sram_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Int actual = 0;
	Byte *addr;
	Self *s;
	DPRINTF(("sram read\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	off = s->seekpos;
	DPRINTF(("sram read off = %d (%d), buf = %#X\n", off, off - 64*1024, addr));

	actual = read_sram(e, s, off, addr, len);

	s->seekpos += actual;
	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
	return NO_ERROR;
}

C(f_sram_write)			/* write (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int off;
	Int len;
	Byte *addr;
	Self *s;
	DPRINTF(("sram write\n"));

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	off = s->seekpos;

	if (!write_sram(e, s, off, addr, len))
	    len = -2;

	if (len > 0)
		s->seekpos = off + len;

	PUSH(e, len);
	return NO_ERROR;
}

static const Initentry sram_methods[] =
{
	{ "open",          f_sram_open,            INVALID_FCODE },
	{ "close",         f_sram_close,           INVALID_FCODE },
	{ "seek",          f_sram_seek,            INVALID_FCODE },
	{ "read",          f_sram_read,            INVALID_FCODE },
	{ "read-bytes",    f_sram_read_bytes,      INVALID_FCODE },
	{ NULL,            NULL },
};

static const Initentry sram_rw_methods[] =
{
	{ "write",         f_sram_write,           INVALID_FCODE },
	{ "write-bytes",   f_sram_write_bytes,     INVALID_FCODE },
	{ NULL,            NULL },
};

static Retcode
sram_mkdev(Environ *e, Package *ppkg, uInt addr, uInt size, char *name,
		char *devtype, Bool rw)
{
	Package *pkg = new_pkg_name(ppkg, name);
	Byte *prop;
	Int plen = 0;
	Retcode ret;

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	prop_set_str(pkg->props, "device_type", CSTR, devtype, CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, addr);
	prop_encode_int(prop + plen, &plen, size);
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret != NO_ERROR)
		return ret;

	ret = init_entries(e, pkg->dict, sram_methods);

	if (ret == NO_ERROR && rw)
		ret = init_entries(e, pkg->dict, sram_rw_methods);

	return ret;
}

#define EDB7312_PA_BOOTROM 0x70000000
#define EBD7312_SZ_BOOTROM 0x80

CC(sram_install)
{
	Retcode ret;
	Retcode ret2;

	ret = sram_mkdev(e, e->currpkg, EDB7312_PA_SRAM, EDB7312_SZ_SRAM,
			"sram", "memory", TRUE);
	ret2 = sram_mkdev(e, e->currpkg, EDB7312_PA_BOOTROM, EBD7312_SZ_BOOTROM,
			"boot-rom", "memory", FALSE);

	return ret == NO_ERROR ? ret2 : ret;
}
