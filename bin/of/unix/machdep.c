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
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include "defs.h"
#include "pci.h"
#include "fs.h"
#include "exe.h"


/* externs for Forth words that we use below */
EC(f_get);
EC(f_getdoub);
EC(f_getquad);
EC(f_set);
EC(f_setdoub);
EC(f_setquad);


/* defines for machine-dependent address - all PCI stuff for now */
#define PCI_BASE 0x50000000UL
#define PCI_SIZE 0x20000000UL
#define PCI_BLOCKSIZE (1024*1024)
#define PCI_BLOCKSHIFT 20

#define PCI_BLOCKS ((PCI_SIZE)/(PCI_BLOCKSIZE))
#define PCI_OFFSETMASK ((PCI_BLOCKSIZE)-1)
#define PCI_BLOCKMASK ((PCI_BLOCKS)-1)

unsigned short pci_map[PCI_BLOCKS];


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


/* debug printf using failsafe_write()
   - probably won't need to be modified much
 */
void
dprintf(const char *fmt, ...)
{
	va_list args;
	static Byte buf[2048], buf2[2048];
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
	failsafe_write(buf2, s2 - buf2);
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
#if defined STANDALONE_GUI && defined unix
	static Byte mem[MALLOC_POOL * 5];
#else
	static Byte mem[MALLOC_POOL];
#endif
	
	g_machine_memory = mem;
	g_machine_memory_size = sizeof mem;
	g_machine_memory_used = sizeof mem;
	DPRINTF(("machine_initialize: memory=%p size=%#x size=%#x\n",
			g_machine_memory, g_machine_memory_size, g_machine_memory_used));
	return init_malloc(mem, sizeof mem);
}


/* Reset everything and start everything again.  This is the equivalent of a
   jump to main(), or whatever else makes sense for the platform.
   Used by "reset-all".
 */
Retcode
machine_reset_all(Environ *e)
{
	return R_END;
}


/* Display a value "n" on some LED display, or whatever is appropriate, on
   the hardware.  This is used by the Forth/FCode word "display-status"
   as a sort of last-resort.
 */
void
machine_led_write(Environ *e, Int n)
{
	dprintf("LED=%d\n", n);
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

	return TRUE;
}

/* Write a value to an address which may be invalid.  If invalid, return
   FALSE, else TRUE.  Again, this may need to catch a signal or something
   to detect access to invalid addresses.
 */
Bool
machine_probe_write(Environ *e, Cell addr, Cell value, int size)
{
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

	return TRUE;
}


/* Translate an address into a valid PCI bus address.
   Used below to read/write machine registers.
 */
/* This routine is only used to implement the FAKE PCI bus in fakepci.c */
Bool
machine_pci_translate(Cell addr, Int *pci_addr, Bool *io)
{
	int block;

	if (addr < PCI_BASE || addr >= PCI_BASE + PCI_SIZE)
		return FALSE;

	block = pci_map[((addr - PCI_BASE) >> PCI_BLOCKSHIFT)];

	if (block == -1)
		return FALSE;

	*pci_addr = ((addr - PCI_BASE) & PCI_OFFSETMASK) |
			(block & 0x7FFF) << PCI_BLOCKSHIFT;
	*io = !!(block & 0x8000);
	return TRUE;
}


/* Read a register from a device or bus with a single operation.
   First see if this is a legitimate FAKE PCI address, and if so, perform the
   appropriate PCI memory or I/O read operation.  Otherwise, assume that
   reading from a C pointer dereference will be OK.
 */
void
machine_reg_read(Environ *e, Cell addr, Cell *value, int size)
{
	Int pci_addr;
	Bool io;

	if (machine_pci_translate(addr, &pci_addr, &io))
	{
		Int v;

		if (io)
			pci_io_read(0, pci_addr, &v, size);
		else
			pci_mem_read(0, pci_addr, &v, size);

		*value = v;
		return;
	}

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
   if this is a legitimate FAKE PCI address, and if so, perform the appropriate
   PCI memory or I/O write operation.  Otherwise, assume that copying to
   a C pointer dereference will be OK.
 */
void
machine_reg_write(Environ *e, Cell addr, Cell value, int size)
{
	Int pci_addr;
	Bool io;

	if (machine_pci_translate(addr, &pci_addr, &io))
	{
		if (io)
			pci_io_write(0, pci_addr, value, size);
		else
			pci_mem_write(0, pci_addr, value, size);

		return;
	}

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
	struct timeval tval;
	
	gettimeofday(&tval, NULL);
	tv->sec = tval.tv_sec;
	tv->nsec = tval.tv_usec * 1000;
}

/* cannot call this usleep() as that conflicts with a <unistd.h> routine */
void
u_sleep(uInt us)
{
	if (CKSP(g_e, 0, 1))
		return;

	PUSH(g_e, us / 1000);
	execute_word(g_e, "ms");
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

		if ((~v2 & mask) != (v1 & mask))
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


/* CPU-register access routines - "reg" is a machine-dependent token
   used to access some other list of words or assembly code to read/write
   a value from a specific register - it may even be a pointer to a string
 */
Retcode
cpu_register_read(Environ *e, Cell reg, Cell *value)
{
	return E_UNIMPLEMENTED;
}

Retcode
cpu_register_write(Environ *e, Cell reg, Cell value)
{
	return E_UNIMPLEMENTED;
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
	EC(f_probe_pci);
	EC(install_fake_disk_driver);

	PUSH(e, "/pci");
	PUSH(e, 4);
	ret = f_find_device(e);
	
	if (ret == NO_ERROR)
		ret = f_probe_pci(e);

	if (ret == NO_ERROR)
		ret = install_fake_disk_driver(e);

	/* make console device aliases if none exist */
	if (ret == NO_ERROR &&
			find_table(e->aliases->props, "keyboard", CSTR) == NULL)
	{
		ret = make_devalias(e, "keyboard", CSTR, "/failsafe", CSTR);

		if (ret == NO_ERROR)
			ret = make_devalias(e, "screen", CSTR, "/failsafe", CSTR);
	}

	return ret;
}

/* Secondary diagnostics run after the console is up for error messages.
   More rigorous testing should go here rather than at bootup just so that
   errors can be displayed */
CC(machine_secondary_diag)	/* (--) */
{
	return NO_ERROR;
}

/* set e->load to a reasonable place in memory */
CC(machine_init_load)
{
	e->load = e->fbrk + 8*1024;
	return NO_ERROR;
}

/* do whatever is needed to initialize the machine-dependent program
   at e->load
 */
CC(machine_init_program)	/* (--) */
{
	/* see if we have some supported executable image */
	if (exec_is_exec(e))
	{
		exec_free_symbols(e, e->loadsyms);
		e->loadsyms = exec_load_symbols(e);
		return NO_ERROR;
	}

	return E_BAD_IMAGE;
}

/* execute the code at e->load in such a way that it is possible to
   resume Forth (although this is not always possible)
 */
CC(machine_go)				/* (--) */
{
	Retcode ret;

	if (exec_is_exec(e))		/* sanity check */
	{
		ret = exec_load(e);

		/* load the symbols to try to catch errors */
		if (ret == NO_ERROR)
		{
			exec_free_symbols(e, e->loadsyms);
			e->loadsyms = exec_load_symbols(e);
		}

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
	static uByte font[] = {
		#include FONT_FILE
	};
	
	/* push the data for the default font on the stack */
	IFCKSP(e, 0, 6);
	PUSHP(e, font);
	PUSH(e, FONT_WIDTH);
	PUSH(e, FONT_HEIGHT);
	PUSH(e, FONT_ADVANCE);
	PUSH(e, FONT_FIRST);
	PUSH(e, FONT_COUNT);
	return NO_ERROR;
}

EC(front_end);

static const Initentry init_machdep[] =
{
	{ "g_e", (Command)0, INVALID_FCODE, F_NONE, T_ADDR },
	/*{ "gui", front_end, INVALID_FCODE, F_NONE, T_FUNC },*/
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
extern const Initentry init_sun[];
extern const Initentry init_stlb[];

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
	init_sun,
#ifdef STANDALONE
	init_tokenizer,
#endif
	init_machdep,
#ifdef SF_64BIT
	init_stlb,
#endif
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
EC(install_failsafe);
EC(install_display);
EC(install_gui);
EC(install_pci);
EC(install_obptftp);
EC(install_deblocker);
EC(install_disklabel);
EC(install_client_services);
EC(init_options_from_nvram);	/* forward to definition down below */

const Command install_list[] =
{
	install_root,		/* initialize / methods so rest of system works */
	install_chosen,		/* initialize /chosen, should be second */
	install_memory,		/* initialize /memory */
	install_cpu,		/* if there is any need of a /cpu node */
	install_failsafe,
	init_options_from_nvram,
	install_display,
	install_client_services,
	install_pci,
	install_obptftp,
	install_deblocker,
	install_disklabel,
#ifdef STANDALONE_GUI
	install_gui,
#endif
	NULL
};


/* This is used to specify a list of initial PCI drivers to load to handle
   builtin devices.  pci_drivers[] is only used by pci_load_builtin_driver()
   in pci.c.
 */
extern Pci_driver digital_21x4x_driver;

const Pci_driver *pci_drivers[] =
{
	&digital_21x4x_driver,
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
	&exec_aout,
	&exec_coff,
	&exec_elf,
	&exec_elf64,
#if !defined macintosh && !defined BeBox
	&exec_gzip,
#endif
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
	Int ret = fwrite(buf, sizeof (Byte), len, stdout);

	fflush(stdout);
	return ret;
}

/* Like above, only the data should be read from a serial port, or keyboard.
 */
Int
failsafe_read(Byte *buf, Int len)
{
	Int actual = fread(buf, sizeof (Byte), len, stdin);
	
	DPRINTF(("failsafe_read: actual=%d\n", actual));
	return actual == 0 ? -2 : actual;
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
	{ "auto-boot-timeout", "5000" },
	{ "boot-command", "boot" },
	{ "boot-device", "disk" },
	{ "boot-file", "" },
	{ "diag-device", "net" },
	{ "diag-file", "diag" },
	{ "diag-switch?", "true" },
	{ "secondary-diag?", "false" },
	{ "nvramrc", "" },
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
	{ "fcode-debug?", "false" },
	{ "security-mode", NULL },			/* NULL default value prevents */
	{ "security-#badlogins", NULL },	/* set-default(s) from working */
	{ "security-password", NULL },		/* for these three security words */
#ifdef SUN_COMPATIBILITY
	{ "load-base", "0" },
#endif
	{ NULL, NULL }
};

#define NVRAM_FILE	"nvram"


/* This should set the specified parameter in nvram to the specified value.
   The default version updates the entry in the file "nvram".
 */
Retcode
set_nvram(Environ *e, Byte *param, Int plen, Byte *val, Int vlen)
{
	Byte *s;
	FILE *fp, *nfp;

	setstrlen(&param, &plen);

	/* skip nvramrc as it is longer than one line */
	if (compare_strs(param, plen, "nvramrc", CSTR))
		return NO_ERROR;

	setstrlen(&val, &vlen);

	fp = fopen(NVRAM_FILE, "r");
	nfp = fopen(NVRAM_FILE ".new", "w");

	if (nfp == NULL)
		return E_NO_FILE;

	if (fp)
	{
		char *s;
		char buf[1024];

		while (fgets(buf, sizeof buf - 1, fp))
		{
			buf[strlen(buf) - 1] = '\0';	/* kill trailing newline */
			s = strchr(buf, '=');

			if (s == NULL || *buf == '#')
				continue;

			*s++ = '\0';

			/* skip this parameter if it is in the file */
			if (compare_strs(param, plen, buf, CSTR))
				continue;

			fwrite(buf, 1, strlen(buf), nfp);
			putc('=', nfp);
			fwrite(s, 1, strlen(s), nfp);
			putc('\n', nfp);
		}

		fclose(fp);
	}

	s = get_nvram_default(e, param, plen);

	if (vlen && (s == NULL ||
			(vlen != strlen(s) || strncmp(s, val, vlen) != 0)))
	{
		/* not the default value, so append to end of file */
		fwrite(param, 1, plen, nfp);
		putc('=', nfp);
		fwrite(val, 1, vlen, nfp);
		putc('\n', nfp);
	}

	/* finally move the new file over the old one */
	fclose(nfp);
	unlink(NVRAM_FILE);
	rename(NVRAM_FILE ".new", NVRAM_FILE);
	return NO_ERROR;
}

/* This should get the default value of the specified parameter in nvram.
   The usual version simply reads the static list above.
 */
Byte *
get_nvram_default(Environ *e, Byte *param, Int plen)
{
	struct nvram_data *nv;
	
	for (nv = nvram; nv->name; nv++)
		if (compare_strs(param, plen, nv->name, CSTR))
			return nv->val;
	
	return NULL;
}

/* This should get the value of the specified parameter in nvram.
   The static list above should be read as a fall-back if nvram
   is not readable, or has become corrupt.
 */
Byte *
get_nvram(Environ *e, Byte *param, Int plen)
{
	FILE *fp = fopen(NVRAM_FILE, "r");

	if (fp)
	{
		char *s;
		char buf[1024];

		while (fgets(buf, sizeof buf - 1, fp))
		{
			buf[strlen(buf) - 1] = '\0';	/* kill trailing newline */
			s = strchr(buf, '=');

			if (s == NULL || *buf == '#')
				continue;

			*s++ = '\0';

			if (compare_strs(param, plen, buf, CSTR))
			{
				static Byte b[STR_SIZE];
				strncpy(b, s, STR_SIZE - 1);
				fclose(fp);
				return b;
			}
		}

		fclose(fp);
	}

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
	struct nvram_data *nv;
	Retcode ret;

	for (nv = nvram; nv->name; nv++)
	{
		ret = prop_set_str(e->options->props, nv->name, CSTR, nv->val, CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	unlink(NVRAM_FILE);
	return NO_ERROR;
}


/* This is needed to initialize the /options node from nvram.
 */
CC(init_options_from_nvram)
{
	FILE *fp;
	struct nvram_data *nv;
	Retcode ret;

	for (nv = nvram; nv->name; nv++)
	{
		ret = prop_set_str(e->options->props, nv->name, CSTR, nv->val, CSTR);

		if (ret == NO_ERROR)
			ret = create_prop_alias(e, nv->name, CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	fp = fopen(NVRAM_FILE, "r");

	if (fp)
	{
		char *s;
		char buf[1024];

		while (fgets(buf, sizeof buf - 1, fp))
		{
			buf[strlen(buf) - 1] = '\0';	/* kill trailing newline */
			s = strchr(buf, '=');

			if (s == NULL || *buf == '#')
				continue;

			*s++ = '\0';

			/* do not save password under options */
			if (compare_strs(buf, CSTR, "security-password", CSTR))
				*s = '\0';

			ret = prop_set_str(e->options->props, buf, CSTR, s, CSTR);

			if (ret == NO_ERROR)
				ret = create_prop_alias(e, buf, CSTR);

			if (ret != NO_ERROR)
				return ret;
		}

		fclose(fp);
	}

	return NO_ERROR;
}


/* This is used to override nvram parameters arguments by those passed in
   to main().  The args must contain strings of "--parameter value" pairs.
 */
Retcode
machine_init_args(Environ *e, int argc, char *argv[])
{
	Retcode ret = NO_ERROR;
	int i;

	if (e->options->props == NULL)
		return E_INIT_ERROR;

	for (i = 0; i < argc; i += 2)
	{
		if (argv[i][0] != '-' || argv[i][1] != '-')
			break;

		ret = prop_set_str(e->options->props, (Byte*)argv[i] + 2, CSTR,
				(Byte*)argv[i + 1], CSTR);

		if (ret != NO_ERROR)
			return ret;
	}

	return ret;
}
