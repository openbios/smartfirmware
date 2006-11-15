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

/* (c) Copyright 1999-2001 by CodeGen, Inc.  All Rights Reserved. */

/* builtin IDE device driver */

#include "defs.h"

extern uInt g_ep7312_base;

#define EP7312_R(addr)		(*(volatile uInt *)(g_ep7312_base + ((addr) & 0x3FFF)))
#define EP7312_W(addr,val)	(*(volatile uInt *)(g_ep7312_base + ((addr) & 0x3FFF)) = (val))

#define SYSCON1	0x0100
#define MEMCFG2	0x01C0


/* ATA controller methods */

C(f_ide_open)				/* open (-- okay?) */
{
	uInt syscon1;

	/* enable EXCKEN for IDE */
	syscon1 = EP7312_R(SYSCON1) | 0x40000;
	EP7312_W(SYSCON1, syscon1);
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_ide_close)				/* close (--) */
{
	uInt syscon1;

	/* disable EXCKEN for IDE */
	syscon1 = EP7312_R(SYSCON1) & ~0x40000;
	EP7312_W(SYSCON1, syscon1);
	return NO_ERROR;
}

C(f_ide_dma_alloc)			/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ata_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ide_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ata_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}

C(f_ide_decode_unit)		/* decode-unit (str len -- phys.lo phys.hi) */
{
	Byte *str;
	Int slen;
	Cell lo = 0;
	Cell hi = 0;
	Cell err;

	IFCKSP(e, 2, 2);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);

	/* format is: ID,LUN */
	parse_number(16, &str, &slen, &hi, &err, FALSE);

	if (err == FFALSE)
		parse_number(16, &str, &slen, &lo, &err, FALSE);

	PUSH(e, lo);
	PUSH(e, hi);
	return NO_ERROR;
}

C(f_ide_encode_unit)		/* encode-unit (phys.lo phys.hi -- str len) */
{
	static Byte buf[64];
	Cell hi;
	Cell lo;
	Byte *s = buf;

	IFCKSP(e, 2, 2);
	POP(e, hi);
	POP(e, lo);
	*s = '\0';

	bprintf(s, "%x,%x", (unsigned int)hi, (unsigned int)lo);

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}

C(f_ide_selftest)			/* selftest (-- 0|error-code) */
{
	PUSH(e, FFALSE);
	return NO_ERROR;
}

static const Initentry ide_methods[] =
{
	{ "open",				f_ide_open,					INVALID_FCODE },
	{ "close",				f_ide_close,				INVALID_FCODE },
	{ "dma-alloc",			f_ide_dma_alloc,			INVALID_FCODE },
	{ "dma-free",			f_ide_dma_free,				INVALID_FCODE },
	{ "decode-unit",  		f_ide_decode_unit,			INVALID_FCODE },
	{ "encode-unit",		f_ide_encode_unit,			INVALID_FCODE },
	{ "selftest",			f_ide_selftest,				INVALID_FCODE },
	{ NULL,					NULL },
};

static unsigned short
inh(unsigned int addr)
{
	unsigned int t;
	__asm("	ldrh	%0, [%1, #0]" : "=r"(t) : "r"(addr));
	return t;
}

static void
outh(unsigned int addr, unsigned short val)
{
	__asm("	strh	%0, [%1, #0]" : : "r"(val), "r"(addr));
}

static void
ide_mode(int width)
{
	uInt memcfg2 = EP7312_R(MEMCFG2) & 0xFFFFFCFF;

	if (width == 8)
		memcfg2 |= 0x0300;
	else if (width == 16)
		memcfg2 |= 0x0000;

	EP7312_W(MEMCFG2, memcfg2);
}

static Retcode
ide_read(uInt addr, void *value, int size)
{
/*	DPRINTF(("ide_read: addr %#x, value %#x, size %d\n", addr, value, size)); */

	if (addr < EDB7312_VA_IDE)
		return NO_ERROR;

	if (size == 1)
	{
		ide_mode(8);
		*(uByte *)value = *(uByte *)addr;
	}
	else if (size == 2)
	{
		ide_mode(16);
		*(uShort *)value = inh(addr);
	}

	return NO_ERROR;
}

static Retcode
ide_write(uInt addr, Int value, int size)
{
/*	DPRINTF(("ide_write: addr %#x, value %#x, size %d\n", addr, value, size)); */

	if (addr < EDB7312_VA_IDE)
		return NO_ERROR;

	if (size == 1)
	{
		ide_mode(8);
		*(uByte *)addr = value;
	}
	else if (size == 2)
	{
		ide_mode(16);
		outh(addr, value);
	}

	return NO_ERROR;
}

extern Retcode probe_ata_disks(Environ *e, uInt reg[4], Package *pkg,
	Retcode (*read)(uInt addr, void *value, int size),
	Retcode (*write)(uInt addr, Int value, int size));

C(ide_install)
{
	Int reg[4] = { EDB7312_VA_IDE, EDB7312_VA_IDE+0xE, 0, 0 };
	Retcode ret;
	Package *pkg;
	Byte *prop;
	Int plen = 0;
	EC(f_reg);

	IFCKSP(e, 0, 3);
	pkg = new_pkg_name(e->root, "ide");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	/* set the type of this device */
	prop_set_str(pkg->props, "device_type", CSTR, "ide", CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, EDB7312_PA_IDE);
	prop_encode_int(prop + plen, &plen, 8);

	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, ide_methods);

	if (ret == NO_ERROR)
	{
		if (f_ide_open(e) == NO_ERROR)
			DROP(e);

		ret = probe_ata_disks(e, reg, pkg, ide_read, ide_write);
		f_ide_close(e);
	}

	return ret;
}

#if 0
ide_write: addr 0x1F6, value 0xA0, size 1
ide_write: addr 0x3F6, value 0x4, size 1
ide_write: addr 0x3F6, value 0xA, size 1
ide_write: addr 0x1F6, value 0xA0, size 1
ide_read: addr 0x1F7, value 0xF70F9BFF, size 1
...
ide_write: addr 0x3F6, value 0xA, size 1
ide_write: addr 0x1F6, value 0xB0, size 1
ide_read: addr 0x1F7, value 0xF70F9BFF, size 1
...
ide_write: addr 0x176, value 0xA0, size 1
ide_write: addr 0x376, value 0x4, size 1
ide_write: addr 0x376, value 0xA, size 1
ide_write: addr 0x176, value 0xA0, size 1
ide_read: addr 0x177, value 0xF70F9BFF, size 1
...
ide_write: addr 0x376, value 0xA, size 1
ide_write: addr 0x176, value 0xB0, size 1
ide_read: addr 0x177, value 0xF70F9BFF, size 1
...
#endif
