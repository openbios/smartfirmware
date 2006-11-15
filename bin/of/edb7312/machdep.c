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

/* machine-dependancies here - needs to be customized for a particular system */

/*#define DEBUG*/
/*#define MARK_PROBE */
/*#define MARK_INIT */

#include <stdlib.h>
#include <stdarg.h>

#include "defs.h"
#include "fs.h"
#include "exe.h"
#include "lib7312.h"

/* put and get macros */
#define EP7312_RB(addr)		(*(volatile uByte *)(g_ep7312_base + ((addr) & 0x3FFF)))
#define EP7312_WB(addr,val)	(*(volatile uByte *)(g_ep7312_base + ((addr) & 0x3FFF)) = (val))
#define EP7312_R(addr)		(*(volatile uInt *)(g_ep7312_base + ((addr) & 0x3FFF)))
#define EP7312_W(addr,val)	(*(volatile uInt *)(g_ep7312_base + ((addr) & 0x3FFF)) = (val))

/* all of these are in locore.S */
extern unsigned int get_psr();
extern void flush_caches(void);
extern void hit_flush_cache(Byte *addr, Int len);
extern void flush_cache(Byte *addr, Int len);
extern Int arm_client_interface(Cell array[]);
extern void init_virtual();
void failsafe_init();

extern char start[], edata[], end[];

/* externs for Forth words that we use below */
EC(f_get);
EC(f_getdoub);
EC(f_getquad);
EC(f_set);
EC(f_setdoub);
EC(f_setquad);
EC(f_interact);

/* setup the default font data here */
static Byte font[] = {
	#include FONT_FILE
};

/* These define where memory starts and how much there is of it.  This is
   used to initialize the /memory node in memory.c, so that "claim" and
   "release" will work, and is also used to initialize the memory pool
   available for use by malloc().  Any additional devices of type "memory"
   may be placed beneath the appropriate bus.  #size-cells is assumed to
   be 1 for /memory.
 */
extern uInt *g_clientstate;
extern uInt *g_interruptstate;
uInt g_clientstatebuf[32];
uInt g_interruptstatebuf[32];
uInt g_boot_switches = 0;
Byte *g_machine_memory = NULL;
uInt g_machine_memory_size = 0;
uInt g_machine_memory_offset = 0;		/* offset of used memory */
uInt g_machine_memory_used = 0;
uByte g_default_mac_address[6] = { 0x00, 0x30, 0x23, 0x01, 0x00, 0x01 };

Byte g_motherboard_name[16];
Byte g_motherboard_desc[80];

uInt g_ep7312_base = 0x80000000;
Bool g_probe = FALSE;

/* EP7312 register offsets */
#define PORTA	0x0000
#define PADR	0x0000
#define PBDR	0x0001
#define PDDR	0x0003
#define PADDR	0x0040
#define PBDDR	0x0041
#define PDDDR	0x0043
#define PEDR	0x0080
#define PEDDR	0x00C0
#define SYSCON1	0x0100
#define SYSFLG1	0x0140
#define MEMCFG1	0x0180
#define MEMCFG2	0x01C0
#define INTSR1	0x0240
#define INTMR1	0x0280
#define LCDCON	0x02C0
#define TC1D	0x0300
#define TC2D	0x0340
#define RTCDR	0x0380
#define RTCMR	0x03C0
#define PMPCON	0x0400
#define CODR	0x0440
#define UARTDR1	0x0480
#define UBLCR1	0x04C0
#define SYNCIO	0x0500
#define PALLSW	0x0540
#define PALMSW	0x0580
#define STFCLR	0x05C0
#define BLEOI	0x0600
#define MCEOI	0x0640
#define TEOI	0x0680
#define TC1EOI	0x06C0
#define TC2EOI	0x0700
#define RTCEOI	0x0740
#define UMSEOI	0x0780
#define COEOI	0x07C0
#define HALT	0x0800
#define STDBY	0x0840
#define FBADDR	0x1000
#define SYSCON2	0x1100
#define SYSFLG2	0x1140
#define INTSR2	0x1240
#define INTMR2	0x1280
#define UARTDR2	0x1480
#define UBLCR2	0x14C0
#define SS2DR	0x1500
#define SRXEOF	0x1600
#define SS2POP	0x16C0
#define KBDEOI	0x1700
#define DAIR	0x2000
#define DAIDR0	0x2040
#define DAIDR1	0x2080
#define DAIDR2	0x20C0
#define DAISR	0x2100
#define SYSCON3	0x2200
#define INTSR3	0x2240
#define INTMR3	0x2280
#define LEDFLSH	0x22C0
#define SDCONF	0x2300
#define SDRFPR	0x2340
#define UNIQID	0x2440
#define DAI64FS	0x2600
#define RANDID0	0x2700
#define RANDID1	0x2704
#define RANDID2	0x2708
#define RANDID3	0x270C


/* dummy to avoid normal gcc runtime goo */
extern void __gccmain();
void __gccmain() { }

/* debug printf using failsafe_write() and possibly main console
   - probably won't need to be modified much
 */
void
dprintf(const char *fmt, ...)
{
	va_list args;
	static Byte buf[2048];
	static Byte buf2[2048];
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
	failsafe_write(buf2, s2 - buf2);	/* serial output */
}

#ifdef MARK_PROBE
#define mark_probe(x) dprintf("%s", (x))
#else
#define mark_probe(x)
#endif

#ifdef MARK_INIT
#define mark_init(x) dprintf("%s", (x))
#else
#define mark_init(x)
#endif

extern void arm_init();
void init_timer();

void
arm_init()
{
	extern char edata[], end[];
	extern int main();
	extern int twowaycache(), icachesize(), dcachesize();

	failsafe_init();
	flush_caches();

	/* set the failsafe serial port to a sane state */
#ifdef DEBUG
	failsafe_write("Alive!\r\n", 8);
#endif
	DPRINTF(("clearing bss edata=%#x end=%#x\n", edata, end));
	memset(edata, 0, end - edata);

	DPRINTF(("calling main()\n"));
	main(0, (char **)0);
	DPRINTF(("returned from main()\n"));

	for (;;)
		;
}


/* Probe for the size of available RAM by trying to write and then read
   back numbers.  Avoid writing on ourself (text or data) and assume that
   at least 1Mb of memory is available.  Change MAX_MEM_SIZE in machdep.h
   for custom hardware - 32Mb is the maximum the Cogent boards can handle.
 */

#ifndef MAX_MEM_SIZE
#	define MAX_MEM_SIZE 16	/* in Mb */
#endif
#define MB(v)	(char*)(0xC0000000 + ((v) * 1024 * 1024))

static uInt
probe_memory_size(void)
{
	uInt words[MAX_MEM_SIZE];
	uInt size;
	int i;

	/* first copy the words from memory - 1st Mb is not touched */
	for (i = 1; i < MAX_MEM_SIZE; i++)
		if (MB(i) < start || MB(i) > end)
			words[i] = *(uInt*)MB(i);
	
	/* write in a block number */
	for (i = MAX_MEM_SIZE - 1; i >= 1; i--)
		if (MB(i) < start || MB(i) > end)
			*(uInt*)MB(i) = i;
	
	/* and read it back - where it fails is the end of memory */
	for (i = 1; i < MAX_MEM_SIZE; i++)
		if (MB(i) < start || MB(i) > end)
			if (*(uInt*)MB(i) != i)
				break;
	
	size = i;

	/* restore the copied words */
	for (i = 1; i < size; i++)
		if (MB(i) < start || MB(i) > end)
			*MB(i) = words[i];

	return size * 1024 * 1024;
}

#define RESERVED_MEM	0x180000

/* Initialize the memory pool needed for malloc, and any other hardware as
   necessary.  For standalone use, a large static array is sufficient.
   For an embedded system, this routine should probe the available memory
   and use it to initialize malloc() and to initialize the two global
   g_machine_memory* vars above for "memory.c".
   Returns E_OUT_OF_MEMORY upon any error.
 */

#define MALLOC_BASE	(Byte *)0xF7098000

Retcode
machine_initialize(void)
{
	Retcode ret;
#if 0
	uInt syscon3;
#endif
	uInt syscon1;
	uInt porta;
	extern char end[];

	syscon1 = EP7312_R(SYSCON1);
	EP7312_W(SYSCON1, (syscon1 & ~0xF) | 0xD);
	porta = EP7312_R(PORTA);
	EP7312_W(SYSCON1, syscon1);
	g_boot_switches = porta & 0x3F;

	if (g_boot_switches & 0x08)
		g_boot_switches |= 0x30;

	/* load up all of the control registers */
	EP7312_WB(PADDR, 0x00);		/* Init port directions */
	EP7312_WB(PBDDR, 0xF2);
	EP7312_WB(PDDDR, 0x00);
	EP7312_WB(PEDDR, 0x01);

	EP7312_WB(PADR, 0x00);		/* Init output ports */
	EP7312_WB(PBDR, 0x00);
	EP7312_WB(PDDR, 0x00);
	EP7312_WB(PEDR, 0x00);

#if 0
	SDCONF@2300 00000522
	SDRFPR@2340 00000240
	LCDCON@2400 E60F7C1F
	PALLSW@0580 76543210
	PALMSW@0540 FEDCBA98
	FBADDR@1000 C0000000
	PMCCON@0400 00000800
	PADDR @0040 00000000
	PBDDR @0041 000000F2
	PDDDR @0043 0000002F
	PEDDR @00C0 00000001
	SYSCON1@0100 00041100
	SYSCON2@1100 00000101
	SYSCON3@2200 0000020E
	MEMCFG1@0180 1F101710
	MEMCFG2@01C0 00001F13
#endif

	
	DPRINTF(("in machine_initialize() (sizeof ptr=%d)...\n", sizeof (void*)));

	g_clientstate = g_clientstatebuf;
	g_interruptstate = g_interruptstatebuf;
	g_machine_memory = (Byte*)0xC0000000;
	g_machine_memory_size = probe_memory_size();
	DPRINTF(("after mem-probe: g_machine_memory=%p g_machine_memory_size=%#x\n",
			g_machine_memory, g_machine_memory_size));

	g_machine_memory_offset = 0;
	g_machine_memory_used = 0x100000;

	/* set malloc memory to after end of stack at 1Mb */
	DPRINTF(("calling init_malloc: g_machine_memory=%p, mallocbase=%#x, poolsize=%#x\n",
			g_machine_memory, MALLOC_BASE, MALLOC_POOL));

	if (MALLOC_BASE < end)
		dprintf("Heap may overlap image: %p %p, overlap %d\n", end, MALLOC_BASE, end - MALLOC_BASE);

	mark_init(".A");
	ret = init_malloc(MALLOC_BASE, MALLOC_POOL);
	DPRINTF(("ret = %d\n", ret));

	DPRINTF(("Initializing virtual address map\n"));
	mark_init(".B");
	init_virtual();		/* initalize virtual address space map */
	g_ep7312_base = EDB7312_VA_EP7312;

#if 0
	syscon3 = EP7312_R(SYSCON3);
	syscon3 |= 0x0006;		/* Clk = 73.728MHz */
	EP7312_W(SYSCON3, syscon3);
#endif

	DPRINTF(("starting timer: cpsr=%#x\n", get_psr()));
	mark_init(".C");
	init_timer();
	DPRINTF(("timer is running: cpsr=%#x\n", get_psr()));

	DPRINTF(("machine_initialize ret=%s (%d)\n", err2str(ret), ret));
	mark_init(".D\n");
	return ret;
}

/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
Retcode
machine_reset_all(Environ *e)
{
	uInt mode;
	extern void flush_caches(void);
	extern void flush_tlb(void);
	flush_caches();
	flush_tlb();
	EP7312_W(INTMR1, 0);		/* disable interrupts */
	EP7312_W(INTMR2, 0);		/* disable interrupts */
	EP7312_W(INTMR3, 0);		/* disable interrupts */
    	/* disable the MMU. */
    	mode = 0x70;
	__asm__("	mov    	r0,%0" : : "r"(mode));
	__asm__("	mcr    	p15, 0, r0, c1, c0");
	__asm__("	mov    	r0,r0");		/* stall */
	__asm__("	mov    	r0,r0");
	__asm__("	mov	pc,#0x00000000"	/* reset vector */);

	/* should not get here, but silence the compiler */
	return E_ABORT;
}

void
machine_led_write(Environ *e, Int n)
{
	DPRINTF(("LED=%d\n", n));
}


/* These three routines are used to light some lamp or display some message on
   the hardware to indicate testing in progress, as some tests may affect the
   display.  They're pretty much just for cosmetic purposes and may just call
   machine_led_write() above with some appropriate value.
 */
void
machine_test_begin(Environ *e)
{
	DPRINTF(("begin test(s)\n"));
}

void
machine_test_pass(Environ *e)
{
	DPRINTF(("test(s) passed\n"));
}

void
machine_test_fail(Environ *e)
{
	DPRINTF(("test(s) failed\n"));
}


/* locore.S exception handler will set this to TRUE on any exception */
extern Bool g_fault;


/* Probe an address and return TRUE if the data at the address is
   legitimate, and FALSE if not.  This has to catch an access error
   with an interrupt service routine to set g_fault.
 */
Bool
machine_probe_read(Environ *e, Cell addr, Cell *value, int size)
{
	uShort t; 

	/* only allow unmapped address accesses */
	g_fault = FALSE;
	g_probe = TRUE;

	switch (size)
	{
	case 1:
		*value = *(uByte*)(uPtr)addr;
		break;
	case 2:
		__asm("	ldrh	%0, [%1, #0]" : "=r"(t) : "r"(addr));
		*value = t;
		break;
	case 4:
		*value = *(uInt*)(uPtr)addr;
		break;
#ifdef SF_64BIT
	case 8:
		*value = *(uLong*)(uPtr)addr;
		break;
#endif
	default:
		return FALSE;
	}

	g_probe = FALSE;
	return !g_fault;
}

/* Write a value to an address which may be invalid.  If invalid, return
   FALSE, else TRUE.  Again, this has to catch an exception to set g_fault
   to detect access to invalid addresses.
 */
Bool
machine_probe_write(Environ *e, Cell addr, Cell value, int size)
{
	/* only allow unmapped address accesses */

	g_fault = FALSE;
	g_probe = TRUE;

	switch (size)
	{
	case 1:
		*(uByte*)(uPtr)addr = value;
		break;
	case 2:
		__asm("	strh	%0, [%1, #0]" : : "r"(value), "r"(addr));
		break;
	case 4:
		*(uInt*)(uPtr)addr = value;
		break;
#ifdef SF_64BIT
	case 8:
		*(uLong*)(uPtr)addr = value;
		break;
#endif
	default:
		return FALSE;
	}

	g_probe = FALSE;
	return !g_fault;
}


/* Read a register from a device or bus with a single operation.
   First see if this is a legitimate PCI address, and if so, perform the
   appropriate PCI memory or I/O read operation.  Otherwise, assume that
   reading from a C pointer dereference will be OK.
 */
void
machine_reg_read(Environ *e, Cell addr, Cell *value, int size)
{
	uShort t;

	/* like machine_probe_read except we don't expect a bus fault */
	switch (size)
	{
	case 1:
		*value = *(uByte*)(uPtr)addr;
		break;
	case 2:
		__asm("	ldrh	%0, [%1, #0]" : "=r"(t) : "r"(addr));
		*value = t;
		break;
	case 4:
		*value = *(uInt*)(uPtr)addr;
		break;
#ifdef SF_64BIT
	case 8:
		*value = *(uLong*)(uPtr)addr;
		break;
#endif
	default:
		*value = 0;
		break;
	}
}


/* Write a register to a device or bus with a single operation.  First see
   if this is a legitimate PCI address, and if so, perform the appropriate
   PCI memory or I/O write operation.  Otherwise, assume that copying to
   a C pointer dereference will be OK.
 */
void
machine_reg_write(Environ *e, Cell addr, Cell value, int size)
{
	/* like machine_probe_write except we don't expect a bus fault */
	switch (size)
	{
	case 1:
		*(uByte*)(uPtr)addr = value;
		break;
	case 2:
		__asm("	strh	%0, [%1, #0]" : : "r"(value), "r"(addr));
		break;
	case 4:
		*(uInt*)(uPtr)addr = value;
		break;
#ifdef SF_64BIT
	case 8:
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

char *excptable[] =
{
	"reset",
	"undef insn",
	"sw trap",
	"abort pre",
	"abort data",
	"reserved",
	"irq",
	"fiq"
};

/* C fault handler - called from "exception" in locore.S */
uInt *
fault_handler(uInt *state, int exception)
{
	uInt status;
	uInt addr;
	char *excpname = (exception >= 0 && exception <= 7) ? excptable[exception] : "";
	uInt nextstate[32];

	if (g_probe)
	{
		g_fault = TRUE;
		g_probe = FALSE;
		return state;
	}

	asm(" mrc     	p15, 0, %0, c5, c0" : "=r"(status));
	asm(" mrc     	p15, 0, %0, c6, c0" : "=r"(addr));

	dprintf("Fault! excp=%d (%s) pc=%#x status=%#x domain=%#x address=%#x\r\n", exception, excpname, state[15],
			status & 0xF, (status >> 4) & 0xF, addr);
	dprintf("\t r0 %08X  r1 %08X  r2 %08X  r3 %08X\n", state[1], state[2], state[3], state[4]);
	dprintf("\t r4 %08X  r5 %08X  r6 %08X  r7 %08X\n", state[5], state[6], state[7], state[8]);
	dprintf("\t r8 %08X  r9 %08X r10 %08X r11 %08X\n", state[9], state[10], state[11], state[12]);
	dprintf("\tr12 %08X r13 %08X r14 %08X psr %08X\n", state[13], state[14], state[15], state[0]);
	g_clientstate = nextstate;
	f_interact(g_e);
	g_clientstate = state;
	return state;
}

/* C interrupt handler - called from "interrupt" in locore.S
 */
unsigned long g_timer;

#define NUM_VECTORS	48

typedef void (*intr_func)(uInt *state, int vec);

extern intr_func g_interrupt_vectors[NUM_VECTORS];

uInt *
intr_handler(uInt *state)
{
	uInt intr_stat = EP7312_R(INTSR1);
	uInt intr_mask = EP7312_R(INTMR1);
	int i;

#if 0
	dprintf("Interrupt pc %08X state %#x\n", state[16], state);
	dprintf("\tspsr %08X ssp %08X cpsr %08X\n", state[0], state[17], state[18]); 
	dprintf("\t r0 %08X  r1 %08X  r2 %08X  r3 %08X\n", state[1], state[2], state[3], state[4]);
	dprintf("\t r4 %08X  r5 %08X  r6 %08X  r7 %08X\n", state[5], state[6], state[7], state[8]);
	dprintf("\t r8 %08X  r9 %08X r10 %08X r11 %08X\n", state[9], state[10], state[11], state[12]);
	dprintf("\tr12 %08X r13 %08X r14 %08X r15 %08X\n", state[13], state[14], state[15], state[16]);
#endif

#ifdef DEBUG2
	DPRINTF(("intr_handler: stat1 %#x, mask1 %#x\n", intr_stat, intr_mask));
#endif
	intr_stat &= intr_mask;

	if (intr_stat)
		for (i = 0; i < 16; i++)
			if ((intr_stat & (1 << i)) && g_interrupt_vectors[i])
				(*g_interrupt_vectors[i])(state, i);

	intr_stat = EP7312_R(INTSR2);
	intr_mask = EP7312_R(INTMR2);
#ifdef DEBUG2
	DPRINTF(("intr_handler: stat2 %#x, mask2 %#x\n", intr_stat, intr_mask));
#endif
	intr_stat &= intr_mask;

	if (intr_stat)
		for (i = 0; i < 16; i++)
			if ((intr_stat & (1 << i)) && g_interrupt_vectors[16 + i])
				(*g_interrupt_vectors[16 + i])(state, 16 + i);

	intr_stat = EP7312_R(INTSR3);
	intr_mask = EP7312_R(INTMR3);
#ifdef DEBUG2
	DPRINTF(("intr_handler: stat3 %#x, mask3 %#x\n", intr_stat, intr_mask));
#endif
	intr_stat &= intr_mask;

	if (intr_stat)
		for (i = 0; i < 16; i++)
			if ((intr_stat & (1 << i)) && g_interrupt_vectors[32 + i])
				(*g_interrupt_vectors[32 + i])(state, 32 + i);

#ifdef DEBUG2
	DPRINTF(("intr_handler: exit\n"));
#endif

	return state;
}

void
set_intr_handler(int vec, intr_func func)
{
	uInt mask;

	DPRINTF(("set_intr_handler: vec %d, func %#x\n", vec, func));

	if (vec < 0 || vec >= NUM_VECTORS)
		return;

	g_interrupt_vectors[vec] = func;
	DPRINTF(("set_intr_handler: vector set\n"));

	if (vec < 16)
	{
		mask = EP7312_R(INTMR1);
		DPRINTF(("set_intr_handler: mask %#x\n", mask));
		mask |= 1 << vec;
		DPRINTF(("set_intr_handler: new mask %#x\n", mask));
		EP7312_W(INTMR1, mask);
		DPRINTF(("set_intr_handler: new mask written\n"));
	}
	else if (vec < 32)
	{
		mask = EP7312_R(INTMR2);
		mask |= 1 << (vec - 16);
		EP7312_W(INTMR2, mask);
	}
	else 
	{
		mask = EP7312_R(INTMR3);
		mask |= 1 << (vec - 32);
		EP7312_W(INTMR3, mask);
	}

	DPRINTF(("set_intr_handler: handler set\n"));
}

void
clear_intr_handler(int vec)
{
	uInt mask;

	if (vec < 0 || vec >= NUM_VECTORS)
		return;

	g_interrupt_vectors[vec] = NULL;

	if (vec < 16)
	{
		mask = EP7312_R(INTMR1);
		mask &= ~(1 << vec);
		EP7312_W(INTMR1, mask);
	}
	else if (vec < 32)
	{
		mask = EP7312_R(INTMR2);
		mask &= ~(1 << (vec - 16));
		EP7312_W(INTMR2, mask);
	}
	else 
	{
		mask = EP7312_R(INTMR3);
		mask &= ~(1 << (vec - 16));
		EP7312_W(INTMR3, mask);
	}
}

void
timer_interrupt(uInt *state, int vec)
{
	static int in_client = 0;
#ifdef DEBUG2
	DPRINTF(("timer interrupt\n"));
#endif
	g_timer++;
	EP7312_W(TC1EOI, 1);		/* clear interrupt */

	if (g_e && g_e->callback && !in_client)
	{
		Cell array[4 + 12];	/* netbsd appears to expect this much space */
		in_client = 1;
		array[0] = (Cell)"tick";
		array[1] = 1;		/* one argument */
		array[2] = 1;		/* one return */
		array[3] = (Cell)state;
		machine_callback(g_e, g_e->callback, array);
		in_client = 0;
	}
}

void
init_timer()
{
	uInt syscon1;
	DPRINTF(("About to write count to TC1D\n"));
	EP7312_W(TC1D, 19);		/* 100Hz */
	DPRINTF(("count written TC1D\n"));
	syscon1 = EP7312_R(SYSCON1);
	DPRINTF(("syscon1 = %#x\n", syscon1));
	EP7312_W(SYSCON1, (syscon1 & ~0x30) | 0x10);
	EP7312_W(TC1EOI, 1);		/* clear interrupt */
	DPRINTF(("interrupt cleared\n"));
	set_intr_handler(8, timer_interrupt);
	DPRINTF(("handler set\n"));
}

/* Get the current time and store it as seconds + nanoseconds in a Time_value.
 */
void
machine_gettime(Environ *e, Time_value *tv)
{
#if 0
	uInt rtclock;
	uInt temp;
#endif
#if 1
	DPRINTF(("intrcount=%#x cpsr=%#x\n", g_timer, get_psr()));
	tv->sec = g_timer / 100;
	tv->nsec = (g_timer - tv->sec * 100) * 10 * 1000 * 1000;
#endif
#if 0
	rtclock = EP7312_R(RTCDR);
	temp = EP7312_R(RTCDR);

	while (temp != rtclock)
	{
		rtclock = temp;
		temp = EP7312_R(RTCDR);
	}

	tv->sec = rtclock;
	tv->nsec = 0;
#endif
}

/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
	while (us--)
	{
		int i = 5;
		
		while (i--)
			;
	}
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
	return (g_boot_switches & 0x20) != 0;
}

C(machdep_create_aliases)
{
	Retcode ret = NO_ERROR;
	Retcode ret2;

	ret2 = make_devalias(e, "console", CSTR, "/console", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "screen", CSTR, "/display@C0000000", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "keyboard", CSTR, "/keyboard@40010000", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "ttya", CSTR, "/uart@80000480", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "ttyb", CSTR, "/uart@80001480", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "net", CSTR, "/ethernet@20000000", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "disk", CSTR, "/flash@0", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "disk2", CSTR, "/flash@10000000:0", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	ret2 = make_devalias(e, "disk3", CSTR, "/flash@10000000:1", CSTR);
	ret = (ret != NO_ERROR ? ret : ret2);
	return ret;
}

/* This must probe and initialize all the hardware in the system and place
   it in the device tree at the appropriate places.  The default code will
   initialize a PCI bus, and will also install and probes an ISA bus if a
   PCI-ISA bridge chip is found.
 */
CC(machine_probe_all)		/* (--) */
{
	EC(probe_ep7312_display);
	EC(kbd_install);
	EC(uart_install);
	EC(console_install);
	EC(sram_install);
	EC(misc_install);
	EC(install_cs8900a_driver);
	EC(install_flash);
	EC(install_nand);
	EC(ide_install);
	EC(mmu_add_physical_mappings);
	EC(mmu_init_translations);
	Retcode ret;

	mark_probe(".0");
	DPRINTF(("machine_probe_all: enter\n"));
	PUSH(e, "/");
	PUSH(e, 1);
	ret = execute_word(e, "find-device");
	DPRINTF(("machine_probe_all: find-device / ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	prop_set_int(e->currpkg->props, "#address-cells", CSTR, 1);
	prop_set_int(e->currpkg->props, "#size-cells", CSTR, 1);

	strcpy(g_motherboard_name, "EDB7312");
	strcpy(g_motherboard_desc, "Cirrus EDB7312 developement system");

	prop_set_str(e->currpkg->props, "CODEGEN,vendor", CSTR,
		"Cogent Computer Systems, Inc.", CSTR);
	prop_set_str(e->currpkg->props, "CODEGEN,board", CSTR,
		g_motherboard_name, CSTR);
	prop_set_str(e->currpkg->props, "CODEGEN,description", CSTR,
		g_motherboard_desc, CSTR);

	if (*(uByte volatile *)(EDB7312_VA_LPT + 9) == 0x8A)
		prop_set_int(e->currpkg->props, "COGENT,epld-rev", CSTR,
			*(uByte volatile *)(EDB7312_VA_LPT + 8));

	DPRINTF(("machine_probe_all: about to probe display\n"));
	mark_probe(".1");
	ret = probe_ep7312_display(e);
	DPRINTF(("machine_probe_all: probe display ret=%d\n", ret));

	mark_probe(".2");
	if (ret == NO_ERROR)
	    ret = kbd_install(e);

	DPRINTF(("machine_probe_all: probe keyboard ret=%d\n", ret));

	mark_probe(".3");
	if (ret == NO_ERROR)
	    ret = uart_install(e);

	DPRINTF(("machine_probe_all: probe uart ret=%d\n", ret));

	mark_probe(".4");
	if (ret == NO_ERROR)
	    ret = console_install(e);

	DPRINTF(("machine_probe_all: install console ret=%d\n", ret));

	mark_probe(".5");
	if (ret == NO_ERROR)
	    ret = install_cs8900a_driver(e);

	DPRINTF(("before install_flash: ret = %d\n", ret));

	mark_probe(".6");
	if (ret == NO_ERROR)
	    ret = install_flash(e);

	DPRINTF(("after install_flash: ret = %d\n", ret));

	mark_probe(".7");
	if (ret == NO_ERROR)
	    ret = install_nand(e);

	DPRINTF(("after install_nand: ret = %d\n", ret));

	mark_probe(".8");
	if (ret == NO_ERROR)
	    ret = ide_install(e);

	DPRINTF(("after ide_install: ret = %d\n", ret));

	mark_probe(".9");
	if (ret == NO_ERROR)
	    ret = sram_install(e);

	DPRINTF(("after sram_install: ret = %d\n", ret));

	mark_probe(".10");
	if (ret == NO_ERROR)
	    ret = misc_install(e);

	DPRINTF(("after misc_install: ret = %d\n", ret));

	mark_probe(".11");
	if (ret == NO_ERROR)
	    ret = machdep_create_aliases(e);
	else
	    machdep_create_aliases(e);

	DPRINTF(("after create_aliases: ret = %d\n", ret));

	if (get_config_bool(e, "physical-mappings?", CSTR))
	{
		mark_probe(".12");
		if (ret == NO_ERROR)
		    ret = mmu_add_physical_mappings(e);
	}

	mark_probe(".13");
	mmu_init_translations(e);
	mark_probe(".14\n");
	return ret;
}

static void
cscanf(Environ *e, Byte *str)
{
	uInt len;
	expect_line(e, str, STR_SIZE, &len, FALSE);

	if (len < 0)
		str[0] = '\0';
	else if (len > 0 && str[len - 1] == '\n')
		str[len - 1] = '\0';
	else
		str[len] = '\0';
}

static Bool
parse_mac(Environ *e, Byte *str, uByte mac_addr[])
{
	extern Bool parse_mac_addr(Environ *e, Byte **str, uByte mac_addr[],
			const char *errstr);
	Bool ok = parse_mac_addr(e, &str, mac_addr, "boot prompt");

	if (!ok)
		mac_addr[0] = 1;

	return ok;
}

/* parse an IP addr from a string - we expect either the dotted-decimal
   representation x.y.z.q or a hex string with an optional "0x" prefix
 */
static Bool
parse_ip(Environ *e, Byte *str, uByte ip[])
{
	Int i;
	Cell error, val;
	Int len = strlen(str);
	Byte *ostr = str;
	
	for (i = 0; i < 4; i++)
	{
		parse_number(10, &str, &len, &val, &error, FALSE);
		DPRINTF(("parse_ip: str=%s len=%d val=%Cd error=%Cd\n",
				str, len, val, error));
		
		if (!error || 
				(i < 3 && *str == '.') ||
				(i == 3 && (*str == ',' || *str == '\0')))
			ip[i] = (uByte)val;
		else
		{
			/* some sort of error - try parsing it again as a hex string */
			str = ostr;
			len = strlen(str);

			/* skip any optional "0x" prefix */
			if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
			{
				*str += 2;
				len -= 2;
			}

			parse_number(16, &str, &len, &val, &error, FALSE);

			if (!error || *str == ',' || *str == '\0')
			{
				ip[0] = (val >> (3 * BYTE_SIZE)) & BYTE_MASK;
				ip[1] = (val >> (2 * BYTE_SIZE)) & BYTE_MASK;
				ip[2] = (val >> BYTE_SIZE) & BYTE_MASK;
				ip[3] = val & BYTE_MASK;
				break;
			}

			ip[0] = 0;
			ip[1] = 0;
			ip[2] = 0;
			ip[3] = 0;
			return FALSE;
		}
		
		if (*str == '.')		/* leave pointer at comma */
			str++;
	}

	DPRINTF(("parse_ip: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]));
	return TRUE;
}

#define MAC_ADDR_SIZE	6

static Bool
get_mac_addr(Environ *e, Byte *dev, uByte *mac_addr)
{
	Retcode ret;
	Instance *inst;
	Entry *ent;

	/* get the default MAC address from the serial EEPROM */
	IFCKSP(e, 0, 2);
	PUSH(e, dev);
	PUSH(e, strlen(dev));
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return FALSE;

	IFCKSP(e, 1, 0);
	POPT(e, inst, Instance *);

	if (inst == NULL)
		return FALSE;

	ent = find_table(inst->package->props, "local-mac-address", CSTR);

	PUSH(e, (uPtr)inst);
	ret = execute_word(e, "close-dev");

	if (ret != NO_ERROR)
		return FALSE;

	if (!ent || ent->len != MAC_ADDR_SIZE)
		return FALSE;

	memcpy(mac_addr, ent->v.array, MAC_ADDR_SIZE);
	return TRUE;
}

/* Secondary diagnostics run after the console is up for error messages.
   More rigorous testing should go here rather than at bootup just so that
   errors can be displayed */
CC(machine_secondary_diag)	/* (--) */
{
	Retcode ret;
	Bool ok;

	if (g_boot_switches & 0x08)
	{
		Byte str[STR_SIZE];
		uByte def_mac_addr[MAC_ADDR_SIZE];
		uByte mac_addr[MAC_ADDR_SIZE];
		uByte client_ip[4];
		uByte server_ip[4];

		g_boot_switches &= ~0x20;		/* turn diag-switch off temporarily */

		/* get the default MAC address from the serial EEPROM */
		if (!get_mac_addr(e, "net", def_mac_addr))
			memset(def_mac_addr, '\0', MAC_ADDR_SIZE);

		/* if the MAC address is bogus then clear it */
		if (def_mac_addr[0] & 3)
			memset(def_mac_addr, '\0', MAC_ADDR_SIZE);

		memcpy(mac_addr, def_mac_addr, MAC_ADDR_SIZE);

		/* default client and server IP addresses for Cogent */
		client_ip[0] = 192;
		client_ip[1] = 168;
		client_ip[2] = 0;
		client_ip[3] = 88;
		server_ip[0] = 192;
		server_ip[1] = 168;
		server_ip[2] = 0;
		server_ip[3] = 89;

		while (1)
		{
			Bool def = FALSE;

			/* prompt for MAC address */
			if (mac_addr[0] || mac_addr[1] || mac_addr[2] ||
					mac_addr[3] || mac_addr[4] || mac_addr[5])
				def = TRUE;

			if (def)
				cprintf(e, "Enter MAC address: [%02X:%02X:%02X:%02X:%02X:%02X] ",
						mac_addr[0], mac_addr[1], mac_addr[2],
						mac_addr[3], mac_addr[4], mac_addr[5]);
			else
				cprintf(e, "Enter MAC address: ");

			cscanf(e, str);

			if (def && str[0] == '\0')
				break;

			ok = parse_mac(e, str, mac_addr);

			if (ok && (mac_addr[0] & 0x3) == 0)
				break;
		}

		while (1)
		{
			/* prompt for Client-IP address */
			cprintf(e, "Enter Client-IP address: [%d.%d.%d.%d] ", client_ip[0],
					client_ip[1], client_ip[2], client_ip[3]);
			cscanf(e, str);

			if (str[0] == '\0')
				break;

			ok = parse_ip(e, str, client_ip);

			if (ok)
				break;
		}

		while (1)
		{
			/* prompt for Server-IP address */
			cprintf(e, "Enter Server-IP address: [%d.%d.%d.%d] ", server_ip[0],
					server_ip[1], server_ip[2], server_ip[3]);
			cscanf(e, str);

			if (str[0] == '\0')
				break;

			ok = parse_ip(e, str, server_ip);

			if (ok)
				break;
		}

		if (def_mac_addr[0] != mac_addr[0] ||
				def_mac_addr[1] != mac_addr[1] ||
				def_mac_addr[2] != mac_addr[2] ||
				def_mac_addr[3] != mac_addr[3] ||
				def_mac_addr[4] != mac_addr[4] ||
				def_mac_addr[5] != mac_addr[5])
		{
			/* initialize serial EEPROM and set MAC address */
			bprintf(str, "cd net");
			ret = interp_text(e, str, CSTR);
			bprintf(str, "format-eeprom forth");
			ret = interp_text(e, str, CSTR);
			bprintf(str, "set-mac-address net %02X:%02X:%02X:%02X:%02X:%02X",
					mac_addr[0], mac_addr[1], mac_addr[2],
					mac_addr[3], mac_addr[4], mac_addr[5]);
			ret = interp_text(e, str, CSTR);

			if (get_mac_addr(e, "net", mac_addr))
				cprintf(e, "MAC address set to %02X:%02X:%02X:%02X:%02X:%02X\n",
						mac_addr[0], mac_addr[1], mac_addr[2],
						mac_addr[3], mac_addr[4], mac_addr[5]);
			else
				cprintf(e, "Failed to set MAC address\n");
		}

		bprintf(str, "%d.%d.%d.%d", client_ip[0], client_ip[1], client_ip[2],
				client_ip[3]);
		save_config(e, "client-ip", CSTR, str, CSTR, FALSE);
		bprintf(str, "%d.%d.%d.%d", server_ip[0], server_ip[1], server_ip[2],
				server_ip[3]);
		save_config(e, "server-ip", CSTR, str, CSTR, FALSE);
		g_boot_switches |= 0x20;		/* turn diag-switch back on */
	}

	return NO_ERROR;
}

/* set e->load to a reasonable place in memory with a lot of room
 */
CC(machine_init_load)
{
	/* load after whatever we want to reserve for the stack */
	e->load = (Byte*)(uPtr)get_config_int(e, "load-base", CSTR);
	return NO_ERROR;
}

/* setup to launch a program at e->load - verify known executable type
 */
CC(machine_init_program)	/* (--) */
{
	return exec_is_exec(e) ? NO_ERROR : E_BAD_IMAGE;
}

int
launch(void *r0, int r1, char *r2[], char *r3[], uInt entrypoint)
{
	asm("	ldr	r4, [fp, #4]");
	asm("	mov	pc,r4");
	return 0;
}

Byte *g_load_high;

/* launch the image at e->load
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
		"system_type=Cirrus EP7312",
		"baseboard=dddddddddddddddddddddddddddddddddddddddd",
		NULL
	};

	bprintf(envv[0], "client_interface=0x%X", arm_client_interface);
	argv[argc + 1] = (char*)arm_client_interface;

	bprintf(envv[1], "memsize=%d", g_machine_memory_size / 1024 / 1024);
	argv[argc + 2] = (char*)g_machine_memory_size;

	bprintf(envv[3], "baseboard=%s", g_motherboard_desc);

	if (!exec_is_exec(e))		/* sanity check */
		return E_BAD_IMAGE;

	ret = exec_load(e);

	/* -1 means we are not supposed to launch through e->entrypoint */
	if (ret != NO_ERROR || e->entrypoint == -1)
		return ret;

	/* load symbols from symtab */
	exec_free_symbols(e, e->loadsyms);
	e->loadsyms = exec_load_symbols(e);

	DPRINTF(("entrypoint=%#lx...", e->entrypoint));

	/* unmap unused memory */
	if (g_load_high)
	{
		uPtr high = (uPtr)g_load_high;
		high = (high + 4096) & ~4095;
		IFCKSP(e, 0, 2);
		PUSH(e, high);
		PUSH(e, 0xF0F00000 - high);
		ret = execute_static_method_name(e, e->mmu, "unmap", CSTR);

		if (ret != NO_ERROR)
		{
			cprintf(e, "mmu->unmap failed: %s (%d)\n", err2str(ret), ret);
			return ret;
		}
	}

	DPRINTF(("flushing caches..."));
	flush_caches();

	/* should not return */
	DPRINTF(("launching...\n"));
	DPRINTF(("ep=%#x\n", e->entrypoint));
	ret = launch((void*)arm_client_interface, argc, argv, envv, e->entrypoint);

	DPRINTF(("launch failed: %s (%d)\n", err2str(ret), ret));
	machine_reset_all(e);
	return ret;
}

/* hack so exe image loaders can claim/map memory as per ARM binding
   - assumes loaded images are contiguous in memory
 */
static Retcode
do_claim_memory(Environ *e, Byte *addr, uInt len, Bool req)
{
	Byte *phys;
	Byte *retaddr;
	Retcode ret;

	DPRINTF(("do_claim_memory: e %p addr %p len %#x end %p\n", e, addr, len, addr + len - 1));

	/* get physical address */
	phys = (char*)((uPtr)addr - 0xF0000000 + 0xC0100000);

	/* claim addrs from memory */
	IFCKSP(e, 0, 3);
	PUSHP(e, phys);
	PUSHP(e, len);
	PUSH(e, 0);	/* alignment */
	ret = execute_static_method_name(e, e->memory, "claim", CSTR);

	if (ret != NO_ERROR)
	{
		if (!req)
			return NO_ERROR;

		cprintf(e, "memory->claim failed: %s (%d): %p %#x\n", err2str(ret), ret, phys, len);
		return ret;
	}
	
	POPT(e, retaddr, Byte*);

	/* sanity check - make sure the memory we requested is indeed claimed */
	if (retaddr != phys)
	{
		cprintf(e, "memory->claim failed: %p != %p\n", retaddr, phys);
		return E_INVALID_MEMORY;
	}

	/* claim virtual addrs from mmu */
	IFCKSP(e, 0, 3);
	PUSHP(e, addr);
	PUSHP(e, len);
	PUSHP(e, 0);	/* alignment */
	ret = execute_static_method_name(e, e->mmu, "claim", CSTR);

	if (ret != NO_ERROR)
	{
		cprintf(e, "mmu->claim failed: %s (%d): %p %#x\n", err2str(ret), ret, addr, len);
		return ret;
	}

	POPT(e, retaddr, Byte*);

	/* sanity check - make sure the memory we requested is indeed claimed */
	if (retaddr != addr)
	{
		cprintf(e, "mmu->claim failed: %p != %p\n", retaddr, addr);
		return E_INVALID_MEMORY;
	}

	return NO_ERROR;
}

Retcode
machine_claim_memory(Environ *e, Byte *addr, uInt len)
{
	Retcode ret;

	DPRINTF(("machine_claim_memory: e %p addr %p len %#x end %p\n", e, addr, len, addr + len - 1));

	/* if region isn't in system memory space then don't do anything */
	if ((uPtr)addr < 0xF0000000 || (uPtr)addr + len > 0xF0F00000 || len == 0)
		return NO_ERROR;

	/* set high-water mark to unmap unused memory */
	if (addr + len > g_load_high)
		g_load_high = addr + len;

	if ((uPtr)addr & 0xFFF)
	{
		/* handle the first unaligned page */
		uInt l2 = 0x1000 - ((uPtr)addr & 0xFFF);

		if (l2 > len)
			l2 = len;

		/* claim on page aligned page, ignore errors */
		do_claim_memory(e, (Byte *)((uPtr)addr & ~0xFFF), 0x1000, FALSE);
		len -= l2;
		addr += l2;

		if (len == 0)
			return NO_ERROR;
	}

	if (len >= 0x1000)
	{
		ret = do_claim_memory(e, addr, len & ~0xFFF, TRUE);

		if (ret != NO_ERROR)
			return ret;
	}

	addr += len & ~0xFFF;
	len -= len & ~0xFFF;

	if (len)
	{
		/* claim on page aligned page at the end, ignore errors */
		do_claim_memory(e, addr, 0x1000, FALSE);
	}

	return NO_ERROR;
}

/* execute the specified client callback function - this will work for
   C code
 */
int
machine_callback(Environ *e, Callback *func, Cell *array)
{
	return (*func)(array);		/* call it - machine-dependent? */
}

/* push the font data on the stack, however it is gathered
 */
CC(machine_font)		/* (-- addr width height advance min-char #glyphs) */
{
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

/* calculate a simple checksum of all currently used memory including the
   code, data, and bss segments, but not the stack - the stack may contain
   who knows what from function calls
 */
static int
calc_csum(Environ *e)
{
	extern char start[], end[];
	int *p = (int*)start;
	int *pe = (int*)end;
	int csum = 0;

	while (p < pe)
		csum += *p++;

	p = (int*)g_machine_memory;
	pe = (int*)(((uPtr)g_machine_memory + g_machine_memory_used) & ~0x3);

	while (p < pe)
		csum += *p++;

	return csum;
}

CC(f_edb7312_banner)		/* banner (--) */
{
	Instance *screen = (Instance*)(uPtr)e->screen;
	Bool oemlogo = FALSE;
	Byte *oem;
	Int res, i;
	Int leading = 0;
	Int width = e->logo_width;
	Int height = e->logo_height;
	Bool logo = FALSE;
	Byte *str;
	Int slen;

	e->no_banner = TRUE;	/* suppress default script */

	if (screen && screen->package &&
			prop_get_str(screen->package->props, "device_type",
					CSTR, &str, &slen) == NO_ERROR &&
			compare_strs(str, slen, "display", CSTR))
	{
		oemlogo = get_config_bool(e, "oem-logo?", CSTR);

		if (oemlogo)
		{
			width = 64;
			height = 64;
		}

		logo = width > 0 && height > 0;
		
		/* indent following text lines */
		if (logo)
			leading = width / e->cwidth + 1;
	}

	e->linesout = 0;

	if (leading)
		cprintf(e, "\033[%dC", leading);

	res = get_config_bool(e, "oem-banner?", CSTR);

	if (res == FTRUE)
	{
		oem = get_config(e, "oem-banner", CSTR);

		if (oem)
			cprintf(e, "%s\n", oem);
	}
	else
	{
		cprintf(e, "Welcome to SmartFirmware\n");

		if (leading)
			cprintf(e, "\033[%dC", leading);

		cprintf(e, "%s %s | v%s\n",
				"Cirrus", MACHINE_TYPE, FIRMWARE_REV);

#ifdef FIRMWARE_BUILD
		if (leading)
			cprintf(e, "\033[%dC", leading);

		cprintf(e, "Built %s\n", FIRMWARE_BUILD);
#endif
	}

	/* THE CODEGEN COPYRIGHT LINES MAY NOT BE DELETED FROM ANY FINAL IMAGE. */
	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, "Copyright 1996-2001 by\n");

	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, "CodeGen, Inc.\n");

	if (leading)
		cprintf(e, "\033[%dC", leading);

	cprintf(e, "All Rights Reserved.\n\n");
	cprintf(e, "Goto <http://www.codegen.com/EDB7312/>\n");
	cprintf(e, "for firmware support and updates.\n");

	if (EP7312_R(UNIQID) == 0 && EP7312_R(RANDID0) == 0 &&
			EP7312_R(RANDID1) == 0 && EP7312_R(RANDID2) == 0 &&
			EP7312_R(RANDID3) == 0)
		cprintf(e, "Warning: CPU maybe older than rev. C\n");

	if (logo)
	{
		extern Byte fb7312_logo_pixmap[];

		for (i = height / e->cheight + 1 - e->linesout; i > 0; i--)
			display_char(e, '\n');

		IFCKSP(e, 0, 4);
		PUSH(e, e->curline - e->linesout);	/* previous line position */
		oem = fb7312_logo_pixmap;

		if (oemlogo)
			(void)get_config_val(e, "oem-logo", CSTR, &oem, &i);

		PUSHP(e, oem);		/* addr */
		PUSH(e, width);		/* width */
		PUSH(e, height);	/* height */
		execute_method_name(e, screen, "draw-logo", CSTR);
	}

	return NO_ERROR;
}

CC(f_write_mem_test)		/* write-mem-test (size iterations do-csum --) */
{
	register int csum = 0;
	int i, docsum;
	int *p;
	int *up;
	int size, count, pg, pgs;
	int indx[4096 * 4];

	IFCKSP(e, 3, 0);
	POP(e, docsum);
	POP(e, count);
	POP(e, size);

	if (!size)
		size = 4 * 1024 * 1024;
	else
		size *= 1024;

	pgs = size / 0x1000;
	size = pgs * 0x1000;

	p = (int*)(((uPtr)g_machine_memory + 2 * g_machine_memory_used + 0x1000) &
			~0xFFF);
	up = (int*)(uPtr)p;

	cprintf(e, "g_machine_memory=%#x g_machine_memory_used=%#x\n",
			g_machine_memory, g_machine_memory_used);
	cprintf(e, "e=%#x p=%#x up=%#x size=%#x pgs=%#x\n",
			e, p, up, size, pgs);

	/* fill in index */
	for (i = 0; i < pgs; i++)
		indx[i] = i;

	for (i = pgs - 1; i > 2; i--)
	{
		int j = rand() % (i - 1);
		int t = indx[i];
		indx[i] = indx[j];
		indx[j] = t;
	}

	/* calculate checksum of all of used memory */
	if (docsum)
	{
		memcpy(g_machine_memory + g_machine_memory_used,
			g_machine_memory, g_machine_memory_used);
		csum = calc_csum(e);
	}

	/* initialize all of memory */
	for (i = 0; i < size / sizeof *p; i++)
		p[i] = (i << (i & 7)) + i;

	for (i = 0; i < count; i++)
	{
		for (pg = 0; pg < pgs / 2; pg++)
		{
			int j, s, e;

			flush_caches();
			s = indx[pg] * 0x1000 / sizeof *p;
			e = s + 0x1000 / sizeof *p;

			for (j = s; j < e; j += 4)
			{
				p[j] = 0;
				p[j + 1] = 0;
				p[j + 2] = 0;
				p[j + 3] = 0;
			}

			flush_caches();

			/* fill the center chunk of memory */
			for (j = s; j < e; j++)
				p[j] = (j << (j & 7)) + j;
		}

#if 0
		cprintf(e, "iter %d (pg=%d)\n", i, pg);
#endif
		machine_led_write(e, i);
	}

	/* make cache and memory coherent */
	flush_caches();

	/* check the uncached memory */
	for (i = 0; i < size / sizeof *p; i++)
		if (up[i] != (i << (i & 7)) + i)
			cprintf(e, "uncached error at %#x: found 0x%d, expected %#x!\n",
					i, up[i], (i << (i & 7)) + i);

	/* check the cached memory */
	for (i = 0; i < size / sizeof *p; i++)
		if (p[i] != (i << (i & 7)) + i)
			cprintf(e, "error at %#x: found %#x, expected %#x!\n",
					i, p[i], (i << (i & 7)) + i);

	/* and verify the checksum, ie that used memory was not touched */
	if (docsum && (i = calc_csum(e)) != csum)
	{
		uByte *x = (uByte *)g_machine_memory;
		uByte *y = (uByte *)g_machine_memory + g_machine_memory_used;
		cprintf(e, "used memory modified - checksum %#x != %#x\n", i, csum);

		for (i = 0; i < g_machine_memory_used; i++)
			if (x[i] != y[i])
			{
				cprintf(e, "first difference at %#x (%d, %#x)\n", &x[i], i, i);
				break;
			}
	}

	/* final sanity check since we know that g_e should always be e */
	if (g_e != e)
		cprintf(e, "global var g_e trashed - %#x != %#x\n", g_e, e);

	return NO_ERROR;
}

C(f_g_e)
{
	IFCKSP(e, 0, 1);
	PUSHP(e, g_e);
	return NO_ERROR;
}

void
dump_bits(Environ *e, char *name, struct bitfield *bits, uInt value)
{
	int i;
	int first = 1;

	cprintf(e, "%s = %#x [", name, value);

	for (i = 0; bits[i].name; i++)
		if (bits[i].width || (value & (1 << bits[i].bit)))
		{
			if (!first)
				cprintf(e, ", ");
			else
				first = 0;

			if (bits[i].width == 0)
				cprintf(e, "%s", bits[i].name);
			else if (bits[i].values)
				cprintf(e, "%s=%s", bits[i].name, 
						bits[i].values[(value >> bits[i].bit) & ((1 << bits[i].width) - 1)]);
			else
				cprintf(e, "%s=%d", bits[i].name, 
						(value >> bits[i].bit) & ((1 << bits[i].width) - 1));
		}

	cprintf(e, "]\n");
}

struct bitfield pxdr_bits[] =
{
	{ "bit0", 0 },
	{ "bit1", 1 },
	{ "bit2", 2 },
	{ "bit3", 3 },
	{ "bit4", 4 },
	{ "bit5", 5 },
	{ "bit6", 6 },
	{ "bit7", 7 },
	{ NULL }
};

struct bitfield pbdr_bits[] =
{
	{ "SMCpres", 0, 1 },
	{ "RTS", 1 },
	{ "RI", 2 },
	{ "B3", 3 },
	{ "NANDcle", 4, 1 },
	{ "NANDale", 5, 1 },
	{ "MBC", 6, 1 },
	{ "SMCcs", 7 },
	{ NULL }
};

struct bitfield pddr_bits[] =
{
	{ "DiagLED", 0 },
	{ "LCDcontE", 1 },
	{ "LCDE", 2 },
	{ "LCDbackE", 3 },
	{ "I2CSDA", 4 },
	{ "I2CSCL", 5 },
	{ NULL }
};

struct bitfield pedr_bits[] =
{
	{ "CODECenb", 0 },
	{ "USBsusp", 1 },
	{ "E2", 2 },
	{ NULL }
};

struct bitfield pxddr_bits[] =
{
	{ "out0", 0 },
	{ "out1", 1 },
	{ "out2", 2 },
	{ "out3", 3 },
	{ "out4", 4 },
	{ "out5", 5 },
	{ "out6", 6 },
	{ "out7", 7 },
	{ NULL }
};

struct bitfield pdddr_bits[] =
{
	{ "in0", 0 },
	{ "in1", 1 },
	{ "in2", 2 },
	{ "in3", 3 },
	{ "in4", 4 },
	{ "in5", 5 },
	{ "in6", 6 },
	{ "in7", 7 },
	{ NULL }
};

struct bitfield syscon1_bits[] =
{
	{ "keyscan", 0, 4 },
	{ "TC1M", 4 },
	{ "TC1S", 5 },
	{ "TC2M", 6 },
	{ "TC2S", 7 },
	{ "UART1EN", 8 },
	{ "BZTOG", 9 },
	{ "BZMOD", 10 },
	{ "DBGEN", 11 },
	{ "LCDEN", 12 },
	{ "CDENTX", 13 },
	{ "CDENRX", 14 },
	{ "SIREN", 15 },
	{ "ADCKSEL", 16, 2 },
	{ "EXCKEN", 18 },
	{ "WAKEDIS", 19 },
	{ "IRTXM", 20 },
	{ NULL }
};

struct bitfield syscon2_bits[] =
{
	{ "SERSEL", 0 },
	{ "KBD6", 1 },
	{ "DRAMZ", 2 },
	{ "KBWEN", 3 },
	{ "SS2TXEN", 4 },
	{ "PCCARD1", 5 },
	{ "PCCARD2", 6 },
	{ "SS2RXEN", 7 },
	{ "UART2EN", 8 },
	{ "SS2MAEN", 9 },
	{ "OSTB", 12 },
	{ "CLKENSL", 13 },
	{ "BUZFREQ", 14 },
	{ NULL }
};

struct bitfield syscon3_bits[] =
{
	{ "ADCCON", 0 },
	{ "CLKCTL", 1, 2 },
	{ "DAISEL", 3 },
	{ "ADCCKNSEN", 4 },
	{ "VERSN", 5, 3 },
	{ "128FS", 9 },
	{ "ENPD67", 10 },
	{ NULL }
};

struct bitfield sysflg1_bits[] =
{
	{ "MCDR", 0 },
	{ "DCDET", 1 },
	{ "WUDR", 2 },
	{ "WUON", 3 },
	{ "DID", 4, 4 },
	{ "CTS", 8 },
	{ "DSR", 9 },
	{ "DCD", 10 },
	{ "UBUSY1", 11 },
	{ "NBFLG", 12 },
	{ "RSTFLG", 13 },
	{ "PFFLG", 14 },
	{ "CLDFLG", 15 },
	{ "RTCDIV", 16, 6 },
	{ "URXFE1", 22 },
	{ "UTXFF1", 23 },
	{ "CRXFE", 24 },
	{ "CTXFF", 25 },
	{ "SSIBUSY", 26 },
	{ "BOOTBIT", 27, 2 },
	{ "ID", 29 },
	{ "VERID", 30, 2 },
	{ NULL }
};

struct bitfield sysflg2_bits[] =
{
	{ "SS2RXOF", 0 },
	{ "RESVAL", 1 },
	{ "RESFRM", 2 },
	{ "SS2RXFE", 3 },
	{ "SS2TXFF", 4 },
	{ "SS2TXUF", 5 },
	{ "CKMODE", 6 },
	{ "UBUSY2", 11 },
	{ "URXFE2", 22 },
	{ "UTXFF2", 23 },
	{ NULL }
};

struct bitfield intsr1_bits[] =
{
	{ "EXTFIQ", 0 },
	{ "BLINT", 1 },
	{ "WEINT", 2 },
	{ "MCINT", 3 },
	{ "CSINT", 4 },
	{ "EINT1", 5 },
	{ "EINT2", 6 },
	{ "EINT3", 7 },
	{ "TC1OI", 8 },
	{ "TC2OI", 9 },
	{ "RTCMI", 10 },
	{ "TINT", 11 },
	{ "UTXINT1", 12 },
	{ "URXINT1", 13 },
	{ "UMSINT", 14 },
	{ "SSEOTI", 15 },
	{ NULL }
};

struct bitfield intsr2_bits[] =
{
	{ "KBDINT", 0 },
	{ "SS2RX", 1 },
	{ "SS2TX", 2 },
	{ "UTXINT2", 12 },
	{ "URXINT2", 13 },
	{ NULL }
};

struct bitfield intsr3_bits[] =
{
	{ "DAIINT", 0 },
	{ NULL }
};

struct bitfield memcfg_bits[] =
{
	{ "BUSWIDTH", 0, 2 },
	{ "WAITSTATES", 2, 4 },
	{ "SQAEN", 6 },
	{ "CLKENB", 7 },
	{ NULL }
};

struct bitfield ledflsh_bits[] =
{
	{ "FlashRate", 0, 2 },
	{ "DutyRatio", 2, 4 },
	{ "Enable", 6 },
	{ NULL }
};

struct bitfield sdconf_bits[] =
{
	{ "CASLAT", 0, 2 },
	{ "SDSIZE", 5, 2 },
	{ "SDWIDTH", 7, 2 },
	{ "CLKCTL", 9 },
	{ "SDACTIVE", 10 },
	{ NULL }
};

struct bitfield lcdcon_bits[] =
{
	{ "VidBufSz", 0, 13 },
	{ "LineLen", 13, 6 },
	{ "PixelPreScale", 19, 6 },
	{ "ACprescale", 19, 5 },
	{ "GSMD1", 30 },
	{ "GSMD2", 31 },
	{ NULL }
};

static char *chip_select_names[] =
{
	"Flash",
	"NAND Flash",
	"Ethernet",
	"LPT/KeyExtRow",
	"USB/PS2",
	"IDE",
	"SRAM",
	"BootROM",
};

C(f_chip_selects)
{
	uInt temp = 0;
	uInt bootbits = (EP7312_R(SYSFLG1) >> 27) & 0x3;
	int i;
	int buswidth;
	int waitstates;
	int wait18r;
	int wait18s;
	int wait36r;
	int wait36s;
	int sqaen;
	int clkenb;
	char label[40];

	for (i = 0; i < 8; i++)
	{
		if (i == 0)
		    temp = EP7312_R(MEMCFG1);
		else if (i == 4)
		    temp = EP7312_R(MEMCFG2);
		else
		    temp >>= 8;

		buswidth = temp & 0x3;
		waitstates = (temp >> 2) & 0xF;
		sqaen = (temp >> 6) & 1;
		clkenb = (temp >> 7) & 1;

		switch ((bootbits << 2) | buswidth)
		{
		case 0: buswidth = 32; break;
		case 1: buswidth = 16; break;
		case 2: buswidth = 8; break;
		case 3: buswidth = 0; break;
		case 4: buswidth = 8; break;
		case 5: buswidth = 0; break;
		case 6: buswidth = 32; break;
		case 7: buswidth = 16; break;
		case 8: buswidth = 16; break;
		case 9: buswidth = 32; break;
		case 10: buswidth = 0; break;
		case 11: buswidth = 8; break;
		case 12: buswidth = 0; break;
		case 13: buswidth = 0; break;
		case 14: buswidth = 0; break;
		case 15: buswidth = 0; break;
		}

		wait18r = 4 - (waitstates & 3);
		wait18s = 3 - ((waitstates >> 2) & 3);
		wait36r = 8 - (waitstates & 7);
		wait36s = 3 - ((waitstates >> 2) & 3);

		bprintf(label, "%s:", chip_select_names[i]);
		cprintf(e, "%-14s %02X: buswidth %2d, wait 18MHz %d,%d 36MHz %d,%d, sqaen %d, clkenb %d\n",
				label, temp & 0xFF, buswidth,
				wait18r, wait18s, wait36r, wait36s, sqaen,
				clkenb);
	}

	return NO_ERROR;
}

C(f_dump_ep7312)
{
	uInt temp;
	dump_bits(e, "PADR", pxdr_bits, EP7312_RB(PADR));
	dump_bits(e, "PBDR", pbdr_bits, EP7312_RB(PBDR));
	dump_bits(e, "PDDR", pddr_bits, EP7312_RB(PDDR));
	dump_bits(e, "PEDR", pedr_bits, EP7312_RB(PEDR));
	dump_bits(e, "PADDR", pxddr_bits, EP7312_RB(PADDR));
	dump_bits(e, "PBDDR", pxddr_bits, EP7312_RB(PBDDR));
	dump_bits(e, "PDDDR", pdddr_bits, EP7312_RB(PDDDR));
	dump_bits(e, "PEDDR", pxddr_bits, EP7312_RB(PEDDR));

	dump_bits(e, "SYSCON1", syscon1_bits, EP7312_R(SYSCON1));
	dump_bits(e, "SYSFLG1", sysflg1_bits, EP7312_R(SYSFLG1));
	temp = EP7312_R(MEMCFG1);
	cprintf(e, "MEMCFG1 = %#x\n", temp);
	dump_bits(e, "MEMCFG1 - CS0", memcfg_bits, temp & 0xFF);
	dump_bits(e, "MEMCFG1 - CS1", memcfg_bits, (temp >> 8) & 0xFF);
	dump_bits(e, "MEMCFG1 - CS2", memcfg_bits, (temp >> 16) & 0xFF);
	dump_bits(e, "MEMCFG1 - CS3", memcfg_bits, (temp >> 24) & 0xFF);
	temp = EP7312_R(MEMCFG2);
	cprintf(e, "MEMCFG2 = %#x\n", temp);
	dump_bits(e, "MEMCFG2 - CS4", memcfg_bits, temp & 0xFF);
	dump_bits(e, "MEMCFG2 - CS5", memcfg_bits, (temp >> 8) & 0xFF);
	dump_bits(e, "MEMCFG2 - SRAM", memcfg_bits, (temp >> 16) & 0xFF);
	dump_bits(e, "MEMCFG2 - BootROM", memcfg_bits, (temp >> 24) & 0xFF);
	dump_bits(e, "INTSR1", intsr1_bits, EP7312_R(INTSR1));
	dump_bits(e, "INTMR1", intsr1_bits, EP7312_R(INTMR1));
	dump_bits(e, "LCDCON", lcdcon_bits, EP7312_R(LCDCON));
	cprintf(e, "TC1D = %#x\n", EP7312_R(TC1D));
	cprintf(e, "TC2D = %#x\n", EP7312_R(TC2D));
	cprintf(e, "RTCDR = %#x\n", EP7312_R(RTCDR));
	cprintf(e, "RTCMR = %#x\n", EP7312_R(RTCMR));
	cprintf(e, "PMPCON = %#x\n", EP7312_R(PMPCON));
	cprintf(e, "CODR = %#x\n", EP7312_R(CODR));
	cprintf(e, "UARTDR1 = %#x\n", EP7312_R(UARTDR1));
	cprintf(e, "UBLCR1 = %#x\n", EP7312_R(UBLCR1));
	cprintf(e, "SYNCIO = %#x\n", EP7312_R(SYNCIO));
	cprintf(e, "PALLSW = %#x\n", EP7312_R(PALLSW));
	cprintf(e, "PALMSW = %#x\n", EP7312_R(PALMSW));

/*	cprintf(e, "STFCLR = %#x\n", EP7312_R(STFCLR));
	cprintf(e, "BLEOI = %#x\n", EP7312_R(BLEOI));
	cprintf(e, "MCEOI = %#x\n", EP7312_R(MCEOI));
	cprintf(e, "TEOI = %#x\n", EP7312_R(TEOI));
	cprintf(e, "TC1EOI = %#x\n", EP7312_R(TC1EOI));
	cprintf(e, "TC2EOI = %#x\n", EP7312_R(TC2EOI));
	cprintf(e, "RTCEOI = %#x\n", EP7312_R(RTCEOI));
	cprintf(e, "UMSEOI = %#x\n", EP7312_R(UMSEOI));
	cprintf(e, "COEOI = %#x\n", EP7312_R(COEOI));
	cprintf(e, "HALT = %#x\n", EP7312_R(HALT));
	cprintf(e, "STDBY = %#x\n", EP7312_R(STDBY));	*/

	cprintf(e, "FBADDR = %#x\n", EP7312_R(FBADDR));
	dump_bits(e, "SYSCON2", syscon2_bits, EP7312_R(SYSCON2));
	dump_bits(e, "SYSFLG2", sysflg2_bits, EP7312_R(SYSFLG2));
	dump_bits(e, "INTSR2", intsr2_bits, EP7312_R(INTSR2));
	dump_bits(e, "INTMR2", intsr2_bits, EP7312_R(INTMR2));
	cprintf(e, "UARTDR2 = %#x\n", EP7312_R(UARTDR2));
	cprintf(e, "UBLCR2 = %#x\n", EP7312_R(UBLCR2));
	cprintf(e, "SS2DR = %#x\n", EP7312_R(SS2DR));
	cprintf(e, "SRXEOF = %#x\n", EP7312_R(SRXEOF));
	cprintf(e, "SS2POP = %#x\n", EP7312_R(SS2POP));
	cprintf(e, "KBDEOI = %#x\n", EP7312_R(KBDEOI));
	cprintf(e, "DAIR = %#x\n", EP7312_R(DAIR));
	cprintf(e, "DAIDR0 = %#x\n", EP7312_R(DAIDR0));
	cprintf(e, "DAIDR1 = %#x\n", EP7312_R(DAIDR1));
/*	cprintf(e, "DAIDR2 = %#x\n", EP7312_R(DAIDR2));	*/
	cprintf(e, "DAISR = %#x\n", EP7312_R(DAISR));
	dump_bits(e, "SYSCON3", syscon3_bits, EP7312_R(SYSCON3));
	dump_bits(e, "INTSR3", intsr3_bits, EP7312_R(INTSR3));
	dump_bits(e, "INTMR3", intsr3_bits, EP7312_R(INTMR3));
	dump_bits(e, "LEDFLSH", ledflsh_bits, EP7312_R(LEDFLSH));
	dump_bits(e, "SDCONF", sdconf_bits, EP7312_R(SDCONF));
	cprintf(e, "SDRFPR = %#x\n", EP7312_R(SDRFPR));
	cprintf(e, "UNIQID = %#x\n", EP7312_R(UNIQID));
	cprintf(e, "RANDID0 = %#x\n", EP7312_R(RANDID0));
	cprintf(e, "RANDID1 = %#x\n", EP7312_R(RANDID1));
	cprintf(e, "RANDID2 = %#x\n", EP7312_R(RANDID2));
	cprintf(e, "RANDID3 = %#x\n", EP7312_R(RANDID3));
	cprintf(e, "DAI64FS = %#x\n", EP7312_R(DAI64FS));
	return NO_ERROR;
}

C(f_pkgq)
{
	Package *pkg;

	IFCKSP(e, 1, 1);
	POPT(e, pkg, Package *);
	PUSH(e, pkg);
	cprintf(e, "%#x: { parent %#x, children %#x, link %#x, dict %#x, props %#x\n",
			pkg, pkg->parent, pkg->children, pkg->link, pkg->dict, pkg->props);
	cprintf(e, "\tinitinst %#x, disp %#x, self %#x }\n", pkg->initinst, pkg->disp,
			pkg->self);
	return NO_ERROR;
}

C(f_instq)
{
	Instance *inst;

	IFCKSP(e, 1, 1);
	POPT(e, inst, Instance *);
	PUSH(e, inst);
	cprintf(e, "%#x: { parent %#x, unit[%#x %#x %#x %#x],\n", inst, inst->parent,
			inst->unit[0], inst->unit[1], inst->unit[2], inst->unit[3]);
	cprintf(e, "\tprobe[%#x %#x %#x %#x], numunits %d, args %#x\n", 
			inst->probe[0], inst->probe[1], inst->probe[2], inst->probe[3],
			inst->numunits, inst->args);
	cprintf(e, "\tpackage %#x, dict %#x, interposed %d, self %#x\n",
			inst->package, inst->dict, inst->interposed, inst->self);
	return NO_ERROR;
}

EC(f_push_config_bool);
EC(f_push_config_int);

static const Initentry init_machdep[] =
{
	{ "write-mem-test", f_write_mem_test, INVALID_FCODE },
	{ "g_e", f_g_e, INVALID_FCODE },

	{ "load-base", f_push_config_int, INVALID_FCODE, F_NONE, T_FUNC },
	{ "banner", f_edb7312_banner, INVALID_FCODE, F_NONE, T_FUNC },
	{ "dump-ep7312", f_dump_ep7312, INVALID_FCODE, F_NONE, T_FUNC },
	{ "chip-selects", f_chip_selects, INVALID_FCODE, F_NONE, T_FUNC },
	{ "pkg?", f_pkgq, INVALID_FCODE, F_NONE, T_FUNC },
	{ "inst?", f_instq, INVALID_FCODE, F_NONE, T_FUNC },

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
extern const Initentry init_fsutils[];
extern const Initentry init_nvedit[];
extern const Initentry init_other[];
extern const Initentry init_device[];
extern const Initentry init_debug[];
extern const Initentry init_fb[];
extern const Initentry init_arm[];
extern const Initentry init_fb7312[];
extern const Initentry init_filesystem[];
extern const Initentry init_tokenizer[];
extern const Initentry init_mmu[];

const Initentry* init_list[] =
{
	init_funcs,
	init_funcs64,
	init_control,
	init_packages,
	init_forth,
	init_admin,
	init_fsutils,
	init_nvedit,
	init_other,
	init_device,
	init_debug,
	init_fb,
	init_arm,
	init_fb7312,
	init_filesystem,
	init_machdep,
	init_mmu,
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
EC(install_cpu);
EC(install_cpu_arm);
EC(install_failsafe);
EC(install_display);
EC(install_obptftp);
EC(install_deblocker);
EC(install_disklabel);
EC(init_options_from_nvram);	/* forward to definition down below */
EC(install_client_services);

const Command install_list[] =
{
	install_root,		/* should be first */
	install_chosen,		/* should be second */
	install_memory,		/* should be third */
	install_cpu_arm,
	init_options_from_nvram,
/*	install_failsafe,		*/
	install_display,
	install_deblocker,
	install_disklabel,
	install_obptftp,
	install_client_services,
	NULL
};

/* List all supported builtin filesystems.  This is not standard OpenFirmware
   but is useful in that disk partitions will be displayed as part of the
   device tree under the builtin disk devices.
 */
extern Filesys g_dos_partition;
extern Filesys g_dos_fat;
extern Filesys g_bsd_partition;
extern Filesys g_bsd_ufs;
extern Filesys g_iso9660_fs;

Filesys *g_filesys[] =
{
	&g_dos_partition,
	&g_dos_fat,
	&g_bsd_partition,
	&g_bsd_ufs,
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
extern Exec_entry exec_coff;
extern Exec_entry exec_elf;
extern Exec_entry exec_gzip;

Exec_entry *g_exec_list[] = 
{
	&exec_fcode,
	&exec_forth,
	&exec_coff,
	&exec_elf,
	&exec_gzip,
	NULL
};

/* non-forth machine-dependent routines here, esp initializing stuff */

/* This is used as a fall-back if a console cannot be opened at boot-time.
   The default is for Unix systems and will definitely need to be changed
   for an embedded system.  The easiest thing would be to send the data
   out of a hardwired serial port.
 */

#ifdef FAILSAFE_DISPLAY
static int fs_x = 0;
static int fs_y = 0;

#define FAILSAFE_DISPLAY_BASE		0xF7000000

void
failsafe_scrollup()
{
	int i;

	for (i = 0; i < 224; i++)
		memcpy((Byte *)FAILSAEF_DISPLAY_BASE + i * 480,
				(Byte *)FAILSAEF_DISPLAY_BASE + (i + 16) * 480, 480);

	memset((Byte *)FAILSAEF_DISPLAY_BASE + 224*480, 0, 16*480);
}

void
failsafe_putchar(Byte ch)
{
	int x;
	int y;
	int i;
	int j;
	Byte *p;
	extern void LCDColorSetPixel(long lX, long lY, CPixel color);

	if (ch == '\r')
	{
		fs_x = 0;
		return;
	}

	if (ch == '\n')
	{
		fs_y++;

		if (fs_y >= 15)
		{
			failsafe_scrollup();
			fs_y = 14;
		}

		return;
	}

	else if (ch < FONT_FIRST || (ch >= FONT_FIRST + FONT_COUNT))
		ch = '?';

	if (fs_x >= 40)
	{
		fs_x = 0;
		fs_y++;
	}

	if (fs_y >= 15)
	{
		failsafe_scrollup();
		fs_y = 14;
	}

	p = font + ((ch - FONT_FIRST) * FONT_ADVANCE * (FONT_HEIGHT - 1));
	x = fs_x * 8;
	y = fs_y * 16;

	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 8; j++)
		{
			CPixel color;

			if (p[i] & (0x80 >> j))
			{
				color.r = 15;
				color.g = 15;
				color.b = 15;
			}
			else
			{
				color.r = 0;
				color.g = 0;
				color.b = 0;
			}

			LCDColorSetPixel(x + j, y + i, color);
		}
	}

	fs_x++;
}

void
failsafe_init()
{
	extern void LCDInitDisplay(void);
	LCDInitDisplay();
}

Int
failsafe_write(Byte *buf, Int len)
{
	Int actual = 0;

	while (len)
	{
		failsafe_putchar(*buf++);
		actual++;
		len--;
	}

	return actual;
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
	Int actual = 0;

	while (len)
	{
		Int i = 0;
/*		Int i = st16c552_read(SIO_LSTAT);	*/

		if (i & 0x9E)		/* some sort of error? */
			dprintf("failsafe_read error: %#x\n", i);

		if (!(i & 0x01))	/* char ready in RBR? */
			break;

/*		*buf = st16c552_read(SIO_RXD);	*/
		buf++;
		actual++;
		len--;
	}

	return (actual == 0) ? -2 : actual;
}
#else

#define UART1_OFFSET		0x0000
#define UART2_OFFSET		0x1000

#define DEBUG_PORT			UART1_OFFSET

#define UART_CTL			0x100
#define UART_CTL_ENB		0x00000100
#define UART_STAT			0x140
#define UART_STAT_BUSY		0x00000800
#define UART_STAT_RXEMPTY	0x00400000
#define UART_STAT_TXFULL	0x00800000
#define UART_INTSTAT		0x240
#define UART_INTMASK		0x280
#define UART_INT_TXEMPTY	0x00001000
#define UART_INT_RXFULL		0x00002000
#define UART_INT_MDMCHG		0x00004000
#define UART_DATA			0x480
#define UART_DATA_MASK		0x000000FF
#define UART_DATA_FRMERR	0x00000100
#define UART_DATA_PARERR	0x00000200
#define UART_DATA_OVERR		0x00000400
#define UART_LINECTL		0x4C0
#define UART_LINECTL_BRD	0x00000FFF
#define UART_LINECTL_BREAK	0x00001000
#define UART_LINECTL_PRTEN	0x00002000
#define UART_LINECTL_EVEN	0x00004000
#define UART_LINECTL_XSTOP	0x00008000
#define UART_LINECTL_FIFOEN	0x00010000
#define UART_LINECTL_WRDLEN	0x00060000

void
failsafe_init()
{
	uInt ctl;

	while (EP7312_R(DEBUG_PORT + UART_STAT) & UART_STAT_BUSY)
		;

	/* 9600 baud, 8bits, no parity, fifo enabled */
	EP7312_W(DEBUG_PORT + UART_LINECTL,
			UART_LINECTL_WRDLEN|UART_LINECTL_FIFOEN|0x0017);
	ctl = EP7312_R(DEBUG_PORT + UART_INTMASK);
	EP7312_W(DEBUG_PORT + UART_INTMASK,
			ctl & ~(UART_INT_RXFULL|UART_INT_TXEMPTY));
	ctl = EP7312_R(DEBUG_PORT + UART_CTL);
	EP7312_W(DEBUG_PORT + UART_CTL, ctl | UART_CTL_ENB);
}

Int
failsafe_write(Byte *buf, Int len)
{
	Int actual = 0;

	while (len)
	{
		while (EP7312_R(DEBUG_PORT + UART_STAT) & UART_STAT_TXFULL)
			;

		EP7312_W(DEBUG_PORT + UART_DATA, buf[actual] & UART_DATA_MASK);
		actual++;
		len--;
	}

	while (EP7312_R(DEBUG_PORT + UART_STAT) & UART_STAT_BUSY)
		;

	return actual;
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
	Int actual = 0;

	while (len)
	{
		Int i = EP7312_R(DEBUG_PORT + UART_STAT);

		if (i & UART_STAT_RXEMPTY)
			break;

		i = EP7312_R(DEBUG_PORT + UART_DATA);

		if (i & 0x700)		/* some sort of error? */
			dprintf("failsafe_read error: %#x\n", i >> 8);


		*buf = i & UART_DATA_MASK;
		buf++;
		actual++;
		len--;
	}

	return (actual == 0) ? -2 : actual;
}
#endif


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
	{ "boot-device", "disk net" },
	{ "boot-file", "kernel" },
	{ "diag-device", "net" },
	{ "diag-file", "diag" },
	{ "diag-switch?", "false" },
	{ "secondary-diag?", "true" },
	{ "nvramrc", "" },
	{ "use-nvramrc?", "false" },
	{ "input-device", "console" },
	{ "output-device", "console" },
	{ "console-devices", "ttya;screen+keyboard" },
/*	{ "console-devices", "screen+keyboard" },	*/
	{ "screen-#columns", "80" },
	{ "screen-#rows", "24" },
	{ "oem-logo?", "false" },
	/* if present, oem-logo is 512-byte array of a 64x64-bit bitmap */
	{ "oem-banner?", "false" },
	{ "oem-banner", "" },
	{ "inverse-video?", "true" },

	{ "load-base", "0xF0000000" },
	{ "fcode-debug?", "false" },
	{ "physical-mappings?", "true" },
/*	{ "mac-address", "FF:FF:FF:FF:FF:FF" },		*/
	{ "boot-protocol", "bootp" },
	{ "client-ip", "0.0.0.0" },
	{ "server-ip", "0.0.0.0" },
	{ NULL, NULL }
};
