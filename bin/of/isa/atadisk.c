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

/* Generic IDE/ATA disk driver
	- used as the base for ISA, PCI, and other IDE controllers.
	- must be given I/O base registers to handle ISA, PCI, and custom chips
 */

#include "defs.h"
#include "../scsi/scsi.h"


/* macros based on OpenBSD's ic/wdreg.h */

/* standard ATA registers - add to I/O base reg */
#define DATA		0x0		/* data register (R/W - 16 bits) */
#define ERROR		0x1		/* error register (R) */
	#define E_BBK        0x80    /* bad block detected */
	#define E_CRC        0x80    /* CRC error (Ultra-DMA only) */
	#define E_UNC        0x40    /* uncorrectable data error */
	#define E_MC         0x20    /* media changed */
	#define E_IDNF       0x10    /* id not found */
	#define E_MCR        0x08    /* media change requested */
	#define E_ABRT       0x04    /* aborted command */
	#define E_TK0NF      0x02    /* track 0 not found */
	#define E_AMNF       0x01    /* address mark not found */

	/* diagostic values returned */
	#define D_NO_ERR		0x01	/* no error detected, drive 0 & 1 */
	#define D_DEV_ERR		0x02	/* device error */
	#define D_SEC_BUF_ERR	0x03	/* sector buffer (inside ctlr) error */
	#define D_ECC_FAIL		0x04	/* ECC circuitry failure */
	#define D_CTL_PROC_ERR	0x05	/* control processor error */
	#define D_DRIVE_1_FAIL	0x80	/* drive1 failed - ANDed with above */

#define PRECOMP		0x1		/* write precompensation (W) */
#define FEATURES	0x1		/* now called features register */
	#define F_DMA		0x01	/* DMA I/O (otherwise PIO) */
	#define F_OVL		0x02	/* overlapped I/O */
#define SECCNT		0x2		/* sector count (R/W) */
#define INTERRUPT	0x2		/* interrupt reason (ATAPI) */
	#define INTR_CMD	0x01	/* command (1) or data (0) */
	#define INTR_IN		0x02	/* I/O from device (1) or to device (0) */
	#define INTR_REL	0x04	/* bus release */
#define SECTOR		0x3		/* first sector number (R/W) */
#define CYL_LO		0x4		/* cylinder address, low byte (R/W) */
#define CYL_HI		0x5		/* cylinder address, high byte (R/W)*/
#define SDH			0x6		/* sector size/drive/head (R/W)*/
	#define SDH_512		0x20    /* forced to 512 byte sector, no ecc */
	#define SDH_IBM		0xa0    /* forced to 512 byte sector, ecc */
	#define SDH_CHS		0x00    /* cylinder/head/sector addressing */
	#define SDH_LBA		0x40    /* logical block addressing */

#define COMMAND		0x7		/* command register (W)  */
	#define C_NOOP			0x00		/* no-op - always fails */
	#define C_RECAL			0x10		/* recalibrate (reset) */
	#define C_READ			0x20		/* read */
	#define C_WRITE			0x30		/* write */
	#define C_VERIFY		0x40		/* verify */
		#define C_ECC			0x02		/* r/w/v with ecc */
		#define C_NORETRY		0x01		/* r/w/v without retrys */
	#define C_FORMAT		0x50		/* format track */
	#define C_DIAGNOSTIC	0x90		/* drive diagnostic */
	#define C_INIT_DRIVE	0x91		/* initialize drive parameters */
	#define C_READ_MULTI	0xC4		/* read multiple */
	#define C_WRITE_MULTI	0xC5		/* write multiple */
	#define C_SET_MULTI 	0xC6		/* set multiple count */
	#define C_IDENTIFY		0xEC		/* identify and get drive info */
	#define C_FEATURES		0xEF		/* set drive features */

	/* Commands for ATAPI devices */
	#define ATAPI_NOP				0x00
	#define ATAPI_PKT_CMD			0xa0
	#define ATAPI_IDENTIFY_DEVICE	0xa1
	#define ATAPI_SOFT_RESET		0x08

#define STATUS		0x7		/* controllor status (R) */
	#define S_BUSY		0x80		/* Controller busy */
	#define S_READY		0x40		/* Selected drive is ready */
	#define S_WRTFLT	0x20		/* Write fault */
	#define S_DMARDY	0x20		/* DMA ready (ATAPI) */
	#define S_SEEKCMPLT	0x10		/* Seek complete */
	#define S_SERVICE	0x10		/* Service (ATAPI) */
	#define S_DREADY	0x08		/* Data ready (DRQ for ATAPI) */
	#define S_ECCCOR	0x04		/* ECC correction made in data */
	#define S_INDEX		0x02		/* Index pulse from selected drive */
	#define S_ERR		0x01		/* Error detect */


/* alternate registers - add to alt I/O base reg */
#define ALTSTS		0x0		/*alternate fixed disk status (R)*/
#define CTLR		0x0		/*fixed disk controller control (W)*/
	#define CTL_4BIT	0x8		/* use four head bits (wd1003) */
	#define CTL_RESET	0x4		/* reset the controller */
	#define CTL_INTR	0x2		/* disable controller interrupts */
#define DIGIN		0x1		/* disk controller input (R)*/


/* following header code copied and reformatted from OpenBSD ata/atareg.h */

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;

/*	$OpenBSD: atareg.h,v 1.2 1999/09/05 21:45:22 niklas Exp $	*/
/*	$NetBSD: atareg.h,v 1.5 1999/01/18 20:06:24 bouyer Exp $	*/

/*
 * Drive parameter structure for ATA/ATAPI.
 * Bit fields: WDC_* : common to ATA/ATAPI
 *             ATA_* : ATA only
 *             ATAPI_* : ATAPI only.
 */
struct ataparams
{
	u_int16_t config;			/* 0: general configuration */
		#define WDC_CFG_ATAPI_MASK    		0xc000
		#define WDC_CFG_ATAPI    			0x8000
		#define	ATA_CFG_REMOVABLE			0x0080
		#define	ATA_CFG_FIXED				0x0040
		#define ATAPI_CFG_TYPE_MASK			0x1f00
		#define ATAPI_CFG_TYPE(x)			(((x) & ATAPI_CFG_TYPE_MASK) >> 8)
		#define ATAPI_CFG_TYPE_DIRECT		0x00
		#define ATAPI_CFG_TYPE_SEQUENTIAL	0x01
		#define ATAPI_CFG_TYPE_CDROM		0x05
		#define ATAPI_CFG_TYPE_OPTICAL		0x07
		#define ATAPI_CFG_TYPE_NODEVICE		0x1F
		#define	ATAPI_CFG_REMOV				0x0080
		#define ATAPI_CFG_DRQ_MASK			0x0060
		#define ATAPI_CFG_STD_DRQ			0x0000
		#define ATAPI_CFG_IRQ_DRQ			0x0020
		#define ATAPI_CFG_ACCEL_DRQ			0x0040
		#define ATAPI_CFG_CMD_MASK			0x0003
		#define ATAPI_CFG_CMD_12			0x0000
		#define ATAPI_CFG_CMD_16			0x0001

/* words 1-9 are ATA only */
	u_int16_t cylinders;		/* 1: # of non-removable cylinders */
	u_int16_t __reserved1;
	u_int16_t heads;			/* 3: # of heads */
	u_int16_t __retired1[2];	/* 4-5: # of unform. bytes/track */
	u_int16_t sectors;			/* 6: # of sectors */
	u_int16_t __retired2[3];

	u_int8_t serial[20];		/* 10-19: serial number */
	u_int16_t __retired3[2];
	u_int16_t __obsolete1;
	u_int8_t revision[8];		/* 23-26: firmware revision */
	u_int8_t model[40];			/* 27-46: model number */
	u_int16_t multi;			/* 47: maximum sectors per irq (ATA) */
	u_int16_t __reserved2;
	u_int16_t capabilities1;	/* 49: capability flags */
		#define WDC_CAP_IORDY			0x0800
		#define WDC_CAP_IORDY_DSBL		0x0400
		#define	WDC_CAP_LBA				0x0200
		#define	WDC_CAP_DMA				0x0100
		#define ATA_CAP_STBY			0x2000
		#define ATAPI_CAP_INTERL_DMA	0x8000
		#define ATAPI_CAP_CMD_QUEUE		0x4000
		#define	ATAPI_CAP_OVERLP		0x2000
		#define ATAPI_CAP_ATA_RST		0x1000

	u_int16_t capabilities2;	/* 50: capability flags (ATA) */

#ifdef LITTLE_ENDIAN
	u_int8_t __junk2;
	u_int8_t oldpiotiming;		/* 51: old PIO timing mode */
	u_int8_t __junk3;
	u_int8_t olddmatiming;		/* 52: old DMA timing mode (ATA) */
#else
	u_int8_t oldpiotiming;		/* 51: old PIO timing mode */
	u_int8_t __junk2;
	u_int8_t olddmatiming;		/* 52: old DMA timing mode (ATA) */
	u_int8_t __junk3;
#endif

	u_int16_t extensions;		/* 53: extentions supported */
		#define WDC_EXT_UDMA_MODES	0x0004
		#define WDC_EXT_MODES		0x0002
		#define WDC_EXT_GEOM		0x0001

/* words 54-62 are ATA only */
	u_int16_t curcylinders;		/* 54: current logical cyliners */
	u_int16_t curheads;			/* 55: current logical heads */
	u_int16_t cursectors;		/* 56: current logical sectors/tracks */
	u_int16_t curcapacity[2];	/* 57-58: current capacity */
	u_int16_t curmulti;			/* 59: current multi-sector setting */
		#define WDC_MULTI_VALID		0x0100
		#define WDC_MULTI_MASK		0x00ff

	u_int16_t capacity[2];		/* 60-61: total capacity (LBA only) */
	u_int16_t __retired4;

#ifdef LITTLE_ENDIAN
	u_int8_t dmamode_supp;		/* 63: multiword DMA mode supported */
	u_int8_t dmamode_act;		/*     multiword DMA mode active */
	u_int8_t piomode_supp;		/* 64: PIO mode supported */
	u_int8_t __junk4;
#else
	u_int8_t dmamode_act;		/*     multiword DMA mode active */
	u_int8_t dmamode_supp;		/* 63: multiword DMA mode supported */
	u_int8_t __junk4;
	u_int8_t piomode_supp;		/* 64: PIO mode supported */
#endif

	u_int16_t dmatiming_mimi;	/* 65: minimum DMA cycle time */
	u_int16_t dmatiming_recom;	/* 66: recomended DMA cycle time */
	u_int16_t piotiming;		/* 67: mini PIO cycle time without FC */
	u_int16_t piotiming_iordy;	/* 68: mini PIO cycle time with IORDY FC */
	u_int16_t __reserved3[2];

/* words 71-72 are ATAPI only */
	u_int16_t pkt_br;			/* 71: time (ns) to bus release */
	u_int16_t pkt_bsyclr;		/* 72: tme to clear BSY after service */
	u_int16_t __reserved4[2];
	u_int16_t queuedepth;		/* 75: */
		#define WDC_QUEUE_DEPTH_MASK 0x0F

	u_int16_t __reserved5[4];
	u_int16_t ata_major;		/* 80: Major version number */
		#define	WDC_VER_ATA1	0x0002
		#define	WDC_VER_ATA2	0x0004
		#define	WDC_VER_ATA3	0x0008
		#define	WDC_VER_ATA4	0x0010
		#define	WDC_VER_ATA5	0x0020

	u_int16_t ata_minor;		/* 81: Minor version number */
	u_int16_t cmd_set1;			/* 82: command set supported */
		#define WDC_CMD1_NOP	0x4000
		#define WDC_CMD1_RB		0x2000
		#define WDC_CMD1_WB		0x1000
		#define WDC_CMD1_HPA	0x0400
		#define WDC_CMD1_DVRST	0x0200
		#define WDC_CMD1_SRV	0x0100
		#define WDC_CMD1_RLSE	0x0080
		#define WDC_CMD1_AHEAD	0x0040
		#define WDC_CMD1_CACHE	0x0020
		#define WDC_CMD1_PKT	0x0010
		#define WDC_CMD1_PM		0x0008
		#define WDC_CMD1_REMOV	0x0004
		#define WDC_CMD1_SEC	0x0002
		#define WDC_CMD1_SMART	0x0001

	u_int16_t cmd_set2;			/* 83: command set supported */
		#define WDC_CMD2_RMSN	0x0010
		#define WDC_CMD2_DM		0x0001
		#define ATA_CMD2_APM	0x0008
		#define ATA_CMD2_CFA	0x0004
		#define ATA_CMD2_RWQ	0x0002

	u_int16_t cmd_ext;			/* 84: command/features supp. ext. */
	u_int16_t cmd1_en;			/* 85: cmd/features enabled */

/* bits are the same as cmd_set1 */
	u_int16_t cmd2_en;			/* 86: cmd/features enabled */

/* bits are the same as cmd_set2 */
	u_int16_t cmd_def;			/* 87: cmd/features default */

#ifdef LITTLE_ENDIAN
	u_int8_t udmamode_supp;		/* 88: Ultra-DMA mode supported */
	u_int8_t udmamode_act;		/*     Ultra-DMA mode active */
#else
	u_int8_t udmamode_act;		/*     Ultra-DMA mode active */
	u_int8_t udmamode_supp;		/* 88: Ultra-DMA mode supported */
#endif

/* 89-92 are ATA-only */
	u_int16_t seu_time;			/* 89: Sec. Erase Unit compl. time */
	u_int16_t eseu_time;		/* 90: Enhanced SEU compl. time */
	u_int16_t apm_val;			/* 91: current APM value */
	u_int16_t __reserved6[35];	/* 92-126: reserved */
	u_int16_t rmsn_supp;		/* 127: remov. media status notif. */
		#define WDC_RMSN_SUPP_MASK	0x0003
		#define WDC_RMSN_SUPP		0x0001

	u_int16_t sec_st;			/* 128: security status */
		#define WDC_SEC_LEV_MAX		0x0100
		#define WDC_SEC_ESE_SUPP	0x0020
		#define WDC_SEC_EXP			0x0010
		#define WDC_SEC_FROZEN		0x0008
		#define WDC_SEC_LOCKED		0x0004
		#define WDC_SEC_EN			0x0002
		#define WDC_SEC_SUPP		0x0001
};

/* end of OpenBSD-based headers */


#define BSIZE	512		/* default blocksize */
#define TIMEOUT	1		/* in seconds */


struct pself
{
	uInt reg, reg2;
	int ctlr, id, type;
	Bool lba, atapi;
	int cmdlen;
	uLong blocks, blocksize;
	uInt sectors, cylinders, heads;
	Retcode (*read)(uInt addr, void *value, int size);
	Retcode (*write)(uInt addr, Int value, int size);
};
typedef struct pself PSelf;

struct self
{
	PSelf *pself;
	Instance *disklabel;
	Instance *deblocker;
};
typedef struct self Self;


static uByte
read1(Self *s, uInt reg)
{
	PSelf *ps = s->pself;
	uByte val;
	Retcode ret;

	ret = ps->read(ps->reg + reg, &val, 1);

#ifdef DEBUG
	if (g_e->debug)
		dprintf("read1: reg=%#x/%#x val=%#x\n", ps->reg, reg, val);
#endif

	if (ret != NO_ERROR)
		cprintf(g_e, "ATA-read1: cannot read byte from reg %#X!\n",
				ps->reg + reg);
	
	return val;
}

static void
write1(Self *s, uInt reg, uByte b)
{
	PSelf *ps = s->pself;
	Retcode ret;

	ret = ps->write(ps->reg + reg, b, 1);

#ifdef DEBUG
	if (g_e->debug)
		dprintf("write1: reg=%#x/%#x val=%#x\n", ps->reg, reg, b);
#endif

	if (ret != NO_ERROR)
		cprintf(g_e, "ATA-write1: cannot write byte to reg %#X!\n",
				ps->reg + reg);
}

static uShort
read2(Self *s, uInt reg)
{
	PSelf *ps = s->pself;
	uShort val;
	Retcode ret;

	ret = ps->read(ps->reg + reg, &val, 2);
	
#ifdef DEBUG
	if (g_e->debug)
		dprintf("Read2: reg=%#x/%#x val=%#x\n", ps->reg, reg, val);
#endif

	if (ret != NO_ERROR)
		cprintf(g_e, "ATA-read2: cannot read word from reg %#X!\n",
				ps->reg + reg);
	
	return val;
}

static void
write2(Self *s, uInt reg, uShort val)
{
	PSelf *ps = s->pself;
	Retcode ret;

	ret = ps->write(ps->reg + reg, val, 2);

#ifdef DEBUG
	if (g_e->debug)
		dprintf("write2: reg=%#x/%#x val=%#x\n", ps->reg, reg, val);
#endif

	if (ret != NO_ERROR)
		cprintf(g_e, "ATA-write1: cannot write byte to reg %#X!\n",
				ps->reg + reg);
}

static Bool
wait_ready(Self *s, uInt mask)
{
	uInt status = 0;
	uLong stime = get_msecs(g_e);
	uLong etime = stime;
	int timeout = 1000 * TIMEOUT;
	int seccnt, sector, cyl_lo, cyl_hi;

	while (timeout > 0)
	{
		etime = get_msecs(g_e);
		timeout -= etime - stime;
		stime = etime;

		seccnt = read1(s, SECCNT);
		sector = read1(s, SECTOR);
		cyl_lo = read1(s, CYL_LO);
		cyl_hi = read1(s, CYL_HI);
		status = read1(s, STATUS);

		if (status & S_ERR)
			break;

		if ((status & S_BUSY) == 0 && (status & mask) == mask)
			return FALSE;
	}

	DPRINTF(("ATA-wait-ready: timeout: status=%#x\n", status));

	if (status & S_ERR)
		cprintf(g_e, "ATA device not present or not responding\n");

	return TRUE;
}

static Bool
wait_notbusy(Self *s)
{
	uInt status = 0;

	for (;;)
	{
		status = read1(s, STATUS);

		if ((status & S_BUSY) == 0)
			return FALSE;
	}

	DPRINTF(("ATA-wait-notbusy: timeout: status=%#x\n", status));
	return TRUE;
}

static Retcode
atapi_cmd(Environ *e, Self *s, char *cmd,
		char *out, int outlen, char *in, int inlen)
{
	PSelf *ps = s->pself;
	int i, len;
	uShort data;

	DPRINTF(("atapi_cmd: cmdlen=%d\n", ps->cmdlen));

	/* send PACKET command */
	write1(s, FEATURES, 0);		/* PIO, no overlap */
	write1(s, SECCNT, 0);		/* no tag */
	write1(s, SECTOR, 0);		/* -na- */
	write1(s, CYL_LO, 0xFE);	/* maximum byte-count in any transfer */
	write1(s, CYL_HI, 0xFF);
	write1(s, SDH, SDH_IBM | (ps->id << 4));	/* device ID */
	write1(s, COMMAND, ATAPI_PKT_CMD);
	u_sleep(100);

	if (wait_notbusy(s))
	{
		DPRINTF(("atapi_cmd: timeout after PKT_CMD initiated\n"));
		return E_ABORT;
	}

	/* check to see that device is waiting for a command */
	data = read1(s, INTERRUPT);

	if (!(data & INTR_CMD) || (data & INTR_IN))
	{
		DPRINTF(("atapi_cmd: not in cmd-ready state: INTR=%#x ERR=%#x\n",
				data, read1(s, ERROR)));
		return E_ABORT;
	}

	/* send SCSI command */
	wait_notbusy(s);

	for (i = 0; (read1(s, STATUS) & S_DREADY) && i < ps->cmdlen; i += 2)
	{
		data = (cmd[i] & 0xFF) | (cmd[i + 1] << 8);
		write2(s, DATA, data);
		wait_notbusy(s);
	}

	/* optional data for command */
	if (outlen)
	{
		wait_notbusy(s);

		for (i = 0; (read1(s, STATUS) & S_DREADY) && i < outlen; i += 2)
		{
			data = (out[i] & 0xFF) | (out[i + 1] << 8);
			write2(s, DATA, data);
			wait_notbusy(s);
		}

		data = read1(s, INTERRUPT);
	}

	/* optional input from command */
	if (inlen)
	{
		wait_notbusy(s);
		len = read1(s, CYL_LO) | ( read1(s, CYL_HI) << 8);
		DPRINTF(("atapi_cmd: inlen=%d len=%d\n", inlen, len));

		for (i = 0; (read1(s, STATUS) & S_DREADY) && i < inlen; i += 2)
		{
			data = read2(s, DATA);
			wait_notbusy(s);
			in[i] = data & 0xFF;
			in[i + 1] = data >> 8;
		}
	}

	return NO_ERROR;
}

static Retcode
init_drive(Environ *e, Self *s)
{
	PSelf *ps = s->pself;
	uShort buf[256];		/* magic size for identify block */
	struct ataparams *ap = (struct ataparams *)buf;
	int i, seccnt, sector, cyl_lo, cyl_hi;

	/* reset the controller once - seems to screw up if done for each disk */
	if (s->pself->id == 0)
	{
		DPRINTF(("init_drive: reset controller %#x/%#x\n", ps->reg, ps->reg2));
		write1(s, SDH, SDH_IBM);
		seccnt = read1(s, SECCNT);
		sector = read1(s, SECTOR);
		cyl_lo = read1(s, CYL_LO);
		cyl_hi = read1(s, CYL_HI);
		u_sleep(10);
		ps->write(ps->reg2 + CTLR, CTL_RESET, 1);
		u_sleep(1000);
	}

	/* allow 4-bits for heads */
	DPRINTF(("init_drive: allow 4-bits for heads\n"));
	ps->write(ps->reg2 + CTLR, CTL_4BIT + CTL_INTR, 1);
	u_sleep(1000);

	/* select the drive */
	DPRINTF(("init_drive: select drive %d\n", s->pself->id));
	write1(s, SDH, SDH_IBM | (s->pself->id << 4));
	u_sleep(100);

	/* expect seccnt=1 sector=1 cyl_lo=0x14 cyl_hi=0xEB for ATAPI device */
	seccnt = read1(s, SECCNT);
	sector = read1(s, SECTOR);
	cyl_lo = read1(s, CYL_LO);
	cyl_hi = read1(s, CYL_HI);

	DPRINTF(("init_drive: seccnt=%#x sector=%#x cyl_lo=%#x cyl_hi=%#x\n",
			seccnt, sector, cyl_lo, cyl_hi));
	if (seccnt == 1 && sector == 1 && cyl_lo == 0x14 && cyl_hi == 0xEB)
		ps->atapi = TRUE;
	else if (wait_ready(s, S_READY))
		return E_ABORT;

	/* identify the drive */
	DPRINTF(("init_drive: identify drive: atapi=%d\n", ps->atapi));
	write1(s, COMMAND, ps->atapi ? ATAPI_IDENTIFY_DEVICE : C_IDENTIFY);

	if (wait_ready(s, S_DREADY))
		return E_ABORT;

	for (i = 0; i < sizeof buf / sizeof *buf; i++)
		buf[i] = read2(s, DATA);

	/* skip ECC bytes, if any */
	while (read1(s, STATUS) & S_DREADY)
		(void)read2(s, DATA);

#ifdef DEBUG
	if (g_e->debug)
		display_memory(e, (Byte*)buf, sizeof buf);
#endif

	if (ps->atapi)
	{
		struct scsi_read_capacity rc;
		struct scsi_read_capacity_data rcd;

		memset(&rc, 0, sizeof rc);
		memset(&rcd, 0, sizeof rcd);
		rc.op_code = READ_CAPACITY;
		ps->type = ATAPI_CFG_TYPE(ap->config);

		if (ps->type == ATAPI_CFG_TYPE_CDROM ||
				ps->type == ATAPI_CFG_TYPE_OPTICAL)
			s->pself->blocksize = 2048;		/* default for CDs */
		else
			s->pself->blocksize = BSIZE;	/* default for disks */

		if (ap->config & ATAPI_CFG_CMD_16)
			ps->cmdlen = 16;
		else
			ps->cmdlen = 12;

		/* send READ_CAPACITY packet to get blocksize */
		if (atapi_cmd(e, s, (char*)&rc, NULL, 0,
				(char*)&rcd, sizeof rcd) != NO_ERROR)
			return E_ABORT;

		/* and store the results */
		ps->lba = TRUE;
		ps->blocks = (rcd.addr_3 << (BYTE_SIZE * 3)) |
				(rcd.addr_2 << (BYTE_SIZE * 2)) |
				(rcd.addr_1 << BYTE_SIZE) |
				rcd.addr_0;
		ps->blocks++;
		ps->blocksize = (rcd.length_3 << (BYTE_SIZE * 3)) |
				(rcd.length_2 << (BYTE_SIZE * 2)) |
				(rcd.length_1 << BYTE_SIZE) |
				rcd.length_0;

		/* in case no media is loaded, fake it */
		if (ps->blocksize == 0)
		{
			if (ps->type == ATAPI_CFG_TYPE_CDROM ||
					ps->type == ATAPI_CFG_TYPE_OPTICAL)
				s->pself->blocksize = 2048;		/* default for CDs */
			else
				s->pself->blocksize = BSIZE;	/* default for disks */
		}
	}
	else
	{
		/* store the magic numbers */
		ps->sectors = ap->sectors;
		ps->cylinders = ap->cylinders;
		ps->heads = ap->heads;

		ps->lba = (ap->capabilities1 & WDC_CAP_LBA) ? TRUE : FALSE;

		if (ps->lba)
			ps->blocks = (ap->capacity[1] << 16) | ap->capacity[0];
		else
			ps->blocks = ap->cylinders * ap->heads * ap->sectors;

		ps->blocksize = BSIZE;
	}

	DPRINTF(("init_drive: status=%#x config=%#x ctlr=%d id=%d lba=%d"
			" blocksize=%lx blocks=%#lx sectors=%#x cylinders=%#x heads=%#x\n",
			read1(s, STATUS),
			ap->config, ps->ctlr, ps->id, ps->lba, ps->blocksize,
			ps->blocks, ps->sectors, ps->cylinders, ps->heads));

	return NO_ERROR;
}

/* ATA disk-drive methods */

C(f_ata_disk_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s;
	Retcode ret;

	DPRINTF(("ata_disk_open: pkg=%p parent=%p\n", pkg, pkg->parent));
	IFCKSP(e, 0, 4);

	if (!pkg)
		return E_NULL_PACKAGE;

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;
	s->pself = pkg->self;

	DPRINTF(("ata_disk_open: CTLR=%#x ID=%#x\n", s->pself->ctlr, s->pself->id));

	/* parse optional arguments */
	if (inst->args && *inst->args)
	{
		Byte *str;
		str = inst->args + 1;
		DPRINTF(("ata_disk_open: args=\"%s\"\n", str));		
	}

	/* create an instance of /packages/deblocker to support read/write/etc */
	PUSH(e, "");	/* arg-str */
	PUSH(e, 0);		/* arg-len */
	PUSHP(e, "deblocker");			/* name-str */
	PUSH(e, strlen("deblocker"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("ata_disk_open: $open-package deblocker ret=%d\n", ret));

	if (ret != NO_ERROR)
	{
		free(s);
		return ret;
	}

	POPT(e, s->deblocker, Instance*);
	DPRINTF(("ata_disk_open: deblocker=%p\n", s->deblocker));

	if (s->deblocker == NULL)
	{
		free(s);
		return E_NULL_INSTANCE;
	}

	/* create an instance of /packages/disk-label to support load/etc */
	PUSHP(e, inst->args + 1);		/* arg-str */
	PUSH(e, *(uByte*)inst->args);	/* arg-len */
	PUSHP(e, "disk-label");			/* name-str */
	PUSH(e, strlen("disk-label"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("ata_disk_open: $open-package disk-label ret=%d\n", ret));

	if (ret != NO_ERROR)
	{
		PUSHP(e, s->deblocker);
		(void)execute_word(e, "close-package");
		free(s);
		return ret;
	}

	POPT(e, s->disklabel, Instance*);
	DPRINTF(("ata_disk_open: disklabel=%p\n", s->disklabel));

	if (s->disklabel == NULL)
	{
		PUSHP(e, s->deblocker);
		(void)execute_word(e, "close-package");
		free(s);
		return E_NULL_INSTANCE;
	}

	PUSH(e, ret == NO_ERROR ? FTRUE : FFALSE);
	return ret;
}

C(f_ata_disk_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Self *s = inst->self;

	DPRINTF(("ata_disk_close\n"));

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->disklabel);
	(void)execute_word(e, "close-package");
	PUSHP(e, s->deblocker);
	(void)execute_word(e, "close-package");

	/* free our self block */
	free(s);
	inst->self = NULL;

	return NO_ERROR;
}

C(f_ata_disk_read_blocks)		/* read (addr block# #blocks -- #read) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;
	Byte *addr, *a;
	uInt block, count;
	uInt blkno, sector, head, cylin;
	uShort data;
	int b, i;

	DPRINTF(("ata_disk_read_blocks: enter\n"));

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;
	s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s || !s->deblocker)
		return E_BAD_INSTANCE;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	DPRINTF(("ata_disk_read_blocks: count=%d block=%#x addr=%p\n",
			count, block, addr));

	a = addr;

	if (s->pself->atapi)
	{
		struct scsi_rw_big rwb;

		memset(&rwb, 0, sizeof rwb);

		DPRINTF(("atapi_disk_read_blocks: big read\n"));
		rwb.op_code = READ_BIG;
		rwb.addr_3 = (block >> 24) & 0xFF;
		rwb.addr_2 = (block >> 16) & 0xFF;
		rwb.addr_1 = (block >> 8) & 0xFF;
		rwb.addr_0 = block & 0xFF;
		rwb.length1 = (count >> 8) & 0xFF;
		rwb.length0 = count & 0xFF;
		rwb.control = 0;

		/* send read packet blocksize */
		if (atapi_cmd(e, s, (char*)&rwb, NULL, 0,
				(char*)addr, s->pself->blocksize * count) != NO_ERROR)
			return E_ABORT;
	}
	else
	{
		for (b = 0; b < count; b++)
		{
			blkno = block + b;

			if (s->pself->lba)
			{
				sector = (blkno >> 0) & 0xff;
				cylin = (blkno >> 8) & 0xffff;
				head = (blkno >> 24) & 0xf;
				head |= SDH_LBA;
			}
			else
			{
				/* Sectors begin with 1, not 0. */
				sector = (blkno % s->pself->sectors) + 1;
				blkno /= s->pself->sectors;
				head = blkno % s->pself->heads;
				blkno /= s->pself->heads;
				cylin = blkno;
				head |= SDH_CHS;
			}

			DPRINTF(("ata_disk_read_blocks: "
					"ctlr=%d id=%d sector=%#x cylin=%#x head=%#x\n",
					s->pself->ctlr, s->pself->id, sector, cylin, head));

			write1(s, SDH, SDH_IBM | (s->pself->id << 4) | head);
			u_sleep(100);

			if (wait_ready(s, S_READY))
			{
				PUSH(e, 0);		/* bogus actual */
				return E_ABORT;
			}

			write1(s, CYL_LO, cylin & 0xFF);
			write1(s, CYL_HI, cylin >> 8);
			write1(s, SECTOR, sector);
			write1(s, SECCNT, 1);
			write1(s, COMMAND, C_READ);

			if (wait_ready(s, S_DREADY))
			{
				PUSH(e, 0);		/* bogus actual */
				return E_ABORT;
			}

			for (i = 0; i < s->pself->blocksize; i += sizeof (uShort))
			{
				data = read2(s, DATA);
				*a++ = data & 0xFF;
				*a++ = data >> 8;
			}

			/* skip ECC bytes, if any */
			while (read1(s, STATUS) & S_DREADY)
				(void)read2(s, DATA);

			DPRINTF(("ata_disk_read_blocks: status=%#x after read\n",
					read1(s, STATUS)));
		}
	}

#ifdef DEBUG
	if (g_e->debug)
		display_memory(e, addr, count * s->pself->blocksize);
#endif

	PUSH(e, count);
	DPRINTF(("ata_disk_read_blocks: return count=%d\n", count));
	return NO_ERROR;
}

C(f_ata_disk_write_blocks)		/* write (addr block# #blocks -- #written) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;
	Byte *addr;
	uInt block, count;

	DPRINTF(("ata_disk_write_blocks: enter\n"));

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;
	s = inst->self;

	if (!pkg)
		return E_NULL_PACKAGE;

	if (!s || !s->deblocker)
		return E_BAD_INSTANCE;

	IFCKSP(e, 3, 1);
	POP(e, count);
	POP(e, block);
	POPT(e, addr, Byte*);
	DPRINTF(("ata_disk_write_blocks: return\n"));
	return E_UNIMPLEMENTED;		/* too dangerous to implement */
}

C(f_ata_disk_block_size)		/* block-size (-- block-len) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->pself)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, s->pself->blocksize);
	return NO_ERROR;
}

C(f_ata_disk_max_transfer)		/* max-transfer (-- max-len) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->pself)
		return E_BAD_INSTANCE;

	PUSH(e, s->pself->blocksize);
	DPRINTF(("ata_disk_max_transfer: %d\n", TOP(e)));
	return NO_ERROR;
}

C(f_ata_disk_size)				/* size (-- d) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
#ifdef __LONGLONG
	Octlet size;
#endif

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->pself)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 2);
#ifdef __LONGLONG
	size = s->pself->blocks * s->pself->blocksize;
	PUSH(e, size & QUADLET_MASK);
	PUSH(e, size >> QUADLET_SIZE);
#else
	PUSH(e, s->pself->blocks * s->pself->blocksize);
	PUSH(e, 0);
#endif

	return NO_ERROR;
}

C(f_ata_disk_blocks)			/* #blocks (-- u) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->pself)
		return E_BAD_INSTANCE;

	IFCKSP(e, 0, 1);
	PUSH(e, s->pself->blocks);
	return NO_ERROR;
}

enum call_package
{
	C_NONE,
	C_DEBLOCKER,
	C_DISKLABEL
};
typedef enum call_package Call_package;

static Retcode
call_package(Environ *e, Call_package pkgid, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Instance *subpkg;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_NULL_INSTANCE;

	subpkg = (pkgid == C_DEBLOCKER) ? s->deblocker : s->disklabel;

	if (!subpkg)
		return E_BAD_INSTANCE;

	return execute_method_name(e, subpkg, method, CSTR);
}

C(f_ata_disk_read)				/* read (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "read");
}

C(f_ata_disk_write)				/* write (addr len -- actual) */
{
	return call_package(e, C_DEBLOCKER, "write");
}

C(f_ata_disk_seek)				/* seek (pos.lo pos.hi -- status) */
{
	return call_package(e, C_DEBLOCKER, "seek");
}

C(f_ata_disk_load)				/* load (addr -- size) */
{
	return call_package(e, C_DISKLABEL, "load");
}

C(f_ata_disk_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Retcode ret = NO_ERROR;

	DPRINTF(("ata_disk selftest: enter\n"));

	if (diag)
		cprintf(e, "ata_disk selftest...\n");

	/* TODO */

	PUSH(e, ret);			/* successful */
	return NO_ERROR;
}

static const Initentry ata_disk_methods[] =
{
	{ "open",           f_ata_disk_open,          INVALID_FCODE },
	{ "close",          f_ata_disk_close,         INVALID_FCODE },

	{ "read-blocks",    f_ata_disk_read_blocks,   INVALID_FCODE },
	{ "write-blocks",   f_ata_disk_write_blocks,  INVALID_FCODE },
	{ "block-size",     f_ata_disk_block_size,    INVALID_FCODE },
	{ "max-transfer",   f_ata_disk_max_transfer,  INVALID_FCODE },

	{ "size",           f_ata_disk_size,          INVALID_FCODE },
	{ "#blocks",        f_ata_disk_blocks,        INVALID_FCODE },

	{ "read",           f_ata_disk_read,          INVALID_FCODE },
	{ "write",          f_ata_disk_write,         INVALID_FCODE },
	{ "seek",           f_ata_disk_seek,          INVALID_FCODE },
	{ "load",           f_ata_disk_load,          INVALID_FCODE },

	{ "selftest",       f_ata_disk_selftest,      INVALID_FCODE },

	{ NULL,             NULL },
};

static Retcode
probe_ata_disk(Environ *e, uInt reg, uInt reg2, Package *pkg, int ctlr, int id,
		Retcode (*read)(uInt addr, void *value, int size),
		Retcode (*write)(uInt addr, Int value, int size))
{
	Retcode ret;
	PSelf *self;
	Byte *prop;
	Int plen = 0;
	PSelf pbuf;
	Self sbuf;

	DPRINTF(("install_ata_disk_driver: reg=%p\n", reg));

	// probe for disk
	memset(&pbuf, 0, sizeof pbuf);
	pbuf.reg = reg;
	pbuf.reg2 = reg2;
	pbuf.ctlr = ctlr;
	pbuf.id = id;
	pbuf.read = read;
	pbuf.write = write;
	memset(&sbuf, 0, sizeof sbuf);
	sbuf.pself = &pbuf;
	ret = init_drive(e, &sbuf);

	if (ret != NO_ERROR)
		return E_NO_DEVICE;

	/* create new package with name property */
	if (pbuf.type == ATAPI_CFG_TYPE_CDROM ||
			pbuf.type == ATAPI_CFG_TYPE_OPTICAL)
		pkg = new_pkg_name(pkg, "cd");
	else
		pkg = new_pkg_name(pkg, "disk");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	/* set the type property of this device */
	prop_set_str(pkg->props, "device_type", CSTR, "block", CSTR);

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 3 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, ctlr);	/* phys.hi - controller */
	prop_encode_int(prop + plen, &plen, id);	/* phys.lo - master/slave */
	prop_encode_int(prop + plen, &plen, 0);		/* size */
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (pbuf.atapi)
		(void)add_property(pkg->props, "atapi", CSTR, "", 0);

	/* allocate space and stash a copy of our dev for our methods */
	self = (PSelf*)malloc(sizeof *self);

	if (self == NULL)
		return E_OUT_OF_MEMORY;

	/* save it */
	pkg->self = self;
	*self = pbuf;

	ret = init_entries(e, pkg->dict, ata_disk_methods);
	DPRINTF(("install_ata_disk_driver: return %s (%d)\n", err2str(ret), ret));
	return ret;
}

Retcode
probe_ata_disks(Environ *e, uInt reg[4], Package *pkg,
		Retcode (*read)(uInt addr, void *value, int size),
		Retcode (*write)(uInt addr, Int value, int size))
{
	probe_ata_disk(e, reg[0], reg[1], pkg, 0, 0, read, write);
	probe_ata_disk(e, reg[0], reg[1], pkg, 0, 1, read, write);

	if (reg[2])
	{
		probe_ata_disk(e, reg[2], reg[3], pkg, 1, 0, read, write);
		probe_ata_disk(e, reg[2], reg[3], pkg, 1, 1, read, write);
	}

	return NO_ERROR;
}
