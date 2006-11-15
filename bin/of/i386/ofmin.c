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

/* (c) Copyright 1996-2000 by CodeGen, Inc.  All Rights Reserved. */

/* machine-dependancies here - needs to be customized for a particular system */

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <zlib.h>

typedef unsigned char uByte;
typedef int Int;
typedef long Long;

#define DPRINTF(args) dprintf args
//#define DPRINTF(args)

extern int init_malloc(void *memory, uInt max);


/* These define where memory starts and how much there is of it.  This is
   used to initialize the /memory node in memory.c, so that "claim" and
   "release" will work, and is also used to initialize the memory pool
   available for use by malloc().  Any additional devices of type "memory"
   may be placed beneath the appropriate bus.  #size-cells is assumed to
   be 1 for /memory.
 */
Byte *g_machine_memory = NULL;
uInt g_machine_memory_size = 0;
uInt g_machine_memory_used = 0;

extern Int failsafe_write(Byte *buf, Int len);


/* macros for PromICE AI2 virtual serial port */
#define	VCOM		0xFFFF8000		/* wherever it is placed in ROM-space */
#define VBURST		32				/* burst value for ROM access */
#define VZERO		(VCOM)				/* reg zero */
#define VONE		(VCOM + VBURST)		/* reg one */
#define VHOSTDATA	(VCOM + 2*VBURST)	/* reg host_data */
#define VSTATUS		(VCOM + 3*VBURST)	/* reg status */
	#define V_FLG	0x08		/* always zero */
	#define V_OVR	0x04		/* host data overflow */
	#define V_HDA	0x02		/* valid host data in buffer */
	#define V_TDA	0x01		/* data transmitter busy */
#define VREG(reg)	(*(volatile unsigned char *)reg)

#undef VIRTUAL_SERIAL_PORT		/* turn on/off PromICE AI2 serial port */

/* macros to access a 16550-compatible serial port for failsafe I/O */
#define COM1_PORT	0x3F8
#define COM2_PORT	0x2F8
#define COM3_PORT	0x3E8
#define COM4_PORT	0x2E8
#define SIO_THR		0x0		/* transmit holding register - write-only */
#define SIO_RBR		0x0		/* receiver buffer register - read-only */
#define SIO_DLL		0x0		/* divisor latch LSB - r/w - DLAB == 1 */
#define SIO_DLM		0x1		/* divisor latch MSB - r/w 0 DLAB == 1 */
#define SIO_IER		0x1		/* interrupt enable register - w/o */
#define SIO_IIR		0x2		/* interrupt identification reg - r/o */
#define SIO_FCR		0x2		/* FIFO control register - w/o */
#define SIO_LCR		0x3		/* line control register - r/w */
#define SIO_MCR		0x4		/* modem control register - r/w */
#define SIO_LSR		0x5		/* line status register - r/w */
#define SIO_MSR		0x6		/* modem status register - r/w */
#define SIO_SCR		0x7		/* scratch pad register - r/w */

#ifndef DEBUG_PORT
#define DEBUG_PORT	COM1_PORT
#endif /* DEBUG_PORT */


/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
	while (us--)
	{
		int i = 100;	// TODO - should adjust itself for CPU speed
		
		while (i--)
			;
	}
}

/* to access the ISA bus through the PCI bus hopefully at default addresses */
uByte
isa_read(uInt addr)
{
	uByte val;

	__asm __volatile("inb %%dx,%0" : "=a" (val) : "d" (addr));
	return val;
}

void
isa_write(uInt addr, uByte val)
{
	uByte v;

	v = val;
	__asm __volatile("outb %0,%%dx" : : "a" (v), "d" (addr));
}

uInt
in4(uInt addr)
{
	uInt val;

	__asm __volatile("inl %%dx,%0" : "=a" (val) : "d" (addr));
	return val;
}

void
out4(uInt addr, uInt val)
{
	uInt v;

	v = val;
	__asm __volatile("outl %0,%%dx" : : "a" (v), "d" (addr));
}

uInt
pci_config_r(int bus, uInt dev, uInt func, uInt reg)
{
	uInt val;
	uInt a = 0x80000000 | (bus << 16 ) | (dev << 11) | (func << 8) | reg;

	out4(0xCF8, a);
	val = in4(0xCFC);
	out4(0xCF8, 0);
	return val;
}

void
pci_config_w(int bus, uInt dev, uInt func, uInt reg, uInt val)
{
	uInt a = 0x80000000 | (bus << 16 ) | (dev << 11) | (func << 8) | reg;

	out4(0xCF8, a);
	out4(0xCFC, val);
	out4(0xCF8, 0);
}

#define config_r(dev, func, reg)		pci_config_r(0, dev, func, reg)
#define config_w(dev, func, reg, val)	pci_config_w(0, dev, func, reg, val)

int
vbprintf(char *buf, const char *fmt, va_list args)
{
	char *sarg, *s;
	uLong iarg;
	int field, hex;
	Int len;
	int rpad, lzero, sign, larg, alt;
	#define FALSE 0
	#define TRUE 1
	#define STR_SIZE 256
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
			for (; *fmt >= '0' && *fmt <= '9'; fmt++)
				field = field * 10 + *fmt - '0';
		
		/* following arg is long instead of int */
		if (*fmt == 'l' || *fmt == 'L' || *fmt == 'q' || *fmt == 'C')
		{
			//if (*fmt != 'C' || sizeof (Cell) > sizeof (Int))
				//larg = TRUE;

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
			if (iarg == 'X')
				hex = 'A';
			else if (iarg == 'p')
			{
				hex = 'A';
				alt = TRUE;
			}

			if (larg || (iarg == 'p' && sizeof (char*) > sizeof (Int)))
				iarg = va_arg(args, uLong);
			else if (hex || iarg == 'u')
				iarg = va_arg(args, uInt);
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
				if (hex)
				{
					int d = iarg & 0xF;
					*--s = (d < 10) ? d + '0' : d - 10 + hex;
					iarg >>= 4;
				}
				else
				{
					*--s = iarg % 10 + '0';
					iarg /= 10;
				}
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

void
dprintf(const char *fmt, ...)
{
	va_list args;
	Byte buf[256];
	Byte buf2[256];
	Byte *s = buf;
	Byte *s2 = buf2;
	//extern int vbprintf(char *, const char *, va_list);

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

#if 0
void
init_piix(void)
{
	static uInt init_0_0[64] =	// /pci/host
	{
		0x71A08086,
		0x22100006,
		0x6000000,
		0x4000,
		0x84000008,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0xA0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x40C,
		0x11000000,
		0x111003,
		0x0,
		0x20202020,
		0x20202020,
		0x0,
		0xAA0,
		0x380A1F20,
		0x1130002,
		0x38140123,
		0x0,
		0x1,
		0x0,
		0x0,
		0x0,
		0x3,
		0x6104,
		0x500,
		0x0,
		0x100002,
		0x1F000203,
		0x0,
		0x0,
		0x0,
		0x30,
		0x0,
		0x1020,
		0x0,
		0x17D3A6D6,
		0xFFFF0C18,
		0x61,
		0x101C5,
		0x9,
		0x0,
		0x0,
		0xBBFFAD4C,
		0x80003E8A,
		0xCFF7D32C,
		0x3E9D,
		0x140,
		0x6000F800,
		0xF20,
		0x0
	};
	static uInt init_1_0[64] =		// /pci/pci
	{
		0x71A18086,
		0x220001F,
		0x6040000,
		0x14000,
		0x0,
		0x0,
		0x10100,
		0x22A00010,
		0x7FF08000,
		0xFFF0FFF0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x800000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	};
	static uInt init_7_0[64] =		// /pci/isa
	{
		0x71108086,
		0x2800007,
		0x6010002,
		0x800000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x01300000,
		0x0, 		// reg 0x50 : 0x75800 ?
		0x0,
		0x0,
		0x0,
		0x8080800B,
		0x10,
		0x80FE00,
		0x0,
		0x0,
		0xC0C0000,
		0x0,
		0x0,
		0x70000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x904005,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x25000000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0xF30,
		0x0
	};
	static uInt init_7_3[64] =		// /pci/power
	{
		0x71138086,
		0x2800000,
		0x6800002,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x401,
		0x0,
		0x0,
		0x0,
		0x1D5800,
		0x2124849,
		0x2000004,
		0x10000002,
		0x40E70290,
		0x10000000,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x1,
		0x0,
		0x0,
		0x0,
		0x441,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0xF30,
		0x0
	};

	int i;

//	for (i = 0; i < sizeof init_0_0 / sizeof *init_0_0; i++)
//		config_w(0, 0, i * 4, init_0_0[i]);

	for (i = 0; i < sizeof init_1_0 / sizeof *init_1_0; i++)
		config_w(1, 0, i * 4, init_1_0[i]);

	for (i = 0; i < sizeof init_7_3 / sizeof *init_7_3; i++)
		config_w(7, 3, i * 4, init_7_3[i]);

	for (i = 0; i < sizeof init_7_0 / sizeof *init_7_0; i++)
		config_w(7, 0, i * 4, init_7_0[i]);
}
#endif /*0*/

void
init_piix(void)
{
	if (config_r(0, 0, 0x00) != 0x71A08086)
		exit();

	/* PIIX4 power-management */
	if (config_r(7, 3, 0x00) != 0x71138086)
		exit();

	config_w(7, 3, 0x4C, 0x00FFFF);
	config_w(7, 3, 0x50, 0x1F5800);
	config_w(7, 3, 0x54, 0x2124849);
	config_w(7, 3, 0x58, 0x2000004);
	config_w(7, 3, 0x5C, 0x10000002);
	//config_w(7, 3, 0x60, 0x40E70290);
	config_w(7, 3, 0x60, 0x40FF03F0);
	config_w(7, 3, 0x64, 0x98000000);
	config_w(7, 3, 0x40, 0x401);
	config_w(7, 3, 0x80, 0x01);
	config_w(7, 3, 0x90, 0x441);

#if 0
	/* /pci/isa */
	config_w(7, 0, 0xC8, 0x25000000);
	config_w(7, 0, 0x80, 0x70000);
	config_w(7, 0, 0xB0, 0x904005);
	config_w(7, 0, 0xF8, 0xF30);

	/* /pci/host */
	config_w(0, 0, 0x50, 0x40C);
	config_w(0, 0, 0x54, 0x11000000);
	config_w(0, 0, 0x58, 0x111003);
	config_w(0, 0, 0x68, 0x0);
	config_w(0, 0, 0x70, 0x380A1F20);
	config_w(0, 0, 0x78, 0x38140123);
	config_w(0, 0, 0xF4, 0x6000F800);
#endif

	/* PIIX4 PCI-ISA bridge */
	if (config_r(7, 0, 0x00) != 0x71108086)
		exit();

	/* /pci/isa */
	config_w(7, 0, 0x4C, 0x03C30000);
	config_w(7, 0, 0x60, 0x8080800B);
	config_w(7, 0, 0x64, 0x10);
	config_w(7, 0, 0x68, 0x80FE00);
	config_w(7, 0, 0x80, 0x70000);
	config_w(7, 0, 0xB0, 0x904005);
	config_w(7, 0, 0xC8, 0x25000000);
	config_w(7, 0, 0x68, 0x80FE00);
	config_w(7, 0, 0x74, 0xC0C0000);
	config_w(7, 0, 0x90, 0x0);
	config_w(7, 0, 0xF8, 0xF30);

	if (config_r(7, 0, 0x60) != 0x8080800B)
		exit();

	/* see if I/O cycles are getting to the PIIX4 */
	config_w(7, 3, 0x40, 0x1000);	/* Power management BAR */
	config_w(7, 3, 0x90, 0x2000);	/* SMB BAR */
	config_w(7, 3, 0x4, 0x7);		/* I/O Enables */
	config_w(7, 3, 0x80, 0x1);		/* enable I/O regs */

	isa_write(0x2005, 0x55);			/* DATA 0 */
	isa_write(0x2006, 0xAA);			/* DATA 1 */

	if (isa_read(0x2005) != 0x55)			/* check DATA 0 */
		exit(1);

	if (isa_read(0x2006) != 0xAA)			/* check DATA 1 */
		exit(2);

#if 0
	dprintf("\nPIIX4 func 3 power-management regs:\n");

	{
		int reg, x;

		for (reg = 0; reg < 0x40; reg += 4)
		{
			if ((reg & 0xF) == 0)
				dprintf("%02X:  ", reg);

			x = in4(0x1000 + reg);
			dprintf("  0x%08X", x);

			if ((reg & 0xF) == 0xC)
				dprintf("\n");
		}

		dprintf("\nPIIX4 func 3 SMB regs:\n");

		for (reg = 0; reg < 0x10; reg += 4)
		{
			if ((reg & 0xF) == 0)
				dprintf("%02X:  ", reg);

			x = in4(0x2000 + reg);
			dprintf("  0x%08X", x);

			if ((reg & 0xF) == 0xC)
				dprintf("\n");
		}
	}

	/* try to get to the keyboard */
    isa_write(0x64, 0xAA);

    while ((isa_read(0x64) & 0x02))
		;

    if (isa_read(0x60) != 0x55)
        exit();

	/* see if we can talk to I/O register APMS status port in PIIX4 */
	isa_write(0xB3, 0xA5);
	
	if (isa_read(0xB3) != 0xA5)
		exit(3);
#endif
}

/* try to turn on the Winbond W83977TF SIO chip and force it to default
   config settings, on the off-chance it's coming up in PnP mode
 */
void
init_winbond(void)
{
	#define WB_MAGIC	0x3F0
	#	define WB_UNLOCK	0x87
	#	define WB_LOCK		0xAA
	#define WB_REG		0x3F0
	#define WB_DATA		0x3F1

	isa_write(WB_MAGIC, WB_UNLOCK);
	isa_write(WB_MAGIC, WB_UNLOCK);

	/* device ID OK? */
	isa_write(WB_REG, 0x20);

	if (isa_read(WB_DATA) != 0x97)
	{
		dprintf("init_winbond: isa_read of WB_DATA (%d): 0x%X\n", 
				WB_DATA, isa_read(WB_DATA));
		return;
	}

	/* set default values explicitly */
	isa_write(WB_REG, 0x22);
	isa_write(WB_DATA, 0xFF);
	isa_write(WB_REG, 0x23);
	isa_write(WB_DATA, 0xFE);
	isa_write(WB_REG, 0x24);
	isa_write(WB_DATA, 0xC4);
	isa_write(WB_REG, 0x25);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0x26);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0x28);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0x2A);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0x2B);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0x2C);
	isa_write(WB_DATA, 0x00);

	/* set defaults for UARTA (device 2) */
	isa_write(WB_REG, 0x7);
	isa_write(WB_DATA, 0x2);
	isa_write(WB_REG, 0x30);
	isa_write(WB_DATA, 0x01);
	isa_write(WB_REG, 0x60);
	isa_write(WB_DATA, 0x03);
	isa_write(WB_REG, 0x61);
	isa_write(WB_DATA, 0xF8);
	isa_write(WB_REG, 0x70);
	isa_write(WB_DATA, 0x04);
	isa_write(WB_REG, 0xF0);
	isa_write(WB_DATA, 0x00);

	/* set defaults for UARTB (device 3) */
	isa_write(WB_REG, 0x7);
	isa_write(WB_DATA, 0x3);
	isa_write(WB_REG, 0x30);
	isa_write(WB_DATA, 0x01);
	isa_write(WB_REG, 0x60);
	isa_write(WB_DATA, 0x02);
	isa_write(WB_REG, 0x61);
	isa_write(WB_DATA, 0xF8);
	isa_write(WB_REG, 0x70);
	isa_write(WB_DATA, 0x03);
	isa_write(WB_REG, 0xF0);
	isa_write(WB_DATA, 0x00);
	isa_write(WB_REG, 0xF1);
	isa_write(WB_DATA, 0x00);

	isa_write(WB_MAGIC, WB_LOCK);
}

void
dump_regs(void)
{
	int bus;
	int numbuses = 1;

    for (bus = 0; bus < numbuses && bus < 4; bus++)
    {
		int dev;

        for (dev = 0; dev < 32; dev++)
        {
			int func, numfunc;
            int header = pci_config_r(bus, dev, 0, 0x0C /*PCI_CONFIG_HEADER*/);
            header = (header >> 16) & 0xFF;

            if (header & 0x80)
                numfunc = 8;
            else
                numfunc = 1;

            if (header & 0x7F)
                numbuses++;

            for (func = 0; func < numfunc; func++)
            {
				int reg;
                int id = pci_config_r(bus, dev, func, 0x0 /*PCI_CONFIG_ID*/);

                if (id == 0xFFFFFFFF || id == 0)
                    continue;

                /* dump the configuration registers for this device */
                dprintf("Registers for bus %d, dev %d, func %d\n",
                        bus, dev, func);

                for (reg = 0; reg < 0xFF; reg += 4)
                {
					int x;

                    if ((reg & 0xF) == 0)
                        dprintf("%02X:  ", reg);

                    x = pci_config_r(bus, dev, func, reg);

                    dprintf("  0x%08X", x);

                    if ((reg & 0xF) == 0xC)
                        dprintf("\n");
                }

                dprintf("\n");
            }
        }
    }
}

/*
 * Access to machine-specific registers (available on 586 and better only)
 * Note: the rd* operations modify the parameters directly (without using
 * pointer indirection), this allows gcc to optimize better
 */

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
                            : "=a" (val1), "=d" (val2) \
                            : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
                          : /* no outputs */ \
                          : "c" (msr), "a" (val1), "d" (val2))

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define rdtscl(low) \
     __asm__ __volatile__ ("rdtsc" : "=a" (low) : : "edx")

#define rdtscll(val) \
     __asm__ __volatile__ ("rdtsc" : "=A" (val))

#define rdpmc(counter,low,high) \
     __asm__ __volatile__("rdpmc" \
                          : "=a" (low), "=d" (high) \
                          : "c" (counter))


void
init_cache(void)
{
  unsigned long low, high;

  __asm__ __volatile__ ("mov %cr0, %eax\n"
                        "and $0x9fffffff,%eax\n"
                        "mov %eax, %cr0\n");

  /* set the first one to cache the first 64 meg. This is ALL WE NEED */
  /* let linux do the heavy lifting ... that's the right place. */
  /* first set the base */
  rdmsr(0x200, low, high);
  low = 0x6; 
  wrmsr(0x200, low, high);

  /* now set the mask */
  rdmsr(0x201, low, high);
  high = 0xf;
  low = 0xfc000800;
  wrmsr(0x201, low, high);
  
  /* now turn them on */
  /* we'd like to include the mtrr.h file but it includes
   * so much other stuff from linux that we can't. 
   */
  rdmsr(0x2ff, low, high);
  low |= 0x800;
  wrmsr(0x2ff, low, high);
}

int g_tst = 0x12345678;		/* make sure .data is correctly intialized */

/* Initialize the memory pool needed for malloc, and any other hardware as
   necessary.  For standalone use, a large static array is sufficient.
   For an embedded system, this routine should probe the available memory
   and use it to initialize malloc() and to initialize the two global
   g_machine_memory* vars above for "memory.c".
   Returns E_OUT_OF_MEMORY upon any error.
 */
void
machine_initialize(void)
{
	extern char end[], erodata[];

#ifdef VIRTUAL_SERIAL_PORT
	while (VREG(VSTATUS) & V_FLG)
		;
	
	(void)VREG(VHOSTDATA);
#endif /* VIRTUAL_SERIAL_PORT */

	/* set the serial port to a sane state */
	isa_write(DEBUG_PORT + SIO_LCR, 0x80);	/* DLAB on */
	isa_write(DEBUG_PORT + SIO_DLL, 12);	/* clock value for 9600 baud */
	isa_write(DEBUG_PORT + SIO_DLM, 0);
	isa_write(DEBUG_PORT + SIO_LCR, 0x03);	/* DLAB off, 8/1/N */
	isa_write(DEBUG_PORT + SIO_MCR, 0x01);	/* drop all lines, raise DTR */
	isa_write(DEBUG_PORT + SIO_IER, 0x00);	/* disable all interrupts */
	isa_write(DEBUG_PORT + SIO_FCR, 0x07);	/* turn on and clear I/O FIFOs */

	failsafe_write("Alive!\r\n", 8);
	DPRINTF(("in machine_initialize() (sizeof ptr=%d)...\n", sizeof (void*)));

	init_piix();
	init_winbond();
	init_cache();
	//dump_regs();

	/* init .bss and copy .data out of ROM to the right palace */
	memset((char*)MEMADDR, 0x000FFFF & (uInt)end, 0);
	memcpy((char*)MEMADDR, erodata + 4, 0x000FFFF & (uInt)end);
	DPRINTF(("g_tst=%#x &g_tst=%#x erodata=%#x end=%#x\n", g_tst, &g_tst,
			erodata + 4, end));

	/* use memory starting at after end of stack for malloc() */
	g_machine_memory = (char*)0;
	g_machine_memory_size = 3 * 1024 * 1024;
	g_machine_memory_used = (uInt)(64 * 1024);
	DPRINTF(("g_machine_memory=%p size=%#x used=%#x\n", g_machine_memory,
			g_machine_memory_size, g_machine_memory_used));
	//(void)init_malloc(g_machine_memory + g_machine_memory_used,
			//g_machine_memory_size - g_machine_memory_used);
	
	DPRINTF(("machine_initialize done\n"));
}

/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
void
reset_all(void)
{
	/* reset system via keyboard controller */
	isa_write(0x64, 0xFE);
}

/* This is used as a fall-back if a console cannot be opened at boot-time.
   The default is for Unix systems and will definitely need to be changed
   for an embedded system.  The easiest thing would be to send the data
   out of a hardwired serial port.
 */
Int
failsafe_write(Byte *buf, Int len)
{
#ifdef VIRTUAL_SERIAL_PORT
	Int actual = 0;
	unsigned char bit;
	unsigned char r;

	while (len)
	{
		/* ready for a char? */
		if (!(VREG(VSTATUS) & V_TDA))
		{
			r = VREG(VONE);

			for (bit = 0x01; bit; bit <<= 1)
			{
				if (*buf & bit)
					r = VREG(VONE);
				else
					r = VREG(VZERO);
			}

			r = VREG(VONE);
			buf++;
			len--;
			actual++;
		}
	}

	return actual;

#else /* !VIRTUAL_SERIAL_PORT */

	Int actual = 0;

	while (len)
	{
		if (isa_read(DEBUG_PORT + SIO_LSR) & 0x20)	/* THR ready for a char? */
		{
			isa_write(DEBUG_PORT + SIO_THR, *buf++);
			actual++;
			len--;
		}
	}

	return actual;

#endif /* !VIRTUAL_SERIAL_PORT */
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
#ifdef VIRTUAL_SERIAL_PORT
	Int actual = 0;

	while (len)
	{
		if (!(VREG(VSTATUS) & V_HDA))	/* char ready  */
			break;

		*buf++ = VREG(VHOSTDATA);
		actual++;
		len--;
	}

	return (actual == 0) ? -2 : actual;

#else /* !VIRTUAL_SERIAL_PORT */

	Int actual = 0;
	Int c;
	
	while (len)
	{
		c = isa_read(DEBUG_PORT + SIO_LSR);	/* char ready in RBR? */

		if (c & 0x9E)		/* some error occurred? */
		{
			actual = -1;
			break;
		}

		if (!(c & 0x01))	/* char ready in RBR? */
			break;

		*buf++ = isa_read(DEBUG_PORT + SIO_RBR) & 0x7F;
		actual++;
		len--;
	}

	return (actual == 0) ? -2 : actual;

#endif /* VIRTUAL_SERIAL_PORT */
}

#include "zarr.h"

int
main(int argc, char **argv)
{
	int (*volatile entry)(void) = (void*)ZRUNADDR;

	machine_initialize();
	dprintf("Hello, world!\n");

	DPRINTF(("copy zrun: entry=%#x zrun=%#x len=%d\n",
			entry, zrun, sizeof zrun));
	memcpy((char*)entry, zrun, sizeof zrun);

	DPRINTF(("call zrun: entry=%#x\n", entry));
	return (*entry)();
}
