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

/* (c) Copyright 2000 by CodeGen, Inc.  All Rights Reserved. */

/* Driver for AMD Lance family 79c97* and hopefully older 7990-compat parts.
 */

/*#define DEBUG*/

#include "defs.h"
#include "pci.h"

extern void pci_format_csr(char *buf, int buflen, uInt csr);


/* This header file is modified from FreeBSD's /sys/i386/isa/ic/Am7990.h. */
/*
 * Am7990, Local Area Network Controller for Ethernet (LANCE)
 *
 * Copyright (c) 1994, Paul Richards. This software may be used,
 * modified, copied, distributed, and sold, in both source and binary
 * form provided that the above copyright and these terms are retained.
 * Under no circumstances is the author responsible for the proper
 * functioning of this software, nor does the author assume any
 * responsibility for damages incurred with its use.
 *
 */

#define CHIP_79C970A	1
#define CHIP_79C971	2
#define CHIP_79C972	3
#define CHIP_79C973	4
#define CHIP_79C975	5

/*
 * The LANCE has four Control and Status Registers(CSRs) which are accessed
 * through two bus addressable ports, the address port (RAP) and the data
 * port (RDP).
 *
 */

#define CSR0	0
#define CSR1	1
#define CSR2	2
#define CSR3	3
#define CSR88	88
#define CSR89	89

#define BCR49	49
#define BCR32	32
#define BCR33	33
#define BCR34	34


/* Control and Status Register Masks */

/* CSR0 */

#define ERR		0x8000
#define BABL	0x4000
#define CERR	0x2000
#define MISS	0x1000
#define MERR	0x0800
#define RINT	0x0400
#define TINT	0x0200
#define IDON	0x0100
#define INTR	0x0080
#define INEA	0x0040
#define RXON	0x0020
#define TXON	0x0010
#define TDMD	0x0008
#define STOP	0x0004
#define STRT	0x0002
#define INIT	0x0001

/*
 * CSR3
 *
 * Bits 3-15 are reserved.
 *
 */

#define BSWP	0x0004
#define ACON	0x0002
#define BCON	0x0001

/* Initialisation block */

struct init_block
{
	uShort mode;		/* Mode register			*/
	uByte  padr[6];		/* Ethernet address			*/
	uByte  ladrf[8];	/* Logical address filter (multicast)	*/
	uShort rdra;		/* Low order pointer to receive ring	*/
	uShort rlen;		/* High order pointer and no. rings	*/
	uShort tdra;		/* Low order pointer to transmit ring	*/
	uShort tlen;		/* High order pointer and no. rings	*/
};

/* Initialisation Block Mode Register Masks */

#define PROM      0x8000   /* Promiscuous Mode */
#define DRCVBC    0x4000   /* Disable Receive Broadcast */
#define DRCVPA    0x2000   /* Disable Receive Physical Address */
#define DLNKTST   0x1000   /* Disable Link Status */
#define DAPC      0x0800   /* Disable Automatic Polarity Correction */
#define MENDECL   0x0400   /* MENDEC Loopback Mode */
#define LRT       0x0200   /* Low Receive Threshold (T-MAU mode only) */
#define TSEL      0x0200   /* Transmit Mode Select  (AUI mode only) */
#define PORTSEL   0x0180   /* Port Select bits */
#define INTL      0x0040   /* Internal Loopback */
#define DRTY      0x0020   /* Disable Retry */
#define FCOLL     0x0010   /* Force Collision */
#define DXMTFCS   0x0008   /* Disable transmit CRC (FCS) */
#define LOOP      0x0004   /* Loopback Enabl */
#define DTX       0x0002   /* Disable the transmitter */
#define DRX       0x0001   /* Disable the receiver */

/*
 * Message Descriptor Structure
 *
 * Each transmit or receive descriptor ring entry (RDRE's and TDRE's)
 * is composed of 4, 16-bit, message descriptors. They contain the following
 * information.
 *
 * 1. The address of the actual message data buffer in user (host) memory.
 * 2. The length of that message buffer.
 * 3. The status information for that particular buffer. The eight most
 *    significant bits of md1 are collectively termed the STATUS of the
 *    descriptor.
 *
 * Descriptor md0 contains LADR 0-15, the low order 16 bits of the 24-bit
 * address of the actual data buffer.  Bits 0-7 of descriptor md1 contain
 * HADR, the high order 8-bits of the 24-bit data buffer address. Bits 8-15
 * of md1 contain the status flags of the buffer. Descriptor md2 contains the
 * buffer byte count in bits 0-11 as a two's complement number and must have
 * 1's written to bits 12-15. For the receive entry md3 has the Message Byte
 * Count in bits 0-11, this is the length of the received message and is valid
 * only when ERR is cleared and ENP is set. For the transmit entry it contains
 * more status information.
 *
 */

struct mds
{
	uShort md0;		/* LADR:15..0 */
	uShort md1;		/* FLAGS:15..8 HADR:7..0 */
	Short  md2;		/* <ones>:15..12 buf-count:11..0 */
	uShort md3;		/* ?:15..12 recv-len:11..0  OR  txflags:15..0 */
};

/* Receive STATUS flags for md1 */

#define OWN		0x8000		/* Owner bit, 0=host, 1=Lance   */
#define MDERR	0x4000		/* Error                        */
#define FRAM	0x2000		/* Framing error error          */
#define OFLO	0x1000		/* Silo overflow                */
#define CRC		0x0800		/* CRC error                    */
#define RBUFF	0x0400		/* Buffer error                 */
#define STP		0x0200		/* Start of packet              */
#define ENP		0x0100		/* End of packet                */
#define HADR	0x00FF		/* High order address bits	*/

/* Receive STATUS flags for md2 */

#define BCNT	0x0FFF		/* Size of data buffer as 2's comp. no. */

/* Receive STATUS flags for md3 */

#define MCNT	0x0FFF		/* Total size of data for received packet */

/* Transmit STATUS flags for md1 */

#define ADD_FCS	0x2000		/* Controls generation of FCS	*/
#define MORE	0x1000		/* Indicates more than one retry was needed */
#define ONE		0x0800		/* Exactly one retry was needed */
#define DEF		0x0400		/* Packet transmit deferred -- channel busy */


/*
 * Transmit status flags for md2
 *
 * Same as for receive descriptor.
 *
 * BCNT   0x0FFF         Size of data buffer as 2's complement number.
 *
 */

/* Transmit status flags for md3 */

#define TBUFF	0x8000		/* Buffer error         */
#define UFLO	0x4000		/* Silo underflow       */
#define LCOL	0x1000		/* Late collision       */
#define LCAR	0x0800		/* Loss of carrier      */
#define RTRY	0x0400		/* Tried 16 times       */
#define TDR 	0x03FF		/* Time domain reflectometry */

/* end of FreeBSD header */



/* registers in 16-bit mode */
#define APROM	0x00		/* 1st 16-bytes are APROM */
#define RDP		0x10		/* control register data port */
#define RAP		0x12		/* control/bus-control register address port */
#define RESET	0x14		/* reset register */
#define BDP		0x16		/* bus-control register data port */


#define MAC_ADDR			6		/* Ethernet hardware address length */
#define MAX_PACKET_SIZE		1520	/* rounded to 8-byte boundary */
#define NUM_DESCRIPTORS		1		/* 1 T/DEs per ring entry */
#define TRING_LEN			0		/* 2^0==1 one ring entry */
#define TRING_SIZE			(1<<TRING_LEN)		/* just one ring entries */
#define RRING_LEN			2		/* 2^2==4 receive ring entries */
#define RRING_SIZE			(1<<RRING_LEN)		/* four receive ring entries */
#define DMA_BUF_SIZE 		(MAX_PACKET_SIZE * (TRING_SIZE+RRING_SIZE+1))
									/* for Tran/Recv packets + T/DEs */

/* instance-specific vars */
struct self
{
	Pci_device_info *devinfo;
	Int physhi, physmid, physlo, sizehi, sizelo;
	volatile uShort *csr;		/* word access to memory-mapped registers */
	uInt chip;
	Bool duplex;				/* true if full duplex */
	Int speed;					/* Speed 10 or 100, 0 = either */
	Instance *obptftp;
	uByte mac_addr[MAC_ADDR];	/* Ethernet hardware address */
	Cell dma;					/* mapped memory for DMA access from host */
	Cell busdma;				/* allocated memory for DMA from PCI bus */
	Byte *sendbuf, *recvbuf;	/* host-addressable pointers to bufs */
	uInt senddma, recvdma;		/* PCI-addressable pointers to same bufs */
	struct mds *tds;			/* TDes */
	uInt tdsdma;				/* DMA access to TDes from PCI */
	struct mds *rds;			/* RDes */
	struct init_block *init;	/* init block */
	uInt initdma;				/* PCI DMA access to init block */
	uInt rdsdma;				/* DMA access to RDes from PCI */
	uInt rdsindex;				/* index of current receive descriptor */
	uInt tdsindex;				/* index of current transmit descriptor */
};

typedef struct self Self;


#define DELAY(n)				u_sleep(n)
#define	IO_SIZE					32		/* 32-byte register space */

/*
#define IO_READ1(s, reg)		(((volatile uByte*)(s)->csr)[(reg)])
#define IO_READ(s, reg)			LE2((s)->csr[(reg) >> 1])
#define IO_WRITE(s, reg, val)	((s)->csr[(reg) >> 1] = LE2((uInt)val))
*/

#ifdef __powerpc__
#define MB()	asm(" eieio"); asm(" sync");
#else
#define MB()
#endif

uByte
IO_READ1(Self *s, int reg)
{
	DELAY(20);
	MB();
	return ((volatile uByte*)s->csr)[reg];
}

uShort
IO_READ(Self *s, int reg)
{
	uShort tmp;
	DELAY(20);
	MB();
	tmp = s->csr[reg >> 1];
	return LE2(tmp);
}

uInt
IO_READ4(Self *s, int reg)
{
	uInt tmp;
	DELAY(20);
	MB();
	tmp = *(volatile uInt *)(&s->csr[reg >> 1]);
	return LE4(tmp);
}

void
IO_WRITE(Self *s, int reg, uInt val)
{
	DELAY(20);
	MB();
	s->csr[reg >> 1] = LE2((uInt)val);
}

void
IO_WRITE4(Self *s, int reg, uInt val)
{
	DELAY(20);
	MB();
	*(volatile uInt *)(&s->csr[reg >> 1]) = LE4((uInt)val);
}

#define CSR_READ(s, reg)		({ IO_WRITE(s, RAP, reg); IO_READ(s, RDP); })
#define CSR_WRITE(s, reg, val)	({ IO_WRITE(s, RAP, reg); IO_WRITE(s, RDP, val); })

#define BCR_READ(s, reg)		({ IO_WRITE(s, RAP, reg); IO_READ(s, BDP); })
#define BCR_WRITE(s, reg, val)	({ IO_WRITE(s, RAP, reg); IO_WRITE(s, BDP, val); })

extern void dump_packet(Environ *e, uByte *packet, uInt len);


#ifdef DEBUG
static void
dump_regs(Environ *e, Self *s)
{
	int i;

	dprintf("regs:");

	for (i = 0; i < 0x8; i += 1)
		dprintf(" %02x:%02x", i, IO_READ1(s, i));

	dprintf("\n     ");

	for (; i < 0x10; i += 1)
		dprintf(" %02x:%02x", i, IO_READ1(s, i));

	dprintf("\n     ");
	CSR_READ(s, 88);

	for (; i < 0x18; i += 2)
		dprintf(" %02x:%04x", i, IO_READ(s, i));

	dprintf("\nCSRs:");

	for (i = 0; i < 128; i++)
	{
		if (i && (i % 8) == 0)
			dprintf("\n     ");

		dprintf(" %3d:%04x", i, CSR_READ(s, i));
	}

	dprintf("\nBCRs:");

	for (i = 0; i < 64; i++)
	{
		if (i && (i % 8) == 0)
			dprintf("\n     ");

		dprintf(" %2d:%04x", i, BCR_READ(s, i));
	}

	dprintf("\n");
}
#else
#	define dump_regs(e, s)	/* e, s */
#endif

static Retcode
read_mac_addr(Environ *e, Package *pkg, Self *s, Pci_device_info *devinfo)
{
#if 1
	int i;

	/* make sure the the signature is 0x57 0x57 - should also check checksum */
	if (IO_READ1(s, APROM + 14) != 0x57 || IO_READ1(s, APROM + 15) != 0x57)
		return E_BAD_ARGUMENT;
	
	/* read the MAC addr from the APROM area */
	for (i = 0; i < MAC_ADDR; i += 1)
		s->mac_addr[i] = IO_READ1(s, APROM + i);
#else
	s->mac_addr[0] = 0x00;		/* fill in something reasonable for */
	s->mac_addr[1] = 0x90;		/* now */
	s->mac_addr[2] = 0x27;
	s->mac_addr[3] = 0x37;
	s->mac_addr[4] = 0x0E;
	s->mac_addr[5] = 0xe7;
	/* 00:90:27:37:0E:E7 */
#endif

	return set_property(pkg->props, "local-mac-address", CSTR,
			(Byte *)s->mac_addr, MAC_ADDR);
}

static int
get_chipid(Environ *e, Self *s, char **namep, char **partnump, int *revp)
{
	int cfg;
	char *name = NULL;
	char *num = NULL;

	cfg = CSR_READ(s, 89) << 16;
	cfg |= CSR_READ(s, 88);
	s->chip = 0;

	switch ((cfg >> 1) & 0x7FF)
	{
	case 0x001:
		/* this is an AMD part */
		switch ((cfg >> 12) & 0xFFFF)
		{
		case 0x0242:
			name = "PCnet-PCI";
			num = "Am79C970";
			break;
		case 0x2621:
			name = "PCnet-PCI II";
			num = "Am79C970A";
			s->chip = CHIP_79C970A;
			break;
		case 0x2623:
			name = "PCnet-PCI Fast";
			num = "Am79C971";
			s->chip = CHIP_79C971;
			break;
		case 0x2624:
			name = "PCnet-PCI Fast+";
			num = "Am79C972";
			s->chip = CHIP_79C972;
			break;
		case 0x2625:
			name = "PCnet-PCI Fast III";
			num = "Am79C973";
			s->chip = CHIP_79C973;
			break;
		case 0x2627:
			name = "PCnet-PCI Fast III";
			num = "Am79C975";
			s->chip = CHIP_79C975;
			break;
		default:
			name = "PCnet";
			break;
		}
	}

	if (namep)
		*namep = name;

	if (partnump)
		*partnump = num;

	if (name && num)
	{
		if (revp)
			*revp = (cfg >> 28) & 0xF;

		return 1;
	}

	if (revp)
		*revp = cfg;

	return 0;
}

static void
setup_buffers(Self *s)
{
	/* setup our buffers
	  - sizes are such that align requirements should work out automagically
	 */
	s->recvbuf = (Byte*)(uPtr)s->dma;
	memset(s->recvbuf, 0, DMA_BUF_SIZE);	/* init everything to zero */
	s->recvdma = (uInt)s->busdma;
	s->sendbuf = s->recvbuf + RRING_SIZE*MAX_PACKET_SIZE;
	s->senddma = s->recvdma + RRING_SIZE*MAX_PACKET_SIZE;
	s->rds = (struct mds *)(s->sendbuf + TRING_SIZE*MAX_PACKET_SIZE);
	s->rdsdma = s->senddma + TRING_SIZE*MAX_PACKET_SIZE;
	s->tds = s->rds + RRING_SIZE;
	s->tdsdma = s->rdsdma + RRING_SIZE * sizeof *s->rds;
	s->init = (struct init_block *)(s->tds + TRING_SIZE);
	s->initdma = s->tdsdma + TRING_SIZE * sizeof *s->tds;
	s->rdsindex = 0;
	s->tdsindex = 0;
	DPRINTF(("lance: setup_buffers: recvbuf/dma=%p/%#x sendbuf/dma=%p/%#x\n",
			s->recvbuf, s->recvdma, s->sendbuf, s->senddma));
	DPRINTF(("            tds=%p/%#x rds=%p/%#x\n",
			s->tds, s->tdsdma, s->rds, s->rdsdma));
}

static int
reset_lance(Environ *e, Self *s)
{
	Int cfg;
	uInt prevcsr = 0;
	uInt csr = 0;
	int i;
	uInt dma;

	/* reset the chip */
	(void)IO_READ(s, RESET);
	IO_WRITE(s, RESET, 1);		/* for NE2100s? */
	DELAY(10);

	/* init CSRs and BCRs */
	cfg = BCR_READ(s, 2);

	if (s->chip != CHIP_79C973 && s->chip != CHIP_79C973)
	    cfg |= 0x06;	/* AWAKE | ASEL */

	BCR_WRITE(s, 2, cfg);	

	cfg = BCR_READ(s, 32) & ~0x38;	/* !XPHYANE !XPHYFD !XPHYSP */

	if (s->duplex)
	{
		BCR_WRITE(s, 9, 0x3);	/* AUIFD | FDEN */
		cfg |= 0x10;			/* XPHYFD */
	}

	if (s->speed == 100)
		cfg |= 0x08;			/* XPHYSP */
	else if (s->speed != 10)
		cfg |= 0x20;			/* XPHYANE */

	BCR_WRITE(s, 32, cfg);

	CSR_WRITE(s, 3, 0x5E00);	/* BABLM|MISSM|MEERM|RINTM|TINTM*/
	CSR_WRITE(s, 4, 0x5004);	/* DMAPLUS | DPOLL | TXSTRTM*/
	CSR_WRITE(s, 5, 0x8000);	/* TOKINTD */

	dma = s->recvdma;

	/* setup recv buf for chip */
	for (i = 0; i < RRING_SIZE; i++)
	{
		s->rds[i].md0 = LE2(dma & DOUBLET_MASK);		/* LADR */
		s->rds[i].md1 = LE2((dma >> DOUBLET_SIZE) | OWN); /* FLAGS HADR */
		s->rds[i].md2 = LE2(-MAX_PACKET_SIZE);	/* <ones>:15..12 buf-count:11..0 */
		s->rds[i].md3 = 0; 					/* recv-len */
		dma += MAX_PACKET_SIZE;
	}

	dma = s->senddma;

	for (i = 0; i < TRING_SIZE; i++)
	{
		s->tds[i].md0 = LE2(dma & DOUBLET_MASK);		/* LADR */
		s->tds[i].md1 = LE2(dma >> DOUBLET_SIZE); /* FLAGS HADR */
		s->tds[i].md2 = LE2(-MAX_PACKET_SIZE);	/* <ones>:15..12 buf-count:11..0 */
		s->tds[i].md3 = 0; 					/* recv-len */
		dma += MAX_PACKET_SIZE;
	}

	/* setup init block */
	s->init->mode = 0;
	memcpy(s->init->padr, s->mac_addr, MAC_ADDR);
	memset(s->init->ladrf, 0xFF, sizeof s->init->ladrf);
	s->init->rdra = LE2(s->rdsdma & DOUBLET_MASK);
	s->init->rlen = LE2((s->rdsdma >> DOUBLET_SIZE) & BYTE_MASK);
	/* len = RRING_SIZE - 1 */
	s->init->rlen |= LE2(RRING_LEN << (BYTE_SIZE+5));
	s->init->tdra = LE2(s->tdsdma & DOUBLET_MASK);
	s->init->tlen = LE2((s->tdsdma >> DOUBLET_SIZE) & BYTE_MASK);
	/* len = TRING_SIZE - 1 */
	s->init->tlen |= LE2(TRING_LEN << (BYTE_SIZE+5));
	CSR_WRITE(s, 1, s->initdma & DOUBLET_MASK);
	CSR_WRITE(s, 2, s->initdma >> DOUBLET_SIZE);
	pci_dma_sync(s->devinfo->hbridge, (uPtr)s->init, (uPtr)s->initdma,
			sizeof *s->init);

	dump_regs(e, s);
	DELAY(1000);
	CSR_WRITE(s, 0, INIT);

	for (i = 0; i < 100000; i++)
	{
	    csr = CSR_READ(s, 0);

	    if (CSR_READ(s, 0) & IDON)
			break;

	    if (csr ^ prevcsr)
			DPRINTF(("CSR0 %#x, prev %#x\n", csr, prevcsr));

	    prevcsr = csr;
	    DELAY(10);
	}

	if (i >= 100000)
	{
		cprintf(e, "init command failed, status = %#x\n", csr);
		return 0;
	}

	DPRINTF(("CSR0 = %#x\n", CSR_READ(s, 0)));

	CSR_WRITE(s, 0, STRT);
	return 1;
}

C(f_lance_open)			/* open (-- okay?) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Bool promiscuous = FALSE;
	Int speed = 0;			/* auto */
	Bool duplex = FALSE;
	Retcode ret;
	Pci_device_info *devinfo;
	Self *s;
	Entry *ent;
	Byte *paddr;
	Int plen, cfg, maclen;
	uByte *macaddr;
	Cell c;
	char *name;
	char *num;
	int rev;

	DPRINTF(("lance_open: pkg=%p\n", pkg));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->devinfo = devinfo;

	/* decode the assigned-addresses property to find the PCI address and size
	   of the CSR registers, which should be mapped into both I/O and memory
	  */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("lance_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	do
	{
		ret = pci_decode_reg_prop(&paddr, &plen, &s->physhi, &s->physmid,
				&s->physlo, &s->sizehi, &s->sizelo);

		if (ret != NO_ERROR)
			return ret;
	}
	while (PCI_PHYSHI_SPACE(s->physhi) != PCI_SPACE_MEM);
	
	DPRINTF(("lance_open: physhi=%#x physmid=%#x physlo=%#x\n",
			s->physhi, s->physmid, s->physlo));
	ret = pci_map_in(devinfo->hbridge, s->physhi, s->physmid, s->physlo,
			IO_SIZE, &c);
	DPRINTF(("lance_open: pci_map_in, ret=%d csr=%#x\n", ret, c));

	if (ret != NO_ERROR)
		return ret;

	/* point to memory-mapped CSRs */
	s->csr = (uShort*)(uPtr)c;

	/* parse arguments - they are all optional - following args
	   are handed off to the obptftp package */
	if (inst->args && *inst->args)
	{
		Byte *str;
		Int len;
		Cell val, error;
		
		for (str = inst->args + 1; *str; str++)
		{
			while (isspace(*str))
				str++;
			
			if (*str == '\0')
				break;

			DPRINTF(("lance: args=\"%s\"\n", str));

			#define IS_END(c)	((c) == ',' || (c) == '\0')
			#define IS_MORE(c)	(!IS_END(c))
		
			if (compare_strs(str, 11, "promiscuous", 11) && IS_END(str[11]))
			{
				str += 11;
				promiscuous = TRUE;
			}
			else if (compare_strs(str, 6, "speed=", 6) && IS_MORE(str[6]))
			{
				str += 6;
				len = strlen(str);
				parse_number(10, &str, &len, &val, &error, FALSE);
				
				if (!error || (str[0] == ',' && val > 0))
				{
					switch (val)
					{
					case 10:
					case 100:
						speed = val;
						break;
						
					case 4:
					case 16:
					case 155:
					case 622:
					case 1000:
					default:
						cprintf(e, "lance: unsupported \"speed=%d\"\n", val);
						break;
					}
				}
			}
			else if (compare_strs(str, 7, "duplex=", 7) && IS_MORE(str[7]))
			{
				str += 7;

				if (compare_strs(str, 4, "half", 4) && IS_END(str[4]))
				{
					duplex = FALSE;
					str += 4;
				}
				else if (compare_strs(str, 4, "full", 4) && IS_END(str[4]))
				{
					duplex = TRUE;
					str += 4;
				}
				else
					cprintf(e, "lance: bad arg format for \"duplex=\"");
			}
			else
			{
				/* pass any remaining args to obptftp */
				break;
			}
			
			if (*str != ',')
				break;
		}
		
		/* remainder goes to obptftp */
		PUSH(e, (uPtr)str);		/* arg-str */
		PUSH(e, strlen(str));	/* arg-len */
	}
	else
	{
		PUSH(e, "");	/* arg-str */
		PUSH(e, 0);		/* arg-len */
	}

	/* enable access to memory and enable bus-mastering */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("lance_open: before cfg=%#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) |
					PCI_COMMAND_MEMENABLE |
					PCI_COMMAND_MASTERENABLE
			);

	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("lance_open: after cfg=%#x\n", cfg));

	/* allocate I/O packet buffers plus descriptors suitable for DMA */
	ret = pci_dma_alloc(devinfo->hbridge, DMA_BUF_SIZE, &s->dma);
	DPRINTF(("lance_open: pci_dma_alloc ret=%d dma=%#x\n", ret, s->dma));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, DMA_BUF_SIZE, 0, &s->busdma);
	DPRINTF(("lance_open: pci_dma_map_in ret=%d virt=%#x\n", ret, s->busdma));

	if (ret != NO_ERROR)
		return ret;

	setup_buffers(s);

	/* try to read in the local MAC addr out of ROM (if any) setting
	   parameter "local-mac-address" if present */
	ret = read_mac_addr(e, pkg, s, devinfo);
	DPRINTF(("lance_open: read_mac_addr ret=%d\n", ret));
	DPRINTF(("read_mac_addr: s->mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
				s->mac_addr[0], s->mac_addr[1], s->mac_addr[2],
				s->mac_addr[3], s->mac_addr[4], s->mac_addr[5]));

	/* create an instance of /packages/obptftp to support "load" -
	   must do this after we read in and set our local MAC addr or the
	   package will get an incorrect value for it - note optional args
	   are already pushed on the stack from above */
	PUSH(e, "obp-tftp");			/* name-str */
	PUSH(e, strlen("obp-tftp"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("lance_open: $open-package ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("lance_open: obptftp=%p\n", s->obptftp));

	if (s->obptftp == NULL)
		return E_NULL_INSTANCE;

	ret = execute_word(e, "mac-address");

	if (ret != NO_ERROR)
		return ret;
	
	POP(e, maclen);
	POPT(e, macaddr, uByte*);

	if (maclen != MAC_ADDR)
		return E_ABORT;

	/* use this returned value for our MAC addr */
	memcpy(s->mac_addr, macaddr, maclen);

	/* let the user know the MAC addr to help configure BOOTP */
	if (diag)
		cprintf(e, "Ethernet MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
				macaddr[0], macaddr[1], macaddr[2],
				macaddr[3], macaddr[4], macaddr[5]);

	if (diag)
		cprintf(e, "DMA buffer virtual %#x, physical %#x\n", s->dma, s->busdma);

	dump_regs(e, s);

	/* reset the chip */
	(void)IO_READ(s, RESET);
	IO_WRITE(s, RESET, 1);		/* for NE2100s? */
	DELAY(10);

	if (get_chipid(e, s, &name, &num, &rev))
	{
		if (diag)
			cprintf(e, "%s (%s), rev %d.\n", name, num, rev);
	}
	else
	{
		if (diag)
			cprintf(e, "Unknown NE2100 part, %#x\n", rev);
	}

	s->duplex = duplex;
	s->speed = speed;

	/* init CSRs and BCRs */
	cfg = BCR_READ(s, 2);

	if (s->chip != CHIP_79C973 && s->chip != CHIP_79C973)
	    cfg |= 0x06;	/* AWAKE | ASEL */

	BCR_WRITE(s, 2, cfg);	

	cfg = BCR_READ(s, 32) & ~0x38;	/* !XPHYANE !XPHYFD !XPHYSP */

	if (duplex)
	{
		BCR_WRITE(s, 9, 0x3);	/* AUIFD | FDEN */
		cfg |= 0x10;			/* XPHYFD */
	}

	if (speed == 100)
		cfg |= 0x08;			/* XPHYSP */
	else if (speed != 10)
		cfg |= 0x20;			/* XPHYANE */

	BCR_WRITE(s, 32, cfg);

	CSR_WRITE(s, 3, 0x5E00);	/* BABLM|MISSM|MEERM|RINTM|TINTM*/
	CSR_WRITE(s, 4, 0x5004);	/* DMAPLUS | DPOLL | TXSTRTM*/
	CSR_WRITE(s, 5, 0x8000);	/* TOKINTD */

	if (!reset_lance(e, s))
	{
		PUSH(e, FFALSE);
		return E_ABORT;
	}

	DPRINTF(("CSR0 = %#x\n", CSR_READ(s, 0)));

	CSR_WRITE(s, 0, STRT);

	dump_regs(e, s);

	DPRINTF(("lance_open: done: ret=%d\n", ret));
	PUSH(e, FTRUE);
	return ret;
}

C(f_lance_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	Retcode ret;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	/* reset the chip */
	(void)CSR_READ(s, RESET);
	DELAY(10);

	if (s->dma)
	{
		ret = pci_dma_map_out(devinfo->hbridge, (uPtr)s->dma,
				s->busdma, DMA_BUF_SIZE);

		if (ret == NO_ERROR)
			ret = pci_dma_free(devinfo->hbridge, s->dma, DMA_BUF_SIZE);


		if (ret != NO_ERROR)
			return ret;
	}

	IFCKSP(e, 0, 1);
	PUSH(e, (uPtr)s->obptftp);
	return execute_word(e, "close-package");
}

#if 0
C(f_lance_dma_alloc)		/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("lance_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_lance_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("lance_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}
#endif

#ifdef DEBUG
C(f_membar_read)			/* membar@ (reg -- value) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 1, 1);
	POP(e, reg);

	DPRINTF(("membar@: reg=%d\n", reg));

	if (reg < 0x10)
	    val = IO_READ1(s, reg);
	else
	    val = IO_READ(s, reg);

	PUSH(e, val);
	return NO_ERROR;
}

C(f_membar_write)			/* membar! (val reg --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 2, 0);
	POP(e, reg);
	POP(e, val);

	DPRINTF(("membar!: reg=%d\n", reg));

	if (reg >= 0x10)
	    IO_WRITE(s, reg, val);

	return NO_ERROR;
}

C(f_csr_read)			/* csr@ (reg -- value) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 1, 1);
	POP(e, reg);

	DPRINTF(("csr@: reg=%d\n", reg));
	val = CSR_READ(s, reg);
	PUSH(e, val);
	return NO_ERROR;
}

C(f_csr_write)			/* csr! (val reg --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 2, 0);
	POP(e, reg);
	POP(e, val);

	DPRINTF(("csr!: reg=%d\n", reg));
	CSR_WRITE(s, reg, val);
	return NO_ERROR;
}

C(f_bcr_read)			/* bcr@ (reg -- value) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 1, 1);
	POP(e, reg);

	DPRINTF(("bcr@: reg=%d\n", reg));
	val = BCR_READ(s, reg);
	PUSH(e, val);
	return NO_ERROR;
}

C(f_bcr_write)			/* bcr! (val reg --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt reg;
	uInt val;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;

	IFCKSP(e, 2, 0);
	POP(e, reg);
	POP(e, val);

	DPRINTF(("bcr!: reg=%d\n", reg));
	BCR_WRITE(s, reg, val);

	return NO_ERROR;
}
#endif

static char *
md3_error_str(uInt md3)
{
	if (md3 & LCAR)
		return "lost carrier";

	if (md3 & UFLO)
		return "silo underflow";

	if (md3 & LCOL)
		return "late collision";

	if (md3 & RTRY)
		return "excessive retried";

	if (md3 & TBUFF)
		return "buffer error";
	
	return "error 2000";
}

static char *
md1_error_str(uInt md1)
{
	if (md1 & OFLO)
		return "Silo overflow";

	if (md1 & RBUFF)
		return "Buffer error";

	if (md1 & FRAM)
		return "Framing error";

	if (md1 & CRC)
		return "CRC error";

	if (md1 & MDERR)
		return "receive error";

	return "error unknown";
}

C(f_lance_read)				/* read (addr len -- actual) */
{
	Int len, actual = -2;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	struct mds *rds = &s->rds[s->rdsindex];
	Byte *recvbuf = s->recvbuf + s->rdsindex * MAX_PACKET_SIZE;
	uInt recvdma = s->recvdma + s->rdsindex * MAX_PACKET_SIZE;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->rds, (uPtr)s->rdsdma, sizeof s->rds);

#ifdef DEBUG
	DPRINTF(("lance_read: addr=%p len=%d\n", addr, len));
	DPRINTF(("lance_read: rdsindex=%d, recvdma=%#x, recvbuf=%#x\n", s->rdsindex,
			recvdma, recvbuf));

	{
		int i;

		for (i = 0; i < RRING_SIZE; i++)
			DPRINTF(("lance_read: rds[%d]: md0 %#x, md1 %#x, md2 %#x, md3 %#x\n",
					i, LE2(s->rds[i].md0), LE2(s->rds[i].md1), LE2(s->rds[i].md2),
					LE2(s->rds[i].md3)));
	}
#endif

	/* see if we have an error */
	if (rds->md1 & LE2(MDERR))
	{
		cprintf(e, "amd_lance: packet-recv error: %s (%#x)\n", 
				md1_error_str(LE2(rds->md1)), LE2(rds->md1));
		reset_lance(e, s);	/* kick it in the ass */
	}
	else if ((LE2(rds->md1) & (OWN | STP | ENP)) == (STP | ENP))
	{
		/* we have a packet */
		actual = LE2(rds->md3) & MCNT;
		
		if (actual < len)
			len = actual;
		
		pci_dma_sync(devinfo->hbridge, (uPtr)recvbuf, (uPtr)recvdma, actual);
		memcpy(addr, recvbuf, len);
#ifdef DEBUG
		dump_packet(e, recvbuf, actual);
#endif

		/* reset recv buf for chip */
		rds->md0 = LE2(recvdma & DOUBLET_MASK);		/* LADR */
		rds->md2 = LE2(-MAX_PACKET_SIZE);
		rds->md3 = 0; 					/* recv-len */
		rds->md1 = LE2((recvdma >> DOUBLET_SIZE) | OWN); /* FLAGS HADR */
		CSR_WRITE(s, 0, STRT);	/* re-start if needed */

		s->rdsindex++;

		if (s->rdsindex == RRING_SIZE)
			s->rdsindex = 0;
	}

	DPRINTF(("lance_read: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

#ifdef DEBUG
void
dumpbuf(Environ *e, uByte *buf, Int len)
{
    Int i;

    for (i = 0; i < len; i++)
    {
	if ((i & 0xF) == 0)
	    cprintf(e, "%04X: ", i);

	if ((i & 3) == 0)
	    cprintf(e, " ");

	cprintf(e, " %02X", buf[i]);

	if ((i & 0xF) == 0xF)
	    cprintf(e, "\n");
    }

    if (i & 0xF)
	cprintf(e, "\n");
}
#endif

C(f_lance_write)			/* write (addr len -- actual) */
{
	Int len, actual = 0;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	uInt csr;
	uInt prevcsr;
	uInt prevstat;
	int i;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("lance_write: addr=%p len=%d\n", addr, len));

	if (len > MAX_PACKET_SIZE)
		len = MAX_PACKET_SIZE;

	/* copy in the data and setup tds */
	memcpy(s->sendbuf, addr, len);
#ifdef DEBUG
	DPRINTF(("Transmit buffer: len=%d\n", len));
	dumpbuf(e, (uByte *)s->sendbuf, len);
#endif
	s->tds->md0 = LE2(s->senddma & DOUBLET_MASK);		/* LADR */
	s->tds->md2 = LE2(-len);	/* <ones>:15..12 buf-count:11..0 */
	s->tds->md3 = 0; 					/* tx-flags */
	s->tds->md1 = LE2((s->senddma >> DOUBLET_SIZE) | OWN | STP | ENP);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma, len);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->tds, (uPtr)s->tdsdma, sizeof s->tds);

	CSR_WRITE(s, 0, TDMD | STRT);
	DELAY(1000);

	prevcsr = 0;
	prevstat = 0;

	for (i = 0; i < 100000; i++)
	{
		uShort md1 = s->tds->md1;
		uInt stat = pci_config_read(devinfo->hbridge, devinfo->bus,
				devinfo->dev, devinfo->func, 0x4);
		csr = CSR_READ(s, 0);

#ifdef DEBUG
		if (prevcsr ^ csr)
		{
			char buf[256];
			pci_format_csr(buf, 256, stat);
			DPRINTF(("waiting, csr %#x, md1 %#x, stat %#x %s\n", csr, md1, stat, buf));
		}

		if (prevstat ^ stat)
		{
			char buf[256];
			pci_format_csr(buf, 256, stat);
			DPRINTF(("waiting, stat %#x %s, csr %#x, md1 %#x\n", stat, buf, csr, md1));
		}
#endif

		if ((csr & STOP) || !(csr & STRT))
		{
			DPRINTF(("Packet engine stoped unexpectedly, %#x\n", csr));
			dump_regs(e, s);
			PUSH(e, 0);
			return NO_ERROR;
		}

		if (!(md1 & LE2(OWN | DEF)))
			break;

		DELAY(10);
		pci_dma_sync(devinfo->hbridge, (uPtr)s->tds, (uPtr)s->tdsdma,
				sizeof s->tds);
		prevcsr = csr;
		prevstat = stat;
	}

	if (s->tds->md3 & LE2(TBUFF | UFLO | LCOL	| LCAR	| RTRY))
		cprintf(e, "amd_lance: transmit-error %s (%#x)\n", 
			md3_error_str(LE2(s->tds->md3)), LE2(s->tds->md3));

	if ((s->tds->md1 & LE2(DEF)) == 0)
		actual = len;

	DPRINTF(("lance_write: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

static Retcode
do_obptftp_method(Environ *e, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	if (!s->obptftp)
		return E_NULL_INSTANCE;

	/* let the TFTP/BOOTP package do the work */
	return execute_method_name(e, s->obptftp, method, CSTR);
}

C(f_lance_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_lance_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

/*
	0x0001=0  0x0002=1  0x0004=2  0x0008=3
	0x0010=4  0x0020=5  0x0040=6  0x0080=7
	0x0100=8  0x0200=9  0x0400=10 0x0800=11
	0x1000=12 0x2000=13 0x4000=14 0x8000=15
*/

uShort csr_test_rw_mask[128] =
{
	0x0040, 0xFFFF, 0xFFFF, 0x5F7F, 0x7D15, 0xC56E, 0x0000, 0x0000,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0400, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
	0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000,
	0x3F00, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x00FF, 0x0FFF, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

uShort csr_test_ro_mask[128] =
{
	0x0000, 0x0000, 0x0000, 0xA080, 0x0000, 0x0000, 0x00FF, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0FFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

uShort csr_test_ro_value[128] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

uShort bcr_test_rw_mask[64] =
{
	0x0000, 0x0000, 0x41BE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x000F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x1000, 0x001E, 0x0083, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

uShort bcr_test_ro_mask[64] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

uShort bcr_test_ro_value[64] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

C(f_lance_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Pci_device_info *devinfo;
	Retcode ret;
	Self self;
	Self *s = &self;
	Entry *ent;
	uByte macaddr[6];
	Byte *paddr;
	Int plen, cfg;
	Cell c;
	struct init_block *init;
	char *name;
	char *num;
	int rev;
	int errs = 0;
	int oerrs = 0;
	int i;
	uInt t0;
	uInt t2;
	uInt t3;
	uInt csr = 0;
	uInt prevcsr = -1;
	uInt prevstat = -1;
	int len;

	DPRINTF(("ether selftest: begin\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	DPRINTF(("ether selftest: got pkg\n"));
	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	DPRINTF(("ether selftest: got devinfo\n"));

	/* decode the assigned-addresses property to find the PCI address and size
	   of the CSR registers, which should be mapped into both I/O and memory
	  */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("lance_selftest: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	do
	{
		ret = pci_decode_reg_prop(&paddr, &plen, &s->physhi, &s->physmid,
				&s->physlo, &s->sizehi, &s->sizelo);

		if (ret != NO_ERROR)
			return ret;
	}
	while (PCI_PHYSHI_SPACE(s->physhi) != PCI_SPACE_MEM);
	
	DPRINTF(("lance_selftest: physhi=%#x physmid=%#x physlo=%#x\n",
			s->physhi, s->physmid, s->physlo));
	ret = pci_map_in(devinfo->hbridge, s->physhi, s->physmid, s->physlo,
			IO_SIZE, &c);
	DPRINTF(("lance_selftest: pci_map_in, ret=%d csr=%#x\n", ret, c));

	if (ret != NO_ERROR)
		return ret;

	/* point to memory-mapped CSRs */
	s->csr = (uShort*)(uPtr)c;

	/* enable access to memory and enable bus-mastering */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("lance_selftest: before cfg=%#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) |
					PCI_COMMAND_MEMENABLE |
					PCI_COMMAND_MASTERENABLE
			);

	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("lance_selftest: after cfg=%#x\n", cfg));

	/* reset the chip */
	(void)IO_READ(s, RESET);
	IO_WRITE(s, RESET, 1);		/* for NE2100s? */
	DELAY(10);
	cfg = CSR_READ(s, 0);

	/* Test the RAP register */
	t0 = IO_READ(s, RAP);
	IO_WRITE(s, RAP, 0xFFFF);
	t2 = IO_READ(s, RAP);
	IO_WRITE(s, RAP, 0x0000);
	t3 = IO_READ(s, RAP);
	IO_WRITE(s, RAP, t0);

	if (t2 != 0xFF && t3 != 0x00)
	{
		cprintf(e, "RAP register test failed, read (%#x, %#x) expected (0xFF,0x0)\n", t2, t3);
		errs++;
	}

	if (diag)
		cprintf(e, "Testing chip ID register -- ");

	/* Check for a valid device ID */
	if (get_chipid(e, s, &name, &num, &rev))
	{
		if (diag)
			cprintf(e, "%s (%s), rev %d.\n", name, num, rev);
	}
	else
		errs++;

	if (diag)
		cprintf(e, "Testing CSR registers -- ");

	oerrs = errs;

	/* Test the CSR registers */
	for (i = 0; i < 128; i++)
	{
		uShort mask = csr_test_rw_mask[i];
		uShort romask = csr_test_ro_mask[i];
		uShort roval = csr_test_ro_value[i];
		uShort save = CSR_READ(s, i);

		if (mask)
		{
			CSR_WRITE(s, i, mask | (save & ~mask));		/* write ones */
			t2 = CSR_READ(s, i);
			CSR_WRITE(s, i, save & ~mask);				/* write zeros */
			t3 = CSR_READ(s, i);
			CSR_WRITE(s, i, save);						/* restore */
		}
		else if (romask)
		{
			t2 = CSR_READ(s, i);
			t3 = t2;
		}
		else
			continue;

		if ((t2 & mask) != mask || (t3 & mask) != 0)
		{
			cprintf(e, "CSR%d register test failed, read %#x,%#x expected %#x,%#x(%#x)\n",
					i, t2, t3, mask | (save & ~mask), save & ~mask,
					(t2 & mask) & (~t3 & mask));
			errs++;
		}

		if ((t2 & romask) != roval || (t3 & romask) != roval)
		{
			cprintf(e, "CSR%d register test failed, read %#x,%#x expected %#x\n",
					i, t2 & romask, t2 & romask, roval);
			errs++;
		}
	}

	if (diag && oerrs == errs)
		cprintf(e, "PASSED\n");

	if (diag)
		cprintf(e, "Testing BCR registers -- ");

	oerrs = errs;

	/* Test the BCR registers */
	for (i = 0; i < 64; i++)
	{
		uShort mask = bcr_test_rw_mask[i];
		uShort romask = bcr_test_ro_mask[i];
		uShort roval = bcr_test_ro_value[i];
		uShort save = BCR_READ(s, i);

		if (mask)
		{
			BCR_WRITE(s, i, mask | (save & ~mask));		/* write ones */
			t2 = BCR_READ(s, i);
			BCR_WRITE(s, i, save & ~mask);				/* write zeros */
			t3 = BCR_READ(s, i);
			BCR_WRITE(s, i, save);						/* restore */
		}
		else if (romask)
		{
			t2 = BCR_READ(s, i);
			t3 = t2;
		}
		else
			continue;

		if ((t2 & mask) != mask || (t3 & mask) != 0)
		{
			cprintf(e, "BCR%d register test failed, read %#x,%#x expected %#x,%#x(%#x)\n",
					i, t2, t3, mask | (save & ~mask), save & ~mask,
					(t2 & mask) & (~t3 & mask));
			errs++;
		}

		if ((t2 & romask) != roval || (t3 & romask) != roval)
		{
			cprintf(e, "BCR%d register test failed, read %#x,%#x expected %#x\n",
					i, t2 & romask, t2 & romask, roval);
			errs++;
		}
	}

	if (diag && oerrs == errs)
		cprintf(e, "PASSED\n");

	/* reset the chip again */
	(void)IO_READ(s, RESET);
	IO_WRITE(s, RESET, 1);		/* for NE2100s? */
	DELAY(10);
	cfg = CSR_READ(s, 0);

	if (!diag)
	{
		if (errs)
			PUSH(e, E_SELFTEST_FAILED);
		else
			PUSH(e, 0);

		return NO_ERROR;
	}

	cprintf(e, "ether selftest\n");

	if (diag)
		cprintf(e, "Testing chip initialization -- ");

	oerrs = errs;

	/* setup dma buffer */
	/* allocate I/O packet buffers plus descriptors suitable for DMA */
	ret = pci_dma_alloc(devinfo->hbridge, DMA_BUF_SIZE, &s->dma);
	DPRINTF(("lance_selftest: pci_dma_alloc ret=%d dma=%#x\n", ret, s->dma));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, DMA_BUF_SIZE, 0, &s->busdma);
	DPRINTF(("lance_selftest: pci_dma_map_in ret=%d virt=%#x\n", ret, s->busdma));

	if (ret != NO_ERROR)
		return ret;

	/* setup our buffers
	  - sizes are such that align requirements should work out automagically
	 */
	s->recvbuf = (Byte*)(uPtr)s->dma;
	memset(s->recvbuf, 0, DMA_BUF_SIZE);	/* init everything to zero */
	s->recvdma = (uInt)s->busdma;
	s->sendbuf = s->recvbuf + MAX_PACKET_SIZE;
	s->senddma = s->recvdma + MAX_PACKET_SIZE;
	s->rds = (struct mds*)(s->sendbuf + MAX_PACKET_SIZE);
	s->rdsdma = s->senddma + MAX_PACKET_SIZE;
	s->tds = s->rds + NUM_DESCRIPTORS;
	s->tdsdma = s->rdsdma + NUM_DESCRIPTORS * sizeof *s->rds;
	s->init = (struct init_block*)(s->tds + NUM_DESCRIPTORS);
	s->initdma = s->tdsdma + NUM_DESCRIPTORS * sizeof *s->tds;
	DPRINTF(("lance_selftest: recvbuf/dma=%p/%#x sendbuf/dma=%p/%#x\n",
			s->recvbuf, s->recvdma, s->sendbuf, s->senddma));
	DPRINTF(("            tds=%p/%#x rds=%p/%#x\n",
			s->tds, s->tdsdma, s->rds, s->rdsdma));

	/* set a reasonable MAC address */
	macaddr[0] = 0x00;
	macaddr[1] = 0x90;
	macaddr[2] = 0x27;
	macaddr[3] = 0x37;
	macaddr[4] = 0x0E;
	macaddr[5] = 0xE7;

	/* Init the chip */
	/* init CSRs and BCRs */

	cfg = BCR_READ(s, 2);
	if (s->chip != CHIP_79C973 && s->chip != CHIP_79C973)
	    cfg |= 0x06;	/* AWAKE | ASEL */
	BCR_WRITE(s, 2, cfg);	

	cfg = BCR_READ(s, 32);

/*	CSR_WRITE(s, 3, 0x5F00);	/ * BABLM|MISSM|MEERM|RINTM|TINTM|IDONM */
	CSR_WRITE(s, 3, 0x5E00);	/* BABLM|MISSM|MEERM|RINTM|TINTM*/

	CSR_WRITE(s, 4, 0x5004);	/* DMAPLUS | DPOLL | TXSTRTM*/
	CSR_WRITE(s, 5, 0x8000);	/* TOKINTD */
	CSR_WRITE(s, 12, 0xFFFF);	/* PHYSADDR */
	CSR_WRITE(s, 13, 0xFFFF);
	CSR_WRITE(s, 14, 0xFFFF);

	CSR_WRITE(s, 16, 0xFFFF);
	CSR_WRITE(s, 17, 0xFFFF);
	CSR_WRITE(s, 18, 0xFFFF);
	CSR_WRITE(s, 19, 0xFFFF);
	CSR_WRITE(s, 20, 0xFFFF);
	CSR_WRITE(s, 21, 0xFFFF);
	CSR_WRITE(s, 24, 0xFFFF);
	CSR_WRITE(s, 25, 0xFFFF);
	CSR_WRITE(s, 26, 0xFFFF);
	CSR_WRITE(s, 27, 0xFFFF);
	CSR_WRITE(s, 28, 0xFFFF);
	CSR_WRITE(s, 29, 0xFFFF);
	CSR_WRITE(s, 30, 0xFFFF);
	CSR_WRITE(s, 31, 0xFFFF);
	CSR_WRITE(s, 34, 0xFFFF);
	CSR_WRITE(s, 35, 0xFFFF);

/*	dump_regs(e, s);	*/

	/* setup recv buf for chip */
	s->rds->md0 = LE2(s->recvdma & DOUBLET_MASK);		/* LADR */
	s->rds->md1 = LE2((s->recvdma >> DOUBLET_SIZE) | OWN); /* FLAGS HADR */
	s->rds->md2 = LE2(-MAX_PACKET_SIZE);	/* <ones>:15..12 buf-count:11..0 */
	s->rds->md3 = 0; 					/* recv-len */

	s->tds->md0 = LE2(s->senddma & DOUBLET_MASK);		/* LADR */
	s->tds->md1 = LE2((s->senddma >> DOUBLET_SIZE) | STP | ENP); /* FLAGS HADR */
	s->tds->md2 = LE2(-1);	/* <ones>:15..12 buf-count:11..0 */
	s->tds->md3 = 0; 					/* tx-flasg */

	/* setup init block */
	s->init->mode = 0;
	memcpy(s->init->padr, macaddr, MAC_ADDR);
	memset(s->init->ladrf, 0xFF, sizeof init->ladrf);
	s->init->rdra = LE2(s->rdsdma & DOUBLET_MASK);
	s->init->rlen = LE2((s->rdsdma >> DOUBLET_SIZE) & BYTE_MASK);	/* len==0 */
	s->init->tdra = LE2(s->tdsdma & DOUBLET_MASK);
	s->init->tlen = LE2((s->tdsdma >> DOUBLET_SIZE) & BYTE_MASK);	/* len==0 */
	CSR_WRITE(s, 1, s->initdma & DOUBLET_MASK);
	CSR_WRITE(s, 2, s->initdma >> DOUBLET_SIZE);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->init, (uPtr)s->initdma,
			sizeof *s->init);

/*	dumpbuf(e, (uByte *)s->init, 64);	*/
	(void)CSR_READ(s, 0);

	if (CSR_READ(s, 0) & IDON)
		DPRINTF(("IDON before INIT\n"));

	DELAY(10);
	CSR_WRITE(s, 0, INIT);
	DELAY(10);

	for (i = 0; i < 1000; i++)
	{
	    csr = CSR_READ(s, 0);

	    if (CSR_READ(s, 0) & IDON)
		break;

	    if (csr ^ prevcsr)
		DPRINTF(("CSR0 %#x, prev %#x\n", csr, prevcsr));

	    prevcsr = csr;
	    DELAY(10);
	}

	if (i >= 1000)
	{
		cprintf(e, "init command failed, status = %#x\n", csr);
		errs++;
	}

	CSR_WRITE(s, 0, TDMD | STRT);	/* let the engine read the descriptors */
	DELAY(1000);
	CSR_WRITE(s, 0, STOP);		/* stop so we can read the regs */
	DELAY(1000);

/*	dump_regs(e, s);	*/

	/* verify registers were intialized correctly */
	t0 = CSR_READ(s, 12);

	if ((t0 & 0xFF) != macaddr[0] || ((t0 >> 8) & 0xFF) != macaddr[1])
	{
		cprintf(e, "CSR12 failed init, %#x expected %#x\n", t0,
				(macaddr[1] << 8) | macaddr[0]);
		errs++;
	}

	t0 = CSR_READ(s, 13);

	if ((t0 & 0xFF) != macaddr[2] || ((t0 >> 8) & 0xFF) != macaddr[3])
	{
		cprintf(e, "CSR13 failed init, %#x expected %#x\n", t0,
				(macaddr[3] << 8) | macaddr[2]);
		errs++;
	}

	t0 = CSR_READ(s, 14);

	if ((t0 & 0xFF) != macaddr[4] || ((t0 >> 8) & 0xFF) != macaddr[5])
	{
		cprintf(e, "CSR14 failed init, %#x expected %#x\n", t0,
				(macaddr[5] << 8) | macaddr[4]);
		errs++;
	}

	t0 = (CSR_READ(s, 17) << 16) | CSR_READ(s, 16);

	if (t0 != s->initdma)	
	{
		cprintf(e, "CSR16/17 failed init, %#x expected %#x\n", t0, s->initdma);
		errs++;
	}

	t0 = (CSR_READ(s, 19) << 16) | CSR_READ(s, 18);

	if (t0 != (s->recvdma & 0xFFFFFF))
	{
		cprintf(e, "CSR18/19 failed init, %#x expected %#x\n", t0, s->recvdma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 21) << 16) | CSR_READ(s, 20);

	if (t0 != (s->senddma & 0xFFFFFF))
	{
		cprintf(e, "CSR20/21 failed init, %#x expected %#x\n", t0, s->senddma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 25) << 16) | CSR_READ(s, 24);

	if (t0 != (s->rdsdma & 0xFFFFFF))
	{
		cprintf(e, "CSR24/25 failed init, %#x expected %#x\n", t0, s->rdsdma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 27) << 16) | CSR_READ(s, 26);

	if (t0 != (s->rdsdma & 0xFFFFFF))
	{
		cprintf(e, "CSR26/27 failed init, %#x expected %#x\n", t0, s->rdsdma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 29) << 16) | CSR_READ(s, 28);

	if (t0 != (s->rdsdma & 0xFFFFFF))
	{
		cprintf(e, "CSR28/29 failed init, %#x expected %#x\n", t0, s->rdsdma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 31) << 16) | CSR_READ(s, 30);

	if (t0 != (s->tdsdma & 0xFFFFFF))
	{
		cprintf(e, "CSR30/31 failed init, %#x expected %#x\n", t0, s->tdsdma & 0xFFFFFF);
		errs++;
	}

	t0 = (CSR_READ(s, 35) << 16) | CSR_READ(s, 34);

	if (t0 != (s->tdsdma & 0xFFFFFF))
	{
		cprintf(e, "CSR34/35 failed init, %#x expected %#x\n", t0, s->tdsdma & 0xFFFFFF);
		errs++;
	}

	if (diag && oerrs == errs)
		cprintf(e, "PASSED\n");

	if (diag)
		cprintf(e, "Testing loopback operation -- ");

	oerrs = errs;

	/* set up loop back mode */
	t0 = CSR_READ(s, 15);		/* Set MODE register */
	CSR_WRITE(s, 15, t0 | LOOP | INTL | DTX);

	/* send a packet */
	/* copy in the data and setup tds */
	memcpy(s->sendbuf, macaddr, 6);		/* source address */
	memcpy(&s->sendbuf[6], macaddr, 6);	/* dest address */
	s->sendbuf[12] = 8;
	s->sendbuf[13] = 0;
	len = 64;

	for (i = 14; i < len; i++)
		s->sendbuf[i] = i;

#ifdef DEBUG
	dumpbuf(e, s->sendbuf, len);
#endif

	s->tds->md0 = LE2(s->senddma & DOUBLET_MASK);		/* LADR */
	s->tds->md2 = LE2(-len);	/* <ones>:15..12 buf-count:11..0 */
	s->tds->md3 = 0; 					/* tx-flags */
	s->tds->md1 = LE2((s->senddma >> DOUBLET_SIZE) | OWN | STP | ENP);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma, len);

	CSR_WRITE(s, 0, TDMD | STRT);
	DELAY(1000);

	prevcsr = 0;
	prevstat = 0;

	for (i = 0; i < 10000; i++)
	{
		char buf[256];
		uShort md1;
		uInt stat;

		csr = CSR_READ(s, 0);

		if ((csr & STOP) || !(csr & STRT))
		{
			cprintf(e, "Packet engine stoped unexpectedly, %#x\n", csr);
			errs++;
			break;
		}

		md1 = s->tds->md1;
		stat = pci_config_read(devinfo->hbridge, devinfo->bus,
				devinfo->dev, devinfo->func, 0x4);

		if (!(md1 & LE2(OWN | DEF)))
			break;	/* The packet got sent */

		if (prevcsr ^ csr)
		{
			pci_format_csr(buf, 256, stat);
			DPRINTF(("waiting, csr %#x, md1 %#x, stat %#x %s\n", csr, md1, stat, buf));
		}
		else if (prevstat ^ stat)
		{
			pci_format_csr(buf, 256, stat);
			DPRINTF(("waiting, stat %#x %s, csr %#x, md1 %#x\n", stat, buf, csr, md1));
		}

		pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma, len);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->init, (uPtr)s->initdma,
			sizeof *s->init);
		DELAY(10);
		prevcsr = csr;
		prevstat = stat;
	}

	DELAY(1000);

	if (s->tds->md3 & LE2(TBUFF | UFLO | LCOL | LCAR | RTRY))
	{
		cprintf(e, "amd_lance: transmit-error %#x\n", LE2(s->tds->md3));
		errs++;
	}
	if (s->tds->md1 & LE2(DEF))
	{
		cprintf(e, "amd_lance: transmit-error md1=%#x\n", LE2(s->tds->md1));
		errs++;
	}

	/* see if we have an error */
	if (s->rds->md1 & LE2(MDERR))
	{
		cprintf(e, "amd_lance: packet-recv error: %#x\n", LE2(s->rds->md1));
		errs++;
	}
	else if ((LE2(s->rds->md1) & (OWN | STP | ENP)) == (STP | ENP))
	{
		/* we have a packet */
		len = LE2(s->rds->md3) & MCNT;
#ifdef DEBUG
		dumpbuf(e, s->recvbuf, len);
#endif

		if (len - 4 != 64)
		{
			cprintf(e, "Send/Receive packet length mismatch, %d, 64\n", len);
			errs++;
		}
		
		pci_dma_sync(devinfo->hbridge, (uPtr)s->recvbuf,
				(uPtr)s->recvdma, len);

		if (memcmp(s->recvbuf, macaddr, 6) != 0)
		{
			cprintf(e, "Send/Receive packet src mac address does not match\n");
			errs++;
		}
		else if (memcmp(&s->recvbuf[6], macaddr, 6) != 0)
		{
			cprintf(e, "Send/Receive packet dest mac address does not match\n");
			errs++;
		}
		else
		{
			for (i = 14; i < len - 4; i++)
				if (s->recvbuf[i] != i)
				{
					cprintf(e, "Send/Receive packet do not match, offset %d, %#x, %#x\n", i, s->recvbuf[i], i);
					errs++;
					break;
				}
		}

		/* reset recv buf for chip */
		s->rds->md0 = LE2(s->recvdma & DOUBLET_MASK);		/* LADR */
		s->rds->md2 = LE2(-MAX_PACKET_SIZE);
		s->rds->md3 = 0; 					/* recv-len */
		s->rds->md1 = LE2((s->recvdma >> DOUBLET_SIZE) | OWN); /* FLAGS HADR */
		CSR_WRITE(s, 0, STRT);	/* re-start if needed */
	}
	else
	{
		cprintf(e, "amd_lance: no packet received durring loopback: %#x\n",
			LE2(s->rds->md1));
		errs++;
	}

	/* turn off loop back mode */
	CSR_WRITE(s, 15, t0 & ~(LOOP | INTL | DTX));

	if (diag && oerrs == errs)
		cprintf(e, "PASSED\n");

	if (errs)
		PUSH(e, E_SELFTEST_FAILED);
	else
		PUSH(e, 0);			/* successful */

	return NO_ERROR;
}

/*
 * Routines to read/write eeprom
 */

#define	ADDRESS_LENGTH	6		/* 93C46 16 bit wide ONLY */

/* EEPROM opcode definitions */

#define READ_OP 	0x2		/* 0b10 */
#define EWEN_OP 	0x0		/* 0b00 */
#define EWEN_ADR 	(0x3<<(ADDRESS_LENGTH-2))
#define ERASE_OP 	0x3		/* 0b11 */
#define ERAL_OP 	0x0		/* 0b00 */
#define WRITE_OP 	0x1		/* 0b01 */
#define WRAL_OP 	0x0		/* 0b00 */
#define EWDS_OP 	0x0 		/* 0b00 */
#define EWDS_ADR 	0x0		/* 0b000000 */

/* uwire interface definitions */
#define BCR19_UWIRE_ON 	0x0010		/* sets EEN */
#define BCR19_ECS 	0x0014		/* sets EEN, ECS */
#define BCR19_ESK_HIGH 	0x0002		/* sets ESK */
#define BCR19_START 	0x0015		/* sets EEN, ECS, and EDI */
#define BCR19_STOP 	0x0000          /* no bits set */
#define BCR19_PREAD 	0x4000          /* sets PREAD only */
#define DELAY_TIME 	50              /* delay for uwire handshake. */

/* description of eeprom contents for various chips */
#define	DATA_MAC	1
#define	DATA_CSR	2
#define	DATA_BCR	3
#define	DATA_HWID	4
#define	DATA_USER	5
#define	DATA_LCSUM	6
#define	DATA_WW		7
#define	DATA_HCSUM	8
#define	DATA_FILLER	9
#define	DATA_CONSTANT	10

struct prom_field
{
	int  type;
	int  addr;
};

typedef struct prom_field Prom_field;

Prom_field  am79c970A_prom[] =
{
	{ DATA_MAC, 0 },
	{ DATA_FILLER, 0 },
	{ DATA_FILLER, 0 },
	{ DATA_CONSTANT, 0 },
	{ DATA_HWID, 0 },
	{ DATA_USER, 0 },
	{ DATA_LCSUM, 0 },
	{ DATA_WW, 0 },
	{ DATA_BCR, 4 },
	{ DATA_BCR, 5 },
	{ DATA_BCR, 18 },
	{ DATA_BCR, 2 },
	{ DATA_BCR, 6 },
	{ DATA_BCR, 7 },
	{ DATA_BCR, 9 },
	{ DATA_HCSUM, 0 },
	{ DATA_BCR, 22 },
	{ DATA_CONSTANT, 0 },
	{ 0, 0 }
};

Prom_field  am79c973_prom[] =
{
	{ DATA_MAC, 0 },
	{ DATA_FILLER, 0 },
	{ DATA_FILLER, 0 },
	{ DATA_CSR, 116 },
	{ DATA_HWID, 0 },
	{ DATA_USER, 0 },
	{ DATA_LCSUM, 0 },
	{ DATA_WW, 0 },
	{ DATA_BCR, 2 },
	{ DATA_BCR, 4 },
	{ DATA_BCR, 5 },
	{ DATA_BCR, 6 },
	{ DATA_BCR, 7 },
	{ DATA_BCR, 9 },
	{ DATA_BCR, 18 },
	{ DATA_BCR, 22 },
	{ DATA_BCR, 23 },
	{ DATA_BCR, 24 },
	{ DATA_BCR, 25 },
	{ DATA_BCR, 26 },
	{ DATA_BCR, 27 },
	{ DATA_BCR, 32 },
	{ DATA_BCR, 33 },
	{ DATA_BCR, 35 },
	{ DATA_BCR, 36 },
	{ DATA_BCR, 37 },
	{ DATA_BCR, 38 },
	{ DATA_BCR, 39 },
	{ DATA_BCR, 40 },
	{ DATA_BCR, 41 },
	{ DATA_BCR, 42 },
	{ DATA_BCR, 43 },
	{ DATA_BCR, 48 },
	{ DATA_BCR, 49 },
	{ DATA_BCR, 50 },
	{ DATA_BCR, 51 },
	{ DATA_BCR, 52 },
	{ DATA_BCR, 53 },
	{ DATA_BCR, 54 },
	{ DATA_HCSUM, 0 },
	{ 0, 0 }
};

static Prom_field *
ee_get_fields(void *s)
{
	int cfg;

	cfg = CSR_READ(s, 89) << 16;
	cfg |= CSR_READ(s, 88);

	switch ((cfg >> 12) & 0xFFFF)
	{
	case 0x2621:
		return am79c970A_prom;
	case 0x2625:
		return am79c973_prom;
	case 0x2623:				/* Am79C971 */
	case 0x2624:				/* Am79C972 */
	case 0x0242:				/* Am79C970 */
	case 0x2627:				/* Am79C975 */
	default:
		break;
	}

	return NULL;
}

uDoublet  
uwire_in(void *device)
{
	/* EEEN high, EECS high, SK low */
	BCR_WRITE(device, 19, BCR19_ECS);
	DELAY(1);			/* Tcss=100ns, SK=1us. */

	/* EEEN high, EECS high, SK high */
	BCR_WRITE(device, 19, BCR19_ECS | BCR19_ESK_HIGH);
	DELAY(1);			/* Tpdo=1000ns, SK=1us. */

	/* Get data bit. */
	return BCR_READ(device, 19) & 0x0001;
}

void    
uwire_out(void *device, unsigned int value)
{
	/* EEEN high, EECS high, SK low, data in bit 0. */
	value = value & 0x0015;
	BCR_WRITE(device, 19, value);	/* Present CS and DATA. */
	DELAY(1);			/* Tcss=100ns, Tdis=200ns. */

	/* SK high. */
	BCR_WRITE(device, 19, value | BCR19_ESK_HIGH);
	DELAY(1);			/* Tdih=200ns, SK=1us. */
}

void    
ee_opcode(void *device, unsigned char opcode)
{
	/* Write MSB of opcode. */
	uwire_out(device, ((opcode & 0x0002) >> 1) | BCR19_ECS);

	/* Write LSB of opcode. */
	uwire_out(device, (opcode & 0x0001) | BCR19_ECS);
}

void    
ee_addr(void *device, unsigned char address)
{
	int i;

	/* Address bits are written MSB to LSB. */
	for (i = 0; i < ADDRESS_LENGTH; i++ )
		uwire_out(device, ((address >> ((ADDRESS_LENGTH - 1) - i)) &
				0x0001) | BCR19_ECS);
}

void    
ee_ewen(void *device)
{
	uwire_out(device, BCR19_START);	/* Give START condition to EEPROM. */
	ee_opcode(device, EWEN_OP);	/* Write ewen opcode. */

	/* ee_addr() is used here although this is not really an address */
	/* being written. Note that the ewen instruction is required to be */
	/* 11 clock cycles long by the EEPROM, hence the remaining dummy */
	/* zeroes are written in the last address locations.  */
	ee_addr(device, EWEN_ADR);	/* Write remaining 2 bits of opcode and */
					/* -six dummy zeroes out to EEPROM. */
	uwire_out(device, BCR19_STOP);	/* Clear EEN and ECS. */
}

void    
ee_ewds(void *device)
{
	uwire_out(device, BCR19_START);	/* Give START condition to EEPROM. */
	ee_opcode(device, EWDS_OP);	/* Write ewds opcode. */

	/* ee_addr() is used here although this is not really an address being */
	/* written. Note that the ewds instruction is required to be 11 clock */
	/* cycles long by the EEPROM, hence the remaining dummy zeroes are written */
	/* in the last address locations.  */

	ee_addr(device, EWDS_ADR);	/* Write remaining 2 bits of opcode and */
					/* -six dummy zeroes out to EEPROM. */
	uwire_out(device, BCR19_STOP);	/* Clear EEN and ECS. */
}

void    
ee_era(void *device, int addr)
{
	/* See if command line option disables EEPROM physical I/O. */
	ee_ewen(device);		/* EEPROM erase/write enable. */
	uwire_out(device, BCR19_START);	/* Output START bit to EEPROM. */
	ee_opcode(device, ERASE_OP);	/* Output ERASE opcode to EEPROM. */
	ee_addr(device, addr);		/* Output address to EEPROM. */

	/* ECS low = start word erase cycle. */
	BCR_WRITE(device, 19, BCR19_UWIRE_ON);
	DELAY(1);			/* Tcs = 250ns. */

	/* Wait for EEPROM erase cycle to complete (4-10ms). */
	BCR_WRITE(device, 19, BCR19_ECS);

	while (!(BCR_READ(device, 19) & 0x0001))
		;

	/* EEEN low, EECS low. */
	uwire_out(device, BCR19_STOP);

	/* EEPROM erase/write disable. */
	ee_ewds(device);
}

uDoublet  
ee_read(void *device, int addr)
{
	uDoublet data = 0xBEEF;
	int i;

	/* See if command line option disables EEPROM physical I/O. */
	uwire_out(device, BCR19_START);    /* Output START bit to EEPROM. */
	ee_opcode(device, READ_OP);        /* Output READ opcode to EEPROM. */
	ee_addr(device, addr);             /* Write address to EEPROM. */

	/* Read 16 bits of EEPROM data. */
	for (i = 0, data = 0; i < 16; i++)
		data |= uwire_in(device) << (15 - i);

	/* EEEN low, EECS low. */
	uwire_out(device, BCR19_STOP);

	/* Optional programmer view of EEPROM word. */
	return data;
}

uDoublet
ee_read_sum(void *device, int addr, int length)
{
	int x;
	int csum;

	csum = 0;

	while(length--)
	{
		x = ee_read(device, addr);
		csum += (x & 0xFF) + ((x>>8) & 0xFF);
	}

	return csum;
}

void    
ee_write(void *device, int addr, uDoublet data)
{
	int i;

	/* Optional programmer view of EEPROM word. */
	/* See if command line option disables EEPROM physical I/O. */
	/* Erase the target location before writing. */
	ee_era(device, addr);

	ee_ewen(device);		/* EEPROM erase/write enable. */
	uwire_out(device, BCR19_START);	/* Output START bit to EEPROM. */
	ee_opcode(device, WRITE_OP);	/* Output WRITE opcode to EEPROM. */
	ee_addr(device, addr);		/* Output address to EEPROM. */

	/* Write 16 bits of EEPROM data. */
	for (i = 0; i < 16; ++i)
	    uwire_out(device, ((data >> (15 - i)) & 0x0001) | BCR19_ECS);

	/* ECS low = start word programming cycle. */
	BCR_WRITE(device, 19, BCR19_UWIRE_ON);
	DELAY(1);			/* Tcs = 250ns. */

	    /* Wait for EEPROM programming cycle to complete (4-10ms). */
	BCR_WRITE(device, 19, BCR19_ECS);

	while (!(BCR_READ(device, 19) & 0x0001))
		;

	/* EEEN low, EECS low. */
	uwire_out(device, BCR19_STOP);

	/* EEPROM erase/write disable. */
	ee_ewds(device);
}

C(f_lance_eeprom_dump)			/* eeprom-dump (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Prom_field *pf;
	int i;

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;
	pf = ee_get_fields(s);

	if (!pf)
	{
		cprintf(e, "no eeprom info - update driver\n");
		return NO_ERROR;		/* XXX */
	}

	for(i = 0; pf->type; i++, pf++)
		switch (pf->type)
		{
		case DATA_MAC:
			cprintf(e, "mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
					ee_read(s, i) & 0xFF, (ee_read(s, i) >> 8) & 0xFF,
					ee_read(s, i + 1) & 0xFF, (ee_read(s, i + 1) >> 8) & 0xFF,
					ee_read(s, i + 2) & 0xFF, (ee_read(s, i + 2) >> 8) & 0xFF);
			break;
		case DATA_CSR:
			cprintf(e, "CSR %d: %04x\n", pf->addr, ee_read(s, i));
			break;
		case DATA_BCR:
			cprintf(e, "BCR %d: %04x\n", pf->addr, ee_read(s, i));
			break;
		case DATA_HWID:
			cprintf(e, "hw id: %04x\n", ee_read(s, i));
			break;
		case DATA_USER:
			cprintf(e, "user: %04x\n", ee_read(s, i));
			break;
		case DATA_LCSUM:
			cprintf(e, "low checksum: %04x\n", ee_read(s, i));
			break;
		case DATA_WW:
			cprintf(e, "WW: %04x\n", ee_read(s, i));
			break;
		case DATA_HCSUM:
			cprintf(e, "high checksum: %04x\n", ee_read(s, i));
			break;
		case DATA_FILLER:
			break;
		case DATA_CONSTANT:
			cprintf(e, "reserved: %04x\n", ee_read(s, i));
			break;
		}

	return NO_ERROR;
}

C(f_lance_eeprom_setmac)		/* eeprom-setmac (-- 0|error-code) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	uInt oui;
	uInt ether_id;
	Prom_field *pf;
	int i;
	int csum;
	int hcsum_addr = 0;
	unsigned char mac_addr[6];

	if (!inst || !inst->self)
		return E_BAD_INSTANCE;

	s = inst->self;
	IFCKSP(e, 2, 0);
	POP(e, oui);
	POP(e, ether_id);

	/* Innomedia = 0x001099 = 4249 */
	mac_addr[0] = (oui >> 16) & 0xFF;
	mac_addr[1] = (oui >> 8) & 0xFF;
	mac_addr[2] = oui & 0xFF;
	mac_addr[3] = (ether_id >> 16) & 0xFF;
	mac_addr[4] = (ether_id >> 8) & 0xFF;
	mac_addr[5] = ether_id & 0xFF;

	pf = ee_get_fields(s);

	if (!pf)
	{
		cprintf(e, "no eeprom info - update driver\n");
		return NO_ERROR;		/* XXX */
	}

	cprintf(e, "setting mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
	for(i = 0; pf->type; i++, pf++)
		switch(pf->type)
		{
		case DATA_MAC:
			ee_write(s, i, (mac_addr[1] << 8) + mac_addr[0]);
			ee_write(s, i+1, (mac_addr[3] << 8) + mac_addr[2]);
			ee_write(s, i+2, (mac_addr[5] << 8) + mac_addr[4]);
			break;
		case DATA_CSR:
			ee_write(s, i, CSR_READ(s, pf->addr));
			break;
		case DATA_BCR:
			ee_write(s, i, BCR_READ(s, pf->addr));
			break;
		case DATA_HWID:
			ee_write(s, i, 0x1100);
			break;
		case DATA_USER:
			ee_write(s, i, 0x0000);
			break;
		case DATA_LCSUM:
		case DATA_CONSTANT:
			ee_write(s, i, pf->addr);
			break;
		case DATA_WW:
			ee_write(s, i, 0x5757);
			break;
		case DATA_HCSUM:
			ee_write(s, i, 0x0000);
			hcsum_addr = i;
			break;
		case DATA_FILLER:
			break;
		}

	/* compute and write checksums */
	csum = ee_read_sum(s, 0, 6);
	csum += ee_read_sum(s, 7, 1);
	ee_write(s, 6, csum);

	csum = 0xFF - (ee_read_sum(s, 0, i) & 0xFF);
	ee_write(s, hcsum_addr, csum << 8);

	return NO_ERROR;
}

static const Initentry lance_methods[] =
{
	{ "open",      f_lance_open,          INVALID_FCODE },
	{ "close",     f_lance_close,         INVALID_FCODE },
#if 0
	{ "dma-alloc", f_lance_dma_alloc,     INVALID_FCODE },
	{ "dma-free",  f_lance_dma_free,      INVALID_FCODE },
#endif
	{ "read",      f_lance_read,          INVALID_FCODE },
	{ "write",     f_lance_write,         INVALID_FCODE },
	{ "load",      f_lance_load,          INVALID_FCODE },
	{ "ping",      f_lance_ping,          INVALID_FCODE },
	{ "selftest",  f_lance_selftest,      INVALID_FCODE },
	{ "eeprom-dump", f_lance_eeprom_dump, INVALID_FCODE },
	{ "eeprom-setmac", f_lance_eeprom_setmac, INVALID_FCODE },
#ifdef DEBUG
	{ "membar@",   f_membar_read,         INVALID_FCODE },
	{ "membar!",   f_membar_write,        INVALID_FCODE },
	{ "csr@",      f_csr_read,            INVALID_FCODE },
	{ "csr!",      f_csr_write,           INVALID_FCODE },
	{ "bcr@",      f_bcr_read,            INVALID_FCODE },
	{ "bcr!",      f_bcr_write,           INVALID_FCODE },
#endif
	{ NULL,        NULL },
};

PCI(install_lance_driver)
{
	Retcode ret;
	Pci_device_info *newinfo;

	DPRINTF(("install_lance_driver: pkg=%p devinfo=%p"
			" vendid=%#x devid=%#x\n",
			pkg, devinfo, devinfo->vendid, devinfo->devid));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "network", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, lance_methods);

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	pkg->self = (struct pself*)newinfo;		/* and save it */

	return ret;
}

Pci_driver amd_lance_ethernet =
{
	{ 0, 0, 0, 0, 0, 0x20000, 0x1022, 0x2000, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_lance_driver
};
