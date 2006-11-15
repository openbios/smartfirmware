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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved */

#ifndef __PCI_H__
#define __PCI_H__

#define PCI_PHYSHI_N(addr)     ((uInt)(((addr) >> 31) & 0x01))
#define PCI_PHYSHI_P(addr)     ((uInt)(((addr) >> 30) & 0x01))
#define PCI_PHYSHI_T(addr)     ((uInt)(((addr) >> 29) & 0x01))
#define PCI_PHYSHI_SPACE(addr) ((uInt)(((addr) >> 24) & 0x07))
#define PCI_PHYSHI_BUS(addr)   ((uInt)(((addr) >> 16) & 0xFF))
#define PCI_PHYSHI_DEV(addr)   ((uInt)(((addr) >> 11) & 0x1F))
#define PCI_PHYSHI_FUNC(addr)  ((uInt)(((addr) >> 8) & 0x07))
#define PCI_PHYSHI_REG(addr)   ((uInt)((addr) & 0xFF))

#define PCI_PHYSHI_MK(n, p, t, sp, bus, dev, func, reg)  \
		((uInt)(((uInt)n << 31) | (p << 30) | (t << 29) | (sp << 24) | \
		(bus << 16) | (dev << 11) | (func << 8) | reg))

#define PCI_SPACE_CONFIG		0
#define PCI_SPACE_IO			1
#define PCI_SPACE_MEM			2
#define PCI_SPACE_XMEM			3
#define PCI_SPACE_BUS			4

struct pci_header_methods;

struct pci_device_info
{
	int hbridge;
	int bus;
	int dev;
	int func;
	int header_type;
	int classcode;
	int vendid;
	int devid;
	int subsysvendid;
	int subsysdevid;
	int revisionid;
	struct pci_self *self;		/* device-specific use */
	struct pci_header_methods const *methods;
};

typedef struct pci_device_info Pci_device_info;

struct pci_address_block
{
	struct pci_address_block *next;
	uInt baselo;
	uInt basehi;
	uInt sizelo;
	uInt sizehi;
};

typedef struct pci_address_block Pci_address_block;

/* the pci_addresses structure is used for allocating PCI bus resources */
/* The PCI bus binding */
/* specifies 6 PCI bus resources that may be allocated: bus numbers, */
/* 10 bit i/o addresses, 32 bit i/o addresses, 20 bit memory addresses, */
/* 32 bit memory addresses, and 64 bit memory addresses.  10 bit i/o */
/* addresses are a subset of 32 bit i/o addresses with the upper bits zero. */
/* similarly 20 bit and 32 bit memory addresses are a subset of each other */
/* and of 64 bit memory addresses with the upper bits zero. */
/* memory spaces that sit behind a PCI-PCI bridge may be either prefetchable */
/* or non-prefetchable. */
struct pci_addresses
{
	Pci_address_block *busaddrs;		/* Bus number allocator */
	Pci_address_block *ioaddrs;			/* I/O space allocator */
	Pci_address_block *memaddrs;		/* Memory space allocator */
	Pci_address_block *prefetchaddrs;	/* Prefetchable Memory space */
};

typedef struct pci_addresses Pci_addresses;

struct pci_header_methods
{
	void (*getextenedinfo)(Environ *e, Pci_device_info *devinfo);
	Retcode (*probe)(Environ *e, Package *cpkg, Pci_device_info *devinfo,
			Pci_addresses *addrs);
	Retcode (*romsize)(Environ *e, Package *pkg, Pci_device_info *devinfo,
			uInt *size);
	Retcode (*writeromaddr)(Environ *e, Package *pkg, Pci_device_info *devinfo,
			uInt addr);
	Retcode (*addroffsetlist)(Environ *e, Package *pkg,
			Pci_device_info *devinfo, uChar **offsets, int *numoffsets);
	Retcode (*getaddrinfo)(Environ *e, Package *pkg, Pci_device_info *devinfo,
			int offset, uInt *physhi, uInt *sizehi, uInt *sizelo);
	Retcode (*writeaddr)(Environ *e, Package *pkg, Pci_device_info *devinfo,
			int offset, uInt physhi, uInt physmid, uInt physlo);
};

typedef struct pci_header_methods Pci_header_methods;

struct pci_driver
{
	Pci_device_info match;
	Pci_device_info mask;
	Retcode (*install)(Environ *e, Package *cpkg, Pci_device_info *devinfo,
			Pci_addresses *addrs);
};

typedef struct pci_driver Pci_driver;

/* to declare drivers easily */
#define EPCI(name) extern Retcode name(Environ *e, Package *pkg, \
		Pci_device_info *devinfo, Pci_addresses *addrs)
#define PCI(name) EPCI(name); \
		Retcode name(Environ *e, Package *pkg, Pci_device_info *devinfo, \
		Pci_addresses *addrs)

struct pci_header
{
	short vendid;			/* 0x00 */
	short devid;			/* 0x02 */
	short cmdreg;			/* 0x04 */
	short statreg;			/* 0x06 */
	char revid;				/* 0x08 */
	int classcode : 24;		/* 0x09 */
	char cachelinesize;		/* 0x0C */
	char latencytimer;		/* 0x0D */
	char headertype;		/* 0x0E */
	char BIST;				/* 0x0F */
	int reserved[11];		/* 0x10 */
	char interruptline;		/* 0x3C */
	char interruptpin;		/* 0x3D */
	short reserved2;		/* 0x3E */
};

typedef struct pci_header Pci_header;


/* byte offsets to PCI config registers */
#define PCI_CONFIG_ID				0x00
#define PCI_CONFIG_COMMAND			0x04
#define PCI_CONFIG_STATUS			PCI_CONFIG_COMMAND
#define PCI_CONFIG_CLASSREV			0x08
#define PCI_CONFIG_HEADER			0x0C
#define PCI_CONFIG_LATENCY			PCI_CONFIG_HEADER
#define PCI_CONFIG_BIST				PCI_CONFIG_HEADER
#define PCI_CONFIG_CACHE			PCI_CONFIG_HEADER
#define PCI_CONFIG_BAR0				0x10
#define PCI_CONFIG_BAR1				0x14
#define PCI_CONFIG_BAR2				0x18
#define PCI_CONFIG_BAR3				0x1C
#define PCI_CONFIG_BAR4				0x20
#define PCI_CONFIG_BAR5				0x24
#define PCI_CONFIG_CARDBUS			0x28
#define PCI_CONFIG_SUBSYSTEM		0x2C
#define PCI_CONFIG_ROMADDR			0x30
#define PCI_CONFIG_RESERVED_0		0x34
#define PCI_CONFIG_RESERVED_1		0x38
#define PCI_CONFIG_BRIDGE_ROMADDR	0x38
#define PCI_CONFIG_INTERRUPT		0x3C

/* bits in PCI_CONFIG_COMMAND register */
#define PCI_COMMAND_IOENABLE			0x0001
#define PCI_COMMAND_MEMENABLE			0x0002
#define PCI_COMMAND_MASTERENABLE		0x0004
#define PCI_COMMAND_SPECCYCLEENABLE		0x0008
#define PCI_COMMAND_MWIENABLE			0x0010
#define PCI_COMMAND_PALLETESNOOPENABLE	0x0020
#define PCI_COMMAND_PARITYERRENABLE		0x0040
#define PCI_COMMAND_WAITENABLE			0x0080
#define PCI_COMMAND_SYSTEMERRENABLE		0x0100
#define PCI_COMMAND_FASTBACK2BACKENABLE	0x0200

/* bits in PCI_CONFIG_STATUS register */
#define PCI_STATUS_CAPABILITIESLIST		0x0010
#define PCI_STATUS_66MHZ				0x0020
#define PCI_STATUS_UDF					0x0040
#define PCI_STATUS_FASTBACK2BACKXFER	0x0080
#define PCI_STATUS_DATAPARITY			0x0100
#define PCI_STATUS_DEVSELTIMING			0x0600
#define PCI_STATUS_SIGTARGETABORT		0x0800
#define PCI_STATUS_RECVTARGETABORT		0x1000
#define PCI_STATUS_RECVMASTERABORT		0x2000
#define PCI_STATUS_SIGSYSTEMERR			0x4000
#define PCI_STATUS_PARITYDETECT			0x8000

struct pci_header_type0
{
	short vendid;
	short devid;
	short cmdreg;
	short statreg;
	char revid;
	int classcode : 24;
	char cachelinesize;
	char latencytimer;
	char headertype;
	char BIST;
	int baseaddr0;
	int baseaddr1;
	int baseaddr2;
	int baseaddr3;
	int baseaddr4;
	int baseaddr5;
	int cisptr;
	short subsysvendid;
	short subsysdevid;
	int exprombaseaddr;
	int reserved;
	int reserved2;
	char interruptline;
	char interruptpin;
	char mingnt;
	char maxlat;
};

typedef struct pci_header_type0 Pci_header_type0;

/* byte offsets to PCI-PCI bridge config registers */
#define PPB_CONFIG_ID				0x00
#define PPB_CONFIG_COMMAND			0x04
#define PPB_CONFIG_STATUS			PPB_CONFIG_COMMAND
#define PPB_CONFIG_CLASSREV			0x08
#define PPB_CONFIG_HEADER			0x0C
#define PPB_CONFIG_LATENCY			PPB_CONFIG_HEADER
#define PPB_CONFIG_BIST				PPB_CONFIG_HEADER
#define PPB_CONFIG_CACHE			PPB_CONFIG_HEADER
#define PPB_CONFIG_BAR0				0x10
#define PPB_CONFIG_BAR1				0x14
#define PPB_CONFIG_BUSNUM			0x18
#define PPB_CONFIG_IOBASE			0x1C
#define PPB_CONFIG_IOLIMIT			PPB_CONFIG_IOBASE
#define PPB_CONFIG_SECSTATUS		PPB_CONFIG_IOBASE
#define PPB_CONFIG_MEMBASE			0x20
#define PPB_CONFIG_MEMLIMIT			PPB_CONFIG_MEMBASE
#define PPB_CONFIG_PMEMBASE			0x24
#define PPB_CONFIG_PMEMLIMIT		PPB_CONFIG_PMEMBASE
#define PPB_CONFIG_PMEMBASEH		0x28
#define PPB_CONFIG_PMEMLIMITH		0x2C
#define PPB_CONFIG_IOBASEH			0x30
#define PPB_CONFIG_RESERVED_0		0x34
#define PPB_CONFIG_ROMADDR			0x38
#define PPB_CONFIG_INTERRUPT		0x3C

struct pci_header_type1
{
	short vendid;
	short devid;
	short cmdreg;
	short statreg;
	char revid;
	int classcode : 24;
	char cachelinesize;
	char latencytimer;
	char headertype;
	char BIST;
	int baseaddr0;
	int baseaddr1;
	char primbusnum;
	char secbusnum;
	char subbusnum;
	char seclattimer;
	char iobase;
	char iolimit;
	short secstatus;
	short membase;
	short memlimit;
	short premembase;
	short prememlimit;
	int premembasehi;
	int prememlimithi;
	short iobasehi;
	short iolimithi;
	int reserved;
	int exprombaseaddr;
	char interruptline;
	char interruptpin;
	short bridgecontrol;
};

typedef struct pci_header_type1 Pci_header_type1;

#define CARDBUS_CONFIG_ID			0x00
#define CARDBUS_CONFIG_COMMAND		0x04
#define CARDBUS_CONFIG_STATUS		CARDBUS_CONFIG_COMMAND
#define CARDBUS_CONFIG_CLASSREV		0x08
#define CARDBUS_CONFIG_HEADER		0x0C
#define CARDBUS_CONFIG_LATENCY		CARDBUS_CONFIG_HEADER
#define CARDBUS_CONFIG_BIST			CARDBUS_CONFIG_HEADER
#define CARDBUS_CONFIG_CACHE		CARDBUS_CONFIG_HEADER
#define CARDBUS_CONFIG_BAR0			0x10
#define CARDBUS_CONFIG_SECSTATUS	0x14
#define CARDBUS_CONFIG_BUSNUM		0x18
#define CARDBUS_CONFIG_MEMBASE0		0x1C
#define CARDBUS_CONFIG_MEMLIMIT0	0x20
#define CARDBUS_CONFIG_MEMBASE1		0x24
#define CARDBUS_CONFIG_MEMLIMIT1	0x28
#define CARDBUS_CONFIG_IOBASE0		0x2C
#define CARDBUS_CONFIG_IOLIMIT0		0x30
#define CARDBUS_CONFIG_IOBASE1		0x34
#define CARDBUS_CONFIG_IOLIMIT1		0x38
#define CARDBUS_CONFIG_INTERRUPT	0x3C
#define CARDBUS_CONFIG_SUBSYSTEM	0x40
#define CARDBUS_CONFIG_LEGACYBASE	0x44

struct pci_header_type2
{
	short vendid;
	short devid;
	short cmdreg;
	short statreg;
	char revid;
	int classcode : 24;
	char cachelinesize;
	char latencytimer;
	char headertype;
	char BIST;
	int excabaseaddr;
	short reserved;
	short secstatus;
	char primbusnum;
	char secbusnum;
	char subbusnum;
	char seclattimer;
	int membase0;
	int memlimit0;
	int membase1;
	int memlimit1;
	short iobase0;
	short iobasehi0;
	short iolimit0;
	short iolimithi0;
	short iobase1;
	short iobasehi1;
	short iolimit1;
	short iolimithi1;
	char interruptline;
	char interruptpin;
	short bridgecontrol;
	short subsysvendid;
	short subsysdevid;
	int legacybaseaddr;
	int reserved2[14];
};

typedef struct pci_header_type2 Pci_header_type2;

extern const Pci_driver *pci_drivers[];
extern const Pci_header_methods * const pci_header_methods[];
extern const int pci_num_headers;

extern Int pci_num_host_bridges();
extern Int pci_find_host_bridge_number(Environ *e, Package *pkg);
extern Retcode pci_init_addresses(Int hbridge, Pci_addresses *addrs);
extern Retcode pci_config_map_intr(Int hbridge, Int bus, Int dev,
		Int pin, Int *intr);
extern Retcode pci_bus_package(Environ *e, Int hbridge, Package **pkg);
extern Int pci_config_read(Int hbridge, Int bus, Int dev, Int func, Int addr);
extern void pci_config_write(Int hbridge, Int bus, Int dev, Int func, Int addr,
		Int value);
extern Retcode pci_intr_ack(Int hbridge);
extern Retcode pci_special_cycle(Int hbridge, Int bus, Int data);
extern Retcode pci_mem_read(Int hbridge, uInt addr, void *value, int size);
extern Retcode pci_mem_write(Int hbridge, uInt addr, Int value, int size);
extern Retcode pci_mem_read64(Int hbridge, uInt addrhi, uInt addrlo,
		void *value, int size);
extern Retcode pci_mem_write64(Int hbridge, uInt addrhi, uInt addrlo, Int value,
		int size);
extern Retcode pci_io_read(Int hbridge, uInt addr, void *value, int size);
extern Retcode pci_io_write(Int hbridge, uInt addr, Int value, int size);
extern Retcode pci_map_in(Int hbridge, Int phys_hi, Int phys_mid, Int phys_low,
		Cell size, Cell *virt);
extern Retcode pci_map_out(Int hbridge, Cell virt, Cell size);
extern Retcode pci_dma_alloc(Int hbridge, Cell size, Cell *virt);
extern Retcode pci_dma_free(Int hbridge, Cell virt, Cell size);
extern Retcode pci_dma_map_in(Int hbridge, Cell virt, Cell size, Int cacheable,
		Cell *devaddr);
extern Retcode pci_dma_map_out(Int hbridge, Cell virt, Cell devaddr, Cell size);
extern Retcode pci_dma_sync(Int hbridge, Cell virt, Cell devaddr, Cell size);

/* routines for PCI address space allocator */

extern void pci_init_pool(Pci_addresses *addrs);
extern void pci_destroy_pool(Pci_addresses *addrs);
extern int pci_allocate_buses(Pci_addresses *addrs, int num);
extern uInt pci_allocate_io10(Pci_addresses *addrs, uInt size);
extern uInt pci_allocate_io16(Pci_addresses *addrs, uInt size);
extern uInt pci_allocate_io32(Pci_addresses *addrs, uInt size);
extern uInt pci_allocate_mem20(Pci_addresses *addrs, uInt size);
extern uInt pci_allocate_mem32(Pci_addresses *addrs, uInt size);
extern int pci_allocate_mem64(Pci_addresses *addrs, uInt sizehi, uInt sizelo,
		uInt *addrhi, uInt *addrlo);
extern uInt pci_allocate_pmem32(Pci_addresses *addrs, uInt size);
extern int pci_allocate_pmem64(Pci_addresses *addrs, uInt sizehi, uInt sizelo,
		uInt *addrhi, uInt *addrlo);
extern int pci_free_bus_range(Pci_addresses *addrs, int base, int size);
extern int pci_free_io_range(Pci_addresses *addrs, uInt base, uInt size);
extern int pci_free_mem_range(Pci_addresses *addrs, uInt basehi, uInt baselo,
	uInt sizehi, uInt sizelo);
extern int pci_free_pmem_range(Pci_addresses *addrs, uInt basehi, uInt baselo,
	uInt sizehi, uInt sizelo);
extern int pci_allocate_fixed_block(Pci_address_block **blocks,
		uInt addrhi, uInt addrlo, uInt sizehi, uInt sizelo);
extern int pci_find_largest_block(Pci_address_block **blocks, 
		uInt alignhi, uInt alignlo, uInt minhi, uInt minlo,
		uInt maskhi, uInt masklo, uInt *addrhi, uInt *addrlo,
		uInt *sizehi, uInt *sizelo);

/* handy for builtin drivers to set things up easily */
extern void pci_load_reg_and_name_props(Environ *e, Package *pkg,
		Pci_device_info *devinfo);
extern Retcode pci_decode_reg_prop(Byte **arr, Int *len, Int *physhi,
		Int *physmid, Int *physlo, Int *sizehi, Int *sizelo);
extern Retcode pci_probe_bus(Environ *e, Package *buspkg, Int hbridge, Int bus,
		Pci_addresses *addrs, int devs);
extern Retcode pci_load_expansion_rom(Environ *e, Package *pkg,
		Pci_device_info *devinfo, Pci_addresses *addrs);
extern Retcode pci_load_builtin_driver(Environ *e, Package *pkg,
		Pci_device_info *devinfo, Pci_addresses *addrs);
extern void pci_encode_range_prop(Byte *arr, Int *len, Cell physhi,
		Cell physmid, Cell physlo, Cell sizehi, Cell sizelo);

/* CardBus functions */
extern Retcode cardbus_load_bridge_properties(Environ *e, Package *pkg);
extern Retcode cardbus_probe_pci_bridge(Environ *e, Package *pkg,
	Pci_device_info *devinfo, Pci_addresses *addrs, int devs);
extern Retcode cardbus_probe(Environ *e, Package *cpkg,
	Pci_device_info *devinfo, Pci_addresses *addrs);

#endif /* __PCI_H__ */
