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

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <zlib.h>

typedef unsigned char uByte;
typedef int Int;

#define DPRINTF(args) dprintf args
//#define DPRINTF(args)


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
extern int init_malloc(void *memory, uInt max);

int
_start(void)
{
	extern int main(int argc, char **argv);

	failsafe_write("Zlive!\r\n", 8);
	return main(0, 0);
}


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

void
dprintf(const char *fmt, ...)
{
	va_list args;
	Byte buf[256];
	Byte buf2[256];
	Byte *s = buf;
	Byte *s2 = buf2;
	extern int vbprintf(char *, const char *, va_list);

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

int
main(int argc, char **argv)
{
	Byte *dest = (Byte*)LOADADDR;
	uLong destlen = (uInt)(1 * 1024 * 1024);
	const Byte *source = (Byte*)ROMADDR;
	uInt sourcelen = *(uInt*)(source + 8);
	int (*volatile entry)(void) = (int (*)())dest;
	int ret;
	static int g_tst = 0x12345678;		/* make sure .data is intialized */
	extern char end[];

	dprintf("Hello, zrun world!\n");
	DPRINTF(("g_tst=%#x &g_tst=%#x end=%#x\n", g_tst, &g_tst, end));

	/* use memory starting at after end of stack for malloc() */
	g_machine_memory = (char*)0;
	g_machine_memory_size = (uInt)dest;
	g_machine_memory_used = (uInt)(128 * 1024);
	DPRINTF(("g_machine_memory=%p size=%#x used=%#x\n", g_machine_memory,
			g_machine_memory_size, g_machine_memory_used));
	(void)init_malloc(g_machine_memory + g_machine_memory_used,
			g_machine_memory_size - g_machine_memory_used);

	if (source[0] != '@' || source[1] != '#' ||
			source[2] != '$' || source[3] != '%')
	{
		dprintf("Bad compressed-image magic: %x.%x.%x.%x\n",
				source[0], source[1], source[2], source[3]);
		return -4;
	}

	//for (ret = 0; ret < sourcelen; ret += 1024)
		//dprintf(" %#x:%#x.%#x.%#x.%#x", ret, source[ret],
				//source[ret + 1], source[ret + 2], source[ret + 3]);
	//dprintf("\n");

	//dprintf("dest=%#x destlen=%#x source=%#x sourcelen=%#x entry=%#x",
			//dest, destlen, source, sourcelen, entry);
	dprintf("uncompressing %d bytes into...", sourcelen);
	ret = uncompress(dest, &destlen, source + 12, sourcelen);
	dprintf("%d bytes\n", destlen);

	switch (ret)
	{
	case Z_OK:
		break;

	case Z_MEM_ERROR:
		dprintf("not enough memory\n");
		return -1;

	case Z_BUF_ERROR:
		dprintf("output buffer too small\n");
		return -1;

	case Z_DATA_ERROR:
		dprintf("input appears to be corrupt or not compressed\n");
		return -1;

	default:
		dprintf("unknown error %d\n", ret);
		return -1;
	}

	dprintf("Launching through entrypoint %#X...\n", entry);
	return (*entry)();
}
