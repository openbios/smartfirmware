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

/* Driver for NE2000-compatible Ethernet.  Some code copied from
   FreeBSD sys/i386/isa/if_edreg.h and sys/i386/isa/if_ed.h
 */

#include "defs.h"
#include "isa.h"

extern void dump_packet(Environ *e, uByte *pkt, uInt len);


#define MAC_ADDR	6	/* Ethernet hardware address length */


/*
 * Copyright (C) 1993, David Greenman. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 */
/*
 * National Semiconductor DS8390 NIC register definitions
 *
 *
 * Modification history
 *
 * Revision 2.2  1993/11/29  16:33:39  davidg
 * From Thomas Sandford <t.d.g.sandford@comp.brad.ac.uk>
 * Add support for the 8013W board type
 *
 * Revision 2.1  1993/11/22  10:52:33  davidg
 * patch to add support for SMC8216 (Elite-Ultra) boards
 * from Glen H. Lowe
 *
 * Revision 2.0  93/09/29  00:37:15  davidg
 * changed double buffering flag to multi buffering
 * made changes/additions for 3c503 multi-buffering
 * ...companion to Rev. 2.0 of 'ed' driver.
 *
 * Revision 1.1  93/06/23  03:01:07  davidg
 * Initial revision
 *
 */

/*
 * Page 0 register offsets
 */
#define ED_P0_CR	0x00	/* Command Register */

#define ED_P0_CLDA0	0x01	/* Current Local DMA Addr low (read) */
#define ED_P0_PSTART	0x01	/* Page Start register (write) */

#define ED_P0_CLDA1	0x02	/* Current Local DMA Addr high (read) */
#define ED_P0_PSTOP	0x02	/* Page Stop register (write) */

#define ED_P0_BNRY	0x03	/* Boundary Pointer */

#define ED_P0_TSR	0x04	/* Transmit Status Register (read) */
#define ED_P0_TPSR	0x04	/* Transmit Page Start (write) */

#define ED_P0_NCR	0x05	/* Number of Collisions Reg (read) */
#define ED_P0_TBCR0	0x05	/* Transmit Byte count, low (write) */

#define ED_P0_FIFO	0x06	/* FIFO register (read) */
#define ED_P0_TBCR1	0x06	/* Transmit Byte count, high (write) */

#define ED_P0_ISR	0x07	/* Interrupt Status Register */

#define ED_P0_CRDA0	0x08	/* Current Remote DMA Addr low (read) */
#define ED_P0_RSAR0	0x08	/* Remote Start Address low (write) */

#define ED_P0_CRDA1	0x09	/* Current Remote DMA Addr high (read) */
#define ED_P0_RSAR1	0x09	/* Remote Start Address high (write) */

#define ED_P0_RBCR0	0x0a	/* Remote Byte Count low (write) */
#define ED_P0_RBCR1	0x0b	/* Remote Byte Count high (write) */

#define ED_P0_RSR	0x0c	/* Receive Status (read) */
#define ED_P0_RCR	0x0c	/* Receive Configuration Reg (write) */

#define ED_P0_CNTR0	0x0d	/* frame alignment error counter (read) */
#define ED_P0_TCR	0x0d	/* Transmit Configuration Reg (write) */

#define ED_P0_CNTR1	0x0e	/* CRC error counter (read) */
#define ED_P0_DCR	0x0e	/* Data Configuration Reg (write) */

#define ED_P0_CNTR2	0x0f	/* missed packet counter (read) */
#define ED_P0_IMR	0x0f	/* Interrupt Mask Register (write) */

/*
 * Page 1 register offsets
 */
#define ED_P1_CR	0x00	/* Command Register */
#define ED_P1_PAR0	0x01	/* Physical Address Register 0 */
#define ED_P1_PAR1	0x02	/* Physical Address Register 1 */
#define ED_P1_PAR2	0x03	/* Physical Address Register 2 */
#define ED_P1_PAR3	0x04	/* Physical Address Register 3 */
#define ED_P1_PAR4	0x05	/* Physical Address Register 4 */
#define ED_P1_PAR5	0x06	/* Physical Address Register 5 */
#define ED_P1_CURR	0x07	/* Current RX ring-buffer page */
#define ED_P1_MAR0	0x08	/* Multicast Address Register 0 */
#define ED_P1_MAR1	0x09	/* Multicast Address Register 1 */
#define ED_P1_MAR2	0x0a	/* Multicast Address Register 2 */
#define ED_P1_MAR3	0x0b	/* Multicast Address Register 3 */
#define ED_P1_MAR4	0x0c	/* Multicast Address Register 4 */
#define ED_P1_MAR5	0x0d	/* Multicast Address Register 5 */
#define ED_P1_MAR6	0x0e	/* Multicast Address Register 6 */
#define ED_P1_MAR7	0x0f	/* Multicast Address Register 7 */

/*
 * Page 2 register offsets
 */
#define ED_P2_CR	0x00	/* Command Register */
#define ED_P2_PSTART	0x01	/* Page Start (read) */
#define ED_P2_CLDA0	0x01	/* Current Local DMA Addr 0 (write) */
#define ED_P2_PSTOP	0x02	/* Page Stop (read) */
#define ED_P2_CLDA1	0x02	/* Current Local DMA Addr 1 (write) */
#define ED_P2_RNPP	0x03	/* Remote Next Packet Pointer */
#define ED_P2_TPSR	0x04	/* Transmit Page Start (read) */
#define ED_P2_LNPP	0x05	/* Local Next Packet Pointer */
#define ED_P2_ACU	0x06	/* Address Counter Upper */
#define ED_P2_ACL	0x07	/* Address Counter Lower */
#define ED_P2_RCR	0x0c	/* Receive Configuration Register (read) */
#define ED_P2_TCR	0x0d	/* Transmit Configuration Register (read) */
#define ED_P2_DCR	0x0e	/* Data Configuration Register (read) */
#define ED_P2_IMR	0x0f	/* Interrupt Mask Register (read) */

/*
 *		Command Register (CR) definitions
 */

/*
 * STP: SToP. Software reset command. Takes the controller offline. No
 *	packets will be received or transmitted. Any reception or
 *	transmission in progress will continue to completion before
 *	entering reset state. To exit this state, the STP bit must
 *	reset and the STA bit must be set. The software reset has
 *	executed only when indicated by the RST bit in the ISR being
 *	set.
 */
#define ED_CR_STP	0x01

/*
 * STA: STArt. This bit is used to activate the NIC after either power-up,
 *	or when the NIC has been put in reset mode by software command
 *	or error.
 */
#define ED_CR_STA	0x02

/*
 * TXP: Transmit Packet. This bit must be set to indicate transmission of
 *	a packet. TXP is internally reset either after the transmission is
 *	completed or aborted. This bit should be set only after the Transmit
 *	Byte Count and Transmit Page Start register have been programmed.
 */
#define ED_CR_TXP	0x04

/*
 * RD0, RD1, RD2: Remote DMA Command. These three bits control the operation
 *	of the remote DMA channel. RD2 can be set to abort any remote DMA
 *	command in progress. The Remote Byte Count registers should be cleared
 *	when a remote DMA has been aborted. The Remote Start Addresses are not
 *	restored to the starting address if the remote DMA is aborted.
 *
 *	RD2 RD1 RD0	function
 *	 0   0   0	not allowed
 *	 0   0   1	remote read
 *	 0   1   0	remote write
 *	 0   1   1	send packet
 *	 1   X   X	abort
 */
#define ED_CR_RD0	0x08
#define ED_CR_RD1	0x10
#define ED_CR_RD2	0x20

/*
 * PS0, PS1: Page Select. The two bits select which register set or 'page' to
 *	access.
 *
 *	PS1 PS0		page
 *	 0   0		0
 *	 0   1		1
 *	 1   0		2
 *	 1   1		reserved
 */
#define ED_CR_PS0	0x40
#define ED_CR_PS1	0x80
/* bit encoded aliases */
#define ED_CR_PAGE_0	0x00 /* (for consistency) */
#define ED_CR_PAGE_1	0x40
#define ED_CR_PAGE_2	0x80

/*
 *		Interrupt Status Register (ISR) definitions
 */

/*
 * PRX: Packet Received. Indicates packet received with no errors.
 */
#define ED_ISR_PRX	0x01

/*
 * PTX: Packet Transmitted. Indicates packet transmitted with no errors.
 */
#define ED_ISR_PTX	0x02

/*
 * RXE: Receive Error. Indicates that a packet was received with one or more
 *	the following errors: CRC error, frame alignment error, FIFO overrun,
 *	missed packet.
 */
#define ED_ISR_RXE	0x04

/*
 * TXE: Transmission Error. Indicates that an attempt to transmit a packet
 *	resulted in one or more of the following errors: excessive
 *	collisions, FIFO underrun.
 */
#define ED_ISR_TXE	0x08

/*
 * OVW: OverWrite. Indicates a receive ring-buffer overrun. Incoming network
 *	would exceed (has exceeded?) the boundary pointer, resulting in data
 *	that was previously received and not yet read from the buffer to be
 *	overwritten.
 */
#define ED_ISR_OVW	0x10

/*
 * CNT: Counter Overflow. Set when the MSB of one or more of the Network Talley
 *	Counters has been set.
 */
#define ED_ISR_CNT	0x20

/*
 * RDC: Remote Data Complete. Indicates that a Remote DMA operation has completed.
 */
#define ED_ISR_RDC	0x40

/*
 * RST: Reset status. Set when the NIC enters the reset state and cleared when a
 *	Start Command is issued to the CR. This bit is also set when a receive
 *	ring-buffer overrun (OverWrite) occurs and is cleared when one or more
 *	packets have been removed from the ring. This is a read-only bit.
 */
#define ED_ISR_RST	0x80

/*
 *		Interrupt Mask Register (IMR) definitions
 */

/*
 * PRXE: Packet Received interrupt Enable. If set, a received packet will cause
 *	an interrupt.
 */
#define ED_IMR_PRXE	0x01

/*
 * PTXE: Packet Transmit interrupt Enable. If set, an interrupt is generated when
 *	a packet transmission completes.
 */
#define ED_IMR_PTXE	0x02

/*
 * RXEE: Receive Error interrupt Enable. If set, an interrupt will occur whenever a
 *	packet is received with an error.
 */
#define ED_IMR_RXEE 	0x04

/*
 * TXEE: Transmit Error interrupt Enable. If set, an interrupt will occur whenever
 *	a transmission results in an error.
 */
#define ED_IMR_TXEE	0x08

/*
 * OVWE: OverWrite error interrupt Enable. If set, an interrupt is generated whenever
 *	the receive ring-buffer is overrun. i.e. when the boundary pointer is exceeded.
 */
#define ED_IMR_OVWE	0x10

/*
 * CNTE: Counter overflow interrupt Enable. If set, an interrupt is generated whenever
 *	the MSB of one or more of the Network Statistics counters has been set.
 */
#define ED_IMR_CNTE	0x20

/*
 * RDCE: Remote DMA Complete interrupt Enable. If set, an interrupt is generated
 *	when a remote DMA transfer has completed.
 */
#define ED_IMR_RDCE	0x40

/*
 * bit 7 is unused/reserved
 */

/*
 *		Data Configuration Register (DCR) definitions
 */

/*
 * WTS: Word Transfer Select. WTS establishes byte or word transfers for
 *	both remote and local DMA transfers
 */
#define ED_DCR_WTS	0x01

/*
 * BOS: Byte Order Select. BOS sets the byte order for the host.
 *	Should be 0 for 80x86, and 1 for 68000 series processors
 */
#define ED_DCR_BOS	0x02

/*
 * LAS: Long Address Select. When LAS is 1, the contents of the remote
 *	DMA registers RSAR0 and RSAR1 are used to provide A16-A31
 */
#define ED_DCR_LAS	0x04

/*
 * LS: Loopback Select. When 0, loopback mode is selected. Bits D1 and D2
 *	of the TCR must also be programmed for loopback operation.
 *	When 1, normal operation is selected.
 */
#define ED_DCR_LS	0x08

/*
 * AR: Auto-initialize Remote. When 0, data must be removed from ring-buffer
 *	under program control. When 1, remote DMA is automatically initiated
 *	and the boundary pointer is automatically updated
 */
#define ED_DCR_AR	0x10

/*
 * FT0, FT1: Fifo Threshold select.
 *		FT1	FT0	Word-width	Byte-width
 *		 0	 0	1 word		2 bytes
 *		 0	 1	2 words		4 bytes
 *		 1	 0	4 words		8 bytes
 *		 1	 1	8 words		12 bytes
 *
 *	During transmission, the FIFO threshold indicates the number of bytes
 *	or words that the FIFO has filled from the local DMA before BREQ is
 *	asserted. The transmission threshold is 16 bytes minus the receiver
 *	threshold.
 */
#define ED_DCR_FT0	0x20
#define ED_DCR_FT1	0x40

/*
 * bit 7 (0x80) is unused/reserved
 */

/*
 *		Transmit Configuration Register (TCR) definitions
 */

/*
 * CRC: Inhibit CRC. If 0, CRC will be appended by the transmitter, if 0, CRC
 *	is not appended by the transmitter.
 */
#define ED_TCR_CRC	0x01

/*
 * LB0, LB1: Loopback control. These two bits set the type of loopback that is
 *	to be performed.
 *
 *	LB1 LB0		mode
 *	 0   0		0 - normal operation (DCR_LS = 0)
 *	 0   1		1 - internal loopback (DCR_LS = 0)
 *	 1   0		2 - external loopback (DCR_LS = 1)
 *	 1   1		3 - external loopback (DCR_LS = 0)
 */
#define ED_TCR_LB0	0x02
#define ED_TCR_LB1	0x04

/*
 * ATD: Auto Transmit Disable. Clear for normal operation. When set, allows
 *	another station to disable the NIC's transmitter by transmitting to
 *	a multicast address hashing to bit 62. Reception of a multicast address
 *	hashing to bit 63 enables the transmitter.
 */
#define ED_TCR_ATD	0x08

/*
 * OFST: Collision Offset enable. This bit when set modifies the backoff
 *	algorithm to allow prioritization of nodes.
 */
#define ED_TCR_OFST	0x10

/*
 * bits 5, 6, and 7 are unused/reserved
 */

/*
 *		Transmit Status Register (TSR) definitions
 */

/*
 * PTX: Packet Transmitted. Indicates successful transmission of packet.
 */
#define ED_TSR_PTX	0x01

/*
 * bit 1 (0x02) is unused/reserved
 */

/*
 * COL: Transmit Collided. Indicates that the transmission collided at least
 *	once with another station on the network.
 */
#define ED_TSR_COL	0x04

/*
 * ABT: Transmit aborted. Indicates that the transmission was aborted due to
 *	excessive collisions.
 */
#define ED_TSR_ABT	0x08

/*
 * CRS: Carrier Sense Lost. Indicates that carrier was lost during the
 *	transmission of the packet. (Transmission is not aborted because
 *	of a loss of carrier)
 */
#define ED_TSR_CRS	0x10

/*
 * FU: FIFO Underrun. Indicates that the NIC wasn't able to access bus/
 *	transmission memory before the FIFO emptied. Transmission of the
 *	packet was aborted.
 */
#define ED_TSR_FU	0x20

/*
 * CDH: CD Heartbeat. Indicates that the collision detection circuitry
 *	isn't working correctly during a collision heartbeat test.
 */
#define ED_TSR_CDH	0x40

/*
 * OWC: Out of Window Collision: Indicates that a collision occurred after
 *	a slot time (51.2us). The transmission is rescheduled just as in
 *	normal collisions.
 */
#define ED_TSR_OWC	0x80

/*
 *		Receiver Configuration Register (RCR) definitions
 */

/*
 * SEP: Save Errored Packets. If 0, error packets are discarded. If set to 1,
 *	packets with CRC and frame errors are not discarded.
 */
#define ED_RCR_SEP	0x01

/*
 * AR: Accept Runt packet. If 0, packet with less than 64 byte are discarded.
 *	If set to 1, packets with less than 64 byte are not discarded.
 */
#define ED_RCR_AR	0x02

/*
 * AB: Accept Broadcast. If set, packets sent to the broadcast address will be
 *	accepted.
 */
#define ED_RCR_AB	0x04

/*
 * AM: Accept Multicast. If set, packets sent to a multicast address are checked
 *	for a match in the hashing array. If clear, multicast packets are ignored.
 */
#define ED_RCR_AM	0x08

/*
 * PRO: Promiscuous Physical. If set, all packets with a physical addresses are
 *	accepted. If clear, a physical destination address must match this
 *	station's address. Note: for full promiscuous mode, RCR_AB and RCR_AM
 *	must also be set. In addition, the multicast hashing array must be set
 *	to all 1's so that all multicast addresses are accepted.
 */
#define ED_RCR_PRO	0x10

/*
 * MON: Monitor Mode. If set, packets will be checked for good CRC and framing,
 *	but are not stored in the ring-buffer. If clear, packets are stored (normal
 *	operation).
 */
#define ED_RCR_MON	0x20

/*
 * bits 6 and 7 are unused/reserved.
 */

/*
 *		Receiver Status Register (RSR) definitions
 */

/*
 * PRX: Packet Received without error.
 */
#define ED_RSR_PRX	0x01

/*
 * CRC: CRC error. Indicates that a packet has a CRC error. Also set for frame
 *	alignment errors.
 */
#define ED_RSR_CRC	0x02

/*
 * FAE: Frame Alignment Error. Indicates that the incoming packet did not end on
 *	a byte boundary and the CRC did not match at the last byte boundary.
 */
#define ED_RSR_FAE	0x04

/*
 * FO: FIFO Overrun. Indicates that the FIFO was not serviced (during local DMA)
 *	causing it to overrun. Reception of the packet is aborted.
 */
#define ED_RSR_FO	0x08

/*
 * MPA: Missed Packet. Indicates that the received packet couldn't be stored in
 *	the ring-buffer because of insufficient buffer space (exceeding the
 *	boundary pointer), or because the transfer to the ring-buffer was inhibited
 *	by RCR_MON - monitor mode.
 */
#define ED_RSR_MPA	0x10

/*
 * PHY: Physical address. If 0, the packet received was sent to a physical address.
 *	If 1, the packet was accepted because of a multicast/broadcast address
 *	match.
 */
#define ED_RSR_PHY	0x20

/*
 * DIS: Receiver Disabled. Set to indicate that the receiver has entered monitor
 *	mode. Cleared when the receiver exits monitor mode.
 */
#define ED_RSR_DIS	0x40

/*
 * DFR: Deferring. Set to indicate a 'jabber' condition. The CRS and COL inputs
 *	are active, and the transceiver has set the CD line as a result of the
 *	jabber.
 */
#define ED_RSR_DFR	0x80

/*
 * receive ring descriptor
 *
 * The National Semiconductor DS8390 Network interface controller uses
 * the following receive ring headers.  The way this works is that the
 * memory on the interface card is chopped up into 256 bytes blocks.
 * A contiguous portion of those blocks are marked for receive packets
 * by setting start and end block #'s in the NIC.  For each packet that
 * is put into the receive ring, one of these headers (4 bytes each) is
 * tacked onto the front. The first byte is a copy of the receiver status
 * register at the time the packet was received.
 */
struct ed_ring	{
#ifdef NE2000_BROKEN
	uByte	next_packet;	/* pointer to next packet	*/
	uByte	rsr;			/* receiver status */
#else
	uByte	rsr;			/* receiver status */
	uByte	next_packet;	/* pointer to next packet	*/
#endif
	uByte	count0, count1;	/* bytes in packet (length + 4)	*/
};

/*
 * 				Common constants
 */
#define ED_PAGE_SIZE		256		/* Size of RAM pages in bytes */
#define ED_TXBUF_SIZE		6		/* Size of TX buffer in pages */

/*
 *		 Definitions for Novell NE1000/2000 boards
 */

/*
 * Register offsets/total
 */
#define ED_NOVELL_NIC_OFFSET	0x00
#define ED_NOVELL_ASIC_OFFSET	0x10
#define ED_NOVELL_IO_PORTS	32

/*
 * Remote DMA data register; for reading or writing to the NIC mem
 *	via programmed I/O (offset from ASIC base)
 */
#define ED_NOVELL_DATA		0x00

/*
 * Reset register; reading from this register causes a board reset
 */
#define ED_NOVELL_RESET		0x0f


/* [...] bunch of other stuff to support other cards removed */


/********* end of copied code from if_edreg.h *********/

/* assume NE2000 with 16k of on-board RAM */
#define ED_MEM_SIZE			(8192*2)	/* size of memory on board */
#define ED_MEM_START		(8192*2)	/* mem does not start at 0 */
#define ED_MEM_END			(ED_MEM_START + ED_MEM_SIZE)

#define ED_TX_PAGE_START	(ED_MEM_START / ED_PAGE_SIZE)

#define ED_REC_PAGE_START	(ED_TX_PAGE_START + ED_TXBUF_SIZE)
#define ED_REC_PAGE_STOP	(ED_TX_PAGE_START + ED_MEM_SIZE / ED_PAGE_SIZE)

#define ED_MEM_RING			(ED_MEM_START + ED_PAGE_SIZE * ED_TXBUF_SIZE)


/* instance-specific vars */
struct self
{
	Isa_device *dev;
	Instance *obptftp;
	uByte mac_addr[MAC_ADDR];	/* Ethernet hardware address */
};
typedef struct self Self;


#define outb(s, r, b) isa_io_write((s)->dev->physlo + (r), (b), 1);
#define inb(s, r, b) isa_io_read((s)->dev->physlo + (r), &(b), 1);


void
pio_read(Self *s, Int src, uByte *dst, Int len)
{
	uShort val;
	uByte b;
	int i = 100;

	DPRINTF(("pio_read: src=%p dst=%p len=%d\n", src, dst, len));
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* select page 0 */

	if (len & 1)		/* round */
		len++;

	/* setup DMA count */
	outb(s, ED_P0_RBCR0, len);
	outb(s, ED_P0_RBCR1, len >> 8);

	/* set up source addr */
	outb(s, ED_P0_RSAR0, src);
	outb(s, ED_P0_RSAR1, src >> 8);

	outb(s, ED_P0_CR, ED_CR_RD0 | ED_CR_STA);
	DPRINTF(("pio_read: 0"));

	while ((len -= 2) >= 0)
	{
		isa_io_read(s->dev->physlo + ED_NOVELL_ASIC_OFFSET +
				ED_NOVELL_DATA, &val, 2);
		DPRINTF(("x%04X", val));

#ifdef LITTLE_ENDIAN
		*dst++ = val & 0xFF;
		*dst++ = val >> 8;
#else
		*dst++ = val >> 8;
		*dst++ = val & 0xFF;
#endif
	}

	/* wait for DMA complete */
	do
	{
		u_sleep(10);
		inb(s, ED_P0_ISR, b);
	} while ((b & ED_ISR_RDC) != ED_ISR_RDC && i--);

	outb(s, ED_P0_ISR, ED_ISR_RDC);
	DPRINTF(("\n"));
}

void
pio_write(Self *s, uByte *src, Int dst, Int len)
{
	uShort val;
	uByte b;
	int i = 100;

	DPRINTF(("pio_write: src=%p dst=%p len=%d\n", src, dst, len));
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* select page 0 */
	outb(s, ED_P0_ISR, ED_ISR_RDC);		/* reset DMA complete */

	if (len & 1)		/* round */
		len++;

	/* setup DMA count */
	outb(s, ED_P0_RBCR0, len);
	outb(s, ED_P0_RBCR1, len >> 8);

	/* set up destination addr */
	outb(s, ED_P0_RSAR0, dst);
	outb(s, ED_P0_RSAR1, dst >> 8);

	outb(s, ED_P0_CR, ED_CR_RD1 | ED_CR_STA);

	while ((len -= 2) >= 0)
	{
#ifdef LITTLE_ENDIAN
		val = *src++;
		val |= *src++ << 8;
#else
		val = *src++ << 8;
		val |= *src++;
#endif
		isa_io_write(s->dev->physlo + ED_NOVELL_ASIC_OFFSET +
				ED_NOVELL_DATA, val, 2);
	}

	/* wait for DMA complete */
	do
	{
		u_sleep(10);
		inb(s, ED_P0_ISR, b);
	} while ((b & ED_ISR_RDC) != ED_ISR_RDC && i--);

	outb(s, ED_P0_ISR, ED_ISR_RDC);
}


Retcode
read_mac_addr(Environ *e, Package *pkg, Self *s)
{
	Int i;
	uByte romdata[16];

	pio_read(s, 0, romdata, 16);

#ifdef DEBUG
	dprintf("read_mac_addr: 0");

	for (i = 0; i < 16; i++)
		dprintf("x%02x", romdata[i]);

	dprintf("\n");
#endif

#ifdef LITTLE_ENDIAN
	for (i = 0; i < MAC_ADDR; i++)
		s->mac_addr[i] = romdata[i * 2];
#else
	for (i = 0; i < MAC_ADDR; i++)
		s->mac_addr[i] = romdata[i * 2 + 1];
#endif

	return NO_ERROR;
}


C(f_ne2000_open)			/* open (-- okay?) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Bool promiscuous = FALSE;
	Retcode ret;
	Isa_device *dev;
	Self *s;
	Int i, maclen;
	uByte b, *macaddr;

	DPRINTF(("ne2000_open: pkg=%p\n", pkg));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->dev = dev;

	/* parse arguments - they are all optional - following args
	   are handed off to the obptftp package */
	if (inst->args && *inst->args)
	{
		Byte *str = inst->args + 1;
		Int len = *(uByte*)inst->args;
		
		while (len--)
		{
			while (len && isspace(*str))
			{
				str++;
				len--;
			}
			
			if (*str == '\0')
				break;

			DPRINTF(("ne2000_ether: args=\"%s\"\n", str));

			#define IS_END(c)	((c) == ',' || (c) == '\0' || (len) == 0)
			#define IS_MORE(c)	(!IS_END(c))
		
			if (compare_strs(str, 11, "promiscuous", 11) && IS_END(str[11]))
			{
				str += 11;
				len -= 11;
				promiscuous = TRUE;
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
		if (diag)
			cprintf(e, "File and args: \"%s\"\n", str);

		PUSH(e, (uPtr)str);		/* arg-str */
		PUSH(e, len + 1);	/* arg-len */
	}
	else
	{
		PUSH(e, "");	/* arg-str */
		PUSH(e, 0);		/* arg-len */
	}

#ifdef DEBUG
	inb(s, ED_P0_CR, b);
	dprintf("ne2000_read: CR=%#x\n", b);
	inb(s, ED_P0_ISR, b);
	dprintf("ne2000_read: ISR=%#x\n", b);
	inb(s, ED_P0_RCR, b);
	dprintf("ne2000_read: RCR=%#x\n", b);
	inb(s, ED_P0_DCR, b);
	dprintf("ne2000_read: DCR=%#x\n", b);
#endif

	/* reset the chip */
	isa_io_read(dev->physlo + ED_NOVELL_ASIC_OFFSET + ED_NOVELL_RESET, &b, 1);
	isa_io_write(dev->physlo + ED_NOVELL_ASIC_OFFSET + ED_NOVELL_RESET, b, 1);
	u_sleep(500);
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STP);
	u_sleep(500);
	outb(s, ED_P0_RCR, ED_RCR_MON);		/* no packets in memory for now */

	/* 16-bit wide xfers in local-host order */
#ifdef LITTLE_ENDIAN
	outb(s, ED_P0_DCR, ED_DCR_FT1 | ED_DCR_WTS | ED_DCR_LS);
#else
	outb(s, ED_P0_DCR, ED_DCR_FT1 | ED_DCR_WTS | ED_DCR_LS | ED_DCR_BOS);
#endif

	outb(s, ED_P0_PSTART, ED_MEM_SIZE / ED_PAGE_SIZE);
	outb(s, ED_P0_PSTOP, ED_MEM_SIZE * 2 / ED_PAGE_SIZE);

	/* try to read in the local MAC addr out of ROM and use it
	   to intialize the parameter "local-mac-address" */
	ret = read_mac_addr(e, pkg, s);
	DPRINTF(("read_mac_addr: s->mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
				s->mac_addr[0], s->mac_addr[1], s->mac_addr[2],
				s->mac_addr[3], s->mac_addr[4], s->mac_addr[5]));

	if (ret == NO_ERROR)
		ret = set_property(pkg->props, "local-mac-address", CSTR,
				(Byte*)s->mac_addr, MAC_ADDR);

	/* create an instance of /packages/obptftp to support "load" -
	   must do this after we read in and set our local MAC addr or the
	   package will get an incorrect value for it - note optional args
	   are already pushed on the stack from above */
	PUSH(e, "obp-tftp");			/* name-str */
	PUSH(e, strlen("obp-tftp"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("ne2000_open: $open-package ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("ne2000_open: obptftp=%p\n", s->obptftp));

	if (s->obptftp == NULL)
		return E_NULL_INSTANCE;

	/* get the mac-address, which should be our local-mac-address */
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

	/* initialize for both xmit and recv */
	outb(s, ED_P0_RBCR0, 0);			/* reset counters */
	outb(s, ED_P0_RBCR1, 0);
	outb(s, ED_P0_TCR, ED_TCR_LB0);		/* internal loopback (temporary) */

	outb(s, ED_P0_TPSR, ED_TX_PAGE_START);		/* init xmit buf ring */
	outb(s, ED_P0_PSTART, ED_REC_PAGE_START);	/* init recv buf ring */
	outb(s, ED_P0_PSTOP, ED_REC_PAGE_STOP);		/* init recv page stop */
	outb(s, ED_P0_BNRY, ED_REC_PAGE_START);		/* init recv boundary */

	outb(s, ED_P0_IMR, 0x00);			/* disable all interrupts */
	outb(s, ED_P0_ISR, 0xFF);			/* clear any pending interrupts */

	/* initialize ethernet MAC (hardware) address */
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_1 | ED_CR_STP);	/* page 1 */

	for (i = 0; i < MAC_ADDR; i++)
		outb(s, ED_P1_PAR0 + i, macaddr[i]);

	outb(s, ED_P1_CURR, ED_REC_PAGE_START);	/* set current packet */

	/* set RCR */
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_1 | ED_CR_STP);	/* page 1 */

	if (promiscuous)
	{
		for (i = 0; i < MAC_ADDR; i++)
			outb(s, ED_P1_MAR0, 0xFF);		/* multicast filter */

		outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STP);	/* page 0 */
		outb(s, ED_P0_RCR, ED_RCR_PRO | ED_RCR_AB);	/* promiscuous+broadcast */
	}
	else
	{
		for (i = 0; i < MAC_ADDR; i++)
			outb(s, ED_P1_MAR0, 0x00);		/* no multicast */

		outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STP);	/* page 0 */
		outb(s, ED_P0_RCR, ED_RCR_AB);		/* accept broadcasts */
	}

	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* start interface */
	outb(s, ED_P0_TCR, 0);		/* turn off loopback */

	DPRINTF(("ne2000_open: done\n"));
	PUSH(e, FTRUE);
	return ret;
}

C(f_ne2000_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Isa_device *dev;
	Retcode ret;

	if (!pkg)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	/* stop the card */
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STP);

	IFCKSP(e, 0, 1);
	PUSH(e, (uPtr)s->obptftp);
	ret = execute_word(e, "close-package");

	free(s);
	return ret;
}

C(f_ne2000_dma_alloc)		/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ne2000_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ne2000_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ne2000_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}


C(f_ne2000_read)				/* read (addr len -- actual) */
{
	Int l, len, pkt, actual = -2;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Isa_device *dev;
	uByte isr, curr, b;
	struct ed_ring hdr;

	if (!pkg)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("ne2000_read: addr=%p len=%d\n", addr, len));

	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* page 0 */
	inb(s, ED_P0_ISR, isr);
	DPRINTF(("ne2000_read: ISR=%#x\n", isr));
	outb(s, ED_P0_ISR, isr);	/* ack any interrupts */

	if (isr & ED_RSR_PRX)	/* got a packet */
	{
		inb(s, ED_P0_RSR, b);
		DPRINTF(("ne2000_read: RSR=%#x\n", b));

		inb(s, ED_P0_BNRY, b);			/* boundary - start of pkt */
		outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_1 | ED_CR_STP);	/* page 1 */
		inb(s, ED_P1_CURR, curr);		/* current - end of pkt */
		DPRINTF(("ne2000_read: before BNRY=%#x CURR=%#x\n", b, curr));

		if (b != curr)			/* packet to be read */
		{
			pkt = ED_MEM_RING + (b - ED_REC_PAGE_START) * ED_PAGE_SIZE;
			pio_read(s, pkt, (uByte*)&hdr, sizeof hdr);
			DPRINTF((
		"ne2000_read: pkt=%p RSR=%#x next_packet=%#x count=%#x.%x\n",
					pkt, hdr.rsr, hdr.next_packet, hdr.count0, hdr.count1));

			pkt += sizeof hdr;
			l = actual = ((hdr.count1 << 8) | hdr.count0) - sizeof hdr;

			if (l > len)
				cprintf(e,
						"NE2000 ne2000_read: pktlen %d but only room for %d!\n",
						l, len);

			if (pkt + l > ED_MEM_END)	/* wrapped ring */
			{
				Int t = ED_MEM_END - pkt;
				pio_read(s, pkt, addr, t);
				l -= t;
				pio_read(s, ED_MEM_RING, addr + t, l);
			}
			else
				pio_read(s, pkt, addr, l);

		#ifdef DEBUG
			dump_packet(e, addr, actual);
		#endif
			outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STP);	/* page 0 */
			/*outb(s, ED_P0_BNRY, curr);*/	/* update boundary reg */
			outb(s, ED_P0_BNRY, ED_REC_PAGE_START);		/* init recv boundary */
			outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_1 | ED_CR_STP);	/* page 1 */
			outb(s, ED_P1_CURR, ED_REC_PAGE_START);	/* set current packet */
			outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* start again */
		}
	}

	DPRINTF(("ne2000_read: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

C(f_ne2000_write)			/* write (addr len -- actual) */
{
	Int len, actual = 0;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Isa_device *dev;
	uByte b;

	if (!pkg)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("ne2000_write: addr=%p len=%d\n", addr, len));
#ifdef DEBUG
	dump_packet(e, addr, len);
#endif

	if (len < 60)		/* min data length is 46, plus 14 for header */
		len = 60;

	pio_write(s, addr, ED_MEM_START, len);

	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);	/* page 0 */
	outb(s, ED_P0_TPSR, ED_TX_PAGE_START);		/* TX start page */
	outb(s, ED_P0_TBCR0, len);
	outb(s, ED_P0_TBCR1, len >> 8);
	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_TXP | ED_CR_STA);	/* xmit */

	do
	{
		inb(s, ED_P0_ISR, b);
		DPRINTF(("ne2000_write: ISR=%#x\n", b));
	} while (!(b & (ED_ISR_PTX | ED_ISR_TXE)));

	if (b & ED_ISR_PTX)
		actual = len;

	outb(s, ED_P0_ISR, ED_ISR_PTX | ED_ISR_TXE);	/* ack */
	inb(s, ED_P0_TSR, b);
	DPRINTF(("ne2000_write: TSR=%#x\n", b));

	outb(s, ED_P0_CR, ED_CR_RD2 | ED_CR_STA);		/* start */
	DPRINTF(("ne2000_write: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

static Retcode
do_obptftp_method(Environ *e, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	Isa_device *dev;

	if (!pkg)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	if (!s->obptftp)
		return E_NULL_INSTANCE;

	/* let the TFTP/BOOTP package do the work */
	return execute_method_name(e, s->obptftp, method, CSTR);
}

C(f_ne2000_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_ne2000_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

C(f_ne2000_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Isa_device *dev;

	DPRINTF(("ether selftest: begin\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	DPRINTF(("ether selftest: got pkg\n"));
	dev = (Isa_device*)pkg->self;

	if (!dev)
		return E_BAD_PACKAGE;

	DPRINTF(("ether selftest: got dev\n"));

	/* TODO */

	if (diag)
	{
		cprintf(e, "ether selftest\n");
		/* TODO */
	}

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}


static const Initentry ne2000_methods[] =
{
	{ "open",      f_ne2000_open,          INVALID_FCODE },
	{ "close",     f_ne2000_close,         INVALID_FCODE },
	{ "dma-alloc", f_ne2000_dma_alloc,     INVALID_FCODE },
	{ "dma-free",  f_ne2000_dma_free,      INVALID_FCODE },
	{ "read",      f_ne2000_read,          INVALID_FCODE },
	{ "write",     f_ne2000_write,         INVALID_FCODE },
	{ "load",      f_ne2000_load,          INVALID_FCODE },
	{ "ping",      f_ne2000_ping,          INVALID_FCODE },
	{ "selftest",  f_ne2000_selftest,      INVALID_FCODE },
	{ NULL,        NULL },
};

ISA(ne2000_probe)
{
	uByte b;

	/* reset the board */
	DPRINTF(("ne2000_probe: enter physlo=%#x\n", dev->physlo));
	isa_io_read(dev->physlo + ED_NOVELL_ASIC_OFFSET + ED_NOVELL_RESET, &b, 1);
	isa_io_write(dev->physlo + ED_NOVELL_ASIC_OFFSET + ED_NOVELL_RESET, b, 1);
	u_sleep(500);
	isa_io_write(dev->physlo + ED_P0_CR, ED_CR_RD2 | ED_CR_STP, 1);
	u_sleep(500);

	/* verify CR and ISR registers say the board is reset */
	isa_io_read(dev->physlo + ED_P0_CR, &b, 1);
	DPRINTF(("ne2000_probe: CR=%#x\n", b));

	if ((b & (ED_CR_RD2 | ED_CR_TXP | ED_CR_STA | ED_CR_STP)) !=
			(ED_CR_RD2 | ED_CR_STP))
		return E_NO_DEVICE;

	u_sleep(500);
	isa_io_read(dev->physlo + ED_P0_ISR, &b, 1);
	DPRINTF(("ne2000_probe: ISR=%#x\n", b));

	if ((b & ED_ISR_RST) != ED_ISR_RST)
		return E_NO_DEVICE;

	DPRINTF(("ne2000_probe: success!\n"));
	return NO_ERROR;
}

ISA(ne2000_install)
{
	return new_isa_device(e, dev);
}

Isa_device ne2000_0x300_3 =
{
	"ethernet",
	"network",
	ISA_IO_ADDRESS,
	0x300,
	32,
	{ 3, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0xC000,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ne2000_probe,
	ne2000_install,
	ne2000_methods
};

Isa_device ne2000_0x280_5 =
{
	"ethernet",
	"network",
	ISA_IO_ADDRESS,
	0x280,
	32,
	{ 5, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0xC800,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ne2000_probe,
	ne2000_install,
	ne2000_methods
};

Isa_device ne2000_0x280_10 =
{
	"ethernet",
	"network",
	ISA_IO_ADDRESS,
	0x280,
	32,
	{ 10, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0xC800,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ne2000_probe,
	ne2000_install,
	ne2000_methods
};
