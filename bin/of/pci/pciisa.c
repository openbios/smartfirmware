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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* All ISA accesses go though the PCI-ISA bridge and hence through
   the various pcibase.c routines. */

#include "defs.h"
#include "pci.h"
#include "isa.h"

static Retcode
pciisa_mem_read(uInt addr, void *value, int size)
{
	return pci_mem_read(0, addr, value, size);
}

static Retcode
pciisa_mem_write(uInt addr, Int value, int size)
{
	return pci_mem_write(0, addr, value, size);
}

static Retcode
pciisa_io_read(uInt addr, void *value, int size)
{
	return pci_io_read(0, addr, value, size);
}

static Retcode
pciisa_io_write(uInt addr, Int value, int size)
{
	return pci_io_write(0, addr, value, size);
}

static Retcode
pciisa_map_in(Int physhi, Int physlo, Cell size, Cell *virt)
{
	return pci_map_in(0, physhi, 0, physlo, size, virt);
}

static Retcode
pciisa_map_out(Cell virt, Cell size)
{
	return pci_map_out(0, virt, size);
}

static Retcode
pciisa_dma_alloc(Cell size, Cell *virt)
{
	return pci_dma_alloc(0, size, virt);
}

static Retcode
pciisa_dma_free(Cell virt, Cell size)
{
	return pci_dma_free(0, virt, size);
}

static Retcode
pciisa_dma_map_in(Cell virt, Cell size, Int cacheable, Cell *devaddr)
{
	return pci_dma_map_in(0, virt, size, cacheable, devaddr);
}

static Retcode
pciisa_dma_map_out(Cell virt, Cell devaddr, Cell size)
{
	return pci_dma_map_out(0, virt, devaddr, size);
}

static Retcode
pciisa_dma_sync(Cell virt, Cell devaddr, Cell size)
{
	return pci_dma_sync(0, virt, devaddr, size);
}

Retcode
install_pciisa(Environ *e)
{
	isa_mem_read = pciisa_mem_read;
	isa_mem_write = pciisa_mem_write;
	isa_io_read = pciisa_io_read;
	isa_io_write = pciisa_io_write;
	isa_map_in = pciisa_map_in;
	isa_map_out = pciisa_map_out;
	isa_dma_alloc = pciisa_dma_alloc;
	isa_dma_free = pciisa_dma_free;
	isa_dma_map_in = pciisa_dma_map_in;
	isa_dma_map_out = pciisa_dma_map_out;
	isa_dma_sync = pciisa_dma_sync;
	return NO_ERROR;
}

PCI(install_pciisa_driver)
{
	EC(f_is_install);
	EC(f_is_remove);

	DPRINTF(("install_pciisa_driver: pkg=%p devinfo=%p\n", pkg, devinfo));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	install_pciisa(e);		/* install ISA device access routines */
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND, 0xFFFF0007);

	/* then install the builtin ISA-bus drivers */
	return install_isa(e, pkg);
}

/* SIS 85C496 PCI-ISA bridge */
Pci_driver sis_85C496_driver =
{
	{ 0, 0, 0, 0, 0, 0x060000, 0x1039, 0x0496, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_pciisa_driver
};

/* Intel 82378ZB PCI-ISA bridge and variants */
Pci_driver intel_82378_driver =
{
	{ 0, 0, 0, 0, 0, 0, 0x8086, 0x484, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_pciisa_driver
};

/* Intel 82371*B (PIIX*) PCI-ISA bridges */
Pci_driver intel_piix_driver =
{
	{ 0, 0, 0, 0, 0, 0x060100, 0x8086, 0x0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0, 0, 0 },
	install_pciisa_driver
};

/* Generic PCI-ISA bridge [was 0x060100] */
Pci_driver generic_pci_isa_driver =
{
	{ 0, 0, 0, 0, 0, 0x060100, 0x0, 0x0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0x0, 0x0, 0, 0, 0 },
	install_pciisa_driver
};

#if 0
/* Intel 82371FB (PIIX) PCI-ISA bridge */
Pci_driver intel_piix_driver =
{
	{ 0, 0, 0, 0, 0, 0x060100, 0x8086, 0x122E, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_pciisa_driver
};

/* Intel 82371SB (PIXX3) PCI-ISA bridge */
Pci_driver intel_piix3_driver =
{
	{ 0, 0, 0, 0, 0, 0x060100, 0x8086, 0x7000, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_pciisa_driver
};

/* Intel 82371AB (PIXX4) PCI-ISA bridge */
Pci_driver intel_piix3_driver =
{
	{ 0, 0, 0, 0, 0, 0x565, 0x10AD, 0x7110, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_pciisa_driver
};
#endif
