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

/* Driver for NCR/Symbios 53C8xx family of SCSI PCI chips.
   Uses /packages/scsi to do the SCSI-specific commands.
 */

#include "defs.h"
#include "pci.h"
#include "scsi.h"

/* define this to turn on special DMA-test hacks - originally intended
   to test PCI bus hang on MIPS NKK system */
/*#define DMA_TEST*/


/* NCR struct def copied and modified from OpenBSD's /sys/pci/ncrreg.h.
   Original copyrights follow.
 */

/**************************************************************************
**
**  $Id: ncrscsi.c,v 1.12 1999/01/25 22:35:35 parag Exp $
**
**  Device driver for the   NCR 53C810   PCI-SCSI-Controller.
**
**  386bsd / FreeBSD / NetBSD
**
**-------------------------------------------------------------------------
**
**  Written for 386bsd and FreeBSD by
**	wolf@cologne.de		Wolfgang Stanglmeier
**	se@mi.Uni-Koeln.de	Stefan Esser
**
**  Ported to NetBSD by
**	mycroft@gnu.ai.mit.edu
**
**-------------------------------------------------------------------------
**
** Copyright (c) 1994 Wolfgang Stanglmeier.  All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
***************************************************************************
*/


/*-----------------------------------------------------------------
**
**	The ncr 53c810 register structure.
**
**-----------------------------------------------------------------
*/

struct ncr_reg
{
	/* standard PCI registers - normally accessed only through config space */
		uInt	vendor_dev_id;
		uInt	cmd_status;
		uInt	class_rev;
		uInt	cache_latency_header;
		uInt	base0;
		uInt	base1;
		uInt	_not_supported[4];
		uInt	_reserved[5];
		uInt	intr_line_pin_mingnt_maxlat;

		uChar    _gap[0x40];

	/* NCR-specific regs start at 0x80 */

/*00*/  uChar    scntl0;    /* full arb., ena parity, par->ATN  */
		#define ARB		0xC0	/* 0=simple, C=full arbitration */
		#define START	0x20	/* start sequence */
		#define WATN	0x10	/* select with SATN/ on a start sequence */
		#define EPC		0x80	/* enable parity checking */
		#define AAP		0x02	/* assert SATN/ on parity error */
		#define TRG		0x01	/* target role */

/*01*/  uChar    scntl1;    /* no reset                         */
        #define   EXC     0x80  /* extra clock cycle of data setup */
		#define   ADB     0x40	/* assert SCSI data bus */
        #define   ISCON   0x10  /* connected to scsi		    */
        #define   CRST    0x08  /* force reset                      */

/*02*/  uChar    scntl2;    /* no disconnect expected           */
	#define   SDU     0x80  /* cmd: disconnect will raise error */
	#define   CHM     0x40  /* sta: chained mode                */
	#define   WSS     0x08  /* sta: wide scsi send           [W]*/
	#define   WSR     0x01  /* sta: wide scsi received       [W]*/

/*03*/  uChar    scntl3;    /* cnf system clock dependent       */
	#define   EWS     0x08  /* cmd: enable wide scsi         [W]*/

/*04*/  uChar    scid;	/* cnf host adapter scsi address    */
	#define   RRE     0x40  /* r/w:e enable response to resel.  */
	#define   SRE     0x20  /* r/w:e enable response to select  */

/*05*/  uChar    sxfer;	/* ### Sync speed and count         */

/*06*/  uChar    sdid;	/* ### Destination-ID               */

/*07*/  uChar    gpreg;	/* ??? IO-Pins                      */

/*08*/  uChar    sfbr;	/* ### First byte in phase          */

/*09*/  uChar    socl;
	#define   CREQ	  0x80	/* r/w: SCSI-REQ                    */
	#define   CACK	  0x40	/* r/w: SCSI-ACK                    */
	#define   CBSY	  0x20	/* r/w: SCSI-BSY                    */
	#define   CSEL	  0x10	/* r/w: SCSI-SEL                    */
	#define   CATN	  0x08	/* r/w: SCSI-ATN                    */
	#define   CMSG	  0x04	/* r/w: SCSI-MSG                    */
	#define   CCD	  0x02	/* r/w: SCSI-C_D                    */
	#define   CIO	  0x01	/* r/w: SCSI-I_O                    */
	#define   PHASE   (CMSG | CCD | CIO)
	#define   PDATAOUT	0
	#define   PDATAIN	CIO
	#define   PCMD		CCD
	#define   PSTATUS	(CCD | CIO)
	#define   PMSGOUT	(CMSG | CCD)
	#define   PMSGIN	(CMSG | CCD | CIO)

/*0a*/  uChar    ssid;

/*0b*/  uChar    sbcl;		/* same bits as socl above */

/*0c*/  uChar    dstat;
        #define   DFE     0x80  /* sta: dma fifo empty              */
        #define   MDPE    0x40  /* int: master data parity error    */
        #define   BF      0x20  /* int: script: bus fault           */
        #define   ABRT    0x10  /* int: script: command aborted     */
        #define   SSI     0x08  /* int: script: single step         */
        #define   SIR     0x04  /* int: script: interrupt instruct. */
        #define   IID     0x01  /* int: script: illegal instruct.   */

/*0d*/  uChar    sstat0;
        #define   ILF     0x80  /* sta: data in SIDL register lsb   */
        #define   ORF     0x40  /* sta: data in SODR register lsb   */
        #define   OLF     0x20  /* sta: data in SODL register lsb   */
        #define   AIP     0x10  /* sta: arbitration in progress     */
        #define   LOA     0x08  /* sta: arbitration lost            */
        #define   WOA     0x04  /* sta: arbitration won             */
        #define   IRST    0x02  /* sta: scsi reset signal           */
        #define   SDP     0x01  /* sta: scsi parity signal          */

/*0e*/  uChar    sstat1;
	#define   FF3210  0xf0	/* sta: bytes in the scsi fifo      */

/*0f*/  uChar    sstat2;
        #define   ILF1    0x80  /* sta: data in SIDL register msb[W]*/
        #define   ORF1    0x40  /* sta: data in SODR register msb[W]*/
        #define   OLF1    0x20  /* sta: data in SODL register msb[W]*/
        #define   LDSC    0x02  /* sta: disconnect & reconnect      */

/*10*/  uInt    dsa;	/* --> Base page                    */

/*14*/  uChar    istat;	/* --> Main Command and status      */
        #define   CABRT   0x80  /* cmd: abort current operation     */
        #define   SRST    0x40  /* mod: reset chip                  */
        #define   SIGP    0x20  /* r/w: message from host to ncr    */
        #define   SEM     0x10  /* r/w: message between host + ncr  */
        #define   CON     0x08  /* sta: connected to scsi           */
        #define   INTF    0x04  /* sta: int on the fly (reset by wr)*/
        #define   SIP     0x02  /* sta: scsi-interrupt              */
        #define   DIP     0x01  /* sta: host/script interrupt       */

/*15*/  uChar    _15_;
/*16*/	uChar	  _16_;
/*17*/  uChar    _17_;

/*18*/	uChar	  ctest0;
/*19*/  uChar    ctest1;

/*1a*/  uChar    ctest2;
	#define   CSIGP   0x40

/*1b*/  uChar    ctest3;
	#define   CLF	  0x04	/* clear scsi fifo		    */

/*1c*/  uInt    temp;	/* ### Temporary stack              */

/*20*/	uChar	  dfifo;
/*21*/  uChar    ctest4;
/*22*/  uChar    ctest5;
/*23*/  uChar    ctest6;

/*24*/  uInt    dbc;	/* ### Byte count and command       */
/*28*/  uInt    dnad;	/* ### Next command register        */
/*2c*/  uInt    dsp;	/* --> Script Pointer               */
/*30*/  uInt    dsps;	/* --> Script pointer save/opcode#2 */
/*34*/  uInt    scratcha;  /* ??? Temporary register a         */

/*38*/  uChar    dmode;
/*39*/  uChar    dien;
/*3a*/  uChar    dwt;

/*3b*/  uChar    dcntl;	/* --> Script execution control     */
        #define   SSM     0x10  /* mod: single step mode            */
        #define   STD     0x04  /* cmd: start dma mode              */
	#define	  NOCOM   0x01	/* cmd: protect sfbr while reselect */

/*3c*/  uInt    adder;

/*40*/  uShort   sien;	/* -->: interrupt enable            */
/*42*/  uShort   sist;	/* <--: interrupt status            */
        #define   STO     0x0400/* sta: timeout (select)            */
        #define   GEN     0x0200/* sta: timeout (general)           */
        #define   HTH     0x0100/* sta: timeout (handshake)         */
        #define   MA      0x80  /* sta: phase mismatch              */
        #define   CMP     0x40  /* sta: arbitration complete        */
        #define   SEL     0x20  /* sta: selected by another device  */
        #define   RSL     0x10  /* sta: reselected by another device*/
        #define   SGE     0x08  /* sta: gross error (over/underflow)*/
        #define   UDC     0x04  /* sta: unexpected disconnect       */
        #define   RST     0x02  /* sta: scsi bus reset detected     */
        #define   PAR     0x01  /* sta: scsi parity error           */

/*44*/  uChar    slpar;
/*45*/  uChar    swide;
/*46*/  uChar    macntl;
/*47*/  uChar    gpcntl;
/*48*/  uChar    stime0;    /* cmd: timeout for select&handshake*/
/*49*/  uChar    stime1;    /* cmd: timeout user defined        */
/*4a*/  uShort   respid;    /* sta: Reselect-IDs                */

/*4c*/  uChar    stest0;

/*4d*/  uChar    stest1;

/*4e*/  uChar    stest2;
	#define   ROF     0x40	/* reset scsi offset (after gross error!) */
	#define   EXT     0x02  /* extended filtering                     */

/*4f*/  uChar    stest3;
	#define   TE     0x80	/* c: tolerAnt enable */
	#define   CSF    0x02	/* c: clear scsi fifo */

/*50*/  uShort   sidl;	/* Lowlevel: latched from scsi data */
/*52*/  uShort   _52_;
/*54*/  uShort   sodl;	/* Lowlevel: data out to scsi data  */
/*56*/  uShort   _56_;
/*58*/  uShort   sbdl;	/* Lowlevel: data from scsi data    */
/*5a*/  uShort   _5a_;
/*5c*/  uChar    scr0;	/* Working register B               */
/*5d*/  uChar    scr1;	/*                                  */
/*5e*/  uChar    scr2;	/*                                  */
/*5f*/  uChar    scr3;	/*                                  */
/*60*/
};

/* end of BSD code */



/* various macros */
#define OURID			7			/* hardwire our SCSI ID for now */
#define REG_SIZE		0x100		/* size of memory-mapped registers */
#define MAX_TRANSFER	(1024*4)

/* sizes of various buffers */
#define SIZE_CMD		12
#define SIZE_DATA		MAX_TRANSFER
#define SIZE_SHORTDATA	256

#ifdef DMA_TEST
#define BUF_SIZE	(1024*16)
#define DMA_LEN		(BUF_SIZE / 4)
#endif


/* instance-specific vars */
struct self
{
	Int physhi, physmid, physlo, sizehi, sizelo;

	volatile struct ncr_reg *reg;	/* mapped-in pointer to registers */

	/* pointers to cmd/data for I/O to/from user buffers */
	Byte *cmd;
	Int cmdlen;
	Byte *data;
	Int datalen;
	Int dir;
	Int hwerr;

	Int id, lun;			/* SCSI ID/LUN of target device for SCSI cmds */
	Int timeout;			/* timeout in ms for subsequent SCSI cmds */

	/* various data buffers - no need to DMA since we use programmed I/O */
	uChar idmsg;
	uChar rcvmsg;
	uChar status;
	uChar ext;
	uChar sync;
	struct scsi_sense sense_cmd;
	struct scsi_sense_data_new sense_data;
	uChar short_data[SIZE_SHORTDATA];

#ifdef DMA_TEST
	Int alarm_xtok;
	Cell dma;				/* mapped memory for DMA access from host */
	Cell busdma;			/* allocated memory for DMA from PCI bus */
	uInt *script;			/* pointer to script for processor */
	Bool dma_running;		/* dma-test is running? */
#endif
};
typedef struct self Self;


#ifdef DEBUG
static void
dumpregs(Environ *e, Self *s)
{
	dprintf(
		"\tscntl0..3=%#x.%x.%x.%x socl=%#x sbcl=%#x sidl=%#x sbdl=%#x\n",
			s->reg->scntl0, s->reg->scntl1, s->reg->scntl2, s->reg->scntl3,
			s->reg->socl, s->reg->sbcl, s->reg->sidl, s->reg->sbdl);
	dprintf("\tdstat=%#x sstat0..2=%#x.%x.%x istat=%#x sist=%#x\n",
			s->reg->dstat, s->reg->sstat0, s->reg->sstat1, s->reg->sstat2,
			s->reg->istat, s->reg->sist);
}
#define DUMPREGS(e, s)	dumpregs(e, s)
#else
#define DUMPREGS(e, s)
#endif


/* structure of following code adapted from ncr5380.c FreeBSD driver
   for simple ncr8xx programmed I/O without using the script processor */

static void
waitfail(Environ *e, Self *s, char *message)
{
	cprintf(e, "ncr8xx: %s timeout\n", message);
	DPRINTF(("ncr8xx: %s timeout\n", message));
}

#define WAITFOR(condition, count, message) \
{ \
	long c = count; \
	while (c-- && ! (condition)) \
		u_sleep(1); \
	if (c <= 0 && message) \
		waitfail(e, s, message); \
}

static void
sendbyte(Environ *e, Self *s, uChar data, Bool atn)
{
	/*DPRINTF(("sendbyte: %#x\n", data));*/
	s->reg->sodl = data;
	s->reg->scntl1 |= ADB;

	/* drop ATN line if this is the final MSGOUT byte */
	if (atn && (s->reg->socl & CATN))
	{
		s->reg->socl &= ~CATN;
		u_sleep(100);
	}

	s->reg->socl |= CACK;
	WAITFOR(!(s->reg->sbcl & CREQ), 20000, "sendbyte");
	s->reg->socl &= ~CACK;
	s->reg->scntl1 &= ~ADB;
}

static uChar
recvbyte(Environ *e, Self *s)
{
	uChar data;

	data = s->reg->sbdl;
	s->reg->socl |= CACK;
	WAITFOR(!(s->reg->sbcl & CREQ), 20000, "recvbyte");
	s->reg->socl &= ~CACK;
	/*DPRINTF(("recvbyte: %#x\n", data));*/
	return data;
}

static void
clear_intrs(Environ *e, Self *s)
{
	uChar istat, dstat;
	uShort sist;

	do
	{
		istat = s->reg->istat;

		if (istat & DIP)
		{
			dstat = s->reg->dstat;
			DPRINTF(("ncr-clear: istat=%#x dstat=%#x\n", istat, dstat));
		}
		
		if (istat & SIP)
		{
			if (istat & DIP)
				u_sleep(1000);

			sist = s->reg->sist;
			DPRINTF(("ncr-clear: istat=%#x sist=%#x\n", istat, sist));
		}
	} while (istat & (DIP | SIP));
}

static void
reset(Environ *e, Self *s)
{
	s->reg->istat = CABRT | SRST;
	u_sleep(100000);
	s->reg->socl = 0;
	s->reg->istat = 0x00;
	s->reg->dmode = 0x00;
	s->reg->dien = 0x00;
	s->reg->dcntl = 0x00;
	s->reg->sien = LE2(0x0000);
	s->reg->ctest4 = 0;
	s->reg->scntl2 = 0;
	s->reg->scntl3 = 0;
	s->reg->scntl1 = CRST;
	u_sleep(100);
	s->reg->scntl1 &= ~CRST;
	u_sleep(100000);
	clear_intrs(e, s);

	/* set our SCSI ID and response ID to OURID */
	s->reg->scid = OURID;
	s->reg->respid = 1 << OURID;
}

static Bool
arbitrate(Environ *e, Self *s)
{
	clear_intrs(e, s);

	if (s->reg->scntl1 & ISCON)		/* make sure we are not connected */
	{
		s->reg->scntl2 &= ~SDU;
		s->reg->socl &= ~CBSY;	/* disconnect */
		u_sleep(10000);
		clear_intrs(e, s);
	}

	/* begin full arbitration and selection */
	s->reg->scntl0 |= ARB | START | WATN;
	/*WAITFOR((sist = s->reg->sist) & (CMP | STO), 300000, NULL);*/
	WAITFOR(!(s->reg->scntl0 & START), 250000, NULL);

	if (s->reg->sist & CMP)		/* successful */
	{
		s->reg->socl |= CBSY;
		clear_intrs(e, s);
		return TRUE;
	}

	s->reg->scntl0 &= ~(START | WATN);	/* end arbitration */
	s->reg->scntl2 &= ~SDU;
	s->reg->socl &= ~CBSY;
	u_sleep(100);
	DPRINTF(("ncr-arbitrate: timeout\n"));
	DUMPREGS(e, s);
	clear_intrs(e, s);
	return FALSE;
}

static Bool
do_phase(Environ *e, Self *s)
{
#ifdef DEBUG
	static char *phase[] =
	{
		"DATAOUT", "DATAIN",  "CMD", "STATUS",
		"Phase4?", "Phase5?", "MSGOUT", "MSGIN",
	};
	uChar prev = 0;
#endif
	uChar sbcl;

	while (((sbcl = s->reg->sbcl) & CBSY))
	{
		/* valid phase only when REQ is asserted */
		WAITFOR(((sbcl = s->reg->sbcl) & CREQ), 200000, "do_phase");

#ifdef DEBUG
		if (sbcl != prev)
		{
			dprintf("do_phase: %s: SBCL=%#x\n", phase[sbcl & PHASE], sbcl);
			prev = sbcl;
		}
#endif

		switch (sbcl & PHASE)
		{
		case PMSGOUT:
			sendbyte(e, s, s->idmsg, TRUE);
			break;

		case PCMD:
			if (s->cmdlen)
			{
				sendbyte(e, s, *s->cmd, TRUE);
				s->cmd++;
				s->cmdlen--;
			}
			else
				sendbyte(e, s, 0, TRUE);

			break;

		case PSTATUS:
			s->status = recvbyte(e, s);
			break;

		case PDATAOUT:
			if (s->datalen)
			{
				sendbyte(e, s, *s->data, FALSE);
				s->data++;
				s->datalen--;
			}
			else
				sendbyte(e, s, 0, FALSE);

			break;

		case PDATAIN:
			if (s->datalen)
			{
				*s->data = recvbyte(e, s);
				s->data++;
				s->datalen--;
			}
			else
				(void)recvbyte(e, s);

			break;

		case PMSGIN:
			s->rcvmsg = recvbyte(e, s);
			s->reg->scntl2 &= ~SDU;	/* disconnect is expected */
			s->reg->socl &= ~CBSY;	/* disconnect */
			u_sleep(100);
			clear_intrs(e, s);

			if (s->rcvmsg != 0)		/* not complete? */
			{
				DPRINTF(("ncr-do-phase: MSGIN = %#x\n", s->rcvmsg));
				return FALSE;
			}

			return TRUE;

		default:
			DPRINTF(("ncr-do-phase: bad phase %#x\n", sbcl & PHASE));
			DUMPREGS(e, s);
			return FALSE;
		}

		clear_intrs(e, s);
	}

	DPRINTF(("ncr-do-phase: failed SBCL=%#x\n", sbcl));
	DUMPREGS(e, s);
	reset(e, s);
	return FALSE;
}


static Retcode
do_scsi_cmd(Environ *e, Byte *buf, Int buflen, Int dir,
		Byte *cmd, Int cmdlen, Int *hwerr, Int *status)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Pci_device_info *devinfo;
	Self *s;
	Retcode ret = NO_ERROR;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (devinfo == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_INSTANCE;

	if (s->id == OURID)		/* always timeout our own ID */
	{
		*hwerr = -1;
		return NO_ERROR;
	}

	DPRINTF(("do_scsi_cmd: pkg=%p devinfo=%p s=%p ID=%d LUN=%d\n",
			pkg, devinfo, s, s->id, s->lun));
	DPRINTF(("\tbuf=%p buflen=%d dir=%d cmd=%p cmdlen=%d\n",
			buf, buflen, dir, cmd, cmdlen));

	/* sanity checks */
	if (buflen > SIZE_DATA)
	{
		cprintf(e, "do_scsi_cmd: buffer size %d > %d\n", buflen, SIZE_DATA);
		return E_ABORT;
	}

	if (cmdlen > SIZE_CMD)
	{
		cprintf(e, "do_scsi_cmd: cmd size %d > %d\n", cmdlen, SIZE_CMD);
		return E_ABORT;
	}

	s->cmd = cmd;
	s->cmdlen = cmdlen;
	s->data = buf;
	s->datalen = buflen;
	s->dir = dir;
	s->reg->sdid = s->id;			/* set target ID */
	s->idmsg = 0x80 | s->lun;		/* set the LUN; do not disconnect */

	if (arbitrate(e, s))
	{
		if (do_phase(e, s))
			*hwerr = s->hwerr;
		else
			*hwerr = -2;
	}
	else
		*hwerr = -1;

	*status = s->status;
	DPRINTF(("do_scsi_cmd: return: ret=%d hwerr=%#x status=%#x\n",
			ret, *hwerr, *status));
	return ret;
}


C(f_ncr_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Pci_device_info *devinfo;
	Self *s;
	Entry *ent;
	Int plen;
	Byte *paddr;
	Cell c;
	Int cfg;
	Retcode ret = NO_ERROR;

	DPRINTF(("ncr_open: pkg=%p devinfo=%p self=%p\n",
			pkg, pkg->self, ((Pci_device_info*)pkg->self)->self));
	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (devinfo == NULL)
		return E_BAD_PACKAGE;

	/* allocate and clear a self object - no need for DMA-safe mem */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;

	/* decode the assigned-addresses property to find the PCI address */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
	{
		DPRINTF(("ncr_open: no assigned-addresses?!?\n"));
		return E_NO_PROPERTY;
	}
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("ncr_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our registers */
	do
		ret = pci_decode_reg_prop(&paddr, &plen, &s->physhi, &s->physmid,
				&s->physlo, &s->sizehi, &s->sizelo);
	while (plen && PCI_PHYSHI_SPACE(s->physhi) != PCI_SPACE_MEM);

	DPRINTF(("ncr_open: physhi=%#x physmid=%#x physlo=%#x\n",
			s->physhi, s->physmid, s->physlo));
    ret = pci_map_in(devinfo->hbridge, s->physhi, s->physmid, s->physlo,
			REG_SIZE, &c);

	if (ret != NO_ERROR)
		return ret;

	/* point to our memory-mapped registers */
	s->reg = (struct ncr_reg*)(uPtr)c;
	DPRINTF(("ncr_open: sizehi=%#x sizelo=%#x reg=%#x\n",
			s->sizehi, s->sizelo, s->reg));

#ifdef DMA_TEST
	/* allocate script and data buffer memory suitable for DMA */
	ret = pci_dma_alloc(devinfo->hbridge, BUF_SIZE, &s->dma);
	DPRINTF(("ncr_open: pci_dma_alloc ret=%d dma=%#Cx\n", ret, s->dma));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, BUF_SIZE, 0, &s->busdma);
	DPRINTF(("ncr_open: pci_dma_map_in ret=%d busdma=%#Cx\n", ret, s->busdma));

	if (ret != NO_ERROR)
		return ret;

	s->script = (uInt*)(uPtr)s->dma;
#endif

	/* parse optional arguments */
	if (inst->args && *inst->args)
	{
		/* TODO */
		;
	}

	s->timeout = 100; 	/* set initial timeout value to 0.1 second */

	/* enable access to memory and enable bus-mastering */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("ncr_open: before cfg=%#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) | PCI_COMMAND_MEMENABLE | PCI_COMMAND_MASTERENABLE);
	/* now registers may be accessed through s->reg */

	/* software reset to istat; turn off all interrupts */
	reset(e, s);

	DPRINTF(("ncr_open: successful\n"));
	PUSH(e, FTRUE);
	return ret;
}

C(f_ncr_close)			/* close (--) */
{
	Package *pkg;
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s = inst->self;
	Pci_device_info *devinfo;
	Retcode ret = NO_ERROR;

	DPRINTF(("ncr_close\n"));

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (!s)
		return E_BAD_INSTANCE;

#ifdef DMA_TEST
	/* kill any alarm */
	if (s->alarm_xtok)
	{
		PUSH(e, s->alarm_xtok);
		PUSH(e, 0);
		ret = execute_word(e, "alarm");

		if (ret != NO_ERROR)
			return ret;
	}

	/* software reset + abort to the chip */
	s->reg->istat = 0xC0;
	u_sleep(10000);
	s->reg->istat = 0x00;

	ret = pci_dma_map_out(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);

	if (ret == NO_ERROR)
		ret = pci_dma_free(devinfo->hbridge, s->dma, BUF_SIZE);
#endif

	/* map out registers */
    ret = pci_map_out(devinfo->hbridge, (uPtr)s->reg, REG_SIZE);

	free(s);

	return ret;
}

C(f_ncr_dma_alloc)			/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ncr_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ncr_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ncr_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}

C(f_ncr_decode_unit)		/* decode-unit (str len -- phys.lo phys.hi) */
{
	Byte *str;
	Int slen;
	Cell lo = 0;
	Cell hi = 0;
	Cell err;

	IFCKSP(e, 2, 2);
	POP(e, slen);
	POPT(e, str, Byte*);
	setstrlen(&str, &slen);

	/* format is: ID,LUN */
	parse_number(16, &str, &slen, &hi, &err, FALSE);

	if (err == FFALSE)
		parse_number(16, &str, &slen, &lo, &err, FALSE);

	PUSH(e, lo);
	PUSH(e, hi);
	return NO_ERROR;
}

C(f_ncr_encode_unit)		/* encode-unit (phys.lo phys.hi -- str len) */
{
	static Byte buf[64];
	Cell hi;
	Cell lo;
	Byte *s = buf;

	IFCKSP(e, 2, 2);
	POP(e, hi);
	POP(e, lo);
	*s = '\0';

	bprintf(s, "%x,%x", (unsigned int)hi, (unsigned int)lo);

	PUSHP(e, buf);
	PUSH(e, strlen(buf));
	return NO_ERROR;
}

C(f_ncr_max_transfer)		/* max-transfer (-- max-len) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, MAX_TRANSFER);
	return NO_ERROR;
}

C(f_ncr_set_address)		/* set-address (logical-unit# target# --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Pci_device_info *devinfo;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 0);
	POP(e, s->id);
	POP(e, s->lun);
	return NO_ERROR;
}

C(f_ncr_set_timeout)		/* set-timeout (msecs --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Pci_device_info *devinfo;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	IFCKSP(e, 1, 0);
	POP(e, s->timeout);
	return NO_ERROR;
}

/* execute-command (buf-addr buf-len dir cmd-addr cmd-len --
		hw-err? | statbyte 0) */
C(f_ncr_execute_command)
{
	Byte *buf;
	Int buflen;
	Int dir;
	Byte *cmd;
	Int cmdlen;
	Int hwerr, status;
	Retcode ret;

	IFCKSP(e, 5, 2);
	POP(e, cmdlen);
	POPT(e, cmd, Byte*);
	POP(e, dir);
	POP(e, buflen);
	POPT(e, buf, Byte*);

	ret = do_scsi_cmd(e, buf, buflen, dir, cmd, cmdlen, &hwerr, &status);

	if (ret != NO_ERROR)
		return ret;

	if (!hwerr)
		PUSH(e, status);

	PUSH(e, hwerr);
	return NO_ERROR;
}

C(f_ncr_show_children)			/* show-children (--) */
{
	if (e->currpkg == NULL)
		return E_NULL_PACKAGE;

	return scsi_probe(e, e->currpkg, TRUE);
}


/* retry-command (buf-addr buf-len dir cmd cmd-len #retries --
		0 | hw-err? stat | sensebuf 0 stat) */
C(f_ncr_retry_command)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *buf;
	Int buflen;
	Int dir;
	Byte *cmd;
	Int cmdlen;
	Int retries;
	Int hwerr, status;
	Bool gotsense;
	Self *s;
	Retcode ret;
	Package *pkg;
	Pci_device_info *devinfo;

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (devinfo == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_PACKAGE;

	IFCKSP(e, 6, 3);
	POP(e, retries);
	POP(e, cmdlen);
	POPT(e, cmd, Byte*);
	POP(e, dir);
	POP(e, buflen);
	POPT(e, buf, Byte*);

	ret = scsi_retry_cmd(e, buf, buflen, dir, retries, cmd, cmdlen,
			&hwerr, &status, &gotsense, &s->sense_cmd, &s->sense_data);

	if (hwerr)			/* hardware error */
	{
		PUSH(e, hwerr);
		PUSH(e, -1);
		return NO_ERROR;
	}
	else if (status)
	{
		if (gotsense)
		{
			PUSHP(e, &s->sense_data);
			PUSH(e, 0);
		}
		else
			PUSH(e, FTRUE);

		PUSH(e, status);
	}
	else
		PUSH(e, 0);

	return NO_ERROR;
}

C(f_ncr_no_data_command)		/* no-data-command (cmd-addr -- error?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *cmd;
	Int hwerr, status;
	Bool gotsense;
	Retcode ret;
	Self *s;
	Package *pkg;
	Pci_device_info *devinfo;

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (devinfo == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_PACKAGE;

	IFCKSP(e, 1, 1);
	POPT(e, cmd, Byte*);

	ret = scsi_retry_cmd(e, NULL, 0, FFALSE, -1, cmd, 6,
			&hwerr, &status, &gotsense, &s->sense_cmd, &s->sense_data);

	if (ret != NO_ERROR)
		return ret;

	if (hwerr)
		PUSH(e, hwerr | 0x80000000ul);
	else if (status)
		PUSH(e, status);
	else
		PUSH(e, FFALSE);

	return NO_ERROR;
}

/* short-data-command (data-len cmd-addr cmd-len -- error? | data-addr 0) */
C(f_ncr_short_data_command)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *cmd;
	Int cmdlen;
	Int datalen;
	Int hwerr, status;
	Bool gotsense;
	Self *s;
	Retcode ret;
	Package *pkg;
	Pci_device_info *devinfo;

	if (!inst)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (devinfo == NULL)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (s == NULL)
		return E_BAD_PACKAGE;

	IFCKSP(e, 3, 2);
	POP(e, cmdlen);
	POPT(e, cmd, Byte*);
	POP(e, datalen);

	if (datalen <= 0 || datalen >= SIZE_SHORTDATA)
	{
		cprintf(e, "ncr-short-data-command: data len must be > 0 and < %d\n",
				SIZE_SHORTDATA);
		return E_ABORT;
	}

	ret = scsi_retry_cmd(e, (Byte*)s->short_data, datalen, TRUE, -1,
			cmd, cmdlen,
			&hwerr, &status, &gotsense, &s->sense_cmd, &s->sense_data);

	if (ret != NO_ERROR)
		return ret;

	if (hwerr)
		PUSH(e, hwerr | 0x80000000ul);
	else if (status)
		PUSH(e, status);
	else
	{
		PUSHP(e, s->short_data);
		PUSH(e, 0);
	}

	return NO_ERROR;
}

C(f_ncr_diagnose)				/* diagnose (-- error-code | 0) */
{
	/* TODO: perform test-unit-ready and send-diagnostic on current target */

	return NO_ERROR;
}
 
C(f_ncr_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	DPRINTF(("ncr selftest: enter\n"));

	if (diag)
		cprintf(e, "ncr selftest...\n");

	/* TODO */

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}


#ifdef DMA_TEST
/* hack to perform endless DMA transfers -
 */

/* called periodically to check chip */
C(f_ncr_alarm)			/* ncr-alarm (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Pci_device_info *devinfo;
	Self *s;

	if (!pkg)
		return E_NULL_PACKAGE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	if (s->reg->istat & 0x07)
	{
		cprintf(e, "ncr_alarm: ISTAT=%#x DSTAT=%#x SIST=%#x\n",
				s->reg->istat, s->reg->dstat, s->reg->sist);
		s->reg->istat = 0x00;
	}
	else if (s->dma_running)
	{
	/****
		Byte *mem = (Byte*)(uPtr)s->dma;
		Int i = BUF_SIZE;
		Int t = 0;

		while (i--)
			t += *mem++;
	****/

		cprintf(e, ".");
		DPRINTF(("."));
	}

	return NO_ERROR;
}

C(f_ncr_dma_test)			/* ncr-dma-test (-- inst|0) */
{
	Bool diag = diagnostic_mode(e);
	Package *pkg;
	Instance *inst;
	Cell saveinst;
	Entry *ent;
	Pci_device_info *devinfo;
	Self *s;
	uInt *scr;
	Retcode ret;
	Byte name[STR_SIZE];

	DPRINTF(("ncr-dma-test: begin\n"));
	
	/* assume we are at the right place in the tree to find ourself */
	if (!e->currpkg)
		return E_NULL_PACKAGE;
	
	get_device_name(e, e->currpkg, name);
	DPRINTF(("ncr-dma-test: name=\"%s\"\n", name));

	/* call "open-dev" to open ourself */
	PUSHP(e, name);
	PUSH(e, strlen(name));
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	inst = (Instance*)(uPtr)TOP(e);	/* leave it on the stack for our caller */
	
	if (inst == NULL)
		return E_NO_DEVICE;

	/* sanity checks */
	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	devinfo = (Pci_device_info*)pkg->self;

	if (!devinfo)
		return E_BAD_PACKAGE;

	if (diag)
		cprintf(e, "ncr-dma-test: starting DMA test\n");

	/* set the alarm to watch the NCR chip */
	ent = find_table(pkg->dict, "ncr-alarm", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;

	saveinst = e->currinst;
	e->currinst = (uPtr)inst;
	s->alarm_xtok = ent->xtok;
	PUSH(e, s->alarm_xtok);
	PUSH(e, 200);
	ret = execute_word(e, "alarm");
	e->currinst = saveinst;

	if (ret != NO_ERROR)
		return ret;

	/* initialize script with loop to constantly move data from one
	   memory buffer to another */
	scr = s->script;

	DPRINTF(("ncr-dma-test: initializing script\n"));
	scr[0] = LE4(0xC0000000 | DMA_LEN);			/* MOVE MEMORY */
	scr[1] = LE4(s->busdma + DMA_LEN);				/* source addr */
	scr[2] = LE4(s->busdma + DMA_LEN * 2);			/* dest addr */
	scr[3] = LE4(0x80080000);					/* JUMP */
	scr[4] = LE4(s->busdma);						/* to MOVE cmd */
	DPRINTF(("ncr-dma-test: sync\n"));
	pci_dma_sync(devinfo->hbridge, s->dma, s->busdma, BUF_SIZE);

	/* start script processing */
	DPRINTF(("ncr-dma-test: start script %p\n", scr));
	s->reg->dsp = LE4(s->busdma);	/* PCI addr of script */

	DPRINTF(("ncr-dma-test: running\n"));
	s->dma_running = TRUE;
	return NO_ERROR;
}
#endif


static const Initentry ncr_methods[] =
{
	{ "open",				f_ncr_open,					INVALID_FCODE },
	{ "close",				f_ncr_close,				INVALID_FCODE },
	{ "dma-alloc",			f_ncr_dma_alloc,			INVALID_FCODE },
	{ "dma-free",			f_ncr_dma_free,				INVALID_FCODE },
	{ "decode-unit",  		f_ncr_decode_unit,			INVALID_FCODE },
	{ "encode-unit",		f_ncr_encode_unit,			INVALID_FCODE },

	{ "max-transfer",		f_ncr_max_transfer,			INVALID_FCODE },
	{ "set-address",		f_ncr_set_address,			INVALID_FCODE },
	{ "set-timeout",		f_ncr_set_timeout,			INVALID_FCODE },
	{ "show-children",		f_ncr_show_children,		INVALID_FCODE },

	{ "execute-command",	f_ncr_execute_command,		INVALID_FCODE },
	{ "retry-command",		f_ncr_retry_command,		INVALID_FCODE },
	{ "no-data-command",	f_ncr_no_data_command,		INVALID_FCODE },
	{ "short-data-command",	f_ncr_short_data_command,	INVALID_FCODE },

	{ "diagnose",			f_ncr_diagnose,				INVALID_FCODE },
	{ "selftest",			f_ncr_selftest,				INVALID_FCODE },

#ifdef DMA_TEST
	{ "ncr-alarm",      f_ncr_alarm,         INVALID_FCODE },
	{ "ncr-dma-test",   f_ncr_dma_test,      INVALID_FCODE },
#endif

	{ NULL,					NULL },
};

PCI(install_ncr_driver)
{
	Package *savepkg;
	Retcode ret;
	Pci_device_info *newinfo;
	Int cfg;

	DPRINTF(("install_ncr_driver: pkg=%p devinfo=%p\n", pkg, devinfo));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "spi", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, ncr_methods);

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	pkg->self = (struct pself*)newinfo;		/* and save it */

	/* turn on this device if it is not turned on so we can assign addrs */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) | PCI_COMMAND_MEMENABLE | PCI_COMMAND_MASTERENABLE);

	/* assign our address space early so we can probe the SCSI bus */
	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = execute_word(e, "assign-addresses");
	e->currpkg = savepkg;

	if (ret == NO_ERROR)
		ret = prop_set_int(pkg->props, "scsi-initiator-ID", CSTR, OURID);

	/* probe for any SCSI devices connected to this controller */
	if (ret == NO_ERROR)
		ret = scsi_probe(e, pkg, FALSE);

	return ret;
}

Pci_driver ncr_53C8xx_driver =
{
	{ 0, 0, 0, 0, 0, 0x10000, 0x1000, 0x0000, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFF00, 0, 0, 0 },
	install_ncr_driver
};
