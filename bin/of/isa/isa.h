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

/* (c) Copyright 1996-1997 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __ISA_H__
#define __ISA_H__

/* bits of ISA phys.hi - rest are zero */
#define ISA_MEMORY_ADDR		0x00	/* no other bits count if this is true */
#define ISA_IO_ADDRESS		0x01	/* if 0, then memory space */
#define ISA_10_BIT_ADDR		0x02	/* only for I/O space, zero for memory */
#define ISA_11_BIT_ADDR		0x04	/* only for I/O space, zero for memory */

struct isa_device
{
	Byte *name;
	Byte *type;
	Int physhi;		/* or-ed ISA_* bits above */
	Int physlo;		/* I/O or memory address */
	Int size;		/* for "reg" property - number of bytes at physlo */
	Int irq[2];		/* irq[0] is IRQ#, irq[1] is interrupt type */
	Int dma[5];		/* DMA#, mode, width, countwidth, busmaster */
	Int bios;		/* BIOS ROM address */
	Int numreg;		/* number of sets of additional reg props (3 Ints each) */
	Int *reg;		/* ptr to additional "reg" properties, if any */
	Int clkfreq;		/* clock frequency, if any */
	Retcode (*probe)(Environ *e, struct isa_device *dev);
	Retcode (*install)(Environ *e, struct isa_device *dev);
	const Initentry *methods;
	Package *pkg;	/* for the device's use during initialization */
	struct isa_self *self;	/* any device-specific extras go here */
};
typedef struct isa_device Isa_device;

#define E_ISA(func) extern Retcode func(Environ *e, Isa_device *dev)
#define ISA(func) E_ISA(func); Retcode func(Environ *e, Isa_device *dev)

extern Isa_device *isa_devices[];


/* used to install ISA drivers - in isa.c */
extern Retcode install_isa(Environ *e, Package *pkg);
extern Retcode new_isa_device(Environ *e, Isa_device *dev);

/* machine-specific function pointers to access ISA bus */
extern Retcode (*isa_mem_read)(uInt addr, void *value, int size);
extern Retcode (*isa_mem_write)(uInt addr, Int value, int size);
extern Retcode (*isa_io_read)(uInt addr, void *value, int size);
extern Retcode (*isa_io_write)(uInt addr, Int value, int size);
extern Retcode (*isa_map_in)(Int physhi, Int physlo, Cell size, Cell *virt);
extern Retcode (*isa_map_out)(Cell virt, Cell size);
extern Retcode (*isa_dma_alloc)(Cell size, Cell *virt);
extern Retcode (*isa_dma_free)(Cell virt, Cell size);
extern Retcode (*isa_dma_map_in)(Cell virt, Cell size, Int cacheable,
		Cell *devaddr);
extern Retcode (*isa_dma_map_out)(Cell virt, Cell devaddr, Cell size);
extern Retcode (*isa_dma_sync)(Cell virt, Cell devaddr, Cell size);

extern Retcode install_isabase(Environ *e);		/* in isabase.c */
extern Retcode install_pciisa(Environ *e);		/* in pciisa.c */

#endif /* __ISA_H__ */
