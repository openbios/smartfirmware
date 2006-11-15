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

/* (c) Copyright 1996 by CodeGen, Inc.  All Rights Reserved. */

#include "defs.h"
#include "pci.h"
#include "fakepci.h"

#ifdef FAKE_PCI

#define BAR_CSRIO		0		/* CSR in I/O space BAR */
#define BAR_CSRMEM		1		/* CSR in memory space BAR */
#define BAR_ER			7		/* expansion rom BAR */

static Int dc21140_config_default[] =
{
	0x00091011,		/* devid, vendid */
	0x02800000,		/* status, command */
	0x02000011,		/* class code, revid */
	0x00000000,		/* BIST, header type, latency timer, cache line size */
	0x00000001,		/* BAR #0 */
	0x00000000,		/* BAR #1 */
	0x00000000,		/* BAR #2 */
	0x00000000,		/* BAR #3 */
	0x00000000,		/* BAR #4 */
	0x00000000,		/* BAR #5 */
	0x00000000,		/* CIS pointer */
	0x00000000,		/* subsys devid, subsys vendid */
	0x00000000,		/* expansion rom */
	0x00000000,		/* reserved */
	0x00000000,		/* reserved */
	0x00000100,		/* max lat, min gnt, intr pin, intr line */
};

static Int dc21140_config_write_mask[] =
{
	0x00000000,		/* devid, vendid */
	0x00000147,		/* status, command */
	0x00000000,		/* class code, revid */
	0x0000FF00,		/* BIST, header type, latency timer, cache line size */
	0xFFFFFF80,		/* BAR #0 */
	0xFFFFFF80,		/* BAR #1 */
	0x00000000,		/* BAR #2 */
	0x00000000,		/* BAR #3 */
	0x00000000,		/* BAR #4 */
	0x00000000,		/* BAR #5 */
	0x00000000,		/* CIS pointer */
	0x00000000,		/* subsys devid, subsys vendid */
	0x00000000,		/* expansion rom */
	0x00000000,		/* reserved */
	0x00000000,		/* reserved */
	0x000000FF,		/* max lat, min gnt, intr pin, intr line */
};

static uByte dc21140_expansion_rom[] =
{
	0x55, 0xAA, 0x40, 0x00,		/* Expansion ROM header */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,

	 'P',  'C',  'I',  'R',		/* PCI data structure */
	0x11, 0x10, 0x09, 0x00,
	0x00, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x02,
	0x01, 0x00, 0x00, 0x01,
	0x01, 0x80, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00,		/* FCode */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

struct dc21140_data
{
	Int config[16];
	Int csr[32];
	Bool csrio;
	Bool csrmem;
	Bool er;
};

typedef struct dc21140_data DC21140_data;

static Bool
dc21140_init(Fake_pci_dev *self)
{
	DC21140_data *d = (DC21140_data *)malloc(sizeof (DC21140_data));

	if (d == NULL)
		return FALSE;

	self->private = d;
	self->configsize = 64;
	memcpy(d->config, dc21140_config_default, 16 * sizeof (Int));
	memset(d->csr, '\0', 16 * sizeof (Int));
	d->csrio = 0;
	d->csrmem = 0;
	d->er = 0;
	return TRUE;
}

static Int
dc21140_configread(Fake_pci_dev *self, int reg)
{
	DC21140_data *d = (DC21140_data *)self->private;
	return d->config[reg / sizeof (Int)];
}

static void
dc21140_enable_rom(Fake_pci_dev *self, Bool enable, Int old, Int new)
{
	DC21140_data *d = (DC21140_data *)self->private;

	if (d->er)
		fakepci_mem_unmap(old & ~1, 256*1024, self, BAR_ER, 0);

	if (enable)
		fakepci_mem_map(new & ~1, 256*1024, self, BAR_ER, 0);

	d->er = enable;
}

static void
dc21140_enable_io_csr(Fake_pci_dev *self, Bool enable, Int old, Int new)
{
	DC21140_data *d = (DC21140_data *)self->private;

	if (d->csrio)
		fakepci_io_unmap(old & ~1, 128, self, BAR_CSRIO, 0);

	if (enable)
		fakepci_io_map(new & ~1, 128, self, BAR_CSRIO, 0);

	d->csrio = enable;
}

static void
dc21140_enable_mem_csr(Fake_pci_dev *self, Bool enable, Int old, Int new)
{
	DC21140_data *d = (DC21140_data *)self->private;

	if (d->csrmem)
		fakepci_mem_unmap(old & ~1, 128, self, BAR_CSRMEM, 0);

	if (enable)
		fakepci_mem_map(new & ~1, 128, self, BAR_CSRMEM, 0);

	d->csrmem = enable;
}

static void
dc21140_configwrite(Fake_pci_dev *self, int reg, Int val)
{
	DC21140_data *d = (DC21140_data *)self->private;
	int r = reg / sizeof (Int);
	Int mask = dc21140_config_write_mask[r];
	Int oval = d->config[r];
	val = (oval & ~mask) | (val & mask);
	d->config[r] = val;

	switch (reg)
	{
	case 0x04:		/* CFCS */
		dc21140_enable_rom(self,
				(val & 2) && (d->config[0x30 / sizeof (Int)] & 1),
				d->config[0x30 / sizeof (Int)], d->config[0x30 / sizeof (Int)]);
		dc21140_enable_io_csr(self, val & 1, d->config[0x10 / sizeof (Int)],
				d->config[0x10 / sizeof (Int)]);
		dc21140_enable_mem_csr(self, val & 2, d->config[0x14 / sizeof (Int)],
				d->config[0x14 / sizeof (Int)]);
		break;
	case 0x10:		/* BAR #0 */
		dc21140_enable_io_csr(self, d->config[0x04 / sizeof (Int)] & 1, oval,
				val);
		break;
	case 0x14:		/* BAR #1 */
		dc21140_enable_mem_csr(self, d->config[0x04 / sizeof (Int)] & 2, oval,
				val);
		break;
	case 0x30:		/* CBER */
		dc21140_enable_rom(self,
				(d->config[0x04 / sizeof (Int)] & 2) && (val & 1), oval, val);
		break;
	}
}

static int
dc21140_barsize(Fake_pci_dev *self, int bar)
{
	switch (bar)
	{
	case BAR_CSRIO:
	case BAR_CSRMEM:
		return 128;
	case BAR_ER:
		return 256*1024;
	}

	return 0;
}

static int
dc21140_bartype(Fake_pci_dev *self, int bar)
{
	switch (bar)
	{
	case BAR_CSRIO:
		return 2;
	case BAR_CSRMEM:
	case BAR_ER:
		return 1;
	}

	return 0;
}

static Retcode
dc21140_csr_read(Fake_pci_dev *self, Int offset, void *val, int size)
{
	return NO_ERROR;
}

static Retcode
dc21140_csr_write(Fake_pci_dev *self, Int offset, Int val, int size)
{
	return NO_ERROR;
}

static Retcode
dc21140_memread(Fake_pci_dev *self, int bar, Int offset, void *val, int size)
{
	Int v = 0;

	switch (bar)
	{
	case BAR_CSRMEM:
		return dc21140_csr_read(self, offset, val, size);
	case BAR_ER:
		if (offset >= 0 || offset + size - 1 < sizeof dc21140_expansion_rom)
			v = dc21140_expansion_rom[offset] |
					(dc21140_expansion_rom[offset + 1] << BYTE_SIZE) |
					(dc21140_expansion_rom[offset + 1] << (2 * BYTE_SIZE)) |
					(dc21140_expansion_rom[offset + 1] << (3 * BYTE_SIZE));
		break;
	}

	switch (size)
	{
	case 1: *(Byte *)val = v; break;
	case 2: *(Word *)val = v; break;
	case 4: *(Int *)val = v; break;
	}

	return NO_ERROR;
}

static Retcode
dc21140_memwrite(Fake_pci_dev *self, int bar, Int offset, Int val, int size)
{
	if (bar == BAR_CSRMEM)
		return dc21140_csr_write(self, offset, val, size);

	return NO_ERROR;
}

static Retcode
dc21140_ioread(Fake_pci_dev *self, int bar, Int offset, void *val, int size)
{
	if (bar == BAR_CSRIO)
		return dc21140_csr_read(self, offset, val, size);

	return NO_ERROR;
}

static Retcode
dc21140_iowrite(Fake_pci_dev *self, int bar, Int offset, Int val, int size)
{
	if (bar == BAR_CSRIO)
		return dc21140_csr_write(self, offset, val, size);

	return NO_ERROR;
}

Fake_pci_dev_methods dc21140_methods =
{
	dc21140_init,
	dc21140_configread,
	dc21140_configwrite,
	dc21140_barsize,
	dc21140_bartype,
	dc21140_memread,
	dc21140_memwrite,
	dc21140_ioread,
	dc21140_iowrite
};
#endif
