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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* machine-dependancies here - needs to be customized for a particular system */

#include <stdlib.h>
#include <stdarg.h>

//#define DEBUG
#include "defs.h"
#include "isa.h"
#include "fs.h"
#include "exe.h"

Byte logo_pixmap[1];	/* make banner happy - no display/fb */


/* registers for on-chip timers */
#define MBAR	M5206_REGISTERS
#define TMR		(MBAR + 0x100)
#define TRR		(MBAR + 0x104)
#define TCR		(MBAR + 0x108)
#define TCN		(MBAR + 0x10C)
#define TER		(MBAR + 0x111)

/* add to above for eacn timer's registers */
#define TIMER1	0x0
#define TIMER2	0x20

/* macros to talk to registers */
#define out(timer, reg, val)	(*(volatile uByte*)((timer) + (reg)) = (val))
#define in(timer, reg)			(*(volatile uByte*)((timer) + (reg)))
#define outw(timer, reg, val)	(*(volatile uShort*)((timer) + (reg)) = (val))
#define inw(timer, reg)			(*(volatile uShort*)((timer) + (reg)))


/* default entry point from gcc - located at the beginning of .text */
void
start(void)
{
	extern void do_main(void);
	do_main();

	/* kick back to the debugger */
	asm("	move.l	#0,%d0");
	asm("	trap	#15");
}

/* this is so strings are placed after start() to make sure it is at
   the beginning of .text, which makes it easy to load an image */
void
do_main(void)
{
	static char *argv[] = { "SmartFirmware", NULL };
	extern int main(int argc, char *argv[]);

	DPRINTF(("Alive!\n"));

	DPRINTF(("calling main()\n"));
	main(1, argv);
	DPRINTF(("returned from main()\n"));
}


/* debugging hooks to call the builtin dBUG monitor on m5206 eval board
   - these are used by failsafe I/O
 */
void
debug_write(int ch)
{
	asm("	move.l	8(%a6),%d1");
	asm("	move.l	#0x13,%d0");
	asm("	trap	#15");
}

int
debug_read(void)
{
	asm("	move.l	#0x10,%d0");
	asm("	trap	#15");
	asm("	move.l	%d1,%d0");
}

int
debug_char_avail(void)
{
	asm("	move.l	#0x14,%d0");
	asm("	trap	#15");
}


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


/* debug printf using failsafe_write() and possibly main console
   - probably won't need to be modified much
 */
void
dprintf(const char *fmt, ...)
{
	va_list args;
	Byte *s, *s2;
	Byte buf[2048];
	Byte buf2[2048];
	extern int vbprintf(char *, const char *, va_list);

	va_start(args, fmt);
	vbprintf(buf, fmt, args);
	va_end(args);

	/* convert all newlines to carriage-return-newline pairs */
	s = buf;
	s2 = buf2;

	while (*s)
	{
		if (*s == '\n')
			*s2++ = '\r';
		
		*s2++ = *s++;
	}

	*s2 = '\0';
	failsafe_write(buf2, s2 - buf2);	/* serial output */

	/* also try main console if it is alive */
	/*if (g_e && g_e->screen)
		display_text(g_e, buf2, s2 - buf2);*/
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
	extern char end[];
	
	DPRINTF(("machine_initalize\n", ret));

	/*g_machine_memory = mem;
	g_machine_memory_size = sizeof mem;*/

	/* set malloc memory to after end of stack */
	g_machine_memory = (Byte*)(((uLong)end + 16) & ~7);
	g_machine_memory_size = MALLOC_POOL;
	g_machine_memory_used = MALLOC_POOL;
	ret = init_malloc(g_machine_memory, MALLOC_POOL);
	DPRINTF(("machine memory=%p size=%#x\n",
			g_machine_memory, g_machine_memory_size));

	/* initialize timer */
	outw(TIMER1, TMR, 0xFF05);	/* 256 prescale, no intr, clock/16, free run */
	outw(TIMER1, TCN, 0x0);		/* reset to zero */

	DPRINTF(("machine_initalize ret=%d\n", ret));
	return ret;
}


/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
Retcode
machine_reset_all(Environ *e)
{
	asm("	move.l	#0,%d0");
	asm("	trap	#15");
	return E_ABORT;
}


/* Display a value "n" on some LED display, or whatever is appropriate, on
   the hardware.  This is used by the Forth/FCode word "display-status"
   as a sort of last-resort.
 */
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


/* Probe an address and return TRUE if the data at the address is
   legitimate, and FALSE if not.  This has to catch an access error
   with an interrupt service routine to set g_hwfault.
 */
Bool
machine_probe_read(Environ *e, Cell addr, Cell *value, int size)
{
	g_hwfault = NO_ERROR;

	switch (size)
	{
	case 1:
		*value = *(uByte*)(uPtr)addr;
		break;
	case 2:
		*value = *(uShort*)(uPtr)addr;
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

	return g_hwfault == NO_ERROR;
}

/* Write a value to an address which may be invalid.  If invalid, return
   FALSE, else TRUE.  Again, this has to catch an exception to set g_hwfault
   to detect access to invalid addresses.
 */
Bool
machine_probe_write(Environ *e, Cell addr, Cell value, int size)
{
	g_hwfault = NO_ERROR;

	switch (size)
	{
	case 1:
		*(uByte*)(uPtr)addr = value;
		break;
	case 2:
		*(uShort*)(uPtr)addr = value;
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

	return g_hwfault == NO_ERROR;
}


/* Read a register from a device or bus with a single operation.
   First see if this is a legitimate PCI address, and if so, perform the
   appropriate PCI memory or I/O read operation.  Otherwise, assume that
   reading from a C pointer dereference will be OK.
 */
void
machine_reg_read(Environ *e, Cell addr, Cell *value, int size)
{
	/* like machine_probe_read except we don't expect a bus fault */
	switch (size)
	{
	case 1:
		*value = *(uByte*)(uPtr)addr;
		break;
	case 2:
		*value = *(uShort*)(uPtr)addr;
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
		*(uShort*)(uPtr)addr = value;
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


/* Get the current time and store it as seconds + nanoseconds in a Time_value.
 */
void
machine_gettime(Environ *e, Time_value *tv)
{
	static uShort timer = 0;
	static Time_value time = { 0, 0 };
	uShort t = inw(TIMER1, TCN);	/* read timer counter */
	#define NSEC	156250		/* 25Mhz clock */

	*tv = time;
	DPRINTF(("TCN=%#x sec=%d nsec=%d\n",
			inw(TIMER1, TCN), time.sec, time.nsec));

	if (t < timer)
	{
		tv->nsec += (0xFFFFu - timer) * NSEC;

		while (tv->nsec > 1000000000)
		{
			tv->sec++;
			tv->nsec -= 1000000000;
		}

		timer = 0;
	}

	tv->nsec += (t - timer) * NSEC;

	while (tv->nsec > 1000000000)
	{
		tv->sec++;
		tv->nsec -= 1000000000;
	}

	timer = t;
	time = *tv;
}

/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
	while (us--)
		;
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

	DPRINTF(("machine_memory_test: addr=%#Cx len=%#Cx mask=%#Cx\n",
			addr, len, mask));

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


#ifdef SUN_COMPATIBILITY
#include "obptftp.h"
#endif


/* Machine-dependent stuff for admin.c goes here.
   These routines will need to be modified for a particular architecture.
 */

/* This must probe and initialize all the hardware in the system and place
   it in the device tree at the appropriate places.
 */
CC(machine_probe_all)		/* (--) */
{
	Retcode ret = NO_ERROR;
	Package *isa;
	EC(duart_probe);

	DPRINTF(("machine_probe_all: enter\n"));

#ifdef SUN_COMPATIBILITY
	/* create obp-tftp package using Sun's implementation */
	e->currpkg = e->packages;

	if (ret == NO_ERROR)
		ret = interp_fcode(e, fc_image, MAGIC_FNEXT);
#endif

	/* install ISA bus support and probe it */
	e->currpkg = NULL;
	isa = new_package(e->root);
	ret = install_isabase(e);		/* install ISA device access routines */

	if (ret == NO_ERROR)
		ret = install_isa(e, isa);

	if (ret == NO_ERROR)
		ret = duart_probe(e);

	DPRINTF(("machine_probe_all: return=%s (%d)\n", err2str(ret), ret));
	return ret;
}

/* Secondary diagnostics run after the console is up for error messages.
   More rigorous testing should go here rather than at bootup just so that
   errors can be displayed */
CC(machine_secondary_diag)	/* (--) */
{
	return NO_ERROR;
}


/* CPU-register access routines - "reg" is a machine-dependent token
   used to access some other list of words or assembly code to read/write
   a value from a specific register - it may even be a pointer to a string
 */


Retcode
cpu_register_read(Environ *e, Cell creg, Cell *value)
{
	*value = 0;		/* ptr assumed to be in %a0 */

	switch (creg)
	{
	case REG_D0:
		asm("	move.l %d0,(%a0)");
		break;
	case REG_D1:
		asm("	move.l %d1,(%a0)");
		break;
	case REG_D2:
		asm("	move.l %d2,(%a0)");
		break;
	case REG_D3:
		asm("	move.l %d3,(%a0)");
		break;
	case REG_D4:
		asm("	move.l %d4,(%a0)");
		break;
	case REG_D5:
		asm("	move.l %d5,(%a0)");
		break;
	case REG_D6:
		asm("	move.l %d6,(%a0)");
		break;
	case REG_D7:
		asm("	move.l %d7,(%a0)");
		break;

	case REG_A0:
		asm("	move.l %a0,(%a0)");
		break;
	case REG_A1:
		asm("	move.l %a1,(%a0)");
		break;
	case REG_A2:
		asm("	move.l %a2,(%a0)");
		break;
	case REG_A3:
		asm("	move.l %a3,(%a0)");
		break;
	case REG_A4:
		asm("	move.l %a4,(%a0)");
		break;
	case REG_A5:
		asm("	move.l %a5,(%a0)");
		break;
	case REG_A6:
		asm("	move.l %a6,(%a0)");
		break;
	case REG_A7:
		asm("	move.l %a7,(%a0)");
		break;

	case REG_PC:
		asm("	move.l #.,(%a0)");
		break;

	case REG_CCR:
		asm("	move.w %ccr,%d1");
		asm("	move.l %d1,(%a0)");
		break;

	case REG_SR:
		asm("	move.w %sr,%d1");
		asm("	move.l %d1,(%a0)");
		break;

	case REG_VBR:
	case REG_CACR:
	case REG_ACR0:
	case REG_ACR1:
		cprintf(e, "cannot read write-only register\n");
		break;
	}

	return NO_ERROR;
}

Retcode
cpu_register_write(Environ *e, Cell creg, Cell value)
{
	/* value assumed to be 16(%sp) */

	switch (creg)
	{
	case REG_D0:
		asm("	move.l 16(%sp),%d0");
		break;
	case REG_D1:
		asm("	move.l 16(%sp),%d1");
		break;
	case REG_D2:
		asm("	move.l 16(%sp),%d2");
		break;
	case REG_D3:
		asm("	move.l 16(%sp),%d3");
		break;
	case REG_D4:
		asm("	move.l 16(%sp),%d4");
		break;
	case REG_D5:
		asm("	move.l 16(%sp),%d5");
		break;
	case REG_D6:
		asm("	move.l 16(%sp),%d6");
		break;
	case REG_D7:
		asm("	move.l 16(%sp),%d7");
		break;

	case REG_A0:
		asm("	move.l 16(%sp),%a0");
		break;
	case REG_A1:
		asm("	move.l 16(%sp),%a1");
		break;
	case REG_A2:
		asm("	move.l 16(%sp),%a2");
		break;
	case REG_A3:
		asm("	move.l 16(%sp),%a3");
		break;
	case REG_A4:
		asm("	move.l 16(%sp),%a4");
		break;
	case REG_A5:
		asm("	move.l 16(%sp),%a5");
		break;
	case REG_A6:
		asm("	move.l 16(%sp),%a6");
		break;
	case REG_A7:
		asm("	move.l 16(%sp),%a7");
		break;

	case REG_PC:
		asm("	move.l 16(%sp),%a0");
		asm("	jmp (%a0)");
		break;

	case REG_CCR:
		asm("	move.l 16(%sp),%d1");
		asm("	move.w %d1,%ccr");
		break;

	case REG_SR:
		asm("	move.l 16(%sp),%d1");
		asm("	move.w %d1,%sr");
		break;

	case REG_VBR:
		asm("	move.l 16(%sp),%d1");
		asm("	movec  %d1,%vbr");
		break;

	case REG_CACR:
		asm("	move.l 16(%sp),%d1");
		asm("	movec  %d1,%cacr");
		break;

	case REG_ACR0:
		asm("	move.l 16(%sp),%d1");
		asm("	movec  %d1,%acr0");
		break;

	case REG_ACR1:
		asm("	move.l 16(%sp),%d1");
		asm("	movec  %d1,%acr1");
		break;
	}

	return NO_ERROR;
}

C(f_dotregisters)
{
	Cell val;
	Register reg;

	cpu_register_read(e, REG_PC, &val);
	cprintf(e, "PC: %08Cx", val);

	cpu_register_read(e, REG_SR, &val);
	cprintf(e, " SR: %04Cx", val);

	cpu_register_read(e, REG_CCR, &val);
	cprintf(e, " CCR: %08Cx", val);

	cprintf(e, "\nAn:");

	for (reg = REG_A0; reg <= REG_A7; reg++)
	{
		cpu_register_read(e, reg, &val);
		cprintf(e, " %08Cx", val);
	}

	cprintf(e, "\nDn:");

	for (reg = REG_D0; reg <= REG_D7; reg++)
	{
		cpu_register_read(e, reg, &val);
		cprintf(e, " %08Cx", val);
	}

	cprintf(e, "\n");
	return NO_ERROR;
}

/* Coldfire registers */
static const Initentry init_registers[] =
{
	{ "%d0", (Command)REG_D0, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d1", (Command)REG_D1, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d2", (Command)REG_D2, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d3", (Command)REG_D3, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d4", (Command)REG_D4, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d5", (Command)REG_D5, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d6", (Command)REG_D6, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%d7", (Command)REG_D7, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a0", (Command)REG_A0, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a1", (Command)REG_A1, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a2", (Command)REG_A2, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a3", (Command)REG_A3, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a4", (Command)REG_A4, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a5", (Command)REG_A5, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a6", (Command)REG_A6, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%sp", (Command)REG_A6, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%a7", (Command)REG_A7, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%fp", (Command)REG_A7, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%pc", (Command)REG_PC, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%ccr", (Command)REG_CCR, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%sr", (Command)REG_SR, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%vbr", (Command)REG_VBR, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%cacr", (Command)REG_CACR, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%acr0", (Command)REG_ACR0, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },
	{ "%acr1", (Command)REG_ACR1, INVALID_FCODE, F_NONE, T_REGISTER
			HELP("(-- reg) CPU-register - set using \"to\"") },

	{ ".registers", f_dotregisters, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) display all registers") },

	{ NULL, NULL }
};


/* machine-dependent code to allow arbitrary binary code to be
   loaded and executed - useful for assembler support
 */
Retcode
machine_code_jump(Environ *e, void *addr)
{
	asm("	move.l 12(%sp),%a0");
	asm("	jmp (%a0)");

	/* should never get here */
	return NO_ERROR;
}

Retcode
machine_compile_return(Environ *e)
{
	/* unlk %a6 */
	compile_byte(e, 0x5E);
	compile_byte(e, 0x4E);

	/* rts */
	compile_byte(e, 0x75);
	compile_byte(e, 0x4E);
	return NO_ERROR;
}


/* set e->load to a reasonable place in memory with a lot of room
 */
CC(machine_init_load)
{
	/* load after whatever we want to reserve for the stack */
	e->load = g_machine_memory + g_machine_memory_used;
	return NO_ERROR;
}

/* setup to launch a program at e->load - verify known executable type
 */
CC(machine_init_program)	/* (--) */
{
	if (exec_is_exec(e))
	{
		exec_free_symbols(e, e->loadsyms);
		e->loadsyms = exec_load_symbols(e);	/* load any symbols */
		return NO_ERROR;
	}

	return E_BAD_IMAGE;
}

/* launch the image at e->load
 */
CC(machine_go)				/* (--) */
{
	Retcode ret;

	if (!exec_is_exec(e))		/* sanity check */
		return E_BAD_IMAGE;

	ret = exec_load(e);

	/* -1 means we are not supposed to launch through e->entrypoint */
	if (ret != NO_ERROR || e->entrypoint == -1)
		return ret;

	/* load symbol table */
	exec_free_symbols(e, e->loadsyms);
	e->loadsyms = exec_load_symbols(e);
	/* TODO */

	return E_ABORT;		/* should never get here, but silence the compiler */
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
extern const Initentry init_sun[];

const Initentry* init_list[] =
{
	init_funcs,
	//init_funcs64,
	init_control,
	init_packages,
	init_forth,
	init_admin,
	init_nvedit,
	init_other,
	init_device,
	init_debug,
	init_registers,
	//init_fb,
	//init_filesystem,
	init_sun,
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
EC(install_display);
EC(install_pci);
EC(install_obptftp);
EC(install_deblocker);
EC(install_disklabel);
EC(install_failsafe);
EC(install_client_services);
EC(init_options_from_nvram);	/* forward to definition down below */

const Command install_list[] =
{
	install_root,		/* should be first */
	install_chosen,		/* should be second */
	install_memory,		/* should be third */
	install_cpu,
	init_options_from_nvram,
	install_failsafe,
#ifndef SUN_COMPATIBILITY
	install_obptftp,
#endif
	install_display,
	//install_deblocker,
	//install_disklabel,
	install_client_services,
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

Isa_device *isa_devices[] =
{
	&ns16550_com1,
	&ns16550_com2,
	&ne2000_0x300_3,
	NULL
};


#if 0
/* List all supported builtin filesystems.  This is not standard OpenFirmware
   but is useful in that disk partitions will be displayed as part of the
   device tree under the builtin disk devices.
 */
extern Filesys g_dos_partition;
extern Filesys g_dos_fat;
extern Filesys g_bsd_partition;
extern Filesys g_bsd_ufs;

Filesys *g_filesys[] =
{
	&g_dos_partition,
	&g_dos_fat,
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	&g_bsd_partition,
	&g_bsd_ufs,
#endif
	NULL
};
#endif

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
extern Exec_entry exec_raw_binary;

Exec_entry *g_exec_list[] = 
{
	&exec_fcode,
	&exec_forth,
	&exec_coff,
	&exec_elf,
	&exec_gzip,
	&exec_raw_binary,	/* must be last entry if present */
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

	while (len--)
	{
		debug_write(*buf++);
		actual++;
	}

	return actual;
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
	Int actual = 0;

	if (debug_char_avail())
	{
		buf[0] = debug_read();
		actual = 1;
	}

	return (actual == 0) ? -2 : actual;
}


/* This is used to initialize non-volatile RAM, and to mimic it for
   standalone hosts.
 */
static struct nvram_data
{
	char *name;
	char *val;
} nvram[] = 
{
	{ "auto-boot?", "false" },
	{ "auto-boot-timeout", "500" },
	{ "boot-command", "boot" },
	{ "boot-device", "net" },
	{ "boot-file", "" },
	{ "diag-device", "net" },
	{ "diag-file", "diag" },
	{ "diag-switch?", "true" /* "false" */ },
	{ "secondary-diag?", "false" },
	{ "nvramrc",
			/*"devalias keyboard /uart@0\n"*/
			/*"devalias screen /uart@0\n"*/
			"devalias keyboard /failsafe\n"
			"devalias screen /failsafe\n"
			"devalias net /isa/ethernet\n"
			"probe-all install-console banner\n"
			"show-devs\n" 
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
	{ "inverse-video?", "true" },
	{ "security-mode", NULL },			/* NULL default value prevents */
	{ "security-#badlogins", NULL },	/* set-default(s) from working */
	{ "security-password", NULL },		/* for these three security words */
	{ NULL, NULL }
};


/* This should set the specified parameter in nvram to the specified value.
   The default version does nothing.
 */
Retcode
set_nvram(Environ *e, Byte *param, Int plen, Byte *val, Int vlen)
{
	return NO_ERROR;
}

/* This should get the default value of the specified parameter in nvram.
   The usual version simply reads the static list above.
 */
Byte *
get_nvram_default(Environ *e, Byte *param, Int plen)
{
	struct nvram_data *nv;
	int len;
	
	setstrlen(&param, &plen);
	
	for (nv = nvram; nv->name; nv++)
	{
		len = strlen(nv->name);
		
		if (plen == len && compare_strs(param, CSTR, nv->name, len))
			return nv->val;
	}
	
	return NULL;
}

/* This should get the value of the specified parameter in nvram.
   The default version simply reads the static list above.  This should still
   be done as a fall-back if nvram is not readable, or has become corrupt.
 */
Byte *
get_nvram(Environ *e, Byte *param, Int plen)
{
	return get_nvram_default(e, param, plen);
}

/* This should create a new word in nvram.  Space should be allocated
   for maxlen bytes if necessary.
 */
Byte *
new_nvram(Environ *e, Byte *param, Int plen, Int maxlen)
{
	return NULL;
}

/* This is to make sure that any garbage gets flushed out of nvram.
 */
Retcode
reset_nvram(Environ *e)
{
	return NO_ERROR;
}


/* this is needed to initialize the /options node from nvram */
CC(init_options_from_nvram)
{
	struct nvram_data *nv;
	Retcode ret;
	
	for (nv = nvram; nv->name; nv++)
	{
		ret = prop_set_str(e->options->props, nv->name, CSTR, nv->val, CSTR);

		if (ret != NO_ERROR)
			return ret;
	}
	
	return NO_ERROR;
}


/* This is used to override nvram parameters arguments by those passed in
   to main().  The args must contain strings of "-parameter value" pairs.
 */
Retcode
machine_init_args(Environ *e, int argc, char *argv[])
{
	Retcode ret = NO_ERROR;

	if (e->options->props == NULL)
		return E_INIT_ERROR;

	while (argc-- > 0)
	{
		if (argv[argc][0] != '-' || argc == 0)
			break;

		ret = prop_set_str(e->options->props, (Byte*)argv[argc] + 1, CSTR,
				(Byte*)argv[argc + 1], CSTR);

		if (ret != NO_ERROR)
			return ret;

		argc--;
	}

	return ret;
}
