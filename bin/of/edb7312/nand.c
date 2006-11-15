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

/* NAND flash/SmartMedia card driver */

/*#define DEBUG	*/

#include "defs.h"
#include "ep7312.h"

struct device
{
	uInt manufid;
	uInt devid;
	char *partnum;
	uInt blocksize;
	uInt numblocks;
};

typedef struct device Device;

Device nand_device_table[] =
{
	{ 0x98, 0xE6, "TC58V64ADC", 512, 16384, },
	{ 0, 0, 0, 0, 0 },
};

struct self
{
	uInt phys;
	uInt virt;
	Bool sm;		/* true if for SmartMedia card */
	uInt va_portb;
	uInt portb;
	uInt numblocks;
	Instance *disklabel;
	Instance *deblocker;
};

typedef struct self Self;

static uInt
port_in(uInt addr)
{
	return *(uByte volatile *)addr;
}

static void
port_out(uInt addr, uByte data)
{
	*(uByte volatile *)addr = data;
}

static uInt
nand_in(uInt addr)
{
	return *(uByte volatile *)addr;
}

static void
nand_out(uInt addr, uInt data)
{
	*(uByte volatile *)addr = data;
}

void
nand_ctl(Environ *e, Self *s, Bool cle, Bool sm, Bool ale)
{
	uInt portb = s->portb;

	if (cle)
		portb |= 0x10;
	else
		portb &= ~0x10;

	if (sm)
		portb &= ~0x80;		/* this is the reverse of the */
					/* documentation */
	else
		portb |= 0x80;

	if (ale)
		portb |= 0x20;
	else
		portb &= ~0x20;

	if (s->portb != portb)
	{
		port_out(s->va_portb, portb);
		s->portb = portb;
	}
}

void
nand_write(Environ *e, Self *s, uInt data, Bool cle, Bool sm, Bool ale)
{
	nand_ctl(e, s, cle, sm, ale);
	nand_out(s->virt, data);
}

uInt
nand_read(Environ *e, Self *s, Bool cle, Bool sm, Bool ale)
{
	nand_ctl(e, s, cle, sm, ale);
	return nand_in(s->virt);
}

void
init_self(Self *s)
{
	memset(s, '\0', sizeof (Self));
	s->sm = 0;
	s->virt = EDB7312_VA_NAND;
	s->phys = EDB7312_PA_NAND;
	s->va_portb = EDB7312_VA_EP7312	+ 1;
	s->portb = port_in(s->va_portb);
}

Bool
device_id(Environ *e, Self *s, uInt *manufid, uInt *devid)
{
	nand_write(e, s, 0x90, TRUE, s->sm, FALSE);
	nand_write(e, s, 0x00, FALSE, s->sm, TRUE);
	*manufid = nand_read(e, s, FALSE, s->sm, FALSE);
	*devid = nand_read(e, s, FALSE, s->sm, FALSE);
	DPRINTF(("Manufacture id %#x, device id %#x\n", *manufid, *devid));
	return TRUE;
}

C(f_nand_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Retcode ret;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = (Self *)malloc(sizeof (Self));
	
	if (s == NULL)
		return E_OUT_OF_MEMORY;

	inst->self = s;
	init_self(s);

	if (inst->args && inst->args[0])
	{
		/* get device number from init string */
		if (inst->args[1] == '1')
			s->sm = 1;
		else
			s->sm = 0;
	}
	else
		s->sm = 0;

	DPRINTF(("f_nand_open: Device might be present\n"));

	{
		uInt mid;
		uInt did;
		device_id(e, s, &mid, &did);
		cprintf(e, "Manufacture ID %#x, Device ID %#x\n", mid, did);
	}

	/* create an instance of /packages/deblocker to support read/write/etc */
	PUSH(e, "");	/* arg-str */
	PUSH(e, 0);		/* arg-len */
	PUSHP(e, "deblocker");			/* name-str */
	PUSH(e, strlen("deblocker"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("f_nand_open: $open-package deblocker ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->deblocker, Instance*);
	DPRINTF(("f_nand_open: deblocker=%p\n", s->deblocker));

	if (s->deblocker == NULL)
		return E_NULL_INSTANCE;

	/* create an instance of /packages/disk-label to support load/etc */
	PUSHP(e, inst->args + 1);		/* arg-str */
	PUSH(e, *(uByte*)inst->args);	/* arg-len */
	PUSHP(e, "disk-label");			/* name-str */
	PUSH(e, strlen("disk-label"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("f_nand_open: $open-package disk-label ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->disklabel, Instance*);
	DPRINTF(("f_nand_open: disklabel=%p\n", s->disklabel));

	if (s->disklabel == NULL)
		return E_NULL_INSTANCE;

	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_nand_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->deblocker);
	(void)execute_word(e, "close-package");
	PUSHP(e, s->disklabel);
	(void)execute_word(e, "close-package");

	/* free our self block */
	free(s);
	inst->self = NULL;
	return NO_ERROR;
}

C(f_nand_block_size)	/* block-size (-- blocksize) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, 512);
	return NO_ERROR;
}

C(f_nand_max_transfer)	/* max-transfer (-- maxblocks) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, 16*512);
	return NO_ERROR;
}

C(f_nand_ctl)		/* nand-ctl (sm cle ale --) */
{
	Cell ale;
	Cell cle;
	Cell sm;
	Self s;

	IFCKSP(e, 3, 0);
	POP(e, ale);
	POP(e, cle);
	POP(e, sm);
	init_self(&s);
	nand_ctl(e, &s, cle, sm, ale);
	return NO_ERROR;
}

C(f_nand_read)		/* nand-read (sm cle ale -- data) */
{
	Cell ale;
	Cell cle;
	Cell sm;
	Cell data;
	Self s;

	IFCKSP(e, 3, 1);
	POP(e, ale);
	POP(e, cle);
	POP(e, sm);
	init_self(&s);
	data = nand_read(e, &s, cle, sm, ale);
	PUSH(e, data);
	return NO_ERROR;
}

C(f_nand_write)		/* nand-write (sm cle ale data --) */
{
	Cell ale;
	Cell cle;
	Cell sm;
	Cell data;
	Self s;

	IFCKSP(e, 4, 0);
	POP(e, data);
	POP(e, ale);
	POP(e, cle);
	POP(e, sm);
	init_self(&s);
	nand_write(e, &s, data, cle, sm, ale);
	return NO_ERROR;
}

Retcode
nand_read_block(Environ *e, Self *s, Byte *addr, Int blocknum)
{
	/* read the block */
	return NO_ERROR;
}

Retcode
nand_write_block(Environ *e, Self *s, Byte *addr, Int blocknum)
{
	/* write the block */
	return NO_ERROR;
}

C(f_nand_read_blocks)			/* read-blocks (addr block# #blocks -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int blocknum;
	Int numblocks;
	Byte *addr;
	Self *s;
	uInt base;
	Retcode ret = NO_ERROR;
	int i;

	DPRINTF(("enter nand_read\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	base = s->virt;
	
	IFCKSP(e, 3, 1);
	POP(e, numblocks);
	POP(e, blocknum);
	POPT(e, addr, Byte*);
	DPRINTF(("nand_read: &e=%p e=%p addr=%p blocknum=%d numblocks=%d\n", &e, e, addr, blocknum, numblocks));

	for (i = 0; i < numblocks; i++)
	{
		/* read block */
		ret = nand_read_block(e, s, addr, blocknum);

		if (ret != NO_ERROR)
			break;

		addr += 512;
		blocknum++;
	}

	if (i == 0)
		return ret;

	PUSH(e, i);
	return NO_ERROR;
}

C(f_nand_write_blocks)			/* write-blocks (addr block# #blocks -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int blocknum;
	Int numblocks;
	Byte *addr;
	Self *s;
	uInt base;
	Retcode ret = NO_ERROR;
	int i;

	DPRINTF(("enter nand_write\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	base = s->virt;
	
	IFCKSP(e, 3, 1);
	POP(e, numblocks);
	POP(e, blocknum);
	POPT(e, addr, Byte*);
	DPRINTF(("nand_write: &e=%p e=%p addr=%p blocknum=%d numblocks=%d\n", &e, e, addr, blocknum, numblocks));

	for (i = 0; i < numblocks; i++)
	{
		/* write block */
		ret = nand_write_block(e, s, addr, blocknum);

		if (ret != NO_ERROR)
			break;

		addr += 512;
		blocknum++;
	}

	if (i == 0)
		return ret;

	PUSH(e, i);
	return NO_ERROR;
}

C(f_nand_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "nand flash selftest\n");

	PUSH(e, 0);
	return NO_ERROR;
}

static const Initentry nand_methods[] =
{
	{ "open",          f_nand_open,          INVALID_FCODE },
	{ "close",         f_nand_close,         INVALID_FCODE },
	{ "nand-ctl",      f_nand_ctl,           INVALID_FCODE },
	{ "nand-read",     f_nand_read,          INVALID_FCODE },
	{ "nand-write",    f_nand_write,         INVALID_FCODE },
	{ "selftest",      f_nand_selftest,      INVALID_FCODE },
	{ "block-size",    f_nand_block_size,    INVALID_FCODE },
	{ "max-transfer",  f_nand_max_transfer,  INVALID_FCODE },
	{ "read-blocks",   f_nand_read_blocks,   INVALID_FCODE },
	{ "write-blocks",  f_nand_write_blocks,  INVALID_FCODE },
	{ NULL,            NULL },
};

CC(install_nand)
{
	Retcode ret;
	Package *pkg;
	Byte *prop;
	Int plen = 0;
	Self s;

	pkg = new_pkg_name(e->currpkg, "flash");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	prop_set_str(pkg->props, "device_type", CSTR, "block", CSTR);
	DPRINTF(("nand_install: pkg=%p self=%p\n", pkg, pkg->self));

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, EDB7312_PA_NAND);
	prop_encode_int(prop + plen, &plen, 0x1000);
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, nand_methods);

	init_self(&s);
	DPRINTF(("PORT B = %#x\n", s.portb));

	{
		uInt mid;
		uInt did;
		device_id(e, &s, &mid, &did);
		DPRINTF(("Manufacture ID %#x, Device ID %#x\n", mid, did));
	}

	return ret;
}

void
compute_ecc(uByte *data, uByte *ecc)
{
	int i;
	int j;
	int b;
	uInt check = 0;

	for (i = 0; i < 256; i++)
		for (b = 0; b < 8; b++)
		{
			int a = (i << 3) | b;

			for (j = 0; j < 11; j++)
			{
				if (a & (1 << j))
				{
					if (data[i] & (1 << b))
						check ^= 1 << (j * 2);
				}
				else if (data[i] & (1 << b))
					check ^= 1 << (j * 2 + 1);
			}
		}

	ecc[0] = (check >> 6) & 0xFF;
	ecc[1] = (check >> 14) & 0xFF;
	ecc[2] = (check << 2) & 0xFC;
}
