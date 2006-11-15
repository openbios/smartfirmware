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

#include "defs.h"
#include "pci.h"
#include "fakepci.h"

#ifdef FAKE_PCI

#define BAR_CSR			0		/* CSR BAR */
#define BAR_ER			7		/* expansion rom BAR */
#define BAR_CSR_SZ		1024		/* CSR BAR size*/
#define BAR_ER_SZ		(256*1024)	/* expansion rom BAR size */

static Int pcidiskdev_config_default[] =
{
	0x00018765,		/* devid, vendid */
	0x02800000,		/* status, command */
	0x05020001,		/* class code, revid */
	0x00000000,		/* BIST, header type, latency timer, cache line size */
	0x00000000,		/* BAR #0 */
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

static Int pcidiskdev_config_write_mask[] =
{
	0x00000000,		/* devid, vendid */
	0x00000147,		/* status, command */
	0x00000000,		/* class code, revid */
	0x0000FF00,		/* BIST, header type, latency timer, cache line size */
	0xFFFFFF00,		/* BAR #0 */
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
	0x000000FF,		/* max lat, min gnt, intr pin, intr line */
};

static Int pcidiskdev_csr_default[] =
{
	0x00000000,		/* command/status reg */
	0x00000000,		/* sector */
	0x00000000,		/* DMA address (not used yet) */
	0x00000000,		/* block count */
};

static Int pcidiskdev_csr_write_mask[] =
{
	0x0000FFFF,		/* command/status reg */
	0xFFFFFFFF,		/* sector */
	0xFFFFFFFC,		/* DMA address */
	0x0000FFFF,		/* block count */
};

#define CSR_DMAENB		0x00000001
#define CSR_READY		0x00010000
#define CSR_BUSY		0x00020000
#define CSR_ERROR		0x00040000
#define CSR_CRC			0x00080000

static uByte pcidiskdev_expansion_rom[] =
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

#define CSR_SZ	4

struct pcidiskdev_data
{
	Int config[16];
	Int csr[CSR_SZ];
	uByte buf[512];
	Bool csrenb;
	Bool er;
	Int secnum;
	Int dmaaddr;
};

typedef struct pcidiskdev_data PCIdiskdev_data;

static Bool
pcidiskdev_init(Fake_pci_dev *self)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)malloc(sizeof (PCIdiskdev_data));

	if (d == NULL)
		return FALSE;

	self->private = d;
	self->configsize = 64;
	memset(d, '\0', sizeof (PCIdiskdev_data));
	memcpy(d->config, pcidiskdev_config_default, 16 * sizeof (Int));
	memcpy(d->csr, pcidiskdev_csr_default, CSR_SZ * sizeof (Int));
	d->csrenb = 0;
	d->er = 0;
	return TRUE;
}

static Int
pcidiskdev_configread(Fake_pci_dev *self, int reg)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;
	return d->config[reg / sizeof (Int)];
}

static void
pcidiskdev_enable_rom(Fake_pci_dev *self, Bool enable, Int old, Int new)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;

	if (d->er)
		fakepci_mem_unmap(old & ~1, BAR_ER_SZ, self, BAR_ER, 0);

	if (enable)
		fakepci_mem_map(new & ~1, BAR_ER_SZ, self, BAR_ER, 0);

	d->er = enable;
}

static void
pcidiskdev_enable_mem_csr(Fake_pci_dev *self, Bool enable, Int old, Int new)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;

	if (d->csrenb)
		fakepci_mem_unmap(old & ~1, BAR_CSR_SZ, self, BAR_CSR, 0);

	if (enable)
		fakepci_mem_map(new & ~1, BAR_CSR_SZ, self, BAR_CSR, 0);

	d->csrenb = enable;
}

static void
pcidiskdev_configwrite(Fake_pci_dev *self, int reg, Int val)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;
	int r = reg / sizeof (Int);
	Int mask = pcidiskdev_config_write_mask[r];
	Int oval = d->config[r];
	val = (oval & ~mask) | (val & mask);
	d->config[r] = val;

	switch (reg)
	{
	case 0x04:		/* CFCS */
		pcidiskdev_enable_rom(self,
				(val & 2) && (d->config[0x30 / sizeof (Int)] & 1),
				d->config[0x30 / sizeof (Int)], d->config[0x30 / sizeof (Int)]);
		pcidiskdev_enable_mem_csr(self, val & 2, d->config[0x14 / sizeof (Int)],
				d->config[0x14 / sizeof (Int)]);
		break;
	case 0x10:		/* BAR #0 */
		pcidiskdev_enable_mem_csr(self, d->config[0x04 / sizeof (Int)] & 1, oval,
				val);
		break;
	case 0x30:		/* CBER */
		pcidiskdev_enable_rom(self,
				(d->config[0x04 / sizeof (Int)] & 2) && (val & 1), oval, val);
		break;
	}
}

static int
pcidiskdev_barsize(Fake_pci_dev *self, int bar)
{
	switch (bar)
	{
	case BAR_CSR:
		return BAR_CSR_SZ;
	case BAR_ER:
		return BAR_ER_SZ;
	}

	return 0;
}

static int
pcidiskdev_bartype(Fake_pci_dev *self, int bar)
{
	switch (bar)
	{
	case BAR_CSR:
	case BAR_ER:
		return 1;
	}

	return 0;
}

static void
pcidiskdev_execute_cmd(Fake_pci_dev *self)
{
}

static void
pcidiskdev_udpate_regs(Fake_pci_dev *self)
{
}

static Retcode
pcidiskdev_csr_read(Fake_pci_dev *self, Int offset, void *val, int size)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;
	pcidiskdev_udpate_regs(self);

	if (d->csrenb && offset >= 0 && offset + size - 1 < CSR_SZ*4)
	{
		uInt v = d->csr[offset / sizeof (Int)];

		switch (size)
		{
		case 1:
			*(uByte *)val = v >> (8 << (offset & 3));
			break;
		case 2:
			*(uByte *)val = v >> (8 << (offset & 2));
			break;
		case 4:
			*(uByte *)val = v;
			break;
		}
	}

	return NO_ERROR;
}

static Retcode
pcidiskdev_csr_write(Fake_pci_dev *self, Int offset, Int val, int size)
{
	PCIdiskdev_data *d = (PCIdiskdev_data *)self->private;
	int r = offset / sizeof (Int);
	Int mask;
	Int oval;

	
	if (offset < 0 || (offset + size - 1 > CSR_SZ*4))
		return NO_ERROR;

	mask = pcidiskdev_csr_write_mask[r];
	oval = d->csr[r];

	switch (size)
	{
	case 1:
		mask &= 0xFF << (8 * (offset & 3));
		break;
	case 2:
		mask &= 0xFFFF << (8 * (offset & 2));
		break;
	}

	val = (oval & ~mask) | (val & mask);
	d->csr[r] = val;

	switch (r)
	{
	case 0x00:		/* command/status register */
		pcidiskdev_execute_cmd(self);
		break;
	case 0x01:		/* sector */
		d->secnum = val;
		break;
	case 0x02:
		d->dmaaddr = val;
		break;
	}

	return NO_ERROR;
}

static Retcode
pcidiskdev_memread(Fake_pci_dev *self, int bar, Int offset, void *val, int size)
{
	Int v = 0;

	switch (bar)
	{
	case BAR_CSR:
		return pcidiskdev_csr_read(self, offset, val, size);
	case BAR_ER:
		if (offset >= 0 || offset + size - 1 < sizeof pcidiskdev_expansion_rom)
			v = pcidiskdev_expansion_rom[offset] |
					(pcidiskdev_expansion_rom[offset + 1] << BYTE_SIZE) |
					(pcidiskdev_expansion_rom[offset + 1] << (2 * BYTE_SIZE)) |
					(pcidiskdev_expansion_rom[offset + 1] << (3 * BYTE_SIZE));
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
pcidiskdev_memwrite(Fake_pci_dev *self, int bar, Int offset, Int val, int size)
{
	if (bar == BAR_CSR)
		return pcidiskdev_csr_write(self, offset, val, size);

	return NO_ERROR;
}

static Retcode
pcidiskdev_ioread(Fake_pci_dev *self, int bar, Int offset, void *val, int size)
{
	return NO_ERROR;
}

static Retcode
pcidiskdev_iowrite(Fake_pci_dev *self, int bar, Int offset, Int val, int size)
{
	return NO_ERROR;
}

Fake_pci_dev_methods pcidiskdev_methods =
{
	pcidiskdev_init,
	pcidiskdev_configread,
	pcidiskdev_configwrite,
	pcidiskdev_barsize,
	pcidiskdev_bartype,
	pcidiskdev_memread,
	pcidiskdev_memwrite,
	pcidiskdev_ioread,
	pcidiskdev_iowrite
};
#endif
