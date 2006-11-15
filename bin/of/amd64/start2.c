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

/* (c) Copyright 2002-2003 by CodeGen, Inc.  All Rights Reserved. */

/* 32-bit x86 C startup code for Hammer - started by ROM bootstrap.
 * Must be .org STARTADDR (0xFFFF.0000) from start1.as86.
 * This is the highest 64Kb of ROM that should always be mapped.
 * Data/BSS are set to 0x1000, which is not initalized.  All globals
 * must be unitialized so they end up in BSS (and inited by code).
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/*#define DEBUG*/

#ifdef DEBUG
#define DPRINTF(x)		dprintf x
#else
#define DPRINTF(x)
#endif

/* XXX */
#ifdef ROM_BUILD
    #define AMD8111_LPC_ID	5
    #define AMD8111_PCI_ID	4
#else
    #define AMD8111_LPC_ID	7
    #define AMD8111_PCI_ID	6
#endif


typedef int Retcode;
typedef unsigned long long uLong;	/* 64-bits */
typedef long long Long;	/* 64-bits */
typedef unsigned long uInt;		/* 32-bits */
typedef char Byte;
typedef short Short;
typedef int Int;
typedef unsigned char uByte;
typedef unsigned short uShort;


#define TRUE  1
#define FALSE 0
#define STR_SIZE 512


/* macros to access a 16550-compatible serial port for failsafe I/O */
#define COM1_PORT	0x3F8
#define COM2_PORT	0x2F8
#define COM3_PORT	0x3E8
#define COM4_PORT	0x2E8

#define SIO_THR	0x0		/* transmit holding register - write-only */
#define SIO_RBR	0x0		/* receiver buffer register - read-only */
#define SIO_DLL	0x0		/* divisor latch LSB - r/w - DLAB == 1 */
#define SIO_DLM	0x1		/* divisor latch MSB - r/w 0 DLAB == 1 */
#define SIO_IER	0x1		/* interrupt enable register - w/o */
#define SIO_IIR	0x2		/* interrupt identification reg - r/o */
#define SIO_FCR	0x2		/* FIFO control register - w/o */
#define SIO_LCR	0x3		/* line control register - r/w */
#define SIO_MCR	0x4		/* modem control register - r/w */
#define SIO_LSR	0x5		/* line status register - r/w */
#define SIO_MSR	0x6		/* modem status register - r/w */
#define SIO_SCR	0x7		/* scratch pad register - r/w */

#define DEBUG_PORT	COM1_PORT

#define tostr2(num)	#num
#define tostr(num)	tostr2(num)


/* MUST be first routine in file for entrypoint from ROM */
void
_start(void)
{
    extern void setup(void);

    asm("mov	$" tostr(STACKADDR) "-512,%esp");
    asm("jmp	setup");

    for (;;)
	;
}


/* to access the ISA bus through the PCI bus hopefully at default addresses */
static unsigned char
isa_read(unsigned int addr)
{
    unsigned char val;

    __asm __volatile("inb %%dx,%0" : "=a" (val) : "d" (addr));
    return val;
}

static void
isa_write(unsigned int addr, unsigned int val)
{
    __asm __volatile("outb %0,%%dx" : : "a" ((unsigned char)val), "d" (addr));
}


#ifdef DEBUG
static int
failsafe_write(char *buf, int len)
{
    int actual = 0;

    while (len > 0)
    {
	if (isa_read(DEBUG_PORT + SIO_LSR) & 0x20) /* THR ready for a char? */
	{
	    isa_write(DEBUG_PORT + SIO_THR, *buf++);
	    actual++;
	    len--;
	}
    }

    return actual;
}
#endif

#ifdef ROM_BUILD
static void *
memcpy(void *dst, const void *src, size_t len)
{
    char *d = (char *)dst;
    char *s = (char *)src;

    if (d < s)			/* assume src overlaps end of dst */
    {
	while (len-- > 0)
	    *d++ = *s++;
    }
    else
    {
	s += len;
	d += len;

	while (len-- > 0)
	    *--d = *--s;
    }

    return dst;
}
#endif /* ROM_BUILD */

static size_t
strlen(const char *str)
{
	size_t len = 0;

	if (str)
		while (*str++)
			len++;

	return len;
}

#ifdef DEBUG
static int
vbprintf(char *buf, const char *fmt, va_list args)
{
	char *sarg, *s;
	uLong iarg;
	int field, hex;
	Int len;
	int rpad, lzero, sign, larg, alt;
	char nbuf[STR_SIZE];
	
	if (fmt == NULL || buf == NULL)
		return 0;
		
	while (*fmt)
	{
		if (*fmt != '%')
		{
			*buf++ = *fmt++;
			continue;
		}

		if (*++fmt == '%')
		{
			*buf++ = *fmt++;
			continue;
		}
		
		field = 0;
		rpad = FALSE;
		lzero = FALSE;
		sign = FALSE;
		hex = 0;
		larg = FALSE;
		alt = FALSE;
	
		if (*fmt == '#')
		{
			fmt++;
			alt = TRUE;
		}
	
		if (*fmt == '-')
		{
			fmt++;
			rpad = TRUE;
		}

		if (*fmt == '0')
		{
			fmt++;
			lzero = TRUE;
		}

		if (*fmt == '*')
		{
			field = va_arg(args, Int);
			fmt++;
		}
		else
			for (; isdigit(*fmt); fmt++)
				field = field * 10 + *fmt - '0';
		
		/* following arg is long instead of int */
		if (*fmt == 'l' || *fmt == 'L' || *fmt == 'q')
		{
			larg = TRUE;
			fmt++;

			/* skip "%ll*" style formats */
			if (*fmt == 'l')
				fmt++;
		}

		/* used to be switch (*fmt++) but some gcc revs generate bad code */
		iarg = *fmt++;

		if (iarg == 'c')			/* character */
		{
			iarg = va_arg(args, Int);
			*buf++ = iarg;
		}
		else if (iarg == 'x' || iarg == 'X' || iarg == 'd' || iarg == 'u' ||
				iarg == 'p')
		{
			if (iarg == 'x')
				hex = 'a';
			else if (iarg == 'X')
				hex = 'A';
			else if (iarg == 'p')
			{
				hex = 'a';
				alt = TRUE;
			}

			if (larg)
				iarg = va_arg(args, uLong);
			else if (hex || iarg == 'u')
			{
				iarg = va_arg(args, uInt);

				/* work around for gcc ARM problem */
				iarg &= 0xFFFFFFFF;
			}
			else
				iarg = va_arg(args, Int);

			s = nbuf + STR_SIZE - 1;
			*s = '\0';
			
			if (!hex && (Long)iarg < 0)
			{
				sign = TRUE;
				iarg = -(Long)iarg;
				field--;
			}
			else if (iarg == 0)
			{
				*--s = '0';
				field--;
			}
			
			for (; iarg; field--)
			{
				int d = iarg & 0xF;
				*--s = (d < 10) ? d + '0' : d - 10 + hex;
				iarg >>= 4;
			}
			
			while (field-- > 0)
				*--s = (lzero) ? '0' : ' ';
			
			if (hex && alt)
			{
				*--s = 'x';
				*--s = '0';
			}

			if (sign)
				*--s = '-';
			
			while (*s)
				*buf++ = *s++;
		}
		else if (iarg == 's' || iarg == 'S' || iarg == 'P')		/* string */
		{
			sarg = va_arg(args, char*);

			if (iarg == 'S')			/* Forth-style string */
				len = va_arg(args, Int);
			else if (sarg == NULL)		/* no string */
				len = 0;
			else if (iarg == 'P')		/* Pascal-style string */
				len = *(uByte*)sarg++;
			else						/* C string */
				len = strlen(sarg);
			
			if (!rpad)
			{
				while (field - len > 0)
				{
					*buf++ = ' ';
					field--;
				}
			}

			for (; len-- > 0; field--)
				*buf++ = *sarg++;
			
			if (rpad)
				while (field-- > 0)
					*buf++ = ' ';
		}
	}
	
	*buf = '\0';
	return strlen(buf);
}

static void
dprintf(const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    char buf2[1024];
    char *s = buf;
    char *s2 = buf2;
//    extern int vbprintf(char *, const char *, va_list);

    va_start(args, fmt);
    vbprintf(buf, fmt, args);
    va_end(args);

    /* convert all newlines to carriage-return-newline pairs */
    while (*s)
    {
	if (*s == '\n')
	    *s2++ = '\r';

	*s2++ = *s++;
    }

    *s2 = '\0';

    failsafe_write(buf2, strlen(buf2));
}
#endif /* DEBUG */


#define PT_P		0x0001
#define PT_RW		0x0002
#define PT_US		0x0004
#define PT_PWT		0x0008
#define PT_PCD		0x0010
#define PT_A		0x0020
#define PT_D		0x0040
#define PT_PS		0x0080
#define PT_G		0x0100
#define PT_PAT		0x1000

/* build 24KB worth of page tables, that map VA 0000.0000.000 to 0000.FFFF.FFFF
 * with a one to one mapping to PAs.  All other VA's fault.
 */ 
uLong *
buildpagetables(uLong *pagetable)
{
    uLong *l4 = (uLong *)((uInt)pagetable & ~0xFFF); /* align to 4K */
    uLong *l3 = &l4[512];
    uLong *l2 = &l3[512];
    int i;
    uLong a = 0;

    /* allow caching DRAM area 0..0x2000.0000 / 0x200000 == 256 */
    for (i = 0; i < 256; i++, a += 2*1024*1024)
	l2[i] = a | (PT_PS|PT_US|PT_RW|PT_P);

    for (; i < 4*512; i++, a += 2*1024*1024)
	l2[i] = a | (PT_PS|PT_PCD|PT_US|PT_RW|PT_P);

    l3[0] = (uLong)(uInt)&l2[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[1] = (uLong)(uInt)&l2[1*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[2] = (uLong)(uInt)&l2[2*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[3] = (uLong)(uInt)&l2[3*512] | (PT_PCD|PT_US|PT_RW|PT_P);

    l4[0] = (uLong)(uInt)&l3[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l4[1] = (PT_PCD|PT_US|PT_RW);
    l4[2] = (PT_PCD|PT_US|PT_RW);
    l4[3] = (PT_PCD|PT_US|PT_RW);

    for (i = 4; i < 512; i++)
    {
	l3[i] = (PT_PCD|PT_US|PT_RW);
	l4[i] = (PT_PCD|PT_US|PT_RW);
    }

    /* aliases for kernel VM addrs ffff.ffff.8000.0000 to 0x0 physical */
    l3[510] = (uLong)(uInt)&l2[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l3[511] = (uLong)(uInt)&l2[1*512] | (PT_PCD|PT_US|PT_RW|PT_P);
    l4[511] = (uLong)(uInt)&l3[0*512] | (PT_PCD|PT_US|PT_RW|PT_P);

    return l4;
    return &l2[4*512];
}


#define PCI_PHYSHI_MK(n, p, t, sp, bus, dev, func, reg)  \
		((uInt)(((uInt)n << 31) | (p << 30) | (t << 29) | (sp << 24) | \
		(bus << 16) | (dev << 11) | (func << 8) | reg))

#define PCI_CONFIG_ADDR		0xCF8ul
#define PCI_CONFIG_DATA		0xCFCul
#define LE4(x)			x

#define E_INVALID_POINTER -1
#define E_BAD_ARGUMENT -1
#define NO_ERROR 0


static Retcode
pci_io_write(Int hbridge, uInt addr, Int value, int size)
{
	uByte b;
	uShort s;
	uInt i;

	if (hbridge)
		return E_INVALID_POINTER;

	switch (size)
	{
	case 1:
		b = (uByte)value;
		__asm __volatile("outb %0,%%dx" : : "a" (b), "d" (addr));
		break;

	case 2:
		s = (uShort)value;
		__asm __volatile("outw %0,%%dx" : : "a" (s), "d" (addr));
		break;

	case 4:
		i = (uInt)value;
		__asm __volatile("outl %0,%%dx" : : "a" (i), "d" (addr));
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

static void
pci_config_write(Int hbridge, Int bus, Int dev, Int func, Int addr, Int value)
{
	uInt a;
	
	if (hbridge)
		return;

	addr &= ~3;
	a = PCI_PHYSHI_MK(1, 0, 0, 0, bus, dev, func, addr);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, LE4(a), sizeof a);
	pci_io_write(hbridge, PCI_CONFIG_DATA, LE4(value), sizeof value);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, 0, 4);
	/*DPRINTF((
		"pci_config_write: hbridge=%d bus=%d dev=%d func=%d addr=%#x val=%#x\n",
			hbridge, bus, dev, func, addr, LE4(value)));*/
}


#ifdef ROM_BUILD
static Retcode
pci_io_read(Int hbridge, uInt addr, void *value, int size)
{
	uByte b;
	uShort s;
	uInt i;

	if (hbridge)
		return E_INVALID_POINTER;

	switch (size)
	{
	case 1:
		__asm __volatile("inb %%dx,%0" : "=a" (b) : "d" (addr));
		*(uByte*)value = b;
		break;

	case 2:
		__asm __volatile("inw %%dx,%0" : "=a" (s) : "d" (addr));
		*(uShort*)value = s;
		break;

	case 4:
		__asm __volatile("inl %%dx,%0" : "=a" (i) : "d" (addr));
		*(uInt*)value = i;
		break;

	default:
		return E_BAD_ARGUMENT;
	}

	return NO_ERROR;
}

static Int
pci_config_read(Int hbridge, Int bus, Int dev, Int func, Int addr)
{
	uInt a;
	
	if (hbridge)
		return 0;
		
	addr &= ~3;
	a = PCI_PHYSHI_MK(1, 0, 0, 0, bus, dev, func, addr);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, LE4(a), sizeof a);
	pci_io_read(hbridge, PCI_CONFIG_DATA, (void *)&a, sizeof a);
	pci_io_write(hbridge, PCI_CONFIG_ADDR, 0, 4);
	/*DPRINTF((
		"pci_config_read: hbridge=%d bus=%d dev=%d func=%d addr=%#x val=%#x\n",
			hbridge, bus, dev, func, addr, LE4(a)));*/
	return LE4(a);
}
#endif /* ROM_BUILD */


void
setup(void)
{
#ifndef ROM_BUILD
    /* set the serial port to a sane state */
    isa_write(DEBUG_PORT + SIO_LCR, 0x80);  /* DLAB on */
    isa_write(DEBUG_PORT + SIO_DLL, 12);    /* clock value for 9600 baud */
    isa_write(DEBUG_PORT + SIO_DLM, 0);
    isa_write(DEBUG_PORT + SIO_LCR, 0x03);  /* DLAB off, 8/1/N */
    isa_write(DEBUG_PORT + SIO_MCR, 0x01);  /* drop all lines, raise DTR */
    isa_write(DEBUG_PORT + SIO_IER, 0x00);  /* disable all interrupts */
    isa_write(DEBUG_PORT + SIO_FCR, 0x07);  /* turn on and clear I/O FIFOs */
#endif

    DPRINTF(("Hello, 32-bit C world!\n"));

#ifdef ROM_BUILD
    /* config HT a bit */
    {
	uInt csr = pci_config_read(0, 0, 0, 0, 0x04);
	int capoff;
	uInt capreg;
	uInt unitid = 1;
	int count;

	while (1)
	{
	    DPRINTF(("0,0,04 = 0x%X\n", csr));

	    /* turn on the first uninitialized device */
	    pci_config_write(0, 0, 0, 0, 0x4, csr);

	    /* find the HT capability */
	    capreg = pci_config_read(0, 0, 0, 0, 0x34) & 0xFF;
	    capoff = capreg & 0xFF;
	    DPRINTF(("0,0,34 = 0x%X, capoff = 0x%X\n", capreg, capoff));

	    while (capoff != 0 && capoff != 0xFF)
	    {
		capreg = pci_config_read(0, 0, 0, 0, capoff);

		if ((capreg & 0xFF) == 8)
		    break;

		capoff = (capreg >> 8) & 0xFF;
	    }

	    if (capoff == 0 || capoff == 0xFF)
		break;

	    count = (capreg >> 21) & 0x1F;
	    DPRINTF(("UnitID count = 0x%X\n", count));
	    pci_config_write(0, 0, 0, 0, capoff, capreg | (unitid << 16));
	    DPRINTF(("ID 0x%X\n", pci_config_read(0, 0, unitid, 0, 0x0)));
	    unitid += count;
	}

	DPRINTF(("HT config done\n"));
    }

#if 0
    /* sift through config space to see what we have */
    {
	int i;

	for (i = 0; i < 32; i++)
	    DPRINTF(("%d,0,00 = 0x%X\n", i, pci_config_read(0, 0, i, 0, 0x00)));
    }
#endif

    /* enable 1MB+ of flash space (should already be done by start1.asm86) */

    /* turn on the extended BIOS flash space */
    pci_config_write(0, 0, AMD8111_LPC_ID, 0, 0x40, 0x80072001);

#if 0
    /* sift thourgh flash space looking for our code*/
    {
	unsigned char *flash = (unsigned char *)0xFFC00000; 
	int i;

	DPRINTF(("7,0,40 = 0x%X\n", pci_config_read(0, 0, 7, 0, 0x40)));

	for (i = 0; i < 4*1024*1024; i += 64*1024)
	{
	    DPRINTF(("0x%08X: ", i));
	    DPRINF((" 0x%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\n", 
		    flash[0], flash[1], flash[2], flash[3],
		    flash[4], flash[5], flash[6], flash[7]));

	    flash += 64*1024;
	}
    }
#endif

    /* copy the flash image to DRAM */
    {
	long *from = (long*)memcpy;
	long *to = (long*)LOADADDR - 0x100;
	void (*func)() = (void (*)())to;

	/* first copy memcpy() to RAM */
	memcpy(to, from, (char*)strlen - (char*)memcpy);

	from = (long*)0xFFF00000;
	to = (long*)LOADADDR;
	DPRINTF(("To:   %#x:\n", to));
	DPRINTF(("	0x%02x.%02x.%02x.%02x\n", to[0], to[1], to[2], to[3]));
	DPRINTF(("	0x%02x.%02x.%02x.%02x\n", to[4], to[5], to[6], to[7]));
	DPRINTF(("From: %#x:\n", from));
	DPRINTF(("	0x%02x.%02x.%02x.%02x\n",
		from[0], from[1], from[2], from[3]));
	DPRINTF(("	0x%02x.%02x.%02x.%02x\n",
		from[4], from[5], from[6], from[7]));

	/* copy 1MB - 64KB from ROM to RAM */
	DPRINTF(("copying ROM to RAM..."));
	(*func)(to, from, 0x100000 - 0x10000);
	DPRINTF(("\n"));
    }
#endif /* ROM_BUILD */

    /* SYSCFG - lock bits, turn off fixed MTRRs to access lower 1Mb RAM */
    DPRINTF(("turning off fixed MTRRs to access lower 1Mb RAM..."));
    asm("mov	$0xC0010010, %ecx");
    asm("mov	$0x120601, %eax");
    asm("mov	$0, %edx");
    asm("wrmsr");

    /* PCI intr/ctrl rev - ISAEN SERREN */
    pci_config_write(0, 0, AMD8111_PCI_ID, 0, 0x3C, 0x040600FF);

    /* turn off broken devices, enable IOAPIC */
    pci_config_write(0, 0, AMD8111_LPC_ID, 0, 0x48, 0x0100FD2F);

    /* setup page tables with a VA->PA mapping for low memory
     * with the SmartFirmware code space mapped into its final
     * location in the 64-bit virtual space 
     * - build the page tables we'll need, say, at 64k in low RAM
     */
    DPRINTF(("build page tables\n"));
    (void)buildpagetables((uLong *)STACKADDR);

#ifdef DEBUG
    {
	long *pgtab = (long*)STACKADDR;
	dprintf("page-tables: %#x:\n", pgtab);
	dprintf("	0x%08x.%08x.%08x.%08x\n",
		pgtab[0], pgtab[1], pgtab[2], pgtab[3]);
	dprintf("	0x%08x.%08x.%08x.%08x\n",
		pgtab[4], pgtab[5], pgtab[6], pgtab[7]);
    }
#endif

#ifdef ROM_BUILD
    DPRINTF(("jump to 64-bit startup code at %#x\n", LOADADDR));
#if 0
    asm("	clc;	1:;	jnc	1b;");
#endif
    asm("	wbinvd");
    asm("	jmp	" tostr(LOADADDR));

#else
    
    asm("	jmp	start64");

#endif /* ROM_BUILD */

    /* (these are done in start.S) */
    /* enable paging and switch to long mode */
    /* Branch to the 64-bit code in DRAM */
}
