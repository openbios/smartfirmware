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

/* machine-dependancies here - needs to be customized for a particular system */

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#define DEBUG
#include "defs.h"
#include "pci.h"
#include "isa.h"
#include "exe.h"
#include "fs.h"


/* externs for Forth words that we use below */
EC(f_get);
EC(f_getdoub);
EC(f_getquad);
EC(f_set);
EC(f_setdoub);
EC(f_setquad);


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


/* macros to access a 16550-compatible serial port for failsafe I/O */
#define ISA_BASE	0x80000000ul	/* 0x00000000 */
#define COM1_PORT	0x3F8
#define COM2_PORT	0x2F8
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


/* address where Be boot-ROM leaves the total size of RAM in the system */
#define BE_MEM_SIZE		(*(uLong*)0x3010)


/* entry point for BeROM - hope the stack is setup */
extern __start();

__start()
{
	extern main();
		
	if (PROC_INTR_ID & CPIR_ID)		/* idle CPU1 */
		for (;;)
			;
	
	/* better be only one CPU running from here on in... */

	return main();
}


/* to access the ISA bus through the PCI bus hopefully at default addresses */
extern void isa_write(uInt addr, uByte val);
extern uByte isa_read(uInt addr);

void
isa_write(uInt addr, uByte val)
{
	do_write(ISA_BASE + addr, val, 1);
}

uByte
isa_read(uInt addr)
{
	uByte val;

	do_read(ISA_BASE + addr, &val, 1);
	return val;
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
	
	if (g_e && g_e->screen)
		display_text(g_e, buf2, strlen(buf2));
	else
		failsafe_write(buf2, strlen(buf2));
}



/* assembly to catch memory-reference errors for probe read/write
 */
static int g_mem_error;

#define SSR0	26		/* SRR0 */
#define SPR0	272		/* SGPR0 */
#define SPR1	273		/* SGPR1 */

__asm static void
mem_exception(void)
{
	mtspr	SPR0,r0			/* save r0 */
	
	mfspr	r0,SSR0			/* load SSR0 */
	addi	r0,r0,4			/* add 4 to skip error-causing instruction */
	mtspr	SSR0,r0			/* and store it */
	
	li		r0,1			/* load 1 */
	stw		r0,g_mem_error	/* and store it in a known place for probe routines */
	
	mfspr	r0,SPR0			/* restore r0 */
	rfi						/* return from exception */
}
#define MEM_EXCEPTION_LEN	(4 * 8)		/* MUST MATCH LENGTH OF dsi_exception! */



/* assembly to bump a timer counter every interrupt - hack since we make no
   attempt to provide a general-purpose interrupt hook
 */
static int g_ticks;

__asm static void
timer_intr(void)
{
	mtspr	SPR0,r0			/* save r0 */
	mtspr	SPR1,r1			/* save r1 */
	
	lwz		r0,g_ticks		/* add one to g_ticks */
	addi	r0,r0,1
	stw		r0,g_ticks
	
	lis		r1,0xBFFF		
	ori		r1,r1,0xFFF0	/* load INT-ACK addr into r1 */
	lwz		r0,0(r1)		/* generate INT-ACK cycle */
	
	mfspr	r1,SPR1			/* restore r1 */
	mfspr	r0,SPR0			/* restore r0 */
	rfi						/* return from exception */
}
#define TIMER_INTR_LEN		(4 * 11)	/* MUST MATCH LENGTH OF timer_intr! */



/* to make sure the exception base-addr is 0x0 */
__asm static uInt
get_msr(void)
{
	mfmsr	r3
	blr
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
	
	/* set the serial port to a sane state */
	isa_write(COM2_PORT + SIO_LCR, 0x80);	/* DLAB on */
	isa_write(COM2_PORT + SIO_DLL, 12);		/* clock value for 9600 baud */
	isa_write(COM2_PORT + SIO_DLM, 0);
	isa_write(COM2_PORT + SIO_LCR, 0x03);	/* DLAB off, 8-bits, 1 stop bit, no parity */
	isa_write(COM2_PORT + SIO_MCR, 0x01);	/* drop all lines, raise DTR */
	isa_write(COM2_PORT + SIO_IER, 0x00);	/* disable all interrupts */
	isa_write(COM2_PORT + SIO_FCR, 0x07);	/* turn on and clear I/O FIFOs */

	failsafe_write("Alive!\r\n", 8);
	DPRINTF(("in machine_initialize() (sizeof ptr=%d)...\n", sizeof (void*)));

	/* use memory starting at 0.5Mb from bottom of memory for Forth world */
	g_machine_memory = (Byte*)(512*1024);
	g_machine_memory_size = BE_MEM_SIZE - (uLong)g_machine_memory - MALLOC_POOL;
	g_machine_memory_used = MALLOC_POOL;
	DPRINTF(("g_machine_memory=%p size=%#x used=%#x\n", g_machine_memory,
			g_machine_memory_size, g_machine_memory_used));
	ret = init_malloc(g_machine_memory, MALLOC_POOL);
	
	/* copy memory exception handler to various exception vectors */
	DPRINTF(("copying memory exception handler: func=%#x len=%d\n",
			(long)&mem_exception, MEM_EXCEPTION_LEN));
	memcpy((char*)0x0300, (char*)&mem_exception, MEM_EXCEPTION_LEN);	/* DSI */
	memcpy((char*)0x0600, (char*)&mem_exception, MEM_EXCEPTION_LEN);	/* alignment */
	memcpy((char*)0x1100, (char*)&mem_exception, MEM_EXCEPTION_LEN);	/* data load miss */
	memcpy((char*)0x1200, (char*)&mem_exception, MEM_EXCEPTION_LEN);	/* data store miss */
	
	/* copy timer interrupt handler to general interrupt exception vector */
	DPRINTF(("copying timer interrupt handler: func=%#x len=%d\n",
			(long)&timer_intr, TIMER_INTR_LEN));
	memcpy((char*)0x0500, (char*)&timer_intr, TIMER_INTR_LEN);	/* external interrupt */
	
	DPRINTF(("MSR=%#x\n", get_msr()));
	return ret;
}

/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
__asm Retcode
machine_reset_all(Environ *e)
{
	ba	0x3F00100			/* really 0xFFF00100 with sign-extension */
	/* never returns */
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
machine_gettime(Environ *e, Time_value *tv)
{
	tv->sec += 1;
	tv->nsec += 1000;
}

/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
#if 0
	if (CKSP(g_e, 0, 1))
		return;

	PUSH(g_e, us / 1000);
	execute_word(g_e, "ms");
#endif
	while (us--)
	{
		int i = 50;
		
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
	return FALSE;
}



/* Machine-dependent stuff for admin.c goes here.
   These routines will need to be modified for a particular architecture.
 */

/* This must probe and initialize all the hardware in the system and place
   it in the device tree at the appropriate places.  The default code will
   initialize a PCI bus.
 */
CC(machine_probe_all)		/* (--) */
{
	Retcode ret;

	PUSH(e, "/pci");
	PUSH(e, 4);
	ret = execute_word(e, "find-device");
	
	if (ret != NO_ERROR)
		return ret;
	
	ret = execute_word(e, "probe-pci");
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
	/* put it at the end of our memory - should have plenty of room */
	e->load = g_machine_memory + g_machine_memory_used;
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

/* execute the code at e->load in such a way that it is possible to
   resume Forth
 */
CC(machine_go)				/* (--) */
{
	/* assume it is a simple code-resource/function and just call it */
	Retcode ret;

	if (exec_is_exec(e))		/* sanity check */
	{
		ret = exec_load(e);
		/* For a real system, we would try to launch through
		   e->entrypoint but only if it is not not -1 (to allow
		   launching through address 0).  This allows running Fcode and
		   Forth images without having to do anthing special here to
		   avoid trying to launch through a non-existent entrypoint.
		 */
	}
	else
		ret = E_BAD_IMAGE;

	return ret;
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
extern const Initentry init_tokenizer[];

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
	NULL
};


/* List all init routines that have to be called to initialize packages and
   methods.  This is like init_list[] above, except it is used to initialize
   more complex packages that need an initialization routine to set things up.
   Again, custom packages can easily be added here if desired.
   install_list[] is only used by "table.c".
 */
EC(install_root);
EC(install_memory);
EC(install_stdio);
EC(install_mac);
EC(install_display);
EC(install_failsafe);
EC(install_pci);
EC(install_obptftp);
EC(install_deblocker);
EC(install_disklabel);
EC(init_options_from_nvram);	/* forward to definition down below */

const Command install_list[] =
{
	install_root,		/* should be first */
	install_memory,		/* should be second */
	init_options_from_nvram,
	install_display,
	/*install_failsafe,*/
	install_pci,
	install_obptftp,
	install_deblocker,
	install_disklabel,
	NULL
};


/* This is used to specify a list of initial PCI drivers to load to handle
   builtin devices.  pci_drivers[] is only used by pci_load_builtin_driver()
   in pci.c.
 */

extern Pci_driver pci_display_driver;

extern Pci_driver intel_82378_driver;
extern Pci_driver intel_piix_driver;
extern Pci_driver digital_21x4x_driver;
extern Pci_driver ncr_53C8xx_driver;

const Pci_driver *pci_drivers[] =
{
	&pci_display_driver,
	&intel_82378_driver,
	&intel_piix_driver,
	&digital_21x4x_driver,
	&ncr_53C8xx_driver,
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

Isa_device *isa_devices[] =
{
	&ns16550_com1,
	&ns16550_com2,
	&ns16550_com3,
	&ns16550_com4,
	&isa_keyboard,
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

Filesys *g_filesys[] =
{
	&g_dos_partition,
	&g_dos_fat,
	&g_bsd_partition,
	&g_bsd_ufs,
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

Exec_entry *g_exec_list[] = 
{
	&exec_fcode,
	&exec_forth,
	&exec_coff,
	&exec_elf,
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
		if (isa_read(COM2_PORT + SIO_LSR) & 0x20)	/* THR ready for a char? */
		{
			isa_write(COM2_PORT + SIO_THR, *buf++);
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
		c = isa_read(COM2_PORT + SIO_LSR);	/* char ready in RBR? */

		if (c & 0x9E)		/* some error occurred? */
		{
			actual = -1;
			break;
		}

		if (!(c & 0x01))	/* char ready in RBR? */
			break;

		*buf++ = isa_read(COM2_PORT + SIO_RBR);
		actual++;
		len--;
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
	{ "boot-device", "disk" },
	{ "boot-file", "" },
	{ "diag-device", "net" },
	{ "diag-file", "diag" },
	{ "diag-switch?", "true" },
	{ "secondary-diag?", "false" },
	{ "nvramrc",
			/*"devalias keyboard /pci/isa/serial@i2F8\n"*/
			/*"devalias screen /pci/isa/serial@i2F8\n"*/
			"devalias keyboard /pci/isa/keyboard\n"
			"devalias screen /pci/display\n"
			"devalias net /pci/ethernet\n" 
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
	{ "inverse-video?", "false" },	/* looks better with Be startup screen */
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
