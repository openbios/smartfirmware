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

/* (c) Copyright 1996-2003 by CodeGen, Inc.  All Rights Reserved. */

/* machine-dependancies here - needs to be customized for a particular system */

#include <stddef.h>
#include <stdarg.h>

/*#define DEBUG*/

#include "defs.h"
#include "pci.h"
#include "isa.h"
#include "fs.h"
#include "exe.h"


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
uInt g_machine_memory_offset = 0;

Byte g_motherboard_name[16];
Byte g_motherboard_desc[80];


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

#ifndef DEBUG_PORT
#define DEBUG_PORT	COM1_PORT
#endif /* DEBUG_PORT */


/* to access the ISA bus through the PCI bus hopefully at default addresses */
extern void isa_write(uInt addr, uByte val);
extern uByte isa_read(uInt addr);

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

/* use the RTC go generate a system "tick" */
#define IO_RTC	0x70

static int
rtc_in(int reg)
{
	uByte val;

	isa_write(IO_RTC, reg);
	isa_read(0x84);
	val = isa_read(IO_RTC + 1);
	isa_read(0x84);
	return val;
}

static void
rtc_out(unsigned char reg, unsigned char val)
{
	isa_read(0x84);
	isa_write(IO_RTC, reg);
	isa_read(0x84);
	isa_write(IO_RTC + 1, val);
}


void
dprintf(const char *fmt, ...)
{
	va_list args;
	Byte buf[2048];
	Byte buf2[2048];
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

#if 0
	if (g_e && g_e->screen)
		display_text(g_e, buf2, strlen(buf2));
	else
#endif
		failsafe_write(buf2, strlen(buf2));
}


/* assembly to catch memory-reference errors for probe read/write and
   to bump a timer/counter
 */
static int g_mem_error;


/******
pointer block:
             0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  01234567.89ABCDEF
0x000F5420  5F 4D 50 5F 00 14 0F 00  01 04 7D 00 00 00 00 00  _MP_.... ..}.....

config table:
             0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  01234567.89ABCDEF
0x000F1400  50 43 4D 50 18 01 04 F7  41 4D 44 20 4B 38 20 20  PCMP.... AMD K8  
0x000F1410  53 4F 4C 4F 20 20 20 20  20 20 20 20 00 00 00 00  SOLO         ....
0x000F1420  00 00 1C 00 00 00 E0 FE  64 00 AC 00 00 00 10 03  ........ d.......
0x000F1430  01 0F 00 00 FF FB 8B 07  00 00 00 00 00 00 00 00  ........ ........
0x000F1440  01 00 50 43 49 20 20 20  01 01 50 43 49 20 20 20  ..PCI    ..PCI   
0x000F1450  01 02 50 43 49 20 20 20  01 03 49 53 41 20 20 20  ..PCI    ..ISA   
0x000F1460  02 02 11 01 00 00 C0 FE  03 03 00 00 03 00 02 00  ........ ........
0x000F1470  03 00 00 00 03 01 02 01  03 00 00 00 03 00 02 02  ........ ........
0x000F1480  03 00 00 00 03 03 02 03  03 00 00 00 03 04 02 04  ........ ........
0x000F1490  03 00 0F 00 03 05 02 05  03 00 00 00 03 06 02 06  ........ ........
0x000F14A0  03 00 00 00 03 07 02 07  03 00 05 00 03 08 02 08  ........ ........
0x000F14B0  03 00 00 00 03 09 02 09  03 00 0F 00 03 0A 02 0A  ........ ........
0x000F14C0  03 00 0F 00 03 0B 02 0B  03 00 00 00 03 0C 02 0C  ........ ........
0x000F14D0  03 00 00 00 03 0D 02 0D  03 00 00 00 03 0E 02 0E  ........ ........
0x000F14E0  03 00 00 00 03 0F 02 0F  03 00 00 00 00 1F 02 13  ........ ........
0x000F14F0  03 00 00 00 02 03 02 13  03 00 00 00 01 00 02 10  ........ ........
0x000F1500  03 00 00 00 02 10 02 10  04 03 00 00 03 00 FF 00  ........ ........
0x000F1510  04 01 00 00 00 00 FF 01                           ........         

variable-sized entries:
             0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  01234567.89ABCDEF
0x000F1510                           80 14 00 00 00 00 00 00           ........
0x000F1520  00 00 00 00 00 00 01 00  00 00 00 00 80 14 00 01  ........ ........
0x000F1530  00 00 00 20 00 00 00 00  00 00 E0 DE 00 00 00 00  ... .... ........
0x000F1540  80 14 00 01 00 10 E0 FE  00 00 00 00 00 F0 1F 01  ........ ........
0x000F1550  00 00 00 00 81 08 01 01  00 00 00 00 81 08 02 01  ........ ........
0x000F1560  00 00 00 00 81 08 03 01  00 00 00 00 82 08 00 00  ........ ........
0x000F1570  01 00 00 00 82 08 00 00  00 00 00 00              ........ ....    

******/

uByte mp_config_table[] =
{
    0x50, 0x43, 0x4D, 0x50, 0x18, 0x01, 0x04, /*0xF7*/0xFF,
    0x41, 0x4D, 0x44, 0x20, 0x4B, 0x38, 0x20, 0x20,
    0x53, 0x4F, 0x4C, 0x4F, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE0, 0xFE,
    0x64, 0x00, 0xAC, 0x00, 0x00, 0x00, 0x10, 0x03,
    0x01, 0x0F, 0x00, 0x00, 0xFF, 0xFB, 0x8B, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x01, 0x00, 0x50, 0x43, 0x49, 0x20, 0x20, 0x20,
    0x01, 0x01, 0x50, 0x43, 0x49, 0x20, 0x20, 0x20,
    0x01, 0x02, 0x50, 0x43, 0x49, 0x20, 0x20, 0x20,
    0x01, 0x03, 0x49, 0x53, 0x41, 0x20, 0x20, 0x20,
    0x02, 0x02, 0x11, 0x01, 0x00, 0x00, 0xC0, 0xFE,

    0x03, 0x03, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x01, 0x02, 0x01,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x02,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x02, 0x03,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x04, 0x02, 0x04,
    0x03, 0x00, 0x0F, 0x00, 0x03, 0x05, 0x02, 0x05,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x06, 0x02, 0x06,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x07, 0x02, 0x07,
    0x03, 0x00, 0x05, 0x00, 0x03, 0x08, 0x02, 0x08,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x09, 0x02, 0x09,
    0x03, 0x00, 0x0F, 0x00, 0x03, 0x0A, 0x02, 0x0A,
    0x03, 0x00, 0x0F, 0x00, 0x03, 0x0B, 0x02, 0x0B,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x02, 0x0C,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x0D, 0x02, 0x0D,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x0E, 0x02, 0x0E,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x0F, 0x02, 0x0F,
    0x03, 0x00, 0x00, 0x00, 0x00, /*0x1F*/0x17, 0x02, 0x13,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x13,
    0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x10,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x10, 0x02, 0x10,

    0x04, 0x03, 0x00, 0x00, 0x03, 0x00, 0xFF, 0x00,
    0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01,
    0x80, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0x14, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xE0, 0xDE, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x14, 0x00, 0x01, 0x00, 0x10, 0xE0, 0xFE,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x1F, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x81, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x81, 0x08, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x81, 0x08, 0x03, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x82, 0x08, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x82, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uByte mp_config_ptr[] =
{
    0x5F, 0x4D, 0x50, 0x5F, 0x00, 0x14, 0x0F, 0x00,
    0x01, 0x04, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00
};

void
mptable_init(void)
{
    uByte *target;
    volatile uByte *reg = (uByte*)(uPtr)0xFEC00000;	/* I/O APIC */
    volatile uInt *data = (uInt*)(uPtr)0xFEC00010;	/* I/O APIC */

    DPRINTF(("mptable_init..."));
    memset((uByte*)(uPtr)0xF0000, 0x10000, 0);

    target = (uByte*)(uPtr)0xF1600;
    memcpy(target, mp_config_ptr, sizeof mp_config_ptr);

    target = (uByte*)(uPtr)0xF1400;
    memcpy(target, mp_config_table, sizeof mp_config_table);

    /* minimum init of I/O APIC */
    DPRINTF(("i/o apic init..."));
    *reg = 0x0;
    *data = 0x2000000;
    *reg = 0x2;
    *data = 0x2000000;
    //*reg = 0x10;
    //*data = 0x700;

    /* minimum init of the APIC */
    //DPRINTF(("apic init..."));
    //data = (uInt*)(uPtr)0xFEE00000;	/* APIC */
    //data[0xF0] = 0x10F;
    //data[0x350] = 0x10700;
    //data[0x360] = 0x10400;

    DPRINTF(("done\n"));
}


/* Initialize the memory pool needed for malloc, and any other hardware as
   necessary.  For standalone use, a large static array is sufficient.
   For an embedded system, this routine should probe the available memory
   and use it to initialize malloc() and to initialize the two global
   g_machine_memory* vars above for "memory.c".
   Returns E_OUT_OF_MEMORY upon any error.
 */
Retcode
machine_initialize(void)
{
	Retcode ret;
	extern char edata[], end[];
	extern void init_virtual();

#if 0
	/* set the serial port to a sane state */
	isa_write(DEBUG_PORT + SIO_LCR, 0x80);  /* DLAB on */
	isa_write(DEBUG_PORT + SIO_DLL, 12);    /* clock value for 9600 baud */
	isa_write(DEBUG_PORT + SIO_DLM, 0);
	isa_write(DEBUG_PORT + SIO_LCR, 0x03);  /* DLAB off, 8/1/N */
	isa_write(DEBUG_PORT + SIO_MCR, 0x01);  /* drop all lines, raise DTR */
	isa_write(DEBUG_PORT + SIO_IER, 0x00);  /* disable all interrupts */
	isa_write(DEBUG_PORT + SIO_FCR, 0x07);
				/* turn on and clear I/O FIFOs */
#endif

//	failsafe_write("Alive!\r\n", 8);
	DPRINTF(("in machine_initialize() (sizeof ptr=%d)...\n",
		sizeof (void *)));

	/* clear out the .bss segment */
	DPRINTF(("clear .bss: edata=%#x len=%#x\n", edata, end - edata));
	memset(edata, '\0', end - edata);

	/* init the RTC for a periodic counter */
	rtc_out(0xA, 0x26);		/* ~1ms tick - 0x26 */
	rtc_out(0xB, (rtc_in(0xB) & 0x07));	/* turn off intrs */

	/* reserve our working memory plus some useful chunk */
	g_machine_memory = (char *)0x0;
	g_machine_memory_size = (uInt)end + 4 * 1024 * 1024;
	g_machine_memory_used = (uInt)end;
	DPRINTF(("g_machine_memory=%p size=%#x used=%#x\n", g_machine_memory,
		g_machine_memory_size, g_machine_memory_used));
	ret = init_malloc(g_machine_memory + g_machine_memory_used,
		g_machine_memory_size);

	mptable_init();

	init_virtual();
	DPRINTF(("machine_initialize: ret=%d\n", ret));
	return ret;
}

/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
Retcode
machine_reset_all(Environ *e)
{
    /* set both warm-boot and BIOS bits in HT init ctrl reg */
    pci_config_write(0, 0, 24, 0, 0x6C, 0x30 |
	    pci_config_read(0, 0, 24, 0, 0x6C));

    /* reset system via keyboard controller */
    isa_write(0x64, 0xFE);

    /* hard-reset system via 8111 in case the above fails */
    isa_write(0xCF9, 0x0E);

    return E_UNIMPLEMENTED;	/* should never get here */
}


/* Display a value "n" on some LED display, or whatever is appropriate, on
   the hardware.  This is used by the Forth/FCode word "display-status"
   as a sort of last-resort.
 */
void
machine_led_write(Environ *e, Int n)
{
	char buf[256];
	
	bprintf(buf, "LED=%d\r\n", n);
	failsafe_write(buf, strlen(buf));
}


/* These three routines are used to light some lamp or display some message on
   the hardware to indicate testing in progress, as some tests may affect the
   display.  They're pretty much just for cosmetic purposes and may just call
   machine_led_write() above with some appropriate value.
 */
void
machine_test_begin(Environ *e)
{
}

void
machine_test_pass(Environ *e)
{
}

void
machine_test_fail(Environ *e)
{
}


/* Probe an address and return TRUE if the data at the address is
   legitimate, and FALSE if not.  This may have to catch an access error
   with an interrupt service routine for some hardware.  Our default
   implementation just assumes things will work.
 */
Bool
machine_probe_read(Environ *e, Cell addr, Cell *value, int size)
{
	g_mem_error = 0;

	switch (size)
	{
	case sizeof (Byte):
		*value = *(uByte*)(uPtr)addr;
		break;
	case sizeof (Short):
		*value = *(uShort*)(uPtr)addr;
		break;
	case sizeof (Int):
		*value = *(uInt*)(uPtr)addr;
		break;
#ifdef SF_64BIT
	case sizeof (Long):
		*value = *(uLong*)(uPtr)addr;
		break;
#endif
	default:
		return FALSE;
	}

	return g_mem_error ? FALSE : TRUE;
}

/* Write a value to an address which may be invalid.  If invalid, return
   FALSE, else TRUE.  Again, this may need to catch a signal or something
   to detect access to invalid addresses.
 */
Bool
machine_probe_write(Environ *e, Cell addr, Cell value, int size)
{
	g_mem_error = 0;
	
	switch (size)
	{
	case sizeof (Byte):
		*(uByte*)(uPtr)addr = value;
		break;
	case sizeof (Short):
		*(uShort*)(uPtr)addr = value;
		break;
	case sizeof (Int):
		*(uInt*)(uPtr)addr = value;
		break;
#ifdef SF_64BIT
	case sizeof (Long):
		*(uLong*)(uPtr)addr = value;
		break;
#endif
	default:
		return FALSE;
	}

	return g_mem_error ? FALSE : TRUE;
}


/* Read a register from a device or bus with a single operation.
   Assumes that reading from a C pointer dereference will be OK.
 */
void
machine_reg_read(Environ *e, Cell addr, Cell *value, int size)
{
	if ((uCell)addr < 0xA0000)		/* set aside for I/O space */
	{
		pci_io_read(pci_find_host_bridge_number(e, e->currpkg),
				addr, value, size);
		/*DPRINTF(("machine_reg_read: addr=%#x value=%#x size=%d\n",
				addr, *value, size));*/
		return;
	}

	switch (size)
	{
	case sizeof (Byte):
		*value = *(uByte*)(uPtr)addr;
		break;
	case sizeof (Short):
		*value = *(uShort*)(uPtr)addr;
		break;
	case sizeof (Int):
		*value = *(uInt*)(uPtr)addr;
		break;
#ifdef SF_64BIT
	case sizeof (Long):
		*value = *(uLong*)(uPtr)addr;
		break;
#endif
	default:
		*value = 0;
		break;
	}
}


/* Write a register to a device or bus with a single operation.
   Assumes that copying to a C pointer dereference will be OK.
 */
void
machine_reg_write(Environ *e, Cell addr, Cell value, int size)
{
	if ((uCell)addr < 0xA0000)		/* set aside for I/O space */
	{
		/*DPRINTF(("machine_reg_write: addr=%#x value=%#x size=%d\n",
				addr, value, size));*/
		pci_io_write(pci_find_host_bridge_number(e, e->currpkg),
				addr, value, size);
		return;
	}

	switch (size)
	{
	case sizeof (Byte):
		*(uByte*)(uPtr)addr = value;
		break;
	case sizeof (Short):
		*(uShort*)(uPtr)addr = value;
		break;
	case sizeof (Int):
		*(uInt*)(uPtr)addr = value;
		break;
#ifdef SF_64BIT
	case sizeof (Long):
		*(uLong*)(uPtr)addr = value;
		break;
#endif
	}
}


/* Read an unaligned value of the requested "size" from an "addr".
   This will probably not need to be modified, but check it to be sure.
 */
void
machine_unalign_read(Environ *e, Cell addr, Cell *value, int size)
{
	/* like machine_reg_read except addr may be unaligned */
	uByte *p = (uByte *)(uPtr)addr;
	Cell n = 0;

	switch (size)
	{
	case sizeof (Word):
#ifdef LITTLE_ENDIAN
		n = *p++;
		n |= *p << BYTE_SIZE;
#else
		n = *p++ << BYTE_SIZE;
		n |= *p;
#endif
		break;
	case sizeof (Int):
#ifdef LITTLE_ENDIAN
		n = *p++;
		n |= *p++ << BYTE_SIZE;
		n |= *p++ << (BYTE_SIZE * 2);
		n |= *p << (BYTE_SIZE * 3);
#else
		n = *p++ << (BYTE_SIZE * 3);
		n |= *p++ << (BYTE_SIZE * 2);
		n |= *p++ << BYTE_SIZE;
		n |= *p;
#endif
		break;
	}

	*value = n;
}


/* Write an unaligned value of the requested "size" to an "addr".
   This will probably not need to be modified, but check it to be sure.
 */
void
machine_unalign_write(Environ *e, Cell addr, Cell value, int size)
{
	/* like machine_reg_write except addr may be unaligned */
	uByte *p = (uByte*)(uPtr)addr;

	switch (size)
	{
	case sizeof (Word):
#ifdef LITTLE_ENDIAN
		*p++ = value & BYTE_MASK;
		*p = (value >> BYTE_SIZE) & BYTE_MASK;
#else
		*p++ = (value >> BYTE_SIZE) & BYTE_MASK;
		*p = value & BYTE_MASK;
#endif
		break;
	case sizeof (Int):
#ifdef LITTLE_ENDIAN
		*p++ = value & BYTE_MASK;
		*p++ = (value >> BYTE_SIZE) & BYTE_MASK;
		*p++ = (value >> (BYTE_SIZE * 2)) & BYTE_MASK;
		*p = (value >> (BYTE_SIZE * 3)) & BYTE_MASK;
#else
		*p++ = (value >> (BYTE_SIZE * 3)) & BYTE_MASK;
		*p++ = (value >> (BYTE_SIZE * 2)) & BYTE_MASK;
		*p++ = (value >> BYTE_SIZE) & BYTE_MASK;
		*p = value & BYTE_MASK;
#endif
		break;
	}
}


/* Get the current time and store it as seconds + nanoseconds in a Time_value.
   This will definitely need to be modified for a target.  The current
   definition is for standalone Unix systems and their equivalents.
 */
void
machine_gettime(Environ *e, Time_value *rtv)
{
	static Time_value tv;

	/* polling timer - RTC is set to ~1ms periodic */
	if (rtc_in(0xC) & 0x40)
		tv.nsec += 1000 * 1000;
	else
		tv.nsec++;		/* never return the same value twice */

	if (tv.nsec >= 1000 * 1000 * 1000)
	{
		tv.sec += tv.nsec / 1000 / 1000 / 1000;
		tv.nsec %= 1000 * 1000 * 1000;
	}

	*rtv = tv;
}

/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
	uLong tsc0;
	uLong tsc1;
	extern uLong msr_read(int offset);
#define TSC		0x10

	tsc0 = msr_read(TSC);

	while (1)
	{
		tsc1 = msr_read(TSC);

		if (((tsc1 - tsc0) / 800 / 4 /* XXX */) >= us)
			break;
	}
#if 0
	
	while (us--)
	{
		int i = 10000;	// TODO - should adjust itself for CPU speed
		
		while (i--)
			;
	}
#endif
}


/* This tests memory for the Forth/FCode "memory-test-suite" word.  The
   default is fairly simple but portable, given that all of the requested
   memory range is accessible.
 */
Bool
machine_memory_test(Environ *e, Cell addr, Cell len, Cell mask, Bool diag)
{
	Int cells = len / sizeof (Cell);
	Int i;
	Cell *p;

	if (addr & (sizeof (Cell) - 1))
	{
		len -= sizeof (Cell) - (addr & (sizeof (Cell) - 1));
		addr = (addr + sizeof (Cell) - 1) & ~(sizeof (Cell) - 1);
	}

	p = (Cell*)(uPtr)addr;
	cells = len / sizeof (Cell);

	for (i = 0; i < cells; i++, p++)
	{
		Cell v1 = *p;
		Cell v2;
		*p = v1 ^ mask;
		v2 = *p;
		*p = v1;

		if ((v2 & mask) != (v1 ^ mask))
			return FALSE;
	}

	return TRUE;
}

/* Return TRUE or FALSE depending on if some machine-dependent diagnostic
   switch is depressed.
 */
Bool
machine_diag_switch(Environ *e)
{
	return FALSE;
}



/* Machine-dependent stuff for admin.c goes here.
   These routines will need to be modified for a particular architecture.
 */

/* Probe and initialize all the hardware in the system and place
   it in the device tree at the appropriate places.
 */
CC(machine_probe_all)		/* (--) */
{
	EC( mmu_init_translations);
	Retcode ret;

	DPRINTF(("machine_probe_all()\n"));

	/* point e->currpk to the root node */
	PUSH(e, "/");
	PUSH(e, 1);
	ret = execute_word(e, "find-device");
	DPRINTF(("machine_probe_all: find-device / ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	prop_set_int(e->currpkg->props, "#address-cells", CSTR, 2);
	prop_set_int(e->currpkg->props, "#size-cells", CSTR, 2);
	prop_set_int(e->currpkg->props, "#physical-cells", CSTR, 2);

	strcpy(g_motherboard_name, "Solo");
	strcpy(g_motherboard_desc, "AMD Athlon64 reference platform");

	prop_set_str(e->currpkg->props, "CODEGEN,vendor", CSTR,
		"Advanced Micro Devices, Inc.", CSTR);
	prop_set_str(e->currpkg->props, "CODEGEN,board", CSTR,
		g_motherboard_name, CSTR);
	prop_set_str(e->currpkg->props, "CODEGEN,description", CSTR,
		g_motherboard_desc, CSTR);

	DPRINTF(("machine_probe_all: root properties set\n"));

	/* initialize the MMU properties */
	ret = mmu_init_translations(e);
	DPRINTF(("machine_probe_all: mmu_init ret=%d\n", ret));

	/* point e->currpkg to the PCI bus node */
	PUSH(e, "/pci");
	PUSH(e, 4);
	ret = execute_word(e, "find-device");

	if (ret != NO_ERROR)	/* no /pci node, assume ISA-only system */
	{
		Package *isa = new_package(e->root);
		DPRINTF(("machine_probe_all: NO PCI\n"));
		install_isabase(e);		/* install ISA device access routines */
		return install_isa(e, isa);
	}

	/* now safe to probe the PCI bus - also probes the ISA bus */
	DPRINTF(("machine_probe_all: about to probe PCI\n"));
	ret = execute_word(e, "probe-pci");
	DPRINTF(("machine_probe_all: PCI probe ret = %d\n", ret));
	return ret;
}

/* Secondary diagnostics run after the console is up for error messages.
   More rigorous testing should go here rather than at bootup just so that
   errors can be displayed */
CC(machine_secondary_diag)	/* (--) */
{
	return NO_ERROR;
}

/* set e->load to a reasonable place in memory that's not used by Forth
 */
CC(machine_init_load)
{
	e->load = (Byte*)(uPtr)get_config_int(e, "load-base", CSTR);
	return NO_ERROR;
}

/* do whatever is needed to initialize the machine-dependent program
   at e->load
 */
CC(machine_init_program)	/* (--) */
{
	/* could adjust g_machine_memory_used by e->loadlen, but there seems
	   little point */
	return exec_is_exec(e) ? NO_ERROR : E_BAD_IMAGE;
}

Retcode
flush_caches(void)
{
    __asm("wbinvd");
    return NO_ERROR;
}

void
flush_tlb(void)
{
	uLong x;
	asm(" mov	%%cr3,%0" : "=a"(x));		// Read CR3
	asm(" mov	%0,%%cr3" : : "a"(x));		// Rewrite CR3
}

/* this may need to be in assembly, but seems to work for ELF chello
 */
Retcode
launch(int argc, char *argv[], char *envv[], uLong entry)
{
    Retcode (*volatile func)();
    char *os = get_config(g_e, "operating-system", CSTR);

    func = (Retcode (*)())(entry);
    DPRINTF(("launch: %s: func=%p\n", os, func));

    /* XXX - hacks for booting various OSes */

    if (compare_strs(os, CSTR, "openbsd", CSTR))
    {
	return func(0, 0, 0, 0, 0, 0, 0, 0);
    }
    else if (compare_strs(os, CSTR, "netbsd", CSTR))
    {
	/* XXX - netbsd should eventually support OF directly */
	uInt *stk;

	/* [boot structs copied out of NetBSD x86 stand/lib and include] */
	struct bootinfo {
	    Int nentries;
	    uInt entry[1];
	};
	struct bootinfo bt;

	struct btinfo_common {
	    Int len;
	    Int type;
	};

	#define BTINFO_BOOTPATH 0
	#define BTINFO_BOOTDISK 3
	#define BTINFO_NETIF 4
	#define BTINFO_CONSOLE 6
	#define BTINFO_BIOSGEOM 7
	#define BTINFO_SYMTAB 8
	#define BTINFO_MEMMAP 9

	/* console: name:"com" addr:0x3F8 speed=9600 */
	struct btinfo_console {
	    struct btinfo_common c;
	    Byte devname[16];
	    Int addr;
	    Int speed;
	};
	struct btinfo_console btcons;
	Int bridge = 0;	/* XXX */
	Int bus = 2;	/* XXX */
	Int dev;
	Int func;
	Int maxfunc;
	Int reg;

	/* XXX - turn on all plug-in PCI devs as netbsd does not */
	//for (bus = 0; bus <= 2; bus++)
	{
	    for (dev = 0; dev < 32; dev++)
	    {
		Int id;
		int type;

		/* read and hope we don't bus fault. */
		id = pci_config_read(bridge, bus, dev, 0, PCI_CONFIG_ID);

		if (id == 0xFFFFFFFFU)
		    continue;

		type = (pci_config_read(bridge, bus, dev, 0, PCI_CONFIG_HEADER)
				>> 16) & 0xFF;

		if (type & 0x80)
		    maxfunc = 8;
		else
		    maxfunc = 1;

		for (func = 0; func < maxfunc; func++)
		{
		    id = pci_config_read(bridge, bus, dev, func, PCI_CONFIG_ID);

		    if (id == 0xFFFFFFFFU)
			continue;

		    reg = pci_config_read(bridge, bus, dev, func,
			    PCI_CONFIG_COMMAND);
		    reg |= PCI_COMMAND_IOENABLE |
			    PCI_COMMAND_MEMENABLE |
			    PCI_COMMAND_MASTERENABLE;
		    pci_config_write(bridge, bus, dev, func,
			    PCI_CONFIG_COMMAND, reg);
		}
	    }	/* dev */
	}	/* bus */

	bt.nentries = 1;
	bt.entry[0] = (uInt)(uPtr)&btcons;

	btcons.c.len = sizeof btcons;
	btcons.c.type = BTINFO_CONSOLE;
	strcpy(btcons.devname, "com");
	btcons.addr = DEBUG_PORT;
	btcons.speed = 9600;

	/* push args on stack so that netbsd's 32-bit loader can read them */
	asm("movq %%rsp, %0" : "=q" (stk));	// get stack pointer
	stk -= 8;
	stk[0] = (uInt)(entry & 0x7FFFFFFF),
	stk[1] = 0x2008A,	// howto:single,halt,rdonly,verbose
	stk[2] = 0;
	stk[3] = (uInt)(uPtr)&bt;
	stk[4] = (uInt)(0x100000 + g_e->loadlen);
	stk[5] = 511 * 1024;	// XXX - hardwired RAM size
	stk[6] = 640;
	asm("subq $8*4, %rsp");
	asm("jmp  launch32");
    }
    else if (compare_strs(os, CSTR, "linux", CSTR))
    {
	/* XXX - HACK! - Jump past 32-bit setup code in vmlinux. */
	#define PARAMS		0x9A000
	#define tostr2(num)    #num
	#define tostr(num)     tostr2(num)

	char **cmdline;

	entry = 0xffffffff80100100;
	//entry = 0x100100;

	/* clear bootparam area */
	memset((char*)PARAMS, 0, 0x1000);

	/* let kernel know how much RAM is available */
	*(uInt*)(PARAMS + 0x1E0) = 511 * 1024;	// ALT_MEM_K - XXX

#if 0
	#define E820PTR		(PARAMS + 0x2D0)
	#define E820NR		(PARAMS + 0x1E8)
	#define E820MAP		(PARAMS + 0x2000)

	/* init a map to let kernel know how much RAM is available */
	*(Int*)E820NR = 1;
	*(char**)E820PTR = (char*)E820MAP;
	((Long*)E820MAP)[0] = 0;
	((Long*)E820MAP)[1] = 511 * 1024 * 1024;	// XXX
	((Int*)E820MAP)[4] = 1;
#endif

	/* setup a command line to get some debug output */
	cmdline = (char**)(PARAMS + 0x228);
	*cmdline = (char*)(PARAMS + 0x1000);
	strcpy(*cmdline, "console=ttyS0 earlyprintk=serial,ttyS0,9600");

	/* hack to send boot params to linux kernel */
	asm("	mov $" tostr(PARAMS) ", %esi");
	func = (Retcode (*)())(entry);
	return func();
    }

    func = (Retcode (*)())(entry);
    return func(argc, argv, envv);
}

/* execute the code at e->load in such a way that it is possible to
   resume Forth
 */
CC(machine_go)				/* (--) */
{
	Retcode ret;
	int argc = 2;
	char *argv[] = { "bsd", "-s", NULL, NULL, NULL };
	char *envv[] =
	{
		"client_interface=0xXXXXXXXXXXXXXXXX",
		"memsize=NNNNNNNN",
		"system_type=i386",
		"bootdev=ata()disc(0)rdisk()partition(1)\bsd",
		"osloadoptions=s",	/* force serial console */
		NULL
	};
	extern Int client_interface(Cell array[]);
	extern Retcode launch(int argc, char *argv[], char *envv[],
			unsigned long entry);

	bprintf(envv[0], "client_interface=0x%X", client_interface);
	argv[argc + 1] = (char*)client_interface;

	bprintf(envv[1], "memsize=%d", g_machine_memory_size / 1024 / 1024);
	argv[argc + 2] = (char*)(uPtr)g_machine_memory_size;

	if (!exec_is_exec(e))		/* sanity check */
		return E_BAD_IMAGE;

	ret = exec_load(e);

	/* -1 means we are not supposed to launch through e->entrypoint */
	if (ret != NO_ERROR || e->entrypoint == -1)
		return ret;

	DPRINTF(("entrypoint=%#lx...", e->entrypoint));

	DPRINTF(("flushing caches..."));
	flush_caches();

	DPRINTF(("launching...\n"));
	return launch(argc, argv, envv, e->entrypoint);	/* may not return */
}

/* execute the specified client callback function - this will work for
   C code
 */
int
machine_callback(Environ *e, Callback *func, Cell *array)
{
	return (*e->callback)(array);		/* call it - machine-dependent? */
}

/* push the font data on the stack, however it is gathered
 */
CC(machine_font)		/* (-- addr width height advance min-char #glyphs) */
{
	/* setup the default font data here */
	static Byte font[] = {
		#include FONT_FILE
	};
	
	/* push the data for the default font on the stack */
	IFCKSP(e, 0, 6);
	PUSH(e, font);
	PUSH(e, FONT_WIDTH);
	PUSH(e, FONT_HEIGHT);
	PUSH(e, FONT_ADVANCE);
	PUSH(e, FONT_FIRST);
	PUSH(e, FONT_COUNT);
	return NO_ERROR;
}

C(kbd_read)
{
	#define KBD_CMD		0x64
	#define KBD_DATA	0x60
	#define CMD_OBUF_FULL	0x01
	#define CMD_IBUF_FULL	0x02
	Int cmd;
	Int data = 0;
	Int len = 0;
	Byte buf[STR_SIZE];

	dprintf("kbd_read:");
	cmd = isa_read(KBD_CMD);
	dprintf(" cmd=%#x", cmd);

	while (cmd & CMD_OBUF_FULL)
	{
		data = isa_read(KBD_DATA);
		dprintf(" data=%#x\n", data);
		buf[len] = data;
		len++;
		cmd = isa_read(KBD_CMD);
		dprintf("    kbd_read: cmd=%#x", cmd);
	}

	buf[len] = '\0';
	dprintf(" len=%d\n", len);
	return NO_ERROR;
}

EC(f_push_config_str);
EC(f_push_config_int);
EC(f_dump_hammer);
EC(f_dump_superio);

#include "clkdump.c"

static char *amd8111_rev_names[] =
{
    NULL,
    "A0",
    NULL,
    "B4",
    "C0",
    "D0",
    "C1",
    "C2",
};

static char *amd8151_rev_names[] =
{
    NULL,
    "A0",
    "A1",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    "B0",
    "B1",
    "B2",
};

static char const str_athlon64[] = "Athlon 64";
static char const str_opteron[] = "Opetron";

static struct revinfo
{
    uInt cpuid;
    char const *proc;
    char const *revname;
} opteron_rev_names[] =
{
    { 0x00000F00, str_athlon64, "A0" },
    { 0x00000F01, str_athlon64, "A2" },
    { 0x00000F10, str_opteron, "A0" },
    { 0x00000F11, str_opteron, "A2" },
    { 0x00000F40, str_athlon64, "B0" },
    { 0x00000F41, str_athlon64, "B3" },
    { 0x00000F48, str_athlon64, "C0" },
    { 0x00000F50, str_opteron, "B0" },
    { 0x00000F51, str_opteron, "B3" },
    { 0x00000F58, str_opteron, "C0" },
};

void
cpuid(uInt set, uInt values[])
{
    unsigned long long rax;
    unsigned long long rbx;
    unsigned long long rcx;
    unsigned long long rdx;

    rax = set;
    asm(" cpuid" : "=a" (rax), "=b" (rbx), "=c" (rcx), "=d" (rdx) : "d" (rdx));
    values[0] = rax;
    values[1] = rbx;
    values[2] = rcx;
    values[3] = rdx;
    values[4] = rax >> 32;
    values[5] = rbx >> 32;
    values[6] = rcx >> 32;
    values[7] = rdx >> 32;
}

C(f_sys_info)
{
    int i;
    uInt info[8];
    int rev;
    extern int g_amd8111_device_id;

    cpuid(1, info);

    for (i = 0; i < sizeof opteron_rev_names / sizeof opteron_rev_names[0]; i++)
		if (opteron_rev_names[i].cpuid == info[0])
			break;

    if (i < sizeof opteron_rev_names / sizeof opteron_rev_names[0])
		cprintf(e, "CPU: %s rev %s\n", opteron_rev_names[i].proc,
				opteron_rev_names[i].revname);
    else
		cprintf(e, "CPU: Unknown %#X\n", info[0]);

    rev = pci_config_read(0, 0, g_amd8111_device_id, 0, 0x8) & 0xFF;

    if (rev < sizeof amd8111_rev_names / sizeof amd8111_rev_names[0] &&
			amd8111_rev_names[rev] != NULL)
		cprintf(e, "8111: rev %s\n", amd8111_rev_names[rev]);
    else
		cprintf(e, "8111: Unknown %#X\n", rev);

    rev = pci_config_read(0, 0, 1, 0, 0x8) & 0xFF;

    if (rev < sizeof amd8151_rev_names / sizeof amd8151_rev_names[0] &&
			amd8151_rev_names[rev] != NULL)
		cprintf(e, "8151: rev %s\n", amd8151_rev_names[rev]);
    else
		cprintf(e, "8151: Unknown %#X\n", rev);

    return NO_ERROR;
}

C(f_io_getquad)
{
    Retcode ret;
	Cell addr;
	uInt val;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	ret = pci_io_read(0, addr, &val, 4);

	if (ret == NO_ERROR)
		PUSH(e, val);

	return ret;
}

C(f_io_setquad)
{
	Cell addr;
	Int val;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);
	return pci_io_write(0, addr, val, 4);
}

C(f_io_getdoub)
{
    Retcode ret;
	Cell addr;
	uDoublet val;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	ret = pci_io_read(0, addr, &val, 2);

	if (ret == NO_ERROR)
		PUSH(e, val);

	return ret;
}

C(f_io_setdoub)
{
	Cell addr;
	Int val;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);
	return pci_io_write(0, addr, val, 2);
}

C(f_io_getbyte)
{
    Retcode ret;
	Cell addr;
	uByte val;

	IFCKSP(e, 1, 1);
	POP(e, addr);
	ret = pci_io_read(0, addr, &val, 1);

	if (ret == NO_ERROR)
		PUSH(e, val);

	return ret;
}

C(f_io_setbyte)
{
	Cell addr;
	Int val;

	IFCKSP(e, 2, 0);
	POP(e, addr);
	POP(e, val);
	return pci_io_write(0, addr, val, 1);
}

static Initentry init_machdep[] =
{
	{ "k", kbd_read, INVALID_FCODE },
	{ "flush_caches", (Command)flush_caches, INVALID_FCODE },

	{ "operating-system", f_push_config_str, INVALID_FCODE },
	{ "load-base", f_push_config_int, INVALID_FCODE },
	{ "dump-hammer", f_dump_hammer, INVALID_FCODE },
	{ "dump-superio", f_dump_superio, INVALID_FCODE },
	{ "dump-smb", dump_smb, INVALID_FCODE },
	{ "load-clk", load_clk, INVALID_FCODE },
	{ "sys-info", f_sys_info, INVALID_FCODE },

	{ "io-l@", f_io_getquad, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(io-addr -- data) read IO quadlet") },
	{ "io-l!", f_io_setquad, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data io-addr --) set IO quadlet") },

	{ "io-w@", f_io_getdoub, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(iio-addr -- data) read IO doublet") },
	{ "io-w!", f_io_setdoub, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data io-addr --) set IO doublet") },

	{ "io-b@", f_io_getbyte, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(io-addr -- data) read IO byte") },
	{ "io-b!", f_io_setbyte, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(data io-addr --) set IO byte") },

	{ NULL, NULL }
};

/* List all other init tables that are to be included in the runtime Forth
   image.  This determines which words are available in the runtime image,
   and also makes it easy to add new words in custom files.
   init_list[] is only used by "table.c".
 */
extern const Initentry init_funcs[];
extern const Initentry init_funcs64[];
extern const Initentry init_control[];
extern const Initentry init_packages[];
extern const Initentry init_forth[];
extern const Initentry init_admin[];
extern const Initentry init_nvedit[];
extern const Initentry init_other[];
extern const Initentry init_device[];
extern const Initentry init_debug[];
extern const Initentry init_fb[];
extern const Initentry init_filesystem[];
extern const Initentry init_tokenizer[];
extern const Initentry init_mmu[];
extern const Initentry init_msr[];

const Initentry* init_list[] =
{
	init_funcs,
	init_funcs64,
	init_control,
	init_packages,
	init_forth,
	init_admin,
	init_nvedit,
	init_other,
	init_device,
	init_debug,
	init_fb,
	init_filesystem,
	init_machdep,
	init_mmu,
	init_msr,
	NULL
};


/* List all init routines that have to be called to initialize packages and
   methods.  This is like init_list[] above, except it is used to initialize
   more complex packages that need an initialization routine to set things up.
   Again, custom packages can easily be added here if desired.
   install_list[] is only used by "table.c".
 */
EC(install_root);
EC(install_chosen);
EC(install_memory);
EC(install_cpu_amd64);
EC(install_display);
EC(install_failsafe);
EC(install_pci);
EC(install_flash);
EC(install_obptftp);
EC(install_deblocker);
EC(install_disklabel);
EC(install_client_services);
EC(init_options_from_nvram);	/* forward to definition down below */

const Command install_list[] =
{
	install_root,		/* should be first */
	install_chosen,		/* should be second */
	install_memory,		/* should be third */
	install_cpu_amd64,
	init_options_from_nvram,
	install_display,
	install_client_services,
	install_failsafe,
	install_obptftp,
	install_deblocker,
	install_disklabel,
	install_flash,
	install_pci,
	NULL
};


/* This is used to specify a list of initial PCI drivers to load to handle
   builtin devices.  pci_drivers[] is only used by pci_load_builtin_driver()
   in pci.c.
 */

extern Pci_driver pci_display_driver;
extern Pci_driver pci_display2_driver;
extern Pci_driver amd_lance_ethernet;
extern Pci_driver digital_21x4x_driver;
extern Pci_driver intel_etherexpress_pro;
extern Pci_driver ncr_53C8xx_driver;

/* various PCI-ISA bridges */
extern Pci_driver intel_82378_driver;
extern Pci_driver intel_piix_driver;
extern Pci_driver sis_85C496_driver;
extern Pci_driver generic_pci_isa_driver;

/* USB drivers */
extern Pci_driver usb_ohci_driver;
extern Pci_driver usb_uhci_driver;


/* configure 8111 */
extern Pci_driver amd_8111_driver;


#define FCODE_DRIVER
#ifdef FCODE_DRIVER

/* The following is for debugging Fcode drivers. */

extern unsigned char fcdrv_image[];

PCI(install_fcode_driver)
{
	DPRINTF(("install_fcode_driver: pkg=%p devinfo=%p"
			" vendid=%#x devid=%#x\n",
			pkg, devinfo, devinfo->vendid, devinfo->devid));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	return interp_fcode(e, fcdrv_image, MAGIC_FNEXT);
}

Pci_driver fcode_driver =
{
	// Intel PRO/1000 fcode driver:
	{ 0, 0, 0, 0, 0, 0x020000, 0x8086, 0 /*0x1004 0x100E*/, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0xFF0FFF, 0xFFFFFFFF, 0 /*0xFFFFFFF0*/, 0, 0, 0 },

	install_fcode_driver
};

#endif /* FCODE_DRIVER */


const Pci_driver *pci_drivers[] =
{
	//&pci_display_driver,
	//&pci_display2_driver,

	&amd_8111_driver,

	&amd_lance_ethernet,
	&digital_21x4x_driver,
	&intel_etherexpress_pro,
	&ncr_53C8xx_driver,

	&intel_82378_driver,
	&intel_piix_driver,
	&sis_85C496_driver,

	&generic_pci_isa_driver,

	&usb_ohci_driver,
	//&usb_uhci_driver,

#ifdef FCODE_DRIVER
	&fcode_driver,
#endif	/* FCODE */

	NULL
};


/* This is used to specify a list of builtin ISA drivers to probe and load.
   isa_devices[] is only used by probe_isa() isa.c.
 */
extern Isa_device ns16550_com1;
extern Isa_device ns16550_com2;
extern Isa_device ns16550_com3;
extern Isa_device ns16550_com4;
extern Isa_device isa_keyboard;
extern Isa_device vga_display;
extern Isa_device ne2000_0x300_3;
extern Isa_device ne2000_0x280_5;
extern Isa_device ne2000_0x280_10;
extern Isa_device ata_controller;

Isa_device *isa_devices[] =
{
	&ns16550_com1,
	&ns16550_com2,
	&ns16550_com3,
	&ns16550_com4,
	&isa_keyboard,
	&vga_display,
	&ne2000_0x300_3,
	//&ne2000_0x280_5,
	&ne2000_0x280_10,
	&ata_controller,
	NULL
};


/* List all supported builtin filesystems.  This is not standard OpenFirmware
   but is useful in that partitions will be displayed as part of the
   device tree under the builtin disk devices.
 */
extern Filesys g_dos_partition;
extern Filesys g_dos_fat;
extern Filesys g_bsd_partition;
extern Filesys g_bsd_ufs;
extern Filesys g_linux_ext2fs;
extern Filesys g_iso9660_fs;

Filesys *g_filesys[] =
{
	&g_dos_partition,
	&g_dos_fat,
	&g_bsd_partition,
	&g_bsd_ufs,
	&g_linux_ext2fs,
	&g_iso9660_fs,
	NULL
};


/* List all supported builtin binary/executable image types here.
   This is not standard OpenFirmware but lets us easily plug in new
   binary-image handlers.  Some image handlers may need to be compiled
   with the appropriate macros to detect the correct magic numbers.
 */
extern Exec_entry exec_fcode;
extern Exec_entry exec_forth;
extern Exec_entry exec_aout;
extern Exec_entry exec_coff;
extern Exec_entry exec_elf;
extern Exec_entry exec_elf64;
extern Exec_entry exec_gzip;
extern Exec_entry exec_raw_binary;

Exec_entry *g_exec_list[] = 
{
	&exec_fcode,
	&exec_forth,
	&exec_elf64,
	//&exec_aout,
	//&exec_coff,
	//&exec_elf,
	&exec_gzip,
	&exec_raw_binary,	/* must be the last entry if present */
	NULL
};


/* non-forth machine-dependent routines here, esp initializing stuff */

/* This is used as a fall-back if a console cannot be opened at boot-time.
   The default is for Unix systems and will definitely need to be changed
   for an embedded system.  The easiest thing would be to send the data
   out of a hardwired serial port.
 */
Int
failsafe_write(Byte *buf, Int len)
{
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
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
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
}


/* This is used to initialize non-volatile RAM, and to mimic it for
   standalone hosts.
 */
struct nvram_data
{
	char *name;
	char *val;
} g_nvram[] = 
{
	{ "auto-boot?", "false" },
	{ "auto-boot-timeout", "500" },
	{ "boot-command", "boot" },
	{ "boot-device", "disk" },
	{ "boot-file", "" },
	//{ "diag-device", "net" },
	//{ "diag-file", "diag" },
	{ "diag-switch?", "true" },
	{ "secondary-diag?", "false" },
	{ "nvramrc",
			"devalias screen /failsafe\n"
			"devalias keyboard /failsafe\n"
			/*"devalias screen /pci/isa/serial@i2F8\n"*/
			/*"devalias keyboard /pci/isa/serial@i2F8\n"*/
			/*"devalias screen /pci/isa/display\n"*/
			/*"devalias keyboard /pci/isa/keyboard\n"*/
			"devalias net /pci/pci/ethernet\n"
			"devalias disk /pci/isa/ide/disk@0,0\n"
			"devalias disk1 /pci/isa/ide/disk@0,1\n"
			"devalias cd /pci/isa/ide/cd\n"
			"probe-all install-console banner\n"
			": netbsd \" setenv operating-system netbsd\" eval ;\n"
			": elf \" setenv operating-system elf\" eval ;\n"
	},
	{ "use-nvramrc?", "true" },
	{ "input-device", "keyboard" },
	{ "output-device", "screen" },
	{ "screen-#columns", "80" },
	{ "screen-#rows", "24" },
	{ "oem-logo?", "false" },
	/* if present, oem-logo is 512-byte array of a 64x64-bit bitmap */
	{ "oem-banner?", "false" },
	{ "oem-banner", "This space for rent." },
	{ "inverse-video?", "true" },	/* usually looks better */
	{ "local-mac-address?", "true" },	/* for Sun-compatibility */
	{ "fcode-debug?", "true" },		/* to debug Fcode */

	/* set to "openbsd", "freebsd", "linux", "netbsd", or "unknown" */
	{ "operating-system", "unknown" },

	//{ "operating-system", "netbsd" },
	{ "diag-device", "disk1:0,a" },
	{ "diag-file", "/netbsd" },

	//{ "operating-system", "linux" },
	//{ "diag-device", "disk:1" },
	//{ "diag-file", "/boot/vmlinux" },

	/* non-standard nvram paramters */
	{ "boot-protocol", "dhcp" },
	{ "client-ip", "0.0.0.0" },
	{ "server-ip", "0.0.0.0" },
//	{ "client-ip", "192.168.110.60" },
//	{ "server-ip", "192.168.110.50" },
	{ "load-base", "0x4000000" },

	{ NULL, NULL }
};

#if 0
uInt
machine_nvram_size(Environ *e)
{
	return 0;
}

Retcode
machine_nvram_write(Environ *e, uChar *buf, uInt len)
{
	return E_PROM_WRITE_ERROR;
}

Retcode
machine_nvram_read(Environ *e, uChar *buf, uInt *len)
{
	return E_PROM_READ_ERROR;
}
#endif
