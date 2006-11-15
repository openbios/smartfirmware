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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* USB root node for OHCI-compatible controller chips.
 */

/*#define DEBUG*/

#include "defs.h"
#include "pci.h"
#include "usb.h"
#include "ohci.h"


#define PAGE_SIZE		4096
#define DMA_SIZE		(PAGE_SIZE * 2)
#define NUM_EDS			2
#define NUM_TDS			((PAGE_SIZE - OHCI_HCCA_SIZE - \
								OHCI_ED_SIZE * NUM_EDS) / OHCI_TD_SIZE)

#define CONTROL_ED		0
#define BULK_ED			1

#define TIMEOUT			1000	/* ms to wait for a reply */


struct pci_self
{
	Pci_device_info *devinfo;
	USB_root usb;

	volatile uByte *regs;		/* registers */
	Cell dma;					/* mapped memory for DMA access from host */
	Cell busdma;				/* allocated memory for DMA from PCI bus */

	/* memory allocated for both host and DMA access from dma/busdma block */
	struct ohci_hcca *hcca;
	uInt hccadma;
	ohci_soft_ed_t ed[NUM_EDS];
	ohci_soft_td_t td[NUM_TDS];
	uByte *buf;
	uInt bufdma;
};
typedef struct pci_self Self;

#define OWRITE4(s, r, v)	(*(volatile uInt*)((s)->regs + (r)) = LE4(v))
#define OREAD4(s, r)		LE4(*(volatile uInt*)((s)->regs + (r)))
#define OREAD2(s, r)		LE2(*(volatile uShort*)((s)->regs + (r)))


#ifdef DEBUG
void
ohci_dumpregs(Self *sc)
{
	dprintf("ohci_dumpregs: rev=0x%08x control=0x%08x command=0x%08x\n",
	       OREAD4(sc, OHCI_REVISION),
	       OREAD4(sc, OHCI_CONTROL),
	       OREAD4(sc, OHCI_COMMAND_STATUS));
	dprintf("               intrstat=0x%08x intre=0x%08x intrd=0x%08x\n",
	       OREAD4(sc, OHCI_INTERRUPT_STATUS),
	       OREAD4(sc, OHCI_INTERRUPT_ENABLE),
	       OREAD4(sc, OHCI_INTERRUPT_DISABLE));
	dprintf("               hcca=0x%08x percur=0x%08x ctrlhd=0x%08x\n",
	       OREAD4(sc, OHCI_HCCA),
	       OREAD4(sc, OHCI_PERIOD_CURRENT_ED),
	       OREAD4(sc, OHCI_CONTROL_HEAD_ED));
	dprintf("               ctrlcur=0x%08x bulkhd=0x%08x bulkcur=0x%08x\n",
	       OREAD4(sc, OHCI_CONTROL_CURRENT_ED),
	       OREAD4(sc, OHCI_BULK_HEAD_ED),
	       OREAD4(sc, OHCI_BULK_CURRENT_ED));
	dprintf("               done=0x%08x fmival=0x%08x fmrem=0x%08x\n",
	       OREAD4(sc, OHCI_DONE_HEAD),
	       OREAD4(sc, OHCI_FM_INTERVAL),
	       OREAD4(sc, OHCI_FM_REMAINING));
	dprintf("               fmnum=0x%08x perst=0x%08x lsthrs=0x%08x\n",
	       OREAD4(sc, OHCI_FM_NUMBER),
	       OREAD4(sc, OHCI_PERIODIC_START),
	       OREAD4(sc, OHCI_LS_THRESHOLD));
	dprintf("               desca=0x%08x descb=0x%08x stat=0x%08x\n",
	       OREAD4(sc, OHCI_RH_DESCRIPTOR_A),
	       OREAD4(sc, OHCI_RH_DESCRIPTOR_B),
	       OREAD4(sc, OHCI_RH_STATUS));
	dprintf("               port1=0x%08x port2=0x%08x\n",
	       OREAD4(sc, OHCI_RH_PORT_STATUS(1)),
	       OREAD4(sc, OHCI_RH_PORT_STATUS(2)));
	dprintf("         HCCA: frame_number=0x%04x done_head=0x%08x\n",
	       LE4(sc->hcca->hcca_frame_number),
	       LE4(sc->hcca->hcca_done_head));
}
#else
#	define ohci_dumpregs(s)		/* noop */
#endif


EC(f_usb_open);			/* open (-- okay?) */
EC(f_usb_close);			/* open (-- okay?) */

static const Initentry ohci_methods[] =
{
	{ "open",			f_usb_open,				INVALID_FCODE },
	{ "close",			f_usb_close,			INVALID_FCODE },
	{ "decode-unit",	f_usb_port_decode_unit,	INVALID_FCODE },
	{ "encode-unit",	f_usb_port_encode_unit,	INVALID_FCODE },
	{ NULL,				NULL },
};

Retcode
ohci_device_request(Environ *e, USB_self *us,
		int pkttype, int devaddr, int endpoint,
		usb_device_request_t *request, Byte *data, Int datalen)
{
	Retcode ret = NO_ERROR;
	Self *s;
	int i, j, len, status, ed;
	Byte *dp = data;
	Int dlen = datalen;
	Bool isread = FALSE;
	ohci_soft_td_t *etd;
	ohci_td_t *td;

	DPRINTF(("ohci_device_request: enter: "
			"data=%#x len=%d devaddr=%d endpoint=%d\n",
			data, datalen, devaddr, endpoint));

	if (us->root->pkg->self == NULL)
		return E_NULL_PACKAGE;

	if (((Pci_device_info *)us->root->pkg->self)->self == NULL)
		return E_BAD_PACKAGE;

	s = ((Pci_device_info *)us->root->pkg->self)->self;

	if (request->request_type & UT_READ)
		isread = TRUE;

	/* 1st TD is used for command packet */
	if (pkttype == USB_SETUP)
	{
		memcpy(s->td[0].cbp, request, USB_DEVICE_REQUEST_SIZE);
		s->td[0].td->td_cbp = LE4(s->td[0].cbp_phys);
		s->td[0].td->td_be = LE4(s->td[0].cbp_phys +
				USB_DEVICE_REQUEST_SIZE - 1);
		s->td[0].td->td_nexttd = LE4(s->td[1].physaddr);
		j = OHCI_TD_SETUP | OHCI_TD_NOCC |
				OHCI_TD_TOGGLE_0 | OHCI_TD_NOINTR | OHCI_TD_R;
		DPRINTF(("ohci_device_request: td[0]->td_flags=%#x\n", j));
		s->td[0].td->td_flags = LE4(j);
		i = 1;
		ed = CONTROL_ED;
	}
	else
	{
		i = 0;
		ed = BULK_ED;
	}

	/* init each TD needed for any data packets */
	for (; dlen > 0 && i < NUM_TDS - 1; i++)
	{
		len = (dlen > USB_MIN_PACKET) ? USB_MIN_PACKET : dlen;

		if (!isread)
		{
			memcpy(s->td[i].cbp, dp, len);
			dp += len;
		}
		else
			memset(s->td[i].cbp, 0, USB_MIN_PACKET);

		s->td[i].len = len;
		s->td[i].td->td_cbp = LE4(s->td[i].cbp_phys);
		s->td[i].td->td_be = LE4(s->td[i].cbp_phys + len - 1);
		s->td[i].td->td_nexttd = LE4(s->td[i + 1].physaddr);
		j = (isread ? OHCI_TD_IN : OHCI_TD_OUT) |
				OHCI_TD_NOCC |
				((i & 1) ? OHCI_TD_TOGGLE_1 : OHCI_TD_TOGGLE_0) |
				OHCI_TD_NOINTR |
				OHCI_TD_R;
		s->td[i].td->td_flags = LE4(j);
		dlen -= len;
	}

	if (i >= NUM_TDS - 1)
	{
		cprintf(e, "ohci_device_request: out of transmit descriptors!\n");
		return E_OUT_OF_MEMORY;
	}

	/* last TD is used to to read the status or ack the read */
	s->td[i].len = 0;
	s->td[i].td->td_cbp = (uPtr)NULL;
	s->td[i].td->td_be = (uPtr)NULL;
	s->td[i].td->td_nexttd = LE4(s->td[i + 1].physaddr);
	j = (isread ? OHCI_TD_OUT : OHCI_TD_IN) |
			OHCI_TD_NOCC |
			OHCI_TD_TOGGLE_1 |
			OHCI_TD_SET_DI(1) |
			OHCI_TD_R;
	s->td[i].td->td_flags = LE4(j);
	etd = &s->td[i];
	DPRINTF(("ohci_device_request: end-td=%d\n", i));

	/* init the endpoint descriptor */
	s->ed[ed].ed->ed_nexted = (uPtr)NULL;
	s->ed[ed].ed->ed_headp = LE4(s->td[0].physaddr);
	s->ed[ed].ed->ed_tailp = LE4(s->td[i + 1].physaddr);
	i = OHCI_ED_SET_FA(devaddr) | 
			OHCI_ED_SET_EN(endpoint) | OHCI_ED_DIR_TD | 
			(us->lowspeed ? OHCI_ED_SPEED : 0) | OHCI_ED_FORMAT_GEN |
			OHCI_ED_SET_MAXP(USB_MIN_PACKET);
	DPRINTF(("ohci_device_request: ed[%d]->ed_flags = %#x\n", ed, i));
	s->ed[ed].ed->ed_flags = LE4(i);

	/* tell chip we have a new control descriptor */
	s->hcca->hcca_done_head = LE4((uPtr)NULL);
	pci_dma_sync(s->devinfo->hbridge, (uPtr)s->dma, s->busdma, DMA_SIZE);

	if (pkttype == USB_SETUP)
	{
		OWRITE4(s, OHCI_CONTROL_HEAD_ED, s->ed[ed].physaddr);
		OWRITE4(s, OHCI_COMMAND_STATUS, OHCI_CLF);
	}
	else
	{
		OWRITE4(s, OHCI_BULK_HEAD_ED, s->ed[ed].physaddr);
		OWRITE4(s, OHCI_COMMAND_STATUS, OHCI_BLF);
	}

again:
	for (i = 0; i < TIMEOUT * 1000; i += 10)
	{
		status = OREAD4(s, OHCI_INTERRUPT_STATUS) & (OHCI_WDH | OHCI_UE);

		if (status)
			break;

		/* ack other interrupts blindly */
	#ifdef DEBUG
		if (status & ~OHCI_SF)
			dprintf("ohci_device_request: blind-ack of status=%#x\n", status);
	#endif

		OWRITE4(s, OHCI_INTERRUPT_STATUS, status);
		u_sleep(10);
	}

	if (i >= TIMEOUT * 1000)
	{
		cprintf(e, "ohci_device_request: timeout for addr=%d endpoint=%d\n",
				devaddr, endpoint);
	#ifdef DEBUG
		dprintf("ohci_decice_request: flags:");

		for (i = 0; i < NUM_TDS; i++)
		{
			dprintf(" %d=%#x", i, LE4(s->td[i].td->td_flags));

			if (&s->td[i] == etd)
				break;
		}

		dprintf("\n\t");
		ohci_dumpregs(s);
	#endif
		return E_ABORT;
	}

	DPRINTF(("ohci_device_request: interrupt-status=%#x\n", status));
	pci_dma_sync(s->devinfo->hbridge, (uPtr)s->dma, s->busdma, DMA_SIZE);
	td = NULL;

	for (i = 0; i < NUM_TDS; i++)
	{
		if ((LE4(s->hcca->hcca_done_head) & ~OHCI_DONE_INTRS) ==
				s->td[i].physaddr)
		{
			DPRINTF(("ohci_device_request: hcca_done_head == td[%d]\n", i));
			td = s->td[i].td;
			break;
		}
	}

	if (td == NULL)
	{
		DPRINTF(("ohci_device_request: done head not a transmit-desc?!?\n"));
		OWRITE4(s, OHCI_INTERRUPT_STATUS, status);
		goto again;
	}

	if ((status & OHCI_WDH) && td != etd->td)
	{
		i = OHCI_TD_GET_CC(LE4(td->td_flags));
		DPRINTF(("ohci_device_request: head=%#x td=%#x status=%#x cc=%d\n",
				LE4(s->hcca->hcca_done_head), etd->physaddr, status, i));

		if (i != OHCI_CC_NO_ERROR)
		{
			static char *cc_err[] =
			{
				"no error",
				"CRC fail",
				"bit stuffing",
				"data toggle mismatch",
				"stall",
				"device not responding",
				"PID check failure",
				"unexpected PID",
				"data overrun",
				"data underrun",
				"buffer overrun",
				"buffer underrun",
				"unknown condition 14",
				"not accessed",
				NULL
			};

			cprintf(e, "OHCI: USB error for device %d endpoint %d: ",
					devaddr, endpoint);

			if (i >= OHCI_CC_NO_ERROR && i <= OHCI_CC_NOT_ACCESSED)
				cprintf(e, "%s\n", cc_err[i]);
			else
				cprintf(e, "unknown condition %d\n", i);

			return E_ABORT;
		}

		OWRITE4(s, OHCI_INTERRUPT_STATUS, status);
		goto again;
	}

	OWRITE4(s, OHCI_INTERRUPT_STATUS, status);
	i = OHCI_TD_GET_CC(LE4(td->td_flags));
	DPRINTF(("ohci_device_request: tdcc=%d\n", i));

	if (i != OHCI_CC_NO_ERROR)
	{
		cprintf(e, "ohci_device_request: addr=%d endp=%d condition=%d\n",
				devaddr, endpoint, i);
		return E_ABORT;
	}

	s->ed[ed].ed->ed_flags = LE4(OHCI_ED_SKIP);

	if (isread)
	{
		/* copy data into user buffer */
		pci_dma_sync(s->devinfo->hbridge, (uPtr)s->dma, s->busdma, DMA_SIZE);
		dlen = 0;

		for (i = pkttype == USB_SETUP ? 1 : 0;
				dlen < datalen && s->td[i].len != 0 && i < NUM_TDS; i++)
		{
			memcpy(data + dlen, s->td[i].cbp, s->td[i].len);
			dlen += s->td[i].len;
		}
	}

	DPRINTF(("ohci_device_request: exit retcode=%s\n", err2str(ret)));
	return ret;
}

Retcode
ohci_probe_port(Environ *e, Package *pkg, Self *s, int port)
{
	Retcode ret;
	int i;

	/* power on this port if each port is independently controlled */
	if ((OREAD4(s, OHCI_RH_DESCRIPTOR_A) & OHCI_PSM) &&
			(!(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PPOWER)))
	{
		DPRINTF(("ohci_probe_port(%d): powering-on\n", port));
		OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PPOWER);
	}

	u_sleep(USB_PORT_POWERUP_DELAY * 1000);
	DPRINTF(("ohci_probe_port(%d): status=%#x\n",
			port, OREAD4(s, OHCI_RH_PORT_STATUS(port))));

	if (!(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PCONNECT))
	{
		DPRINTF(("ohci_probe_port(%d): no device\n\t", port));

		if ((OREAD4(s, OHCI_RH_DESCRIPTOR_A) & OHCI_PSM) &&
				(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PPOWER))
			OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PPOWEROFF);

		ohci_dumpregs(s);
		return NO_ERROR;
	}

	DPRINTF(("ohci_probe_port(%d): got one!\n", port));

	for (i = 0; i < 1000 &&
			(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PCONNECTC); i++)
	{
		OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PCONNECTC);
		u_sleep(USB_PORT_RESET_DELAY * 1000);
	}

	if (i >= 1000)
	{
		cprintf(e, "ohci_probe_port: timeout waiting for connect on port %d\n",
				port);
		return NO_ERROR;
	}

	DPRINTF(("ohci_probe_port(%d): enabling\n", port));
	OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PENABLE);
	u_sleep(100);
	OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PCONNECTC | OHCI_PENABLEC);

	DPRINTF(("ohci_probe_port(%d): resetting\n", port));
	OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PRESET);
	u_sleep(USB_RESET_DELAY * 1000);

	for (i = 0; i < 1000 &&
			!(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PRESETC); i++)
		u_sleep(USB_PORT_RESET_DELAY * 1000);

	if (i >= 1000)
	{
		cprintf(e, "ohci_probe_port: timeout waiting for reset on port %d\n",
				port);
		return NO_ERROR;
	}

	DPRINTF(("ohci_probe_port: "));
	OWRITE4(s, OHCI_RH_PORT_STATUS(port), OHCI_PCONNECTC | OHCI_PRESETC);
	ohci_dumpregs(s);

	ret = usb_probe_device(e, pkg, &s->usb,
			(OREAD4(s, OHCI_RH_PORT_STATUS(port)) & OHCI_PSPEED) ?
					TRUE : FALSE, port);

	return ret;
}

PCI(install_usb_ohci_driver)
{
	Retcode ret;
	Self *s;
	Entry *ent;
	Int physhi, physmid, physlo, sizehi, sizelo;
	Int plen;
	Byte *paddr;
	Cell dma, busdma;
	Cell c;
	Package *savepkg;
	Int cfg, i;

	ret = install_usb_root_driver(e, pkg, devinfo, ohci_methods,
			sizeof (Self));

	if (ret != NO_ERROR)
		return ret;

	s = ((Pci_device_info*)pkg->self)->self;
	s->devinfo = devinfo;
	s->usb.pkg = pkg;
	s->usb.nextaddr = 1;
	s->usb.device_request = ohci_device_request;


	/* turn on this device if it is not turned on so we can assign addrs */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) | PCI_COMMAND_MEMENABLE | PCI_COMMAND_MASTERENABLE);

	/* assign our address space so we can probe the USB bus */
	savepkg = e->currpkg;
	e->currpkg = pkg;
	ret = execute_word(e, "assign-addresses");
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;


	/* decode the assigned-addresses property to find the PCI address and
	   size of the registers */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("usb_ohci: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our registers */
	do
	{
		ret = pci_decode_reg_prop(&paddr, &plen, &physhi, &physmid,
				&physlo, &sizehi, &sizelo);
		DPRINTF(("usb_ohci: phys=%#x.%#x.%#x size=%#x.%#x ret=%s\n",
				physhi, physmid, physlo, sizehi, sizelo, err2str(ret)));
	}
	while (ret == NO_ERROR && PCI_PHYSHI_SPACE(physhi) != PCI_SPACE_MEM);
	
	DPRINTF(("usb_ohci: phys=%#x.%#x.%#x size=%#x.%#x\n",
			physhi, physmid, physlo, sizehi, sizelo));
    ret = pci_map_in(devinfo->hbridge, physhi, physmid, physlo, sizelo, &c);

	if (ret != NO_ERROR)
		return ret;

	s->regs = (uByte*)(uPtr)c;
	DPRINTF(("usb_ohci: regs=%p\n", s->regs));


	/* allocate big enough buffer for DMA, EPs, TDs, and data bufs */
	ret = pci_dma_alloc(devinfo->hbridge, DMA_SIZE, &s->dma);
	DPRINTF(("usb_ohci: pci_dma_alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, DMA_SIZE, 0, &s->busdma);
	DPRINTF(("usb_ohci: pci_dma_map_in ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	DPRINTF(("usb_ohci: dma=%p busdma=%p\n", s->dma, s->busdma));

	/* setup our buffers - alignment should work out after aligning HCCA */
	dma = (s->dma + OHCI_HCCA_ALIGN - 1) & ~(OHCI_HCCA_ALIGN - 1);
	busdma = (s->busdma + OHCI_HCCA_ALIGN - 1) & ~(OHCI_HCCA_ALIGN - 1);
	s->hcca = (struct ohci_hcca*)(uPtr)dma;
	s->hccadma = busdma;
	dma += OHCI_HCCA_SIZE;
	busdma += OHCI_HCCA_SIZE;
	memset(s->hcca, 0, OHCI_HCCA_SIZE);
	DPRINTF(("usb_ohci: hcca=%p hccadma=%#x dma=%p busdma=%#x\n",
			s->hcca, s->hccadma, dma, busdma));

	/* allocate space for EDs and TDs - should already be aligned */
	for (i = 0; i < NUM_EDS; i++)
	{
		s->ed[i].ed = (ohci_ed_t*)(uPtr)dma;
		s->ed[i].physaddr = busdma;
		dma += OHCI_ED_SIZE;
		busdma += OHCI_ED_SIZE;
		s->ed[i].ed->ed_nexted = LE4((uPtr)NULL);
		s->ed[i].ed->ed_flags = LE4(OHCI_ED_SKIP);
	}

	for (i = 0; i < NUM_TDS; i++)
	{
		s->td[i].td = (ohci_td_t*)(uPtr)dma;
		s->td[i].physaddr = busdma;
		dma += OHCI_TD_SIZE;
		busdma += OHCI_TD_SIZE;
	}

	/* realign for data-buffers to make sure they are on the same page */
	dma = (dma + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	busdma = (busdma + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	DPRINTF(("usb_ohci: data-bufs: dma=%p busdma=%#x\n", dma, busdma));

	for (i = 0; i < NUM_TDS; i++)
	{
		s->td[i].cbp = (uByte*)(uPtr)dma;
		s->td[i].cbp_phys = busdma;
		dma += USB_MIN_PACKET;
		busdma += USB_MIN_PACKET;
	}

	s->td[i - 1].td->td_nexttd = LE4((uPtr)NULL);

	/* should be end of the DMA memory */
	s->buf = (uByte*)(uPtr)dma;
	s->bufdma = busdma;
	DPRINTF(("usb_ohci: final: dma=%p bufdma=%#x\n", dma, busdma));


	/* reset the chip */
	ohci_dumpregs(s);
	OWRITE4(s, OHCI_COMMAND_STATUS, OHCI_HCR);
	OWRITE4(s, OHCI_CONTROL, OHCI_HCFS_RESET);

	while (OREAD4(s, OHCI_COMMAND_STATUS) & OHCI_HCR)
		u_sleep(10);

#if 0 //PERITEK_HACK
	/* try to set polarity of power on USB */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, 0x50);
	DPRINTF(("usb_ohci: PCI reg 0x50 before = %#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, 0x50, 0x070100);
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, 0x50);
	DPRINTF(("usb_ohci: PCI reg 0x50 after = %#x\n", cfg));
#endif

	/* set pointer to hcca */
	ohci_dumpregs(s);
	OWRITE4(s, OHCI_HCCA, s->hccadma);

	/* set pointers to endpoint-descriptors */
	OWRITE4(s, OHCI_CONTROL_HEAD_ED, s->ed[CONTROL_ED].physaddr);
	OWRITE4(s, OHCI_BULK_HEAD_ED, s->ed[BULK_ED].physaddr);

	/* disable all interrupts and enable control and bulk lists */
	OWRITE4(s, OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
	OWRITE4(s, OHCI_CONTROL, OHCI_CLE | OHCI_BLE | OHCI_HCFS_OPERATIONAL);

	/* set max packet size - 64 bytes or 0x200 bits plus ~10% overhead */
	OWRITE4(s, OHCI_FM_INTERVAL,
			0x02400000 | OHCI_GET_IVAL(OREAD4(s, OHCI_FM_INTERVAL)));
	OWRITE4(s, OHCI_PERIODIC_START, 0x0000);

	/* turn on power for all ports, if wired that way */
	OWRITE4(s, OHCI_RH_DESCRIPTOR_A,
			OREAD4(s, OHCI_RH_DESCRIPTOR_A) | OHCI_NPS);

	/* turn on power to all ports if they are all ganged together */
	if (!(OREAD4(s, OHCI_RH_DESCRIPTOR_A) & OHCI_PSM))
	{
		DPRINTF(("usb_ohci: powering-on all ports\n"));
		OWRITE4(s, OHCI_RH_STATUS, OHCI_LPSC);
		u_sleep(USB_POWER_SETTLE * 1000);
	}

	ohci_dumpregs(s);

	/* probe each port for devices */
	cfg = OHCI_GET_NDP(OREAD4(s, OHCI_RH_DESCRIPTOR_A));

	for (i = 1; ret == NO_ERROR && i <= cfg; i++)
		ret = ohci_probe_port(e, pkg, s, i);

	return ret;
}

Pci_driver usb_ohci_driver =
{
	{ 0, 0, 0, 0, 0, 0xC0310, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0, 0, 0, 0, 0 },
	install_usb_ohci_driver
};
