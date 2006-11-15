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

/* Driver for Chrystal Semiconductor 8900A.
 */

/*#define DEBUG*/

#include "defs.h"
#include "cs8900.h"

#ifdef DEBUG2
extern void dump_packet(Environ *e, uByte *pkt, uInt len);
#define DPRINTF2 DPRINTF
#else
#define DPRINTF2(x)
#endif

#define MAC_ADDR_SIZE		6		/* Ethernet hardware address length */
#define MAX_PACKET_SIZE		0x7FC
#define BUF_SIZE 			(MAX_PACKET_SIZE * 4)

#undef CS8900A_BASE
#define CS8900A_BASE EDB7312_VA_CS8900A


/* instance-specific vars */
struct self
{
	Int phys, size;
	volatile uByte *csr;		/* word access to memory-mapped registers */
	Environ *e;
	Instance *us;
	Instance *obptftp;
	uByte mac_addr[MAC_ADDR_SIZE];	/* Ethernet hardware address */
	Bool promiscuous;		/* 1 = promiscuous */
	Bool duplex;			/* 1 = FDX */
};

typedef struct self Self;

#define DELAY(n)	u_sleep(n)

static void
outport(volatile uByte *addr, uShort val)
{
	__asm("	strh	%0, [%1, #0]" : : "r"(val), "r"(addr));
}

static uShort
inport(volatile uByte *addr)
{
	int val;
	__asm("	ldrh	%0, [%1, #0]" : "=r"(val) : "r"(addr));
	return val & 0xFFFF;
}

static uShort
readreg(volatile uByte *base, int reg)
{
	outport(base + PP_POINTER, reg);
	return inport(base + PP_DATA);
}

static void
writereg(volatile uByte *base, int reg, uShort val)
{
	outport(base + PP_POINTER, reg);
	outport(base + PP_DATA, val);
}

#ifdef DEBUG
struct bitfield rxcfg_bits[] =
{
	{ "Skip_1", 6 },
	{ "StreamE", 7 },
	{ "RxOKiE", 8 },
	{ "RxDMAonly", 9 },
	{ "AutoRxDMAE", 10 },
	{ "BufferCRC", 11 },
	{ "CRCerrorIE", 12 },
	{ "RuntiE", 13 },
	{ "ExtraDataiE", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield rxctl_bits[] =
{
	{ "IAHashA", 6 },
	{ "PromiscuousA", 7 },
	{ "RxOKA", 8 },
	{ "MulticastA", 9 },
	{ "IndividualA", 10 },
	{ "BroadcastA", 11 },
	{ "CRCerrorA", 12 },
	{ "RuntA", 13 },
	{ "ExtraDataA", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield txcfg_bits[] =
{
	{ "LossOfCRSiE", 6 },
	{ "SQErroriE", 7 },
	{ "TxOKiE", 8 },
	{ "OutOfWindowiE", 9 },
	{ "JabberiE", 10 },
	{ "AnycolliE", 11 },
	{ "16colliE", 15 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield txcmd_bits[] =
{
	{ "TxStart", 6, 2 },
	{ "Force", 8 },
	{ "OneColl", 9 },
	{ "InhibitCRC", 12 },
	{ "TxPadDis", 13 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield bufcfg_bits[] =
{
	{ "SWint-X", 6 },
	{ "RxDMAiE", 7 },
	{ "Rdy4TxiE", 8 },
	{ "TxUnderruniE", 9 },
	{ "RxMissiE", 10 },
	{ "Rx128iE", 11 },
	{ "TxColOvfloIE", 12 },
	{ "MissOvfloiE", 13 },
	{ "RxDestiE", 15 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield linectl_bits[] =
{
	{ "SerRxON", 6 },
	{ "SerTxON", 7 },
	{ "AUIonly", 8 },
	{ "AutoAUI/10BT", 9 },
	{ "ModBackoffE", 11 },
	{ "PolarityDis", 12 },
	{ "2-partDefDis", 13 },
	{ "LoRxSquelch", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield selfctl_bits[] =
{
	{ "RESET", 6 },
	{ "SWSuspend", 8 },
	{ "HWSleepE", 9 },
	{ "HWStandbyE", 10 },
	{ "HC0E", 12 },
	{ "HC1E", 13 },
	{ "HCB0", 14 },
	{ "HCB1", 15 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield busctl_bits[] =
{
	{ "ResetRxDMA", 6 },
	{ "DMAextend", 8 },
	{ "UseSA", 9 },
	{ "MemoryE", 10 },
	{ "DMABurst", 11 },
	{ "IOCHRDYE", 12 },
	{ "RxDMAsize", 13 },
	{ "EnableIRQ", 15 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield testctl_bits[] =
{
	{ "DisableLT", 7 },
	{ "ENDECloop", 9 },
	{ "AUIloop", 10 },
	{ "DisableBackoff", 11 },
	{ "FDX", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield rxevent_bits[] =
{
	{ "IAHash", 6 },
	{ "DribbleBits", 7 },
	{ "RxOK", 8 },
	{ "Hashed", 9 },
	{ "IndividualAdr", 10 },
	{ "Broadcast", 11 },
	{ "CRCerror", 12 },
	{ "Runt", 13 },
	{ "ExtraData", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield rxeventalt_bits[] =
{
	{ "IAHash", 6 },
	{ "DribbleBits", 7 },
	{ "RxOK", 8 },
	{ "Hashed", 9 },
	{ "HashIndex", 10, 6 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield txevent_bits[] =
{
	{ "LossOfCRS", 6 },
	{ "SQEerror", 7 },
	{ "TxOK", 8 },
	{ "OutOfWindow", 9 },
	{ "Jabber", 10 },
	{ "NumTxCol", 11, 4 },
	{ "16Col1", 15 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield bufevent_bits[] =
{
	{ "SWint", 6 },
	{ "RxDMAFrame", 7 },
	{ "Rdy4Tx", 8 },
	{ "TxUnderrun", 9 },
	{ "RxMiss", 10 },
	{ "Rx128", 11 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield rxmiss_bits[] =
{
	{ "RxMiss", 6, 10 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield txcol_bits[] =
{
	{ "TxCol", 6, 10 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield linest_bits[] =
{
	{ "LinkOK", 7 },
	{ "AUI", 8 },
	{ "10BT", 9 },
	{ "PolarityOK", 12 },
	{ "CRS", 14 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield selfst_bits[] =
{
	{ "3.3Vact", 6 },
	{ "INITD", 7 },
	{ "SIBUSY", 8 },
	{ "EEPROMpresent", 9 },
	{ "EEPROMok", 10 },
	{ "ELpresent", 11 },
	{ "EESize", 12 },
	{ "Op", 0, 6 },
	{ NULL }
};

struct bitfield busst_bits[] =
{
	{ "TxBidErr", 7 },
	{ "Rdy4TxNOW", 8 },
	{ "Op", 0, 6 },
	{ NULL }
};

static void
dump_regs(Environ *e, volatile uByte *base)
{
	uShort temp;

	dprintf("\n");
	dprintf("EISA ID: %#x\n", readreg(base, PP_EISA_SIG));
	dprintf("Chip ID: %#x\n", readreg(base, PP_CHIPID));
	dprintf("I/O base address: %#x\n", readreg(base, PP_IOBASE));
	dprintf("Interrupt Number: %#x\n", readreg(base, PP_INTNUM));
	dprintf("DMA Channel Number: %#x\n", readreg(base, 0x24));
	dprintf("DMA Start of Frame: %#x\n", readreg(base, 0x26));
	dprintf("DMA Frame Count: %#x\n", readreg(base, 0x28));
	dprintf("RxDMA Byte Count: %#x\n", readreg(base, 0x2A));
	dprintf("Memory Base Address: %#x%04x\n", readreg(base, 0x2E),
			readreg(base, 0x2C));
	dprintf("Boot PROM Base Address: %#x%04x\n", readreg(base, 0x32),
			readreg(base, 0x30));
	dprintf("Boot PROM Address Mask: %#x%04x\n", readreg(base, 0x36),
			readreg(base, 0x34));
	dprintf("EEPROM Command: %#x\n", readreg(base, 0x40));
	dprintf("Receive Frame Byte Counter: %#x\n", readreg(base, 0x50));
	dump_bits(e, "RxCFG", rxcfg_bits, readreg(base, 0x102));
	dump_bits(e, "RxCTL", rxctl_bits, readreg(base, 0x104));
	dump_bits(e, "TxCFG", txcfg_bits, readreg(base, 0x106));
	dump_bits(e, "TxCMD", txcmd_bits, readreg(base, 0x108));
	dump_bits(e, "BufCFG", bufcfg_bits, readreg(base, 0x10A));
	dump_bits(e, "LineCTL", linectl_bits, readreg(base, 0x112));
	dump_bits(e, "SelfCTL", selfctl_bits, readreg(base, 0x114));
	dump_bits(e, "BusCTL", busctl_bits, readreg(base, 0x116));
	dump_bits(e, "TestCTL", testctl_bits, readreg(base, 0x118));

	temp = readreg(base, 0x120);

	switch (temp & 0x3F)
	{
	case 0x04:
		if ((temp & 0x300) != 0x300)
			dump_bits(e, "ISQ - RxEvent", rxevent_bits, temp);
		else
			dump_bits(e, "ISQ - RxEvent", rxeventalt_bits, temp);

		break;
	case 0x08:
		dump_bits(e, "ISQ - TxEvent", txevent_bits, temp);
		break;
	case 0x0C:
		dump_bits(e, "ISQ - BufEvent", bufevent_bits, temp);
		break;
	case 0x10:
		dump_bits(e, "ISQ - RxMiss", rxmiss_bits, temp);
		break;
	case 0x12:
		dump_bits(e, "ISQ - TxCol", txcol_bits, temp);
		break;
	default:
		dprintf("ISQ: %#x\n", temp);
		break;
	}

	temp = readreg(base, 0x124);

	if ((temp & 0x300) != 0x300)
		dump_bits(e, "RxEvent", rxevent_bits, temp);
	else
		dump_bits(e, "RxEvent", rxeventalt_bits, temp);

	dump_bits(e, "TxEvent", txevent_bits, readreg(base, 0x128));
	dump_bits(e, "BufEvent", bufevent_bits, readreg(base, 0x12C));
	dump_bits(e, "RxMiss", rxmiss_bits, readreg(base, 0x130));
	dump_bits(e, "TxCol", txcol_bits, readreg(base, 0x132));
	dump_bits(e, "LineST", linest_bits, readreg(base, 0x134));
	dump_bits(e, "SelfST", selfst_bits, readreg(base, PP_SELFST));
	dump_bits(e, "BusST", busst_bits, readreg(base, 0x138));
	dprintf("TDR: %#x\n", readreg(base, 0x13C));
}
#else
#	define dump_regs(e, s)	/* e, s */
#endif

void
wait_eeprom_busy(Environ *e, Self *s)
{
	uShort temp;

	/* Wait until SIBUSY is clear. Check bit 8 of Self Status register. */
	/* 1 = busy, 0 = not busy */

	for (;;)
	{
		temp = readreg(s->csr, 0x0136);

		if ((temp & 0x0100) == 0)
			break;
	}

	DPRINTF2(("wait_eeprom_busy: temp = %#x\n", temp));
}

static Retcode
write_eeprom(Environ *e, Self *s, uShort *data, int offset, int words)
{
	int i;
	int errs = 0;

	if (offset < 0 || offset + words > 256)
		return E_NUM_OUT_OF_RANGE;

	for (i = 0; i < words; i++)
	{
		wait_eeprom_busy(e, s);

		/* Enable-write command */
		writereg(s->csr, 0x0040, 0x00F0);
		wait_eeprom_busy(e, s);

		/* Write the outWord Value to EEPROM */
		writereg(s->csr, 0x0042, data[i]);

		/* Write command */
		writereg(s->csr, 0x0040, 0x0100 + offset + i);
		wait_eeprom_busy(e, s);

		/* Disable-write command */
		writereg(s->csr, 0x0040, 0x0000);
		wait_eeprom_busy(e, s);

		/* Read back what is written command */
		writereg(s->csr, 0x0040, 0x0200 + offset + i);
		wait_eeprom_busy(e, s);

		/* Delay for a while */
		u_sleep(3000);

		/* Read back and return result */
		if (readreg(s->csr, 0x0042) != data[i])
			errs++;
	}

	return errs ? E_PROM_WRITE_ERROR : NO_ERROR;
}

static Retcode
read_eeprom(Environ *e, Self *s, uShort *data, int offset, int words)
{
	int i;

	if (offset < 0 || offset + words > 256)
		return E_NUM_OUT_OF_RANGE;

	for (i = 0; i < words; i++)
	{
		wait_eeprom_busy(e, s);

		/* Send EEPROM command */
		writereg(s->csr, 0x0040, 0x0200 + offset + i);
		wait_eeprom_busy(e, s);

		/* delay for a while */
		u_sleep(3000);

		/* Read data from EEPROM and return. */
		data[i] = readreg(s->csr, 0x0042);
	}

	return NO_ERROR;
}

C(f_cs8900a_write_eeprom)		/* write-eeprom (addr off cnt -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	Int count;
	Int offset;
	uByte *addr;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, offset);
	POPT(e, addr, uByte *);

	ret = write_eeprom(e, inst->self, (uShort *)addr, offset, count / 2);

	if (ret == NO_ERROR)
		PUSH(e, count);

	return ret;
}

uInt
checksum_byte(uByte *buf, int len)
{
	int i;
	uShort sum = 0;

	for(i = 0; i < len; i++)
		sum += buf[i];

	sum = ~sum + 1;
	return sum << 8;
}

uShort
checksum_word(uByte *buf, int len)
{
	int i;
	uShort sum = 0;

	for(i = 0; i < len; i += 2)
		sum += (buf[i] | (buf[i + 1] << 8));

	return sum = ~sum + 1;
}

static uInt
checksum_lfsr(uByte *buf, int len)
{
	uByte sum;
	int i;
	int j;
	uByte xor;
	uByte byte;

	sum = 0x6A;

	for (i = 0; i < len; i++)
	{
		byte = buf[i];

		for (j = 0; j < 8; j++)
		{
			xor = (sum & 1) ^ ((sum >> 1) & 1);
			sum >>= 1;

			if (xor ^ (byte & 1))
				sum |= 0x80;

			byte >>= 1;
		}
	}

	return sum ^ 0x0A00U;
}

Retcode
write_mac_addr(Environ *e, Self *s, uByte *macaddr, Int macaddrlen)
{
#define EEPROM_BUF_SIZE	128
	uByte buf[EEPROM_BUF_SIZE];
	Retcode ret;
	uShort checksum;

	if (macaddrlen != 6)
		return E_BAD_ARGUMENT;

	/* read the serial eeprom contents */
	ret = read_eeprom(e, s, (uShort *)buf, 0, EEPROM_BUF_SIZE/2);

	if (ret != NO_ERROR)
		return ret;

	/* stash in the MAC address and update the checksums */
	memcpy(&buf[4], macaddr, MAC_ADDR_SIZE);
	memcpy(&buf[0x38], macaddr, MAC_ADDR_SIZE);
	memcpy(&buf[0x50], macaddr, MAC_ADDR_SIZE);
	checksum = checksum_byte(&buf[0x00], 14);
	buf[0xE] = checksum & 0xFF;
	buf[0xF] = (checksum >> 8) & 0xFF;
	checksum = checksum_word(&buf[0x1C*2], 38);
	buf[0x5E] = checksum & 0xFF;
	buf[0x5F] = (checksum >> 8) & 0xFF;

/*
: dump-eeprom
    " /ethernet" open-dev to my-self 200 alloc-mem dup dup FF and 100 swap - + dup
    0 100 " read-eeprom" my-self .s $call-method dump 200 free-mem my-self close-dev 
;
*/
/*
0xF70B3D00  0E A1 58 21 01 02 03 04 05 06 20 00 00 03 00 A0  ..X!...... .....
0xF70B3D10  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
0xF70B3D20  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
0xF70B3D30  FF FF FF FF FF FF FF FF 01 02 03 04 05 06 B0 11  ................
0xF70B3D40  00 00 00 00 00 00 40 00 21 00 00 00 00 00 0F BF  ......@.!.......
0xF70B3D50  01 02 03 04 05 06 00 00 00 00 00 00 00 00 CE 16  ................
0xF70B3D60  0E 63 00 40 78 56 34 12 20 0A FF FF FF FF FF FF  .c.@xV4. .......
0xF70B3D70  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
0xF70B3D80  0E A1 58 21 01 02 03 04 05 06 20 00 00 03 00 A0  ..X!...... .....
0xF70B3D90  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
0xF70B3DA0  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
0xF70B3DB0  FF FF FF FF FF FF FF FF 01 02 03 04 05 06 B0 11  ................
0xF70B3DC0  00 00 00 00 00 00 40 00 21 00 00 00 00 00 0F BF  ......@.!.......
0xF70B3DD0  01 02 03 04 05 06 00 00 00 00 00 00 00 00 CE 16  ................
0xF70B3DE0  0E 63 00 40 78 56 34 12 20 0A FF FF FF FF FF FF  .c.@xV4. .......
0xF70B3DF0  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
*/
/*
    "$addr=0", 
    "$chksum",
    "A10E            ; 14 bytes follow", 
    "2158",           
    "$ieee           ; 3 words of individual address",
    "0020            ; 1 word at PP_020",
    "0300            ; IO base = 300h",
    "$chksum=b       ; 8-bit checksum on config data",   
    "$addr=1C",
    "$chksum",
    "$ieee   ; 3 words of ieee address",
    "11B0    ; ISA flags (IO Mode,IOCHRDY,UseSA,DMA disable,DMA burst,IRQ10)",
    "0000    ; PacketPage Memory base",
    "0000    ; Boot PROM base",
    "0000    ; Boot PROM mask",
    "0040    ; Transmission Control (ignore missing media)",
    "0021    ; Adapter config (10Base-T circuitry only)",
    "0000    ; Reserved - set to 0",
    "0000    ; Product ID",
    "BF0F    ; Mfg Date - 8.15.95",
    "$ieee   ; 3 words of ieee address",
    "0000    ; Reserved - set to 0",
    "0000    ; Reserved - set to 0",   
    "0000    ; Reserved - set to 0",
    "0000    ; Reserved - set to 0",
    "$chksum=w",
    "$chksum",
    "630E    ; EISA ID (low word)",
    "4000    ; EISA ID (high word)",
    "$snum   ; 2 words of serial number",
    "$chksum=L",
*/

	/* write the new serial eeprom contents */
	ret = write_eeprom(e, s, (uShort *)buf, 0, EEPROM_BUF_SIZE/2);

	if (ret != NO_ERROR)
		return ret;

	return NO_ERROR;
}

C(f_cs8900a_set_mac_addr)		/* set-mac-addr (addr size -- ok?) */
{
	uByte *macaddr;
	Int macaddrlen;
	Instance *inst = (Instance *)(uPtr)e->currinst;
	Bool diag = diagnostic_mode(e);
	Retcode ret;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, macaddrlen);
	POPT(e, macaddr, uByte *);

	ret = write_mac_addr(e, inst->self, macaddr, macaddrlen);

	if (ret != NO_ERROR)
		PUSH(e, FFALSE);
	else
	{
		PUSH(e, FTRUE);

		if (diag)
			cprintf(e, "local Ethernet MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
					macaddr[0], macaddr[1], macaddr[2],
					macaddr[3], macaddr[4], macaddr[5]);
	}

	return NO_ERROR;
}

C(f_cs8900a_format_eeprom)		/* format-eeprom ( -- ) */
{
	uByte buf[EEPROM_BUF_SIZE];
	uByte macaddr[MAC_ADDR_SIZE];
	uByte serialnum[4];
	uShort *sbuf;
	Instance *inst;
	Retcode ret;
	int i;

	IFCKSP(e, 0, 2);
	PUSH(e, "/ethernet");
	PUSH(e, 9);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance *);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	macaddr[0] = 0x00;
	macaddr[1] = 0x30;
	macaddr[2] = 0x23;
	macaddr[3] = 0x01;
	macaddr[4] = 0x00;
	macaddr[5] = 0x01;

	serialnum[0] = 0x78;
	serialnum[1] = 0x56;
	serialnum[2] = 0x34;
	serialnum[3] = 0x12;

	for (i = 0; i < EEPROM_BUF_SIZE; i++)
		buf[i] = 0xFF;

	sbuf = (uShort *)buf;
	sbuf[0x00] = 0xA10E;			/* magic number */
	sbuf[0x01] = 0x2158;			/* magic number */
	buf[0x02*2] = macaddr[0];		/* MAC address */
	buf[0x02*2+1] = macaddr[1];
	buf[0x03*2] = macaddr[2];
	buf[0x03*2+1] = macaddr[3];
	buf[0x04*2] = macaddr[4];
	buf[0x04*2+1] = macaddr[5];
	sbuf[0x05] = 0x0020;			/* PP_020 */
	sbuf[0x06] = 0x0300;			/* IO Base = 300h */
	sbuf[0x07] = checksum_byte(buf, 14);	/* checksum */
	buf[0x1C*2] = macaddr[0];		/* repeated mac address */
	buf[0x1C*2+1] = macaddr[1];
	buf[0x1D*2] = macaddr[2];
	buf[0x1D*2+1] = macaddr[3];
	buf[0x1E*2] = macaddr[4];
	buf[0x1E*2+1] = macaddr[5];
	sbuf[0x1F] = 0x11B0;			/* ISA flags */
	sbuf[0x20] = 0x0000;			/* PacketPage Memory base */
	sbuf[0x21] = 0x0000;			/* Boot PROM base */
	sbuf[0x22] = 0x0000;			/* Boot PROM mask */
	sbuf[0x23] = 0x0040;			/* Transmission Control */
	sbuf[0x24] = 0x0021;			/* Adapter config */
	sbuf[0x25] = 0x0000;			/* Reserved */
	sbuf[0x26] = 0x0000;			/* Product ID */
	sbuf[0x27] = 0xCAC9;			/* Mfg Data 9-Jun-2001 */
	buf[0x28*2] = macaddr[0];		/* repeated MAC address */
	buf[0x28*2+1] = macaddr[1];
	buf[0x29*2] = macaddr[2];
	buf[0x29*2+1] = macaddr[3];
	buf[0x2A*2] = macaddr[4];
	buf[0x2A*2+1] = macaddr[5];
	sbuf[0x2B] = 0x0000;			/* Reserved */
	sbuf[0x2C] = 0x0000;			/* Reserved */
	sbuf[0x2D] = 0x0000;			/* Reserved */
	sbuf[0x2E] = 0x0000;			/* Reserved */
	sbuf[0x2F] = checksum_word((uByte *)&sbuf[0x1C], 38);
	sbuf[0x30] = 0x630E;			/* EISA ID, low word */
	sbuf[0x31] = 0x4000;			/* EISA ID, high word */
	buf[0x32*2] = serialnum[0];		/* serial number */
	buf[0x32*2+1] = serialnum[1];
	buf[0x33*2] = serialnum[2];
	buf[0x33*2+1] = serialnum[3];
	sbuf[0x34] = checksum_lfsr((uByte *)&sbuf[0x30], 8);

	ret = write_eeprom(e, inst->self, sbuf, 0, EEPROM_BUF_SIZE/2);

	PUSH(e, inst);
	execute_word(e, "close-dev");

	if (ret != NO_ERROR)
		PUSH(e, FFALSE);
	else
		PUSH(e, FTRUE);

	return NO_ERROR;
}

C(f_cs8900a_read_eeprom)		/* read-eeprom (addr off cnt -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	Int count;
	Int offset;
	uByte *addr;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	IFCKSP(e, 3, 0);
	POP(e, count);
	POP(e, offset);
	POPT(e, addr, uByte *);

	ret = read_eeprom(e, inst->self, (uShort *)addr, offset, count / 2);

	if (ret == NO_ERROR)
		PUSH(e, count);

	return ret;
}

static Retcode
read_mac_addr(Environ *e, Package *pkg, Self *s)
{
	Retcode ret;
	uByte buf[EEPROM_BUF_SIZE];
	uByte mac_addr[MAC_ADDR_SIZE];
	uShort checksum;

	ret = read_eeprom(e, s, (uShort *)buf, 0, EEPROM_BUF_SIZE/2);

	if (ret != NO_ERROR)
		return ret;

	checksum = checksum_byte(buf, 14);	/* checksum */

	if (buf[0x07*2] != (checksum & 0xFF) ||
			buf[0x07*2+1] != ((checksum >> 8) & 0xFF))
		return E_PROM_READ_ERROR;

	checksum = checksum_word(&buf[0x1C*2], 38);

	if (buf[0x2F*2] != (checksum & 0xFF) ||
			buf[0x2F*2+1] != ((checksum >> 8) & 0xFF))
		return E_PROM_READ_ERROR;

	checksum = checksum_lfsr(&buf[0x30*2], 8);

	if (buf[0x34*2] != (checksum & 0xFF) ||
			buf[0x34*2+1] != ((checksum >> 8) & 0xFF))
		return E_PROM_READ_ERROR;

	/* If the MAC address is all F's then we do not have a legit MAC addr */
	if (mac_addr[0] == 0xFF && mac_addr[1] == 0xFF && mac_addr[2] == 0xFF &&
			mac_addr[3] == 0xFF && mac_addr[4] == 0xFF && mac_addr[5] == 0xFF)
		return E_BAD_ARGUMENT;

	/* may need to swap the two halves of data */
#ifdef BIG_ENDIAN
	s->mac_addr[0] = buf[5];
	s->mac_addr[1] = buf[4];
	s->mac_addr[2] = buf[7];
	s->mac_addr[3] = buf[6];
	s->mac_addr[4] = buf[9];
	s->mac_addr[5] = buf[8];
#else
	memcpy(s->mac_addr, &buf[4], sizeof mac_addr);
#endif

	return set_property(pkg->props, "local-mac-address", CSTR,
			(Byte *)s->mac_addr, MAC_ADDR_SIZE);
}

static Retcode
cs8900a_init(Environ *e, Self *s)
{
	unsigned short temp;

	/* Reset the CS8900A device */
	writereg(s->csr, 0x0114, 0x0040);
	u_sleep(3000);
	temp = readreg(s->csr, 0x0136);

	/* wait for reset to complete */
	while ((temp & 0x0080) == 0)
	    temp = readreg(s->csr, 0x0136);

	u_sleep(1000);

	/* load up the registers */
#if 0
	writereg(s->csr, PP_IOBASE, 0x300);	/* I/O Base is 0x300 */
#endif
	writereg(s->csr, PP_INTNUM, 0);		/* interrupt on pin 0 */
	writereg(s->csr, PP_DMACHNUM, 3);		/* No DMA Channel */
	writereg(s->csr, 0x158, s->mac_addr[0] | (s->mac_addr[1] << 8));
	writereg(s->csr, 0x15A, s->mac_addr[2] | (s->mac_addr[3] << 8));
	writereg(s->csr, 0x15C, s->mac_addr[4] | (s->mac_addr[5] << 8));

	/* set FDX bit */
	writereg(s->csr, 0x118, s->duplex ? 0x4000 : 0x0000);

	/* set the RxOK, IndividualAdr and possible the promiscuous bit */
	writereg(s->csr, 0x0104, s->promiscuous ? 0x7F80 : 0x7F00);

	/* enable Tx and Rx */
	writereg(s->csr, 0x0112, 0x00C0);

	return NO_ERROR;
}

static Retcode
destroy_self(Self *s)
{
	Environ *e = s->e;
	Retcode ret = NO_ERROR;

	if (s->obptftp)
	{
		PUSH(e, (uPtr)s->obptftp);
		ret = execute_word(e, "close-package");
	}

	if (s->csr)
	{
		PUSH(e, (uPtr)s->csr);
		PUSH(e, s->size);
		ret = execute_method_name(e, s->us->parent, "map-out", CSTR);
	}

	free(s);
	return ret;
}

C(f_cs8900a_open)			/* open (-- okay?) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Bool promiscuous = FALSE;
	Int speed = 0;			/* auto */
	Bool duplex = FALSE;
	Retcode ret;
	Self *s;
	Entry *ent;
	Byte *paddr;
	Int plen, maclen;
	uByte *macaddr;
	Cell c;
	int id;
	int chip;
	char *chipstr;
	int rev;
	char revstr[2];

	DPRINTF(("cs8900a_open: pkg=%p\n", pkg));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->us = inst;
	s->e = e;

	DPRINTF(("cs8900a_open: s=%p\n", s));

	/* decode the reg property to find the address and size
	 * of the CSR registers.
	 */
	ent = find_table(pkg->props, "reg", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("cs8900a_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	ret = prop_decode_int(&paddr, &plen, &s->phys);

	if (ret == NO_ERROR)
		ret = prop_decode_int(&paddr, &plen, &s->size);
	
	DPRINTF(("cs8900a_open: paddr=%p plen=%d\n", paddr, plen));
	DPRINTF(("cs8900a_open: phys=%#x size=%#x\n", s->phys, s->size));

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	PUSH(e, s->phys);
	PUSH(e, s->size);
	ret = execute_method_name(e, inst->parent, "map-in", CSTR);

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POP(e, c);

	/* point to CSRs for various byte/word/lword accesses */
	s->csr = (uByte*)(uPtr)c;
	DPRINTF(("cs8900a_open: csr=%#x\n", s->csr));

	/* check an make sure we can get the chip id */
	id = readreg(s->csr, PP_CHIPID);
	chip = id & ~0x1F00;
	rev = (id & 0x1F00) >> 8;

	if (chip == 0x0000)
		chipstr = "CS8900";
	else if (chip == 0x4000)
		chipstr = "CS8920";
	else if (chip == 0x4000)
		chipstr = "CS8920M";
	else
		chipstr = "Unknown";

	revstr[0] = rev + 'A';
	revstr[1] = '\0';

	DPRINTF(("product id %#x, chip %#x, name %s, rev %s\n", id, chip, chipstr, revstr));

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

			DPRINTF(("cs8900a: args=\"%s\"\n", str));

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
						speed = val;
						break;
						
					case 4:
					case 16:
					case 100:
					case 155:
					case 622:
					case 1000:
					default:
						cprintf(e, "cs8900a: unsupported \"speed=%d\"\n", val);
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
					cprintf(e, "cs8900a: bad arg format for \"duplex=\"");
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

	/* try to read in the local MAC addr out of ROM and use it
	   to intialize the parameter "local-mac-address" */
	ret = read_mac_addr(e, pkg, s);
	DPRINTF(("cs8900a_open: read_mac_addr ret=%d\n", ret));
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
	DPRINTF(("cs8900a_open: $open-package ret=%d\n", ret));

	if (ret != NO_ERROR)
	{
		destroy_self(s);
		return ret;
	}

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("cs8900a_open: obptftp=%p\n", s->obptftp));

	if (s->obptftp == NULL)
	{
		destroy_self(s);
		return E_NULL_INSTANCE;
	}

	ret = execute_word(e, "mac-address");

	if (ret != NO_ERROR)
	{
		destroy_self(s);
		return ret;
	}
	
	POP(e, maclen);
	POPT(e, macaddr, uByte*);

	if (maclen != MAC_ADDR_SIZE)
	{
		destroy_self(s);
		return E_ABORT;
	}

	/* use this returned value for our MAC addr */
	memcpy(s->mac_addr, macaddr, maclen);

	/* let the user know the MAC addr to help configure BOOTP */
	if (diag)
		cprintf(e, "Ethernet MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
				macaddr[0], macaddr[1], macaddr[2],
				macaddr[3], macaddr[4], macaddr[5]);

	/* reset the chip */
	s->promiscuous = promiscuous;
	s->duplex = duplex;
	cs8900a_init(e, s);

	dump_regs(e, s->csr);

	DPRINTF(("cs8900a_open: initialize station address\n"));

	/* set the media */
	DPRINTF(("cs8900a_open: set media\n"));

	/* ack any interrupts */
	dump_regs(e, s->csr);

	DPRINTF(("cs8900a_open: done: ret=%d\n", ret));
	PUSH(e, FTRUE);
	return ret;
}

C(f_cs8900a_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	/* reset the chip */
	cs8900a_init(e, s);
	return destroy_self(s);
}

C(f_cs8900a_read)				/* read (addr len -- actual) */
{
	Int len, actual = -2;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	uShort temp;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	DPRINTF2(("cs8900a_read: addr=%p len=%d status=%#x\n",
			addr, len, 0));

	temp = readreg(s->csr, 0x124);
	DPRINTF2(("cs8900a_read: RxEvent %#x\n", temp));

	if (temp & 0x7D00)
	{
		/* we have a packet */
		if (temp & 0x900)
		{
			int stat;
			int cnt;

			stat = inport(s->csr);
			cnt = inport(s->csr);

			DPRINTF2(("cs8900a_read: stat %#x, count %d\n", stat, cnt));

			if ((stat & ~0xC00) != 0x104)
			{
				/* packet is a result of an error */
				/* clear the error and return */
				temp = readreg(s->csr, 0x102);
				writereg(s->csr, 0x102, temp | SKIP);
			}
			else
			{
				int i;

				actual = cnt < len ? cnt : len;
				DPRINTF2(("cs8900a_read: actual %d\n", actual));

				for (i = 0; i + 1 < actual; i += 2)
				{
					temp = inport(s->csr);
					addr[i] = temp & 0xFF;
					addr[i + 1] = (temp >> 8) & 0xFF;
				}

				if (i < actual)
				{
					temp = inport(s->csr);
					addr[i] = temp & 0xFF;
				}

				if (actual < cnt)
				{
					/* the packet is bigger than the buffer */
					/* so skip the rest */
					temp = readreg(s->csr, 0x102);
					writereg(s->csr, 0x102, temp | SKIP);
				}

#ifdef DEBUG2
				dump_packet(e, addr, actual);
#endif
			}
		}
		else
		{
			/* packet is a result of an error */
			/* clear the error and return */
			temp = readreg(s->csr, 0x102);
			writereg(s->csr, 0x102, temp | SKIP);
		}
	}

	DPRINTF2(("cs8900a_read: actual=%d\n", actual));

	PUSH(e, actual);
	return NO_ERROR;
}

C(f_cs8900a_write)			/* write (addr len -- actual) */
{
	Int len, actual = 0;
	Byte *addr;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;
	int i;
	uShort temp = 0;
	uShort data;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF2(("cs8900a_write: addr=%p len=%d\n", addr, len));

	if (len > MAX_PACKET_SIZE)
		len = MAX_PACKET_SIZE;

	outport(s->csr + TX_CMD, 0x00C0);
	outport(s->csr + TX_LENGTH, len);
	outport(s->csr + PP_POINTER, 0x0138);

	/* delay for a while */
	for (i = 0; i < 1000000; i++)
	    if ((temp = inport(s->csr + PP_DATA)) ==  0x0118)
		    break;

	if (i >= 1000000)
	{
		cprintf(e, "Bid for transmit failed\n");
		cprintf(e, "Register PP_DATA = %04x\n", temp);
		dump_regs(e, s->csr);
		return E_NET_ERROR;
	}

	/* Copy the TX frame to the on-chip buffer  */
	/* (11) Write XX/2 words to IOBase+0x0 (TX Data Port)  */
	for (i = 0; i + 1 < len; i += 2)
	{
		data = addr[i] & 0xFF;
		data |= (addr[i + 1] & 0xFF) << 8;
		outport(s->csr, data);
	}

	if (i < len)
	{
		data = addr[i] & 0xFF;
		outport(s->csr, data);
	}

	/* should wait for Tx complete at this point */
	outport(s->csr + PP_POINTER, 0x0128);

	/* delay for a while */
	for (i = 0; i < 250000; i++)
	    if ((temp = inport(s->csr + PP_DATA)) != 0x0008)
		    break;

	if (i >= 250000 || (temp & 0x87FF) != 0x108)
	{
		cprintf(e, "Transmit failed\n");
		cprintf(e, "Register PP_DATA = %04x, i = %d\n", temp, i);
		dump_regs(e, s->csr);
		PUSH(e, 0);
		return NO_ERROR;
	}

	actual = len;
	DPRINTF2(("cs8900a_write: actual=%d\n", actual));
	DPRINTF2(("BufEvent = %#x\n", readreg(s->csr, 0x12C)));
	PUSH(e, actual);
	return NO_ERROR;
}

static Retcode
do_obptftp_method(Environ *e, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	if (!s->obptftp)
		return E_NULL_INSTANCE;

	/* let the TFTP/BOOTP package do the work */
	return execute_method_name(e, s->obptftp, method, CSTR);
}

C(f_cs8900a_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_cs8900a_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

C(f_cs8900a_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self self;
	Retcode ret;

	DPRINTF(("ether selftest: begin\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	DPRINTF(("ether selftest: got pkg\n"));

	self.csr = (volatile uByte *)EDB7312_VA_CS8900A;
	ret = read_mac_addr(e, pkg, &self);

	if (ret != NO_ERROR)
	{
		PUSH(e, E_SELFTEST_FAILED);		/* failure */
		return NO_ERROR;
	}

	if (diag)
	{
		cprintf(e, "ether selftest\n");

		/* TODO */
		/* do read/write test of the serial eeprom */
	}

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}


static const Initentry cs8900a_methods[] =
{
	{ "open",      f_cs8900a_open,          INVALID_FCODE },
	{ "close",     f_cs8900a_close,         INVALID_FCODE },
	{ "read",      f_cs8900a_read,          INVALID_FCODE },
	{ "write",     f_cs8900a_write,         INVALID_FCODE },
	{ "load",      f_cs8900a_load,          INVALID_FCODE },
	{ "ping",      f_cs8900a_ping,          INVALID_FCODE },
	{ "selftest",  f_cs8900a_selftest,      INVALID_FCODE },
	{ NULL,        NULL },
};

static const Initentry cs8900a_diag_methods[] =
{
	{ "read-eeprom", f_cs8900a_read_eeprom, INVALID_FCODE },
	{ "write-eeprom", f_cs8900a_write_eeprom, INVALID_FCODE },
	{ "set-mac-addr", f_cs8900a_set_mac_addr, INVALID_FCODE },
	{ "format-eeprom", f_cs8900a_format_eeprom, INVALID_FCODE },
	{ "dump-eeprom", (Command)"\" /ethernet\" open-dev to my-self 200 alloc-mem "
			"dup dup FF and 100 swap - + dup 0 100 \" read-eeprom\" my-self "
			"$call-method dump 200 free-mem my-self close-dev ",
			INVALID_FCODE, F_TOKENIZE, T_FORTH },
	{ NULL,        NULL },
};

#if 0
static void
dump_reg_range(Environ *e, volatile uByte *base, int start, int num)
{
	int i;
	uShort data;

	start &= ~0xF;
	num = (num + 0xF) & ~0xF;

	for (i = start; i < start + num; i += 2)
	{
		data = readreg(base, i);

		if ((i & 0xE) == 0)
			cprintf(e, "%04X:  %04X", i, data);
		else if ((i & 0xE) == 0xE)
			cprintf(e, " %04X\n", data);
		else if ((i & 0xE) == 0x8)
			cprintf(e, "  %04X", data);
		else 
			cprintf(e, " %04X", data);
	}
}

static void
cs8900_dump_regs(Environ *e, volatile uByte *base)
{
	cprintf(e, "CS8900 Registers:\n");
	dump_reg_range(e, base, 0, 0x80);
	dump_reg_range(e, base, 0x100, 0x80);
	dump_reg_range(e, base, 0x400, 0x10);
	dump_reg_range(e, base, 0xA00, 0x10);
}
#endif

Retcode
cs8900a_probe(Environ *e, volatile uByte *base)
{
	uInt id;
	int chip;

	id = readreg(base, PP_EISA_SIG);
	DPRINTF(("id = %#x\n", id));

	if (id != ID_EISA_SIG)
		return E_NO_DEVICE;

	id = readreg(base, PP_CHIPID);
	chip = id & ~0x1F00;

	if (chip != ID_CS8900)
		return E_NO_DEVICE;

	return NO_ERROR;
}

CC(install_cs8900a_driver)
{
	Retcode ret;
	int id;
	int chip;
	char *chipstr;
	int rev;
	char revstr[2];
	Package *pkg;
	Byte *prop;
	Int plen = 0;
	Bool diag = diagnostic_mode(e);
#ifdef DEBUG2
	extern int ethernet_test();
#endif

	ret = cs8900a_probe(e, (volatile uByte *)CS8900A_BASE);

	if (ret != NO_ERROR)
		return ret;

	id = readreg((volatile uByte *)CS8900A_BASE, PP_CHIPID);
	chip = id & ~0x1F00;
	rev = (id & 0x1F00) >> 8;

	if (chip == 0x0000)
	{
		chipstr = "CS8900";

		if (rev >= 8)
		{
		    chipstr = "CS8900A";
		    rev -= 8;
		}
	}
	else if (chip == 0x4000)
		chipstr = "CS8920";
	else if (chip == 0x4000)
		chipstr = "CS8920M";
	else
		chipstr = "Unknown";

	revstr[0] = rev + 'A';
	revstr[1] = '\0';

	DPRINTF(("product id %#x, chip %#x, name %s, rev %s\n", id, chip, chipstr, revstr));

	{
		Self s;
		s.phys = CS8900A_BASE;
		s.size = 16;
		s.csr = (volatile uByte *)CS8900A_BASE;
		s.obptftp = (Instance *)0;
		s.mac_addr[0] = 0;
		s.mac_addr[1] = 0;
		s.mac_addr[2] = 0;
		s.mac_addr[3] = 0;
		s.mac_addr[4] = 0;
		s.mac_addr[5] = 0;
		s.promiscuous = 0;
		s.duplex = 0;
		cs8900a_init(e, &s);
	}

#ifdef DEBUG2
	dump_regs(e, (volatile uByte *)CS8900A_BASE);
	ethernet_test();
	dump_regs(e, (volatile uByte *)CS8900A_BASE);
#endif

	pkg = new_pkg_name(e->root, "ethernet");
	DPRINTF(("install_cs8900a_driver: pkg=%p\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "network", CSTR);

	if (ret == NO_ERROR)
	    ret = prop_set_str(pkg->props, ".chip", CSTR, chipstr, CSTR);

	if (ret == NO_ERROR)
	    ret = prop_set_str(pkg->props, ".rev", CSTR, revstr, CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, EDB7312_PA_CS8900A);
	prop_encode_int(prop + plen, &plen, 0x10);
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, cs8900a_methods);

	if (ret == NO_ERROR && diag)
		ret = init_entries(e, pkg->dict, cs8900a_diag_methods);

	return ret;
}
