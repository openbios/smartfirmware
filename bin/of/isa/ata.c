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
#include "isa.h"


/* ATA controller methods */

C(f_ata_open)				/* open (-- okay?) */
{
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_ata_close)				/* close (--) */
{
	return NO_ERROR;
}

C(f_ata_dma_alloc)			/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ata_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ata_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ata_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}

C(f_ata_decode_unit)		/* decode-unit (str len -- phys.lo phys.hi) */
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

	if (*str == ',')
	{
		str++;
		slen--;
	}

	parse_number(16, &str, &slen, &lo, &err, FALSE);

	PUSH(e, lo);
	PUSH(e, hi);
	return NO_ERROR;
}

C(f_ata_encode_unit)		/* encode-unit (phys.lo phys.hi -- str len) */
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

C(f_ata_selftest)			/* selftest (-- 0|error-code) */
{
	PUSH(e, FFALSE);

	return NO_ERROR;
}

static const Initentry ata_methods[] =
{
	{ "open",				f_ata_open,					INVALID_FCODE },
	{ "close",				f_ata_close,				INVALID_FCODE },
	{ "dma-alloc",			f_ata_dma_alloc,			INVALID_FCODE },
	{ "dma-free",			f_ata_dma_free,				INVALID_FCODE },
	{ "decode-unit",  		f_ata_decode_unit,			INVALID_FCODE },
	{ "encode-unit",		f_ata_encode_unit,			INVALID_FCODE },
	{ "selftest",			f_ata_selftest,				INVALID_FCODE },
	{ NULL,					NULL },
};

ISA(ata_probe)
{
	/* TODO - just assume it is there for now */
	return NO_ERROR;
}

extern Retcode probe_ata_disks(Environ *e, uInt reg[4], Package *pkg,
	Retcode (*read)(uInt addr, void *value, int size),
	Retcode (*write)(uInt addr, Int value, int size));

ISA(ata_install)
{
	Retcode ret = new_isa_device(e, dev);
	Int reg[4] = { 0x1F0, 0x3F6, 0x170, 0x376 };

	if (ret == NO_ERROR)
		ret = probe_ata_disks(e, reg, dev->pkg, isa_io_read, isa_io_write);

	return ret;
}

Int ata_ctlr_reg[] =
{
	ISA_IO_ADDRESS, 0x3F6, 2,
	ISA_IO_ADDRESS, 0x170, 8,
	ISA_IO_ADDRESS, 0x376, 2
};

Isa_device ata_controller =
{
	"ide",
	"ide",
	ISA_IO_ADDRESS,
	0x1F0,
	8,
	{ 14, 0 },			/* IRQ */
	{ 3, 0, 8, 16, 0 },	/* DMA - TODO */
	0x0,				/* BIOS */
	3, ata_ctlr_reg, 	/* extra reg props */
	0,			/* clock freq, N/A here */
	ata_probe,
	ata_install,
	ata_methods
};
