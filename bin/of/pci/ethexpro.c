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

/* (c) Copyright 1998,2000 by CodeGen, Inc.  All Rights Reserved. */

/* Driver for Intel EtherExpress Pro+ family.
 */

#include "defs.h"
#include "pci.h"

/************************************************************************/

/* The following is copied from FreeBSD's sys/pci/if_fxreg.h then modified.
   The original copyrights follow:
 */

/*
 * Copyright (c) 1995, David Greenman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: ethexpro.c,v 1.21 2000/10/18 05:26:29 parag Exp $
 */

#define VENDORID_INTEL	0x8086
#define DEVICEID_i82557	0x1229

#define PCI_MMBA	0x10
#define PCI_IOBA	0x14

/*
 * Control/status registers.
 */
#define	CSR_SCB_RUSCUS		0	/* scb_rus/scb_cus (1 byte) */
#define	CSR_SCB_STATACK		1	/* scb_statack (1 byte) */
#define	CSR_SCB_COMMAND		2	/* scb_command (1 byte) */
#define	CSR_SCB_INTRCNTL	3	/* scb_intrcntl (1 byte) */
#define	CSR_SCB_GENERAL		4	/* scb_general (4 bytes) */
#define	CSR_PORT			8	/* port (4 bytes) */
#define	CSR_FLASHCONTROL	12	/* flash control (2 bytes) */
#define	CSR_EEPROMCONTROL	14	/* eeprom control (2 bytes) */
#define	CSR_MDICONTROL		16	/* mdi control (4 bytes) */

/*
 * FOR REFERENCE ONLY, the old definition of CSR_SCB_RUSCUS:
 *
 *	volatile uByte	:2,
 *				scb_rus:4,
 *				scb_cus:2;
 */

#define PORT_SOFTWARE_RESET		0
#define PORT_SELFTEST			1
#define PORT_SELECTIVE_RESET	2
#define PORT_DUMP				3

#define SCB_RUS_IDLE			0
#define SCB_RUS_SUSPENDED		1
#define SCB_RUS_NORESOURCES		2
#define SCB_RUS_READY			4
#define SCB_RUS_SUSP_NORBDS		9
#define SCB_RUS_NORES_NORBDS	10
#define SCB_RUS_READY_NORBDS	12

#define SCB_CUS_IDLE			0
#define SCB_CUS_SUSPENDED		1
#define SCB_CUS_ACTIVE			2

#define SCB_STATACK_SWI			0x04
#define SCB_STATACK_MDI			0x08
#define SCB_STATACK_RNR			0x10
#define SCB_STATACK_CNA			0x20
#define SCB_STATACK_FR			0x40
#define SCB_STATACK_CXTNO		0x80

#define SCB_COMMAND_CU_NOP			0x00
#define SCB_COMMAND_CU_START		0x10
#define SCB_COMMAND_CU_RESUME		0x20
#define SCB_COMMAND_CU_DUMP_ADR		0x40
#define SCB_COMMAND_CU_DUMP			0x50
#define SCB_COMMAND_CU_BASE			0x60
#define SCB_COMMAND_CU_DUMPRESET	0x70

#define SCB_COMMAND_RU_NOP			0
#define SCB_COMMAND_RU_START		1
#define SCB_COMMAND_RU_RESUME		2
#define SCB_COMMAND_RU_ABORT		4
#define SCB_COMMAND_RU_LOADHDS		5
#define SCB_COMMAND_RU_BASE			6
#define SCB_COMMAND_RU_RBDRESUME	7

/*
 * Command block definitions
 */
struct cb_nop
{
	volatile uShort cb_status;
	volatile uShort cb_command;
	volatile uInt link_addr;
};

struct cb_ias
{
	volatile uShort cb_status;
	volatile uShort cb_command;
	volatile uInt link_addr;
	volatile uByte macaddr[6];
};

struct cb_config
{
	volatile uShort	cb_status;
	volatile uShort	cb_command;
	volatile uInt	link_addr;
	volatile uByte	byte_count;
		#define CB_BYTE_COUNT(n)	((n) & 0x3F)	/* 6 bits */
		/*
			byte_count : 6,
			: 2;
		*/
	volatile uByte	fifo_limit;
		#define CB_RX_FIFO_LIMIT(n)	((n) & 0xF)
		#define CB_TX_FIFO_LIMIT(n)	(((n) & 0x7) << 4)
		/*
			rx_fifo_limit : 4,
			tx_fifo_limit : 3,
			: 1;
		*/
	volatile uByte	adaptive_ifs;
	volatile uByte	unused_b3;
	volatile uByte	rx_dma_bytecount;
		#define CB_RX_DMA_BYTECOUNT(n)	((n) & 0x7F)
		/*
			rx_dma_bytecount : 7,
			: 1;
		*/
	volatile uByte	b5;
		#define CB_TX_DMA_BYTECOUNT(n)	((n) & 0x7F)
		#define CB_DMA_BCE(n)			(((n) & 0x1) << 7)
		/*
			tx_dma_bytecount : 7,
			dma_bce : 1;
		*/
	volatile uByte b6;
		#define CB_LATE_SCB(n)	((n) & 0x01)
		#define CB_TNO_INT(n)	(((n) & 0x01) << 2)
		#define CB_CI_INT(n)	(((n) & 0x01) << 3)
		#define CB_SAVE_BF(n)	(((n) & 0x01) << 7)
		/*
			late_scb : 1,
			: 1,
			tno_int : 1,
			ci_int : 1,
			: 3,
			save_bf : 1;
		*/
	volatile uByte b7;
		#define CB_DISC_SHORT_RX(n)		((n) & 0x01)
		#define CB_UNDERRUN_RETRY(n)	(((n) & 0x03) << 1)
		/*
			disc_short_rx : 1,
			underrun_retry : 2,
			: 5;
		*/
	volatile uByte mediatype;
		#define CB_MEDIATYPE(n)	((n) & 0x01)
		/*
			mediatype : 1,
			: 7;
		*/
	volatile uByte unused_b9;
	volatile uByte b10;
		#define CB_NSAI(n)				(((n) & 0x01) << 3)
		#define CB_PREAMBLE_LENGTH(n)	(((n) & 0x03) << 4)
		#define CB_LOOPBACK(n)			(((n) & 0x03) << 6)
		/*
			: 3,
			nsai : 1,
			preamble_length : 2,
			loopback : 2;
		*/
	volatile uByte linear_priority;
		#define CB_LINEAR_PRIORITY(n)	((n) & 0x07)
		/*
			linear_priority : 3,
			: 5;
		*/
	volatile uByte b12;
		#define CB_LINEAR_PRI_MODE(n)	((n) & 0x01)
		#define CB_INTERFRM_SPACING(n)	(((n) & 0x0F) << 4)
		/*
			linear_pri_mode : 1,
			: 3,
			interfrm_spacing : 4;
		*/
	volatile uByte unused_b13;
	volatile uByte unused_b14;
	volatile uByte b15;
		#define CB_PROMISCUOUS(n)		((n) & 0x01)
		#define CB_BCAST_DISABLE(n)		(((n) & 0x01) << 1)
		#define CB_CRSCDT(n)			(((n) & 0x01) << 7)
		/*
			promiscuous : 1,
			bcast_disable : 1,
			: 5,
			crscdt : 1;
		*/
	volatile uByte unused_b16;
	volatile uByte unused_b17;
	volatile uByte b18;
		#define CB_STRIPPING(n)		((n) & 0x01)
		#define CB_PADDING(n)		(((n) & 0x01) << 1)
		#define CB_RCV_CRC_XFER(n)	(((n) & 0x01) << 2)
		/*
			stripping : 1,
			padding : 1,
			rcv_crc_xfer : 1,
			: 5;
		*/
	volatile uByte b19;
		#define CB_FORCE_FDX(n)		(((n) & 0x1) << 6)
		#define CB_FDX_PIN_EN(n)	(((n) & 0x1) << 7)
		/*
			: 6,
			force_fdx : 1,
			fdx_pin_en : 1;
		*/
	volatile uByte multi_ia;
		#define CB_MULTI_IA(n)	(((n) & 0x01) << 6)
		/*
			: 6,
			multi_ia : 1,
			: 1;
		*/
	volatile uByte mc_all;
		#define CB_MC_ALL(n)	(((n) & 0x01) << 3)
		/*
			: 3,
			mc_all : 1,
			: 4;
		*/
};

#define MAXMCADDR 80
struct cb_mcs
{
	volatile uShort cb_status;
	volatile uShort cb_command;
	volatile uInt link_addr;
	volatile uShort mc_cnt;
	volatile uByte mc_addr[MAXMCADDR][6];
};

#define NTXSEG      30

struct cb_tx
{
	volatile uShort cb_status;
	volatile uShort cb_command;
	volatile uInt link_addr;
	volatile uInt tbd_array_addr;
	volatile uShort byte_count;
	volatile uByte tx_threshold;
	volatile uByte tbd_number;
};

/*
 * Control Block (CB) definitions
 */

/* status */
#define CB_STATUS_OK		0x2000
#define CB_STATUS_C			0x8000
/* commands */
#define CB_COMMAND_NOP		0x0
#define CB_COMMAND_IAS		0x1
#define CB_COMMAND_CONFIG	0x2
#define CB_COMMAND_MCAS		0x3
#define CB_COMMAND_XMIT		0x4
#define CB_COMMAND_RESRV	0x5
#define CB_COMMAND_DUMP		0x6
#define CB_COMMAND_DIAG		0x7
/* command flags */
#define CB_COMMAND_SF		0x0008	/* simple/flexible mode */
#define CB_COMMAND_I		0x2000	/* generate interrupt on completion */
#define CB_COMMAND_S		0x4000	/* suspend on completion */
#define CB_COMMAND_EL		0x8000	/* end of list */

/*
 * RFA definitions
 */

struct rfa
{
	volatile uShort rfa_status;
	volatile uShort rfa_control;
	volatile uInt link_addr;
	volatile uInt rbd_addr;
	volatile uShort actual_size;
	volatile uShort size;
};

#define RFA_STATUS_RCOL		0x0001	/* receive collision */
#define RFA_STATUS_IAMATCH	0x0002	/* 0 = matches station address */
#define RFA_STATUS_S4		0x0010	/* receive error from PHY */
#define RFA_STATUS_TL		0x0020	/* type/length */
#define RFA_STATUS_FTS		0x0080	/* frame too short */
#define RFA_STATUS_OVERRUN	0x0100	/* DMA overrun */
#define RFA_STATUS_RNR		0x0200	/* no resources */
#define RFA_STATUS_ALIGN	0x0400	/* alignment error */
#define RFA_STATUS_CRC		0x0800	/* CRC error */
#define RFA_STATUS_OK		0x2000	/* packet received okay */
#define RFA_STATUS_C		0x8000	/* packet reception complete */

#define RFA_CONTROL_SF		0x08	/* simple/flexible memory mode */
#define RFA_CONTROL_H		0x10	/* header RFD */
#define RFA_CONTROL_S		0x4000	/* suspend after reception */
#define RFA_CONTROL_EL		0x8000	/* end of list */


/*
 * Serial EEPROM control register bits
 */
/* shift clock */
#define EEPROM_EESK		0x01
/* chip select */
#define EEPROM_EECS		0x02
/* data in */
#define EEPROM_EEDI		0x04
/* data out */
#define EEPROM_EEDO		0x08

/*
 * Serial EEPROM opcodes, including start bit
 */
#define EEPROM_OPC_ERASE	0x4
#define EEPROM_OPC_WRITE	0x5
#define EEPROM_OPC_READ		0x6

/*
 * Management Data Interface opcodes
 */
#define MDI_WRITE		0x1
#define MDI_READ		0x2

/*
 * PHY device types
 */
#define PHY_NONE		0
#define PHY_82553A		1
#define PHY_82553C		2
#define PHY_82503		3
#define PHY_DP83840		4
#define PHY_80C240		5
#define PHY_80C24		6
#define PHY_82555		7
#define PHY_DP83840A	10
#define PHY_82555B		11

/*
 * PHY BMCR Basic Mode Control Register
 */
#define PHY_BMCR				0x0
#define PHY_BMCR_FULLDUPLEX		0x0100
#define PHY_BMCR_AUTOEN			0x1000
#define PHY_BMCR_SPEED_100M		0x2000

/*
 * DP84830 PHY, PCS Configuration Register
 */
#define DP83840_PCR				0x17
#define DP83840_PCR_LED4_MODE	0x0002
		/* 1 = LED4 always indicates full duplex */
#define DP83840_PCR_F_CONNECT	0x0020
		/* 1 = force link disconnect function bypass */
#define DP83840_PCR_BIT8		0x0100
#define DP83840_PCR_BIT10		0x0400

/************************************************************************/


#ifdef DEBUG
extern void dump_packet(Environ *e, uByte *pkt, uInt len);
#endif


#define MAC_ADDR			6	/* Ethernet hardware address length */
#define MAX_PACKET_SIZE		0x7FC
#define BUF_SIZE 			(MAX_PACKET_SIZE * 4)


/* instance-specific vars */
struct self
{
	Pci_device_info *devinfo;
	Int physhi, physmid, physlo, sizehi, sizelo;
	Int phy_primary_addr, phy_primary_device;
	Bool phy_10Mbps_only;
	volatile uInt *icsr;		/* long-word access to mmapped registers */
	volatile uShort *scsr;		/* word access to memory-mapped registers */
	volatile uByte *bcsr;		/* byte access to memory-mapped registers */
	Instance *obptftp;
	uByte mac_addr[MAC_ADDR];	/* Ethernet hardware address */
	Cell dma;					/* mapped memory for DMA access from host */
	Cell busdma;				/* allocated memory for DMA from PCI bus */
	Byte *sendbuf, *recvbuf;
	Byte *senddma, *recvdma;
	struct cb_tx *txcb;
};
typedef struct self Self;

#define CSR_READ_1(s, reg)			((s)->bcsr[reg])
#define CSR_READ_2(s, reg)			LE2((s)->scsr[(reg) >> 1])
#define CSR_READ_4(s, reg)			LE4((s)->icsr[(reg) >> 2])
#define CSR_WRITE_1(s, reg, val)	((s)->bcsr[reg] = ((uPtr)val))
#define CSR_WRITE_2(s, reg, val)	((s)->scsr[(reg) >> 1] = LE2((uPtr)val))
#define CSR_WRITE_4(s, reg, val)	((s)->icsr[(reg) >> 2] = LE4((uPtr)val))

#define CSR_SIZE	(7 * 4)		/* number of registers * bytes/reg */

#define DELAY(n)	u_sleep(n)

#ifdef DEBUG
static void
dump_regs(Environ *e, Self *s)
{
	dprintf("\n");
	dprintf("ethxpro: scb_rus/scb_cus: %#x\n", CSR_READ_1(s, CSR_SCB_RUSCUS));
	dprintf("ethxpro: scb_statack: %#x\n", CSR_READ_1(s, CSR_SCB_STATACK));
	dprintf("ethxpro: scb_command: %#x\n", CSR_READ_1(s, CSR_SCB_COMMAND));
	dprintf("ethxpro: scb_intrcntl: %#x\n", CSR_READ_1(s, CSR_SCB_INTRCNTL));
	dprintf("ethxpro: scb_general: %#x\n", CSR_READ_4(s, CSR_SCB_GENERAL));
	dprintf("ethxpro: port: %#x\n", CSR_READ_4(s, CSR_PORT));
	dprintf("ethxpro: flash control: %#x\n", CSR_READ_2(s, CSR_FLASHCONTROL));
	dprintf("ethxpro: eeprom ctrl: %#x\n", CSR_READ_2(s, CSR_EEPROMCONTROL));
	dprintf("ethxpro: mdi control: %#x\n", CSR_READ_4(s, CSR_MDICONTROL));
}
#else
#	define dump_regs(e, s)	/* e, s */
#endif


/* wait for the command to be accepted but not necessarily completed
 */
static void
scb_wait(Self *s)
{
	int i = 10000;

	while (CSR_READ_1(s, CSR_SCB_COMMAND) && --i)
		DELAY(1);
}

static Retcode
read_eeprom(Environ *e, Self *s, uShort *data, int offset, int words)
{
	uShort reg;
	int i, x;

	for (i = 0; i < words; i++)
	{
		CSR_WRITE_2(s, CSR_EEPROMCONTROL, EEPROM_EECS);

		/* Shift in read opcode.  */
		for (x = 3; x > 0; x--)
		{
			if (EEPROM_OPC_READ & (1 << (x - 1)))
				reg = EEPROM_EECS | EEPROM_EEDI;
			else
				reg = EEPROM_EECS;

			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg);
			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg | EEPROM_EESK);
			DELAY(1);
			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}

		/* Shift in address.  */
		for (x = 6; x > 0; x--)
		{
			if ((i + offset) & (1 << (x - 1)))
				reg = EEPROM_EECS | EEPROM_EEDI;
			else
				reg = EEPROM_EECS;

			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg);
			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg | EEPROM_EESK);
			DELAY(1);
			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}

		reg = EEPROM_EECS;
		data[i] = 0;

		/* Shift out data */
		for (x = 16; x > 0; x--)
		{
			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg | EEPROM_EESK);
			DELAY(1);

			if (CSR_READ_2(s, CSR_EEPROMCONTROL) & EEPROM_EEDO)
				data[i] |= (1 << (x - 1));

			CSR_WRITE_2(s, CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}

		CSR_WRITE_2(s, CSR_EEPROMCONTROL, 0);
		DELAY(1);
	}

	return NO_ERROR;
}

static Retcode
read_mac_addr(Environ *e, Package *pkg, Self *s, Pci_device_info *devinfo)
{
	Retcode ret;
	uByte mac_addr[MAC_ADDR];

	ret = read_eeprom(e, s, (uShort*)mac_addr, 0, 3);

	if (ret != NO_ERROR)
		return ret;

	// If the MAC address is all F's then we do not have a legit MAC addr
	if ((mac_addr[0] == 0xFF) && (mac_addr[1] == 0xFF) &&
			(mac_addr[2] == 0xFF) && (mac_addr[3] == 0xFF) &&
			(mac_addr[4] == 0xFF) && (mac_addr[5] == 0xFF))
		return E_BAD_ARGUMENT;

	/* may need to swap the two halves of data */
#ifdef BIG_ENDIAN
	s->mac_addr[0] = mac_addr[1];
	s->mac_addr[1] = mac_addr[0];
	s->mac_addr[2] = mac_addr[3];
	s->mac_addr[3] = mac_addr[2];
	s->mac_addr[4] = mac_addr[5];
	s->mac_addr[5] = mac_addr[4];
#else
	memcpy(s->mac_addr, mac_addr, sizeof mac_addr);
#endif

	if (ret != NO_ERROR)
		return ret;

	return set_property(pkg->props, "local-mac-address", CSTR,
			(Byte *)s->mac_addr, MAC_ADDR);
}

static int
mdi_read(Environ *e, Self *s, int phy, int reg)
{
	int count = 10000;
	int value;

	CSR_WRITE_4(s, CSR_MDICONTROL,
			(MDI_READ << 26) | (reg << 16) | (phy << 21));

	while (((value = CSR_READ_4(s, CSR_MDICONTROL)) & 0x10000000) == 0
			&& count--)
		DELAY(10);

	if (count <= 0)
		cprintf(e, "ethexpro: mdi_read: timed out\n");

	return (value & 0xffff);
}

static void
mdi_write(Environ *e, Self *s, int phy, int reg, int value)
{
	int count = 10000;

	CSR_WRITE_4(s, CSR_MDICONTROL,
			(MDI_WRITE << 26) | (reg << 16) | (phy << 21) | (value & 0xffff));

	while((CSR_READ_4(s, CSR_MDICONTROL) & 0x10000000) == 0 && count--)
		DELAY(10);

	if (count <= 0)
		cprintf(e, "ethexpro: mdi_write: timed out\n");
}

C(f_ethexpro_open)			/* open (-- okay?) */
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
	uShort data;

	DPRINTF(("ethexpro_open: pkg=%p\n", pkg));
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
	DPRINTF(("ethexpro_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	do
		ret = pci_decode_reg_prop(&paddr, &plen, &s->physhi, &s->physmid,
				&s->physlo, &s->sizehi, &s->sizelo);
	while (PCI_PHYSHI_SPACE(s->physhi) != PCI_SPACE_MEM);
	
	DPRINTF(("ethexpro_open: physhi=%#x physmid=%#x physlo=%#x\n",
			s->physhi, s->physmid, s->physlo));
    ret = pci_map_in(devinfo->hbridge, s->physhi, s->physmid, s->physlo,
			CSR_SIZE, &c);

	if (ret != NO_ERROR)
		return ret;

	/* point to CSRs for various byte/word/lword accesses */
	s->icsr = (uInt*)(uPtr)c;
	s->scsr = (uShort*)(uPtr)c;
	s->bcsr = (uByte*)(uPtr)c;

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

			DPRINTF(("ethexpro: args=\"%s\"\n", str));

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
						cprintf(e, "ethexpro: unsupported \"speed=%d\"\n", val);
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
					cprintf(e, "ethexpro: bad arg format for \"duplex=\"");
			}
			else
			{
				/* pass all remaining args to obptftp */
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
	DPRINTF(("ethexpro_open: before cfg=%#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) |
					PCI_COMMAND_MEMENABLE |
					PCI_COMMAND_MASTERENABLE
			);

	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("ethexpro_open: after cfg=%#x\n", cfg));

	/* allocate I/O packet buffers plus descriptors suitable for DMA */
	ret = pci_dma_alloc(devinfo->hbridge, BUF_SIZE, &s->dma);
	DPRINTF(("ethexpro_open: pci_dma_alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, BUF_SIZE, 0, &s->busdma);
	DPRINTF(("ethexpro_open: pci_dma_map_in ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	/* setup our buffers
	  - sizes are such that align requirements should work out automagically
	 */
	s->recvbuf = (Byte*)(uPtr)s->dma;
	s->recvdma = (Byte*)(uPtr)s->busdma;
	memset((char*)s->recvbuf, 0, BUF_SIZE);
	s->sendbuf = s->recvbuf + MAX_PACKET_SIZE * 2;
	s->senddma = s->recvdma + MAX_PACKET_SIZE * 2;
	s->txcb = (struct cb_tx*)s->sendbuf;
	DPRINTF(("ethexpro_open: recvbuf/dma=%p/%p sendbuf/dma=%p/%p txcb=%p\n",
		s->recvbuf, s->recvdma, s->sendbuf, s->senddma, s->txcb));

	/* read PHY info out of PROM */
	read_eeprom(e, s, &data, 6, 1);
	s->phy_primary_addr = data & 0xff;
	s->phy_primary_device = (data >> 8) & 0x3f;
	s->phy_10Mbps_only = data >> 15;

	/* sanity check in case of missing EPROM */
	if (data == 0xFFFF)
	{
	    s->phy_primary_device = PHY_82555B;
	    s->phy_10Mbps_only = 0;
	}

	DPRINTF(("ethexpro_open: primary_addr=%#x _device=%#x 10Mbps_only=%#x\n",
			s->phy_primary_addr, s->phy_primary_device, s->phy_10Mbps_only));

	/* try to read in the local MAC addr out of ROM and use it
	   to intialize the parameter "local-mac-address" */
	ret = read_mac_addr(e, pkg, s, devinfo);
	DPRINTF(("ethexpro_open: read_mac_addr ret=%d\n", ret));
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
	DPRINTF(("ethexpro_open: $open-package ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("ethexpro_open: obptftp=%p\n", s->obptftp));

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

	/* reset the chip */
	CSR_WRITE_4(s, CSR_PORT, PORT_SELECTIVE_RESET);
	DELAY(10);

	/* initialize base of CBL and RFA memory -
		- loading with zero sets it up for regular linear addressing */
	CSR_WRITE_4(s, CSR_SCB_GENERAL, 0);
	CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_BASE);
	scb_wait(s);
	CSR_WRITE_4(s, CSR_SCB_GENERAL, 0);
	CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_RU_BASE);
	scb_wait(s);
	dump_regs(e, s);

	/* setup config command */
	DPRINTF(("ethexpro_open: setup config command\n"));
	{
		static uByte config_template[] = {
			0x0, 0x0,		/* cb_status */
			0x02, 0x80,		/* cb_command */
			0xff, 0xff, 0xff, 0xff,	/* link_addr */
			0x16,	/*  0 */
			0x8,	/*  1 */
			0x0,	/*  2 */
			0x0,	/*  3 */
			0x0,	/*  4 */
			0x80,	/*  5 */
			0xb2,	/*  6 */
			0x3,	/*  7 */
			0x1,	/*  8 */
			0x0,	/*  9 */
			0x26,	/* 10 */
			0x0,	/* 11 */
			0x60,	/* 12 */
			0x0,	/* 13 */
			0xf2,	/* 14 */
			0x48,	/* 15 */
			0x0,	/* 16 */
			0x40,	/* 17 */
			0xf3,	/* 18 */
			0x0,	/* 19 */
			0x3f,	/* 20 */
			0x5	/* 21 */
		};
		struct cb_config *cbp = (struct cb_config*)s->sendbuf;

		memcpy(s->sendbuf, config_template, sizeof config_template);

	#ifdef DEBUG
		display_memory(e, s->sendbuf, sizeof config_template);
	#endif

		cbp->cb_status = 0;
		cbp->cb_command = LE2(CB_COMMAND_CONFIG | CB_COMMAND_EL);
		cbp->link_addr = -1;	/* (no) next command */
		cbp->byte_count |= CB_BYTE_COUNT(22);	/* (22) bytes to config */
		cbp->fifo_limit |= CB_RX_FIFO_LIMIT(8) | CB_TX_FIFO_LIMIT(0);
				/* rx fifo threshold (32 bytes) */
				/* tx fifo threshold (0 bytes) */
		cbp->adaptive_ifs =	0;	/* (no) adaptive interframe spacing */
		cbp->rx_dma_bytecount |= CB_RX_DMA_BYTECOUNT(0);
				/* (no) rx DMA max */
		cbp->b5 |= CB_TX_DMA_BYTECOUNT(0) | CB_DMA_BCE(1);
				/* (no) tx DMA max */
				/* (disable) dma max counters */
		cbp->b6 |= CB_LATE_SCB(0) | CB_TNO_INT(0) | CB_CI_INT(0) |
				CB_SAVE_BF(promiscuous);
				/* (don't) defer SCB update */
				/* (disable) tx not okay interrupt */
				/* (disable) interrupt on CU idle */
				/* save bad frames */
		cbp->b7 |= CB_DISC_SHORT_RX(!promiscuous) | CB_UNDERRUN_RETRY(1);
				/* discard short packets */
				/* retry mode (1) on DMA underrun */
		cbp->mediatype |= CB_MEDIATYPE(!s->phy_10Mbps_only);
				/* interface mode */
		cbp->b10 |= CB_NSAI(1) | CB_PREAMBLE_LENGTH(2) | CB_LOOPBACK(0);
				/* (don't) disable source addr insert */
				/* (7 byte) preamble */
				/* (don't) loopback */
		cbp->linear_priority |= CB_LINEAR_PRIORITY(0);
				/* (normal CSMA/CD operation) */
		cbp->b12 |= CB_LINEAR_PRI_MODE(0) | CB_INTERFRM_SPACING(6);
				/* (wait after xmit only) */
				/* (96 bits of) interframe spacing */
		cbp->b15 |= CB_PROMISCUOUS(promiscuous) | CB_BCAST_DISABLE(0) |
				CB_CRSCDT(0);
				/* promiscuous mode */
				/* (don't) disable broadcasts */
				/* (CRS only) */
		cbp->b18 |=	CB_STRIPPING(!promiscuous) | CB_PADDING(1) |
				CB_RCV_CRC_XFER(0);
				/* truncate rx pkt to byte count */
				/* (do) pad short tx packets */
				/* (don't) xfer CRC to host */
		cbp->b19 |= CB_FORCE_FDX(duplex) | CB_FDX_PIN_EN(1);
				/* force full duplex */
				/* (enable) FDX# pin */
		cbp->multi_ia |= CB_MULTI_IA(0);	/* (don't) accept multiple IAs */
		cbp->mc_all |= CB_MC_ALL(promiscuous);/* accept all multicasts */

	#ifdef DEBUG
		display_memory(e, s->sendbuf, sizeof config_template);
	#endif

		/*
		 * Start the config command/DMA.
		 */
		pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
				sizeof *cbp);
		scb_wait(s);
		CSR_WRITE_4(s, CSR_SCB_GENERAL, s->senddma);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_START);

		/* ...and wait for it to complete. */
		while (!(LE2(cbp->cb_status) & CB_STATUS_C))
			DPRINTF(("ethexpro_open: wait status=%#x\n", LE2(cbp->cb_status)));

		DPRINTF(("ethexpro_open: wait status=%#x\n", LE2(cbp->cb_status)));
	}

	DPRINTF(("ethexpro_open: initialize station address\n"));
	{
		struct cb_ias *cb_ias = (struct cb_ias*)s->sendbuf;

		/* initialize the station address */
		cb_ias->cb_status = 0;
		cb_ias->cb_command = LE2(CB_COMMAND_IAS | CB_COMMAND_EL);
		cb_ias->link_addr = -1;
		memcpy((void*)cb_ias->macaddr, s->mac_addr, MAC_ADDR);

		/* start the IAS (Individual Address Setup) command/DMA  */
		pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
				sizeof *cb_ias);
		scb_wait(s);
		CSR_WRITE_4(s, CSR_SCB_GENERAL, s->senddma);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_START);

		/* ...and wait for it to complete. */
		while (!(LE2(cb_ias->cb_status) & CB_STATUS_C))
			DPRINTF(("ethexpro_open: wait status=%#x\n",
					LE2(cb_ias->cb_status)));

		DPRINTF(("ethexpro_open: wait status=%#x\n", LE2(cb_ias->cb_status)));
	}

	/* initialize transmit control block (TxCB) */
	DPRINTF(("ethexpro_open: initialize TxCB\n"));
	{
		struct cb_tx *txp = (struct cb_tx*)s->sendbuf;

		memset(txp, 0, sizeof *txp);
		txp->cb_status = LE2(CB_STATUS_C | CB_STATUS_OK);
		txp->cb_command = LE2(CB_COMMAND_NOP | CB_COMMAND_S | CB_COMMAND_EL);
		txp->link_addr = -1;
		txp->tbd_array_addr = -1;
		/* CU will execute the NOP and then suspend */
	
		pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
				sizeof *txp);
		scb_wait(s);
		CSR_WRITE_4(s, CSR_SCB_GENERAL, s->senddma);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_START);
	}

	/* initialize receiver buffer (RFA) */
	DPRINTF(("ethexpro_open: initialize RFA\n"));
	{
		struct rfa *rfa = (struct rfa*)s->recvbuf;

		rfa->size = LE2(MAX_PACKET_SIZE - sizeof *rfa);
		rfa->rfa_status = 0;
		rfa->rfa_control = LE2(RFA_CONTROL_EL | RFA_CONTROL_S);
		rfa->actual_size = 0;
		rfa->link_addr = -1;
		rfa->rbd_addr = -1;

		pci_dma_sync(devinfo->hbridge, (uPtr)s->recvbuf, (uPtr)s->recvdma,
				MAX_PACKET_SIZE);
		scb_wait(s);
		CSR_WRITE_4(s, CSR_SCB_GENERAL, s->recvdma);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_RU_START);
	}

	/* set the media */
	DPRINTF(("ethexpro_open: set media\n"));
	{
		switch (s->phy_primary_device)
		{
		case PHY_DP83840:
		case PHY_DP83840A:
			mdi_write(e, s, s->phy_primary_addr, DP83840_PCR,
					mdi_read(e, s, s->phy_primary_addr, DP83840_PCR) |
					DP83840_PCR_LED4_MODE |	/* LED4 always indicates duplex */
					DP83840_PCR_F_CONNECT |	/* force link disconnect bypass */
					DP83840_PCR_BIT10);	/* XXX I have no idea */
			/* fall through */

		case PHY_82553A:
		case PHY_82553C: /* untested */
		case PHY_82555:
		case PHY_82555B:
			if (speed > 0)
			{
				int flags = (speed == 100) ? PHY_BMCR_SPEED_100M : 0;
				flags |= (duplex) ?  PHY_BMCR_FULLDUPLEX : 0;
				mdi_write(e, s, s->phy_primary_addr,
						PHY_BMCR,
						(mdi_read(e, s, s->phy_primary_addr, PHY_BMCR) &
								~(PHY_BMCR_AUTOEN |
								PHY_BMCR_SPEED_100M |
								PHY_BMCR_FULLDUPLEX)) | flags);
			}
			else
			{
				mdi_write(e, s, s->phy_primary_addr,
						PHY_BMCR,
						(mdi_read(e, s, s->phy_primary_addr, PHY_BMCR) |
								PHY_BMCR_AUTOEN));
			}

			break;

		/*
		 * The Seeq 80c24 doesn't have a PHY programming interface, so do
		 * nothing.
		 */
		case PHY_80C24:
			break;

		default:
			cprintf(e, "ethexpro: "
				" warning: unsupported PHY, type = %d, addr = %d\n",
				s->phy_primary_device, s->phy_primary_addr);
		}
	}

	/* ack any interrupts */
	dump_regs(e, s);
	CSR_WRITE_1(s, CSR_SCB_STATACK, CSR_READ_1(s, CSR_SCB_STATACK));

	DPRINTF(("ethexpro_open: done: ret=%d\n", ret));
	PUSH(e, FTRUE);
	return ret;
}

C(f_ethexpro_close)			/* close (--) */
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
	CSR_WRITE_4(s, CSR_PORT, PORT_SELECTIVE_RESET);
	DELAY(10);

	if (s->dma)
	{
		ret = pci_dma_map_out(devinfo->hbridge, (uPtr)s->dma,
				s->busdma, BUF_SIZE);

		if (ret == NO_ERROR)
			ret = pci_dma_free(devinfo->hbridge, s->dma, BUF_SIZE);

	}

	IFCKSP(e, 0, 1);
	PUSH(e, (uPtr)s->obptftp);
	ret = execute_word(e, "close-package");

	return ret;
}

C(f_ethexpro_dma_alloc)		/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ethexpro_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ethexpro_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ethexpro_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}


C(f_ethexpro_read)				/* read (addr len -- actual) */
{
	Int len, actual = -2;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	struct rfa *rfa = (struct rfa*)s->recvbuf;

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

	pci_dma_sync(devinfo->hbridge, (uPtr)s->recvbuf, (uPtr)s->recvdma,
			MAX_PACKET_SIZE);
	DPRINTF(("ethexpro_read: addr=%p len=%d status=%#x\n",
			addr, len, LE2(rfa->rfa_status)));

	if (LE2(rfa->rfa_status) & RFA_STATUS_C)
	{
		/* copy the data into the buffer requested */
		actual = LE2(rfa->actual_size);

		if (actual > len)
			actual = len;

		memcpy(addr, (void*)(rfa + 1), actual);

		/* ack interrupts */
		dump_regs(e, s);
		CSR_WRITE_1(s, CSR_SCB_STATACK, CSR_READ_1(s, CSR_SCB_STATACK));

		/* re-init for another packet */
		rfa->size = LE2(MAX_PACKET_SIZE - sizeof *rfa);
		rfa->rfa_status = 0;
		rfa->rfa_control = LE2(RFA_CONTROL_EL | RFA_CONTROL_S);
		rfa->actual_size = 0;
		rfa->link_addr = -1;
		rfa->rbd_addr = -1;
		scb_wait(s);
		CSR_WRITE_4(s, CSR_SCB_GENERAL, s->recvdma);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_RU_START);
		pci_dma_sync(devinfo->hbridge, (uPtr)s->recvbuf, (uPtr)s->recvdma,
				MAX_PACKET_SIZE);
	}

	DPRINTF(("ethexpro_read: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

C(f_ethexpro_write)			/* write (addr len -- actual) */
{
	Int len, actual = 0;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	struct cb_tx *txp = (struct cb_tx*)s->sendbuf;

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
	DPRINTF(("ethexpro_write: addr=%p len=%d\n", addr, len));

	if (len > MAX_PACKET_SIZE)
		len = MAX_PACKET_SIZE;

	/* copy in the data */
	memcpy(((uByte*)(txp + 1)), addr, len);
	txp->byte_count = LE2(len);
	txp->tbd_number = 0;
	txp->cb_status = 0;
	txp->cb_command = LE2(CB_COMMAND_XMIT | CB_COMMAND_S | CB_COMMAND_EL);

	/* start the transmission */
	pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
			sizeof *txp);
	scb_wait(s);
	CSR_WRITE_4(s, CSR_SCB_GENERAL, s->senddma);
	CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_START);

	/* loop until this packet is sent */
	while (!(LE2(txp->cb_status) & CB_STATUS_C))
	{
		DELAY(10);
		pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
				sizeof *txp);
		scb_wait(s);
		CSR_WRITE_1(s, CSR_SCB_COMMAND, SCB_COMMAND_CU_RESUME);
		DPRINTF(("ethexpro_write: txp->cb_status=%#x\n", LE2(txp->cb_status)));
	}

	/* ack interrupts */
	dump_regs(e, s);
	CSR_WRITE_1(s, CSR_SCB_STATACK, CSR_READ_1(s, CSR_SCB_STATACK));

	txp->cb_command = LE2(CB_COMMAND_NOP | CB_COMMAND_S | CB_COMMAND_EL);
	pci_dma_sync(devinfo->hbridge, (uPtr)s->sendbuf, (uPtr)s->senddma,
			sizeof *txp);

	actual = len;
	DPRINTF(("ethexpro_write: actual=%d\n", actual));
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

C(f_ethexpro_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_ethexpro_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

C(f_ethexpro_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Pci_device_info *devinfo;

	DPRINTF(("ether selftest: begin\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	DPRINTF(("ether selftest: got pkg\n"));
	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	DPRINTF(("ether selftest: got devinfo\n"));

	/* TODO */

	if (diag)
	{
		cprintf(e, "ether selftest\n");
		/* TODO */
	}

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}


static const Initentry ethexpro_methods[] =
{
	{ "open",      f_ethexpro_open,          INVALID_FCODE },
	{ "close",     f_ethexpro_close,         INVALID_FCODE },
	{ "dma-alloc", f_ethexpro_dma_alloc,     INVALID_FCODE },
	{ "dma-free",  f_ethexpro_dma_free,      INVALID_FCODE },
	{ "read",      f_ethexpro_read,          INVALID_FCODE },
	{ "write",     f_ethexpro_write,         INVALID_FCODE },
	{ "load",      f_ethexpro_load,          INVALID_FCODE },
	{ "ping",      f_ethexpro_ping,          INVALID_FCODE },
	{ "selftest",  f_ethexpro_selftest,      INVALID_FCODE },
	{ NULL,        NULL },
};

PCI(install_ethexpro_driver)
{
	Retcode ret;
	Pci_device_info *newinfo;

	DPRINTF(("install_ethexpro_driver: pkg=%p devinfo=%p"
			" vendid=%#x devid=%#x\n",
			pkg, devinfo, devinfo->vendid, devinfo->devid));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "network", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, ethexpro_methods);

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	pkg->self = (struct pself*)newinfo;		/* and save it */

	return ret;
}

Pci_driver intel_etherexpress_pro =
{
	{ 0, 0, 0, 0, 0, 0x20000, 0x8086, 0x1229, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_ethexpro_driver
};

Pci_driver intel_etherexpress_pro_er =
{
	{ 0, 0, 0, 0, 0, 0x20000, 0x8086, 0x1209, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 },
	install_ethexpro_driver
};
