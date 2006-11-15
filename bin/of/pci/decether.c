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

/* Driver for Digital PCI Ethernet chip 21X4X* family.
   Some code cribbed and much modified from BSD if_de.c driver
   for reading the SPROM.  Original copyright follows.
 */

/*-
 * Copyright (c) 1994, 1995 Matt Thomas (matt@lkg.dec.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "defs.h"
#include "pci.h"


#ifdef DEBUG
extern void dump_packet(Environ *e, uByte *pkt, uInt len);
#endif


#define MAC_ADDR	6	/* Ethernet hardware address length */

#define DEV_21040	0x0002
#define DEV_21041	0x0014
#define DEV_21140	0x0009


/* CSR registers accessed through I/O or memory spaces from CSR base addr
   - word offsets from the beginning of mapped CSR space */
#define CSR_BUS_MODE		0
#define CSR_SEND_POLL		2
#define CSR_RECV_POLL		4
#define CSR_RECV_BASE		6
#define CSR_SEND_BASE		8
#define CSR_STATUS			10
#define CSR_OPERATION		12
#define CSR_INTERRUPT		14
#define CSR_OVERFLOW		16

/* these are for 21040 */
#define CSR_ENET_ROM		18
#define CSR_FULL_DUPLEX		22
#define CSR_GP_PORT			24
#define CSR_WATCHDOG		30

/* these are for the 21041 and 21040 */
#define CSR_ROM_CTRL		18
#define CSR_ROM_ADDR		20
#define CSR_TIMER			22
#define CSR_SIA_STATUS		24
#define CSR_SIA_CONN		26
#define CSR_SIA_SENDRECV	28
#define CSR_SIA_GENERAL		30

/* bits in CSR_ROM_CTRL for reading the serial ROM */
#define SROMSEL         0x0800
#define SROMRD          0x4000
#define SROMWR          0x2000
#define SROMDIN         0x0008
#define SROMDOUT        0x0004
#define SROMDOUTON      0x0004
#define SROMDOUTOFF     0x0004
#define SROMCLKON       0x0002
#define SROMCLKOFF      0x0002
#define SROMCSON        0x0001
#define SROMCSOFF       0x0001
#define SROMCS          0x0001

#define	SROMCMD_MODE	4
#define	SROMCMD_WR	5
#define	SROMCMD_RD	6

#define	SROM_BITWIDTH	6


#define CSR_SIZE			(sizeof (uInt) * 32)

/* useful bits in Io_desc->status */
#define IO_OWN				0x80000000u		/* "own" bit - 1=chip, 0=host */
#define IO_ERROR			0x00008100u		/* error summary for last frame */
#define IO_FIRST_DESC		0x00000200u		/* first descriptor in list */
#define IO_LAST_DESC		0x00000100u		/* last descriptor in list */
#define IO_FRAME_LEN(s)		(((s)->status >> 16) & 0x3FFFu)

/* bits in Io_desc->size */
#define IO_END_OF_RING		0x02000000u		/* end of descriptor ring */
#define IO_END_OF_CHAIN		0x01000000u		/* end of descriptor chain */
#define IO_BUF_SIZE(s)		((s) & 0x07FC)
#define IO_BUF_SHIFT		11				/* lshift to set buf2 size */


/* I/O descriptor for recv and send */
struct io_desc
{
	uInt status;
	uInt size;
	uInt buf1, buf2;	/* pointers to I/O buffers */
};
typedef struct io_desc Io_desc;

/* instance-specific vars */
struct self
{
	Pci_device_info *devinfo;
	Int physhi, physmid, physlo, sizehi, sizelo;
	Instance *obptftp;
	uByte mac_addr[MAC_ADDR];	/* Ethernet hardware address */
	volatile uInt *csr;		/* mapped-in pointer to CSR registers */
	Cell dma;				/* mapped memory for DMA access from host */
	Cell busdma;			/* allocated memory for DMA from PCI bus */
	volatile Io_desc *recv;			/* pointers into dma */
	volatile Io_desc *send;
	volatile Byte *recvbuf;			/* pointers into dma */
	volatile Byte *sendbuf;
};
typedef struct self Self;

/* chip only allows using 11 bits for size of I/O buffers */
#define MAX_PACKET_SIZE		0x7FC
#define BUF_SIZE 			(MAX_PACKET_SIZE * 2 + sizeof (Io_desc) * 2)


static Int
srom_delay(Self *s)
{
	Int i;
	Int v = 0;

	for (i = 0; i < 12; i++)
		v += LE4(s->csr[CSR_BUS_MODE]);

	return v;
}

#define SROMOUT(s, val)	(s)->csr[CSR_ROM_CTRL] = LE4(val); srom_delay(s)

static void
srom_idle(Self *s)
{
    uInt bit, csr;
    
    csr  = SROMSEL | SROMRD; SROMOUT(s, csr);
    csr ^= SROMCS; SROMOUT(s, csr);
    csr ^= SROMCLKON; SROMOUT(s, csr);

    /*
     * Write 25 cycles of 0 which will force the SROM to be idle.
     */
    for (bit = 3 + SROM_BITWIDTH + 16; bit > 0; bit--)
	{
        csr ^= SROMCLKOFF; SROMOUT(s, csr);    /* clock low; data not valid */
        csr ^= SROMCLKON; SROMOUT(s, csr);     /* clock high; data valid */
    }

    csr ^= SROMCLKOFF; SROMOUT(s, csr);
    csr ^= SROMCS; SROMOUT(s, csr); SROMOUT(s, csr);
    csr  = 0; SROMOUT(s, csr);
}

     
static void
srom_read(Self *s, uByte *rom)
{
    Int idx; 
    const uInt bitwidth = SROM_BITWIDTH;
    const uInt cmdmask = (SROMCMD_RD << bitwidth);
    const uInt msb = 1 << (bitwidth + 3 - 1);
    uInt lastidx = (1 << bitwidth) - 1;

    srom_idle(s);

    for (idx = 0; idx <= lastidx; idx++)
	{
        unsigned lastbit, data, bits, bit, csr;
        csr  = SROMSEL | SROMRD;        SROMOUT(s, csr);
        csr ^= SROMCSON;                SROMOUT(s, csr);
        csr ^=            SROMCLKON;    SROMOUT(s, csr);
    
        lastbit = 0;

        for (bits = idx|cmdmask, bit = bitwidth + 3; bit > 0; bit--, bits <<= 1)
		{
            const unsigned thisbit = bits & msb;
            csr ^= SROMCLKOFF; SROMOUT(s, csr); /* clock low; data not valid */

            if (thisbit != lastbit)
                csr ^= SROMDOUT; SROMOUT(s, csr);  /* clock low; invert data */

            csr ^= SROMCLKON; SROMOUT(s, csr);     /* clock high; data valid */
            lastbit = thisbit;
        }

        csr ^= SROMCLKOFF; SROMOUT(s, csr);

        for (data = 0, bits = 0; bits < 16; bits++)
		{
            data <<= 1;
            csr ^= SROMCLKON; SROMOUT(s, csr);     /* clock high; data valid */ 
            data |= (s->csr[CSR_ROM_CTRL] & LE4(SROMDIN)) ? 1 : 0;
            csr ^= SROMCLKOFF; SROMOUT(s, csr); /* clock low; data not valid */
        }

		rom[idx*2] = data & 0xFF;
		rom[idx*2+1] = data >> 8;
        csr  = SROMSEL | SROMRD; SROMOUT(s, csr);
        csr  = 0; SROMOUT(s, csr);
    }
}

static unsigned
srom_crc32(const uByte *databuf, Int datalen)
{
	uInt idx, bit, data, crc = 0xFFFFFFFFUL;
	#define	CRC32_POLY	0xEDB88320UL	/* CRC-32 Poly -- Little Endian */

	for (idx = 0; idx < datalen; idx++)
		for (data = *databuf++, bit = 0; bit < 8; bit++, data >>= 1)
			crc = (crc >> 1) ^ (((crc ^ data) & 1) ? CRC32_POLY : 0);

	return crc;
}

#define	srom_crc_ok(databuf)	( \
		((srom_crc32(databuf, 126) & 0xFFFF) ^ 0xFFFF) == \
				((databuf)[126] | ((databuf)[127] << 8)) )


static Retcode
read_mac_addr(Environ *e, Package *pkg, Self *s, Pci_device_info *devinfo)
{
	Int idx;
	uByte rom[128];

	memset(rom, 0, sizeof rom);	/* to catch errors earlier */

	if (devinfo->devid == DEV_21040)
	{
		s->csr[CSR_ENET_ROM] = LE4(1);

		for (idx = 0; idx < 32; idx++)
		{
			Int cnt = 0;
			Int csr;

			while (((csr = s->csr[CSR_ENET_ROM]) & LE4(0x80000000L)) &&
					cnt < 10000)
				cnt++;

			rom[idx] = csr & 0xFF;
		}
	}
	else						/* DC21041, newer chips? */
	{
		Int new_srom_fmt = 0;

		srom_read(s, rom);

#ifdef DEBUG
		{
			int i, j;

			for (i = 0; i < 128; i += 16)
			{
				dprintf("SROM[%03d..%03d] = ", i, i + 15);

				for (j = 0; j < 16; j++)
					dprintf("%02x ", rom[i + j]);

				dprintf("\n");
			}
		}
#endif

		if (srom_crc_ok(rom))
		{
			/* 
			 * SROM CRC is valid therefore it must be in the
			 * new format.
			 */
			new_srom_fmt = 1;
		}
		else if (rom[126] == 0xFF && rom[127] == 0xFF)
		{
			/* 
			 * No checksum is present.  See if the SROM id checks out;
			 * the first 18 bytes should be 0 followed by a 1 followed
			 * by the number of adapters (which we don't deal with yet).
			 */
			for (idx = 0; idx < 18; idx++)
			{
				if (rom[idx] != 0)
					break;
			}

			if (idx == 18 && rom[18] == 1 && rom[19] != 0)
				new_srom_fmt = 2;
		}

		DPRINTF(("read_mac_addr: new_srom_fmt=%d\n", new_srom_fmt));

		if (new_srom_fmt)
		{
			/* 
			 * New SROM format.  Copy out the Ethernet address.
			 * If it contains a DE500-XA string, then it must be
			 * a DE500-XA.
			 */
			memcpy(s->mac_addr, rom + 20, MAC_ADDR);
			goto done;
		}
	}

	if (memcmp(&rom[0], &rom[16], 8) != 0)
	{
		/* 
		 * Some folks don't use the standard ethernet rom format
		 * but instead just put the address in the first MAC_ADDR bytes
		 * of the rom and let the rest be all 0xffs.  (Can we say
		 * ZNYX?) (well sometimes they put in a checksum so we'll
		 * start at 8).
		 */
		for (idx = 8; idx < 32; idx++)
			if (rom[idx] != 0xFF)
				return E_PROM_READ_ERROR;

		/* 
		 * Make sure the address is not multicast or locally assigned
		 * that the OUI is not 00-00-00.
		 */
		if ((rom[0] & 3) != 0)
			return E_PROM_READ_ERROR;

		if (rom[0] == 0 && rom[1] == 0 && rom[2] == 0)
			return E_PROM_READ_ERROR;

		memcpy(s->mac_addr, rom, MAC_ADDR);
	}
	else if (devinfo->devid == DEV_21040)
	{
		for (idx = 0; idx < 32; idx++)
		{
			if (rom[idx] != 0)
				break;
		}

		if (idx != 32)
			return E_PROM_READ_ERROR;

		memcpy(s->mac_addr, rom + idx, MAC_ADDR);
	}
	else
	{
		static const uByte testpat[] =
				{ 0xFF, 0, 0x55, 0xAA, 0xFF, 0, 0x55, 0xAA };

		/* This is the standard DEC address ROM test.  */
		if (memcmp(&rom[24], testpat, 8) != 0)
			return E_PROM_READ_ERROR;

		/* compare the swapped values of the 1st 8 bytes to the 2nd 8 */
		for (idx = 0; idx < 8; idx++)
			if (rom[idx] != rom[15 - idx])
				return E_PROM_READ_ERROR;

		memcpy(s->mac_addr, rom + idx, MAC_ADDR);
	}

done:
	return set_property(pkg->props, "local-mac-address", CSTR,
			(Byte*)s->mac_addr, MAC_ADDR);
}


C(f_ether_open)			/* open (-- okay?) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg = inst->package;
	Bool promiscuous = FALSE;
	Int speed = 10;
	Bool duplex = FALSE;
	Int op = 0x0000;
	Entry *ent;
	Retcode ret;
	Pci_device_info *devinfo;
	Self *s;
	Int plen, i, cfg, maclen;
	Byte *paddr;
	uByte *macaddr;
	Cell c;

	DPRINTF(("ether_open: pkg=%p\n", pkg));
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
	   of the CSR registers, which should be mapped into both I/O and memory */
	ent = find_table(pkg->props, "assigned-addresses", CSTR);

	if (!ent)
		return E_NO_PROPERTY;
	
	paddr = ent->v.array;
	plen = ent->len;
	DPRINTF(("ether_open: paddr=%p plen=%d\n", paddr, plen));
	
	/* find a map into memory space, which we assume is our CSRs */
	do
		ret = pci_decode_reg_prop(&paddr, &plen, &s->physhi, &s->physmid,
				&s->physlo, &s->sizehi, &s->sizelo);
	while (PCI_PHYSHI_SPACE(s->physhi) != PCI_SPACE_MEM);
	
	DPRINTF(("ether_open: physhi=%#x physmid=%#x physlo=%#x\n",
			s->physhi, s->physmid, s->physlo));
    ret = pci_map_in(devinfo->hbridge, s->physhi, s->physmid, s->physlo,
			CSR_SIZE, &c);

	if (ret != NO_ERROR)
		return ret;

	s->csr = (uInt*)(uPtr)c;
	DPRINTF(("ether_open: csr=%p CSR_SIZE=%d\n", s->csr, CSR_SIZE));

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
			DPRINTF(("dec_ether: args=\"%s\"\n", str));

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
						cprintf(e, "dec_ether: unsupported \"speed=%d\"\n",
								val);
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
					cprintf(e, "dec_ether: bad arg format for \"duplex=\"");
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

	/* allocate 4 I/O packet buffers plus descriptors suitable for DMA */
	ret = pci_dma_alloc(devinfo->hbridge, BUF_SIZE, &s->dma);
	DPRINTF(("ether_open: pci_dma_alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	ret = pci_dma_map_in(devinfo->hbridge, s->dma, BUF_SIZE, 0, &s->busdma);
	DPRINTF(("ether_open: pci_dma_map_in ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	/* setup our buffers
	  - sizes are such that align requirements should work out automagically
	 */
	DPRINTF(("ether_open: dma=%#Cx busdma=%#Cx\n", s->dma, s->busdma));
	s->recvbuf = (Byte*)(uPtr)s->dma;
	memset((char*)s->recvbuf, 0, BUF_SIZE);
	s->sendbuf = s->recvbuf + MAX_PACKET_SIZE;
	s->recv = (Io_desc*)(s->recvbuf + MAX_PACKET_SIZE * 2);
	s->send = s->recv + 1;
	DPRINTF(("ether_open: recvbuf=%p sendbuf=%p recv=%p send=%p\n",
		s->recvbuf, s->sendbuf, s->recv, s->send));

	/* setup one I/O descriptor each for recv and send with 1 buffer each */
	s->recv->size = LE4(IO_END_OF_RING |
			IO_BUF_SIZE(MAX_PACKET_SIZE) | 0x02000000u);
	s->recv->buf1 = LE4(s->busdma);
	s->recv->buf2 = 0x0;
	s->send->size = 0x0;			/* filled in later */
	s->send->buf1 = LE4(s->busdma + MAX_PACKET_SIZE);
	s->send->buf2 = 0x0;
	s->recv->status = LE4(IO_OWN);	/* chip owns this buffer */
	s->send->status = 0x0;			/* host owns this buffer */

	DPRINTF(("ether_open: bus: rbuf=%#x sbuf=%#x rcv=%#Cx snd=%#Cx\n",
			LE4(s->recv->buf1), LE4(s->send->buf1),
			s->busdma + MAX_PACKET_SIZE * 2,
			s->busdma + MAX_PACKET_SIZE * 2 + sizeof (Io_desc)));

	/* enable access to memory and enable bus-mastering */
	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("ether_open: before cfg=%#x\n", cfg));
	pci_config_write(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND,
			(cfg & 0xFFFF) |
					PCI_COMMAND_MEMENABLE |
					PCI_COMMAND_MASTERENABLE
			);

	cfg = pci_config_read(devinfo->hbridge, devinfo->bus, devinfo->dev,
			devinfo->func, PCI_CONFIG_COMMAND);
	DPRINTF(("ether_open: after cfg=%#x\n", cfg));

	/* now initialize all the right magic CSR registers */
	s->csr[CSR_BUS_MODE] = LE4(0x01);	/* software reset - clears everything */
	u_sleep(10);
	DPRINTF(("ether_open: CSR_BUS_MODE=%#x\n", LE4(s->csr[CSR_BUS_MODE])));

	s->csr[CSR_BUS_MODE] = LE4(0x4000);	/* cache=8-longword */
	u_sleep(10);
	DPRINTF(("ether_open: CSR_BUS_MODE=%#x\n", LE4(s->csr[CSR_BUS_MODE])));

	s->csr[CSR_RECV_BASE] = LE4(s->busdma + MAX_PACKET_SIZE * 2);
	DPRINTF(("ether_open: CSR_RECV_BASE=%#x\n", LE4(s->csr[CSR_RECV_BASE])));

	s->csr[CSR_SEND_BASE] = LE4(s->busdma +
			MAX_PACKET_SIZE * 2 + sizeof (Io_desc));
	DPRINTF(("ether_open: initialized basic CSRs\n"));

	/* try to read in the local MAC addr out of ROM and use it
	   to intialize the parameter "local-mac-address" */
	ret = read_mac_addr(e, pkg, s, devinfo);
	DPRINTF(("ether_open: read_mac_addr ret=%d\n", ret));
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
	DPRINTF(("ether_open: $open-package ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("ether_open: obptftp=%p\n", s->obptftp));

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

	/* set CSR_OPERATION value based on duplex and promiscuous */
	if (duplex)
		op |= 0x0200;
	
	if (promiscuous)
		op |= 0x0040;
	
	/* chip-specific initializizations */
	switch (devinfo->devid)
	{
	case DEV_21041:
		DPRINTF(("dc21041_open: trying 10-BASE-T port\n"));
		/* first see if 10-BASE-T port is alive */
		s->csr[CSR_OPERATION] = LE4(op);
		s->csr[CSR_SIA_CONN] = 0x0;		/* set to zero before changes */
		s->csr[CSR_SIA_STATUS] = LE4(0x0300);	/* clear bits 8 & 9 */
		s->csr[CSR_SIA_GENERAL] = LE4(0x0008);
		s->csr[CSR_SIA_SENDRECV] = LE4(0x7F3F);
		s->csr[CSR_SIA_CONN] = LE4(0xEF01);		/* try the 10-BASE-T port */
		u_sleep(1000000);

		/* if link failed, then try AUI/BNC ports */
		if (s->csr[CSR_SIA_STATUS] & LE4(0x4))
		{
			DPRINTF(("dc21041_open: trying AUI port - CSR_SIA_STATUS=%#x\n",
					LE4(s->csr[CSR_SIA_STATUS])));
			s->csr[CSR_SIA_CONN] = 0x0;		/* set to zero */
			s->csr[CSR_SIA_STATUS] = LE4(0x0300);	/* clear bits 8 & 9 */
			s->csr[CSR_SIA_GENERAL] = LE4(0x000E);	/* try AUI */
			s->csr[CSR_SIA_SENDRECV] = LE4(0xF7FD);
			s->csr[CSR_SIA_CONN] = LE4(0xEF09);		/* try the AUI/BNC port */
			u_sleep(1000000);

			/* no activity on this port, or activity on other port, try BNC */
			if (!(s->csr[CSR_SIA_STATUS] & LE4(0x0100)) ||
					(s->csr[CSR_SIA_STATUS] & LE4(0x0200)))
			{
				DPRINTF(("dc21041_open: trying BNC - CSR_SIA_STATUS=%#x\n",
						LE4(s->csr[CSR_SIA_STATUS])));
				s->csr[CSR_SIA_CONN] = 0x0;		/* set to zero */
				s->csr[CSR_SIA_STATUS] = LE4(0x0300);	/* clear bits 8 & 9 */
				s->csr[CSR_SIA_GENERAL] = LE4(0x0006);	/* try BNC */
				s->csr[CSR_SIA_SENDRECV] = LE4(0xF7FD);
				s->csr[CSR_SIA_CONN] = LE4(0xEF09);	/* try the AUI/BNC port */
				u_sleep(1000000);
			}
		}

		DPRINTF(("ether_open: dc21041: CSR_SIA_STATUS=%#x\n",
				LE4(s->csr[CSR_SIA_STATUS])));
		
		break;
		
	case DEV_21040:
		s->csr[CSR_OPERATION] = LE4(op);		/* necessary? */
		s->csr[CSR_SIA_CONN] = 0;
		s->csr[CSR_SIA_CONN] = LE4(0xEF01);		/* 10-BASE-T (and only) port */
		u_sleep(1000000);
		/* TODO - more to do here? */
		break;
	
	case DEV_21140:
		if (speed == 100)
			op |= 0x040000;		/* 100 or 10 Mbps */
		
		op |= 0x02000000;		/* must be one */
		s->csr[CSR_OPERATION] = LE4(op);
		break;
	}

	/* initialize the buffer with 16 "perfect" filters - exactly 192 bytes
	   - this method does not care what endian things are
	 */
	DPRINTF(("ether_open: initializing sendbuf\n"));

	for (i = 0; i < 16; i++)
	{
		s->sendbuf[i*12 +  0] = macaddr[0];
		s->sendbuf[i*12 +  1] = macaddr[1];
		s->sendbuf[i*12 +  2] = macaddr[0];
		s->sendbuf[i*12 +  3] = macaddr[1];
		s->sendbuf[i*12 +  4] = macaddr[2];
		s->sendbuf[i*12 +  5] = macaddr[3];
		s->sendbuf[i*12 +  6] = macaddr[2];
		s->sendbuf[i*12 +  7] = macaddr[3];
		s->sendbuf[i*12 +  8] = macaddr[4];
		s->sendbuf[i*12 +  9] = macaddr[5];
		s->sendbuf[i*12 + 10] = macaddr[4];
		s->sendbuf[i*12 + 11] = macaddr[5];
	}

	/* set one of the entries to the hardware boardcast address */
	memset((char*)s->sendbuf + 12, 0xFF, 12);

	/* setup one I/O descriptor each for recv and send with 1 buffer each */
	s->send->size = LE4(IO_END_OF_RING | 0x6A000000u | IO_BUF_SIZE(192));
	s->send->status = LE4(IO_OWN);		/* chip now owns this buffer */
	DPRINTF(("ether_open: pci_dma_sync\n"));
	pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);

	/* send the setup frame to filter only packets addressed to us */
	DPRINTF(("ether_open: clearing CSR_STATUS\n"));
	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	DPRINTF(("ether_open: starting send/receive operations\n"));
	s->csr[CSR_OPERATION] = LE4(0x2002);	/* start send/receive operations */
	DPRINTF(("ether_open: polling to initialize filters\n"));
	s->csr[CSR_SEND_POLL] = LE4(1);			/* send this "packet" */

	/* loop until this packet is actually "sent" */
	while ((s->send->status & LE4(IO_OWN)) &&
			!(s->send->status & LE4(IO_ERROR)) &&
			!(s->csr[CSR_STATUS] & LE4(0x3000)))	/* no error? */
	{
		DPRINTF(("ether_open: setup: CSR_STATUS=%#x STATUS=%#x\n",
				LE4(s->csr[CSR_STATUS]), LE4(s->send->status)));
		pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
	}

	DPRINTF(("ether_open: done: CSR_STATUS=%#x ret=%d\n",
			LE4(s->csr[CSR_STATUS]), ret));
	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	PUSH(e, FTRUE);
	return ret;
}

C(f_ether_close)			/* close (--) */
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

	s->csr[CSR_OPERATION] = 0x0;		/* stop all operations */
	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	s->csr[CSR_BUS_MODE] = LE4(0x01);	/* software reset - clears everything */
	u_sleep(100000);

	ret = pci_dma_map_out(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);

	if (ret == NO_ERROR)
		ret = pci_dma_free(devinfo->hbridge, s->dma, BUF_SIZE);

	if (ret == NO_ERROR)
		ret = pci_map_out(devinfo->hbridge, (uPtr)s->csr, CSR_SIZE);

	IFCKSP(e, 0, 1);
	PUSH(e, (uPtr)s->obptftp);
	ret = execute_word(e, "close-package");

	return ret;
}

C(f_ether_dma_alloc)		/* dma-alloc (dma-len -- dma-buf) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ether_dma_alloc\n"));
	return execute_method_name(e, inst->parent, "dma-alloc", CSTR);
}

C(f_ether_dma_free)			/* dma-free (dma-buf dma-len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	DPRINTF(("ether_dma_free\n"));
	return execute_method_name(e, inst->parent, "dma-free", CSTR);
}


C(f_ether_read)				/* read (addr len -- actual) */
{
	Int len, actual = -2;
	Byte *addr;
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

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
	DPRINTF(("ether_read: addr=%p len=%d\n", addr, len));
	DPRINTF(("ether_read: recv->status=%#x CSR_STATUS=%#x\n",
			LE4(s->recv->status), LE4(s->csr[CSR_STATUS])));

	if (s->recv->status & LE4(IO_OWN))	/* chip still owns this buffer? */
	{
		PUSH(e, -2);			/* no data yet */
		return NO_ERROR;
	}

	if (s->recv->status & LE4(0x8000))	/* error occurred? */
		actual = -1;
	else
	{
		/* return actual size received and copy as much of the data as we can */
		actual = (LE4(s->recv->status) >> 16) & 0x3FFF;
		memcpy(addr, (char*)s->recvbuf, (actual > len) ? len : actual);
	}

	s->recv->size = LE4(IO_END_OF_RING |
			IO_BUF_SIZE(MAX_PACKET_SIZE) | 0x02000000u);
	s->recv->status = LE4(IO_OWN);		/* clear errors, chip owns buffer */
	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	s->csr[CSR_RECV_POLL] = LE4(1);			/* start receive */

	DPRINTF(("ether_read: actual=%d\n", actual));
	PUSH(e, actual);
	return NO_ERROR;
}

C(f_ether_write)			/* write (addr len -- actual) */
{
	Int len, actual = 0;
	Byte *addr;
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
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("ether_write: addr=%p len=%d\n", addr, len));

	pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
	DPRINTF(("ether_write: STATUS=%#x", LE4(s->send->status)));

	while (s->send->status & LE4(IO_OWN))	/* chip still owns this buffer? */
	{
		pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
		DPRINTF(("ether_write: s->send->status=%#x CSR_STATUS=%#x\n",
				LE4(s->send->status), LE4(s->csr[CSR_STATUS])));
	}

	if (len >= MAX_PACKET_SIZE)		/* truncate size - wrong, but safe */
		len = MAX_PACKET_SIZE;

	DPRINTF(("\nether_write: sendbuf=%#x len=%d\n",
			LE4((uPtr)s->sendbuf), len));

	memcpy((char*)s->sendbuf, addr, len);

	if (len & 0x03)		/* LSB 2 bits must be zero, so send 1-3 extra bytes */
		len = (len + 0x04) & ~0x03;

	s->send->size = LE4(IO_END_OF_RING | IO_BUF_SIZE(len) | 0x62000000u);
	s->send->status = LE4(IO_OWN);		/* clear errors, chip owns buffer */
	pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
	DPRINTF(("ether_write: about to transmit\n"));

	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	DPRINTF(("ether_write: about to transmit 2\n"));

	s->csr[CSR_SEND_POLL] = LE4(1);		/* send this packet */
	DPRINTF(("ether_write: about to transmit 3\n"));

	/* loop until this packet is actually sent */
	DPRINTF(("ether_write: transmitting %d", len));
	while ((s->send->status & LE4(IO_OWN)) &&
			!(s->csr[CSR_STATUS] & LE4(0x3000)))
	{
		pci_dma_sync(devinfo->hbridge, (uPtr)s->dma, s->busdma, BUF_SIZE);
		DPRINTF(("."));
	}

	DPRINTF(("\nether_write: CSR_STATUS=%#x send->status=%#x\n",
		LE4(s->csr[CSR_STATUS]), LE4(s->send->status)));

	/* no error occurred? */
	if (!(s->send->status & LE4(IO_OWN)) &&
			!(s->send->status & LE4(IO_ERROR)) &&
			!(s->csr[CSR_STATUS] & LE4(0x3000)))
		actual = len;

	s->csr[CSR_STATUS] = 0xFFFFFFFF;	/* clear all bits */
	DPRINTF(("ether_write: actual=%d\n", actual));

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

C(f_ether_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_ether_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

C(f_ether_selftest)			/* selftest (-- 0|error-code) */
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


static const Initentry ether_methods[] =
{
	{ "open",      f_ether_open,          INVALID_FCODE },
	{ "close",     f_ether_close,         INVALID_FCODE },
	{ "dma-alloc", f_ether_dma_alloc,     INVALID_FCODE },
	{ "dma-free",  f_ether_dma_free,      INVALID_FCODE },
	{ "read",      f_ether_read,          INVALID_FCODE },
	{ "write",     f_ether_write,         INVALID_FCODE },
	{ "load",      f_ether_load,          INVALID_FCODE },
	{ "ping",      f_ether_ping,          INVALID_FCODE },
	{ "selftest",  f_ether_selftest,      INVALID_FCODE },
	{ NULL,        NULL },
};

PCI(install_dec_ether_driver)
{
	Retcode ret;
	Pci_device_info *newinfo;

	DPRINTF(("install_dec_ether_driver: pkg=%p devinfo=%p"
			" vendid=%#x devid=%#x\n",
			pkg, devinfo, devinfo->vendid, devinfo->devid));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	/* setup the basic PCI properties for this device */
	pci_load_reg_and_name_props(e, pkg, devinfo);

	/* set the type of this device */
	ret = prop_set_str(pkg->props, "device_type", CSTR, "network", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, ether_methods);

	/* allocate space and stash a copy of our devinfo for our methods */
	newinfo = (Pci_device_info*)malloc(sizeof *newinfo);

	if (newinfo == NULL)
		return E_OUT_OF_MEMORY;

	*newinfo = *devinfo;					/* copy it */
	pkg->self = (struct pself*)newinfo;		/* and save it */

	return ret;
}

Pci_driver digital_21x4x_driver =
{
	{ 0, 0, 0, 0, 0, 0x20000, 0x1011, 0x0000, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFF00, 0, 0, 0 },
	install_dec_ether_driver
};
