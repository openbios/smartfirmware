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


#include <stdarg.h>

#include "sfclient.h"

extern int bprintf(char *, const char *, ...);
extern void memset(char *, int, int);
extern void memcpy(char *, char *, int);
extern int strlen(char *);
extern void strcpy(char *, char *);
extern void strcat(char *, char *);

/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }

/* copy into memory according to headers */
#define IMAGE_LOC	0xC0400000
#define ZIMAGE_LOC	0xC0028000
#define INITRD_LOC	0xC0600000

static int g_boot_options = 0;
static char *g_nfs_root_path = NULL;
static char *g_server_ip_string = NULL;
static char *g_client_ip_string = NULL;
static char *g_initrd_name = NULL;
static char *g_root_dev_path = NULL;

#define BOOTOPT_NFS	1
#define BOOTOPT_INITRD	2
#define BOOTOPT_DEV	3
#define BOOTOPT_ZIMAGE	4

#define EP7312_BASE 0x80000000
#define EP7312_RB(addr)		(*(volatile uByte *)(EP7312_BASE + ((addr) & 0x3FFF)))
#define EP7312_R(addr)		(*(volatile uInt *)(EP7312_BASE + ((addr) & 0x3FFF)))
#define EP7312_W(addr,val)	(*(volatile uInt *)(EP7312_BASE + ((addr) & 0x3FFF)) = (val))

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
dputs(uByte *buf, uInt len)
{
	while (len)
	{
		while (EP7312_R(DEBUG_PORT + UART_STAT) & UART_STAT_TXFULL)
			;

		EP7312_W(DEBUG_PORT + UART_DATA, *buf++ & UART_DATA_MASK);
		len--;
	}

	while (EP7312_R(DEBUG_PORT + UART_STAT) & UART_STAT_BUSY)
		;
}

extern int vbprintf(char *buf, const char *fmt, va_list args);

int
dprintf(const char *fmt, ...)
{
	char buf[2048];
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vbprintf(buf, fmt, args);
	va_end(args);
	dputs(buf, strlen(buf));
	return ret;
}

#define DPRINTF(x)

uInt g_initrd = 0;
uInt g_initrdsize = 0;

/* I/O OPERATIONS */
typedef uInt	Elf32_Addr;	/* Unsigned program address */
typedef uInt	Elf32_Off;	/* Unsigned file offset */
typedef Int		Elf32_Sword;	/* Signed large integer */
typedef uInt	Elf32_Word;	/* Unsigned large integer */
typedef uShort	Elf32_Half;	/* Unsigned medium integer */

#define EI_NIDENT	16		/* Size of e_ident[] */

typedef struct elfhdr{
	unsigned char	e_ident[EI_NIDENT]; /* ELF Identification */
	Elf32_Half	e_type;		/* object file type */
	Elf32_Half	e_machine;	/* machine */
	Elf32_Word	e_version;	/* object file version */
	Elf32_Addr	e_entry;	/* virtual entry point */
	Elf32_Off	e_phoff;	/* program header table offset */
	Elf32_Off	e_shoff;	/* section header table offset */
	Elf32_Word	e_flags;	/* processor-specific flags */
	Elf32_Half	e_ehsize;	/* ELF header size */
	Elf32_Half	e_phentsize;	/* program header entry size */
	Elf32_Half	e_phnum;	/* number of program header entries */
	Elf32_Half	e_shentsize;	/* section header entry size */
	Elf32_Half	e_shnum;	/* number of section header entries */
	Elf32_Half	e_shstrndx;	/* section header table's "section 
					   header string table" entry offset */
} Elf32_Ehdr;

/* Program Header */
typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* segment offset */
	Elf32_Addr	p_vaddr;	/* virtual address of segment */
	Elf32_Addr	p_paddr;	/* physical address - ignored? */
	Elf32_Word	p_filesz;	/* number of bytes in file for seg. */
	Elf32_Word	p_memsz;	/* number of bytes in mem. for seg. */
	Elf32_Word	p_flags;	/* flags */
	Elf32_Word	p_align;	/* memory alignment */
} Elf32_Phdr;

#define PT_LOAD		1		/* loadable segment */

/* prepare to run the ELF image
 */
void
elf_load(uByte *load, uLong *entrypoint)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)load;
	Elf32_Phdr *phdr;
	int i;

	phdr = (Elf32_Phdr*)(load + hdr->e_phoff);
	*entrypoint = hdr->e_entry;		/* MUST return this */

	/* load all programs in this file */
	for (i = 0; i < hdr->e_phnum; i++, phdr++)
	{
		if (phdr->p_type != PT_LOAD)		/* only loadable types */
			continue;

		DPRINTF(("vaddr %#X, load+off %#X, msize %d, fsize %d\n", phdr->p_vaddr,
				load + phdr->p_offset, phdr->p_memsz, phdr->p_filesz));

		/* copy in the entire chunk of program */
		memcpy((char*)phdr->p_vaddr, load + phdr->p_offset, phdr->p_filesz);

		/* if in-memory size is larger, zero out the difference */
		if (phdr->p_memsz > phdr->p_filesz)
			memset((char*)phdr->p_vaddr + phdr->p_filesz, 0,
					phdr->p_memsz - phdr->p_filesz);
	}
}

void
launch(uInt r0, uInt r1, uInt r2, uInt r3, uInt entrypoint)
{
	asm("	ldr	r4, [fp, #4]");
	asm("	mov	pc,r4");
}

uInt
download_image(char *name, uInt addr)
{
	char device[256];
	Instance *inst;
	Cell args[1];
	Cell returns[2];
	Retcode ret;

	strcpy(device, "net:,");
	strcat(device, name);
	inst = sf_open(device);

	if (inst == NULL)
		return 0;

	args[0] = addr;

	ret = sf_call_method("load", inst, 1, 2, args, returns);
	sf_close(inst);

	if (ret != R_OK || returns[0] != 0)
		return 0;

	return returns[1];
}
/* I/O OPERATIONS */

/* CPU OPERATIONS */
void
disable_interrupts()
{
    EP7312_W(0x280, 0);
    EP7312_W(0x1280, 0);
    EP7312_W(0x2280, 0);
}

void
flush_cache()
{
    	asm("mov     	r0, #0x00000000");
    	asm("mcr     	p15, 0, r0, c7, c7, 0");
}

void
flush_tlb()
{
    	asm("mov     	r0, #0x00000000");
    	asm("mcr     	p15, 0, r0, c8, c7, 0");
	asm("mov	r0,r0");		/* NOP */
	asm("mov	r0,r0");		/* NOP */
}

void
disable_mmu()
{
	asm("mov		r0, #0x00000070");
	asm("mcr		p15, 0, r0, c1, c0");
}

void
bogus(char *fmt, ...)
{
}

void
set_sp(int addr)
{
	int sp;
	int fix;
	asm("mov	%0,sp" : "=r"(sp));	/* get old SP */
	bogus("sp[0] = 0x%X\n", ((int *)sp)[0]);
#ifdef DEBUG
	dprintf("sp[0] = 0x%X\n", ((int *)sp)[0]);
	dprintf("sp[1] = 0x%X\n", ((int *)sp)[1]);
	dprintf("sp[2] = 0x%X\n", ((int *)sp)[2]);
	dprintf("sp[3] = 0x%X\n", ((int *)sp)[3]);
	dprintf("sp[4] = 0x%X\n", ((int *)sp)[4]);
	dprintf("sp[5] = 0x%X\n", ((int *)sp)[5]);
#endif
	fix = addr - sp;
	memcpy((void *)(addr - 4), (void *)(sp - 4), 0x1000);
	asm("mov	sp,r5");		/* load new SP */
	asm("add	ip, sp, #28");		/* create frame */
	asm("sub	fp, ip, #4");
	*(int *)(addr + 12) += fix;		/* fix FP */
	*(int *)(addr + 16) += fix;		/* fix SP */
#ifdef DEBUG
	dprintf("nsp[0] = 0x%X\n", ((int *)addr)[0]);
	dprintf("nsp[1] = 0x%X\n", ((int *)addr)[1]);
	dprintf("nsp[2] = 0x%X\n", ((int *)addr)[2]);
	dprintf("nsp[3] = 0x%X\n", ((int *)addr)[3]);
	dprintf("nsp[4] = 0x%X\n", ((int *)addr)[4]);
	dprintf("nsp[5] = 0x%X\n", ((int *)addr)[5]);
#endif
}
/* CPU OPERATIONS */

/* KEYPAD */
#define SYSCON1	0x80000100
#define PORTA	0x80000000

#define IN(addr) (*(volatile unsigned int *)(addr))
#define OUT(addr, val) (*(volatile unsigned int *)(addr) = (val))

unsigned int
getkeystate()
{
	unsigned int syscon1 = IN(SYSCON1);
	unsigned int porta;
	OUT(SYSCON1, (syscon1 & ~0xF) | 0xD);
	porta = IN(PORTA);
	OUT(SYSCON1, syscon1);
	return porta & 0x3F;
}

unsigned int
waitanykey(unsigned int keys)
{
    unsigned int state = getkeystate();

    while ((state & keys) == 0)
	    state = getkeystate();

    return state;
}
/* KEYPAD */

/* PROPERTY OPERATIONS */
int
sf_strncmp(const char *s1, const char *s2, int len)
{
	if (s1 == NULL)
		s1 = "";

	if (s2 == NULL)
		s2 = "";

	for (; *s1 && *s2 && len--; s1++, s2++)
		if (*s1 != *s2)
			break;

	return (!len && !*s1 && !*s2) ? 0 : *s2 - *s1;
}

Package *
find_device(char *device)
{
	Package *pkg = sf_getroot();

	if (*device != '/')
		return NULL;

	while (*device)
	{
		char *dev2;
		Package *child;
		int ch;
		char name[256];

		device++;
		dev2 = device;

		while (*dev2 != '\0' && *dev2 != '/')
			dev2++;

		ch = *dev2;
		*dev2 = '\0';

		for (child = sf_child(pkg); child; child = sf_peer(child)) 
		{
			int len = sf_getprop(child, "name", name, 256);

			if (len >= 0 && sf_strncmp(device, name, len) == 0)
				break;
		}

		if (child == NULL)
			return NULL;

		*dev2 = ch;
		device = dev2;
		pkg = child;
	}

	return pkg;
}

int
get_string_property(char *device, char *property, char *buf, int buflen)
{
	Instance *devinst;
	Package *devpkg;
	int len;
	int len2;

#ifdef DEBUG
	dprintf("get_str_prop: dev ");
	dprintf(device);
	dprintf(", prop ");
	dprintf(property);
	dprintf("\r\n");
#endif

	devinst = sf_open(device);

	if (devinst == NULL)
	{
#ifdef DEBUG
		dprintf("get_str_prop: device failed to opened\r\n");
#endif
		devpkg = find_device(device);

		if (devpkg == NULL)
			return 0;
	}
	else
	{
#ifdef DEBUG
		dprintf("get_str_prop: device opened\r\n");
#endif
		devpkg = sf_instance_to_package(devinst);

		if (devpkg == NULL)
		{
			sf_close(devinst);
			return 0;
		}
	}

#ifdef DEBUG
	dprintf("get_str_prop: got package\r\n");
#endif

	len = sf_getproplen(devpkg, property);

	if (len <= 0)
	{
		if (devinst)
			sf_close(devinst);

		return 0;
	}

#ifdef DEBUG
	dprintf("get_str_prop: length looks good\r\n");
#endif

	if (len >= buflen)
		len = buflen - 1;

	len2 = sf_getprop(devpkg, property, buf, len);

	if (len2 <= 0)
	{
		if (devinst)
			sf_close(devinst);

		return 0;
	}
		
	buf[len] = '\0';
#ifdef DEBUG
	dprintf("get_str_prop: got string prop \"");
	dprintf(buf);
	dprintf("\"\r\n");
#endif
	return 1;
}
/* PROPERTY OPERATIONS */

/* LINUX KERNEL COMMAND LINE */
uByte *
get_bootp_reply(uInt *len)
{
	static uByte buf[512];
	/* get the /chosen bootreply-packet property */

	if (len)
		*len = sizeof buf;

	if (get_string_property("/chosen", "bootreply-packet", (char *)buf,
			sizeof buf))
		return buf;

	return (uByte *)NULL;
}

#define BOOTP_YIADDR_OFFSET	(14+20+8+16)
#define BOOTP_SIADDR_OFFSET	(14+20+8+20)

char *
get_server_ip_string()
{
	uByte *bootpreply;
	static char str[100];

	bootpreply = get_bootp_reply(NULL);

	if (bootpreply)
	{
		/* format server IP from bootp reply */
		bprintf(str, "%d.%d.%d.%d", bootpreply[BOOTP_SIADDR_OFFSET],
				bootpreply[BOOTP_SIADDR_OFFSET+1],
				bootpreply[BOOTP_SIADDR_OFFSET+2],
				bootpreply[BOOTP_SIADDR_OFFSET+3]);
		return str;
	}

	/* no bootp packet, OK looks from NVRAM server-ip variable */
	if (get_string_property("/options", "server-ip", str, sizeof str))
		return str;

	dprintf("Cannot find the server's IP address, aborting\n");
	sf_exit();
	return NULL;
}

char *
get_client_ip_string()
{
	uByte *bootpreply;
	static char str[100];

	bootpreply = get_bootp_reply(NULL);

	if (bootpreply)
	{
		/* format client IP from bootp reply */
		bprintf(str, "%d.%d.%d.%d", bootpreply[BOOTP_YIADDR_OFFSET],
				bootpreply[BOOTP_YIADDR_OFFSET+1],
				bootpreply[BOOTP_YIADDR_OFFSET+2],
				bootpreply[BOOTP_YIADDR_OFFSET+3]);
		return str;
	}

	/* no bootp packet, OK looks from NVRAM client-ip variable */
	if (get_string_property("/options", "client-ip", str, sizeof str))
		return str;

	dprintf("Cannot find our IP address, aborting\n");
	sf_exit();
	return NULL;
}

char *cmds[] = 
{
	NULL,
};

static void
addcmd(char **buf, char *cmd)
{
	char *s = *buf;
	strcpy(s, cmd);
	s += strlen(s);
	*s++ = ' ';
	*s = '\0';
	*buf = s;
}

char *
mkcmdline()
{
	static char buf[1024];
	static char buf2[1024];
	char *s = buf;
	int i;

	buf[0] = '\0';

	for (i = 0; cmds[i]; i++)
		addcmd(&s, cmds[i]);

	switch (g_boot_options)
	{
	case BOOTOPT_NFS:
		/* add NFS specific options */
		addcmd(&s, "root=/dev/nfs");
		bprintf(buf2, "nfsroot=%s:%s", g_server_ip_string,
				g_nfs_root_path);
		addcmd(&s, buf2);
		bprintf(buf2, "ip=%s", g_client_ip_string);
		addcmd(&s, buf2);
		break;
	case BOOTOPT_INITRD:
		addcmd(&s, "root=/dev/ram0");
		break;
	case BOOTOPT_DEV:
		bprintf(buf2, "root=%s", g_root_dev_path);
		addcmd(&s, buf2);
		break;
	case BOOTOPT_ZIMAGE:
		break;
	}

	return buf;
}
/* LINUX KERNEL COMMAND LINE */

/* BUILD TAGS */
uInt *
buildnonetag(uInt *tags)
{
	*tags++ = 0;			/* num words */
	*tags++ = 0x00000000;		/* NONE */
	return tags;
}

uInt *
buildcoretag(uInt *tags, uInt flags, uInt pagesize, uInt rootdev)
{
	*tags++ = 5;			/* num words */
	*tags++ = 0x54410001;		/* CORE */
	*tags++ = flags;
	*tags++ = pagesize;
	*tags++ = rootdev;
	return tags;
}

uInt *
buildmemtag(uInt *tags, uInt start, uInt size)
{
	*tags++ = 4;			/* num words */
	*tags++ = 0x54410002;		/* MEM */
	*tags++ = start;
	*tags++ = size;
	return tags;
}

uInt *
buildramdisktag(uInt *tags, uInt flags, uInt size, uInt flpstart)
{
	*tags++ = 5;			/* num words */
	*tags++ = 0x54410004;		/* RAMDISK */
	*tags++ = flags;
	*tags++ = size;
	*tags++ = flpstart;
	return tags;
}

uInt *
buildinitrdtag(uInt *tags, uInt start, uInt size)
{
	*tags++ = 4;			/* num words */
	*tags++ = 0x54410005;		/* INITRD */
	*tags++ = start;
	*tags++ = size;
	return tags;
}

uInt *
buildserialnumtag(uInt *tags, uInt hi, uInt lo)
{
	*tags++ = 4;			/* num words */
	*tags++ = 0x54410006;		/* SERIAL */
	*tags++ = hi;
	*tags++ = lo;
	return tags;
}

uInt *
buildboardrevtag(uInt *tags, uInt rev)
{
	*tags++ = 3;			/* num words */
	*tags++ = 0x54410007;		/* REVISION */
	*tags++ = rev;
	return tags;
}

uInt *
buildcmdlinetag(uInt *tags, char *cmdline)
{
	int l = strlen(cmdline) + 1;
	tags[0] = (l + 3 + 8) / 4;	/* num words */
	tags[1] = 0x54410009;		/* CMDLINE */
	strcpy((char *)&tags[2], cmdline);
	tags += tags[0];
	return tags;
}

void
showtags(uInt *tags)
{
	uInt sz;
	uInt t;

	for (; ; tags += sz)
	{
		sz = tags[0];
		t = tags[1];

		switch (t & 0xFFFF)
		{
		case 1:			/* CORE */
			dprintf("CORE: %#x, %#x, %#x\n", tags[2], tags[3],
					tags[4]);
			break;
		case 2:			/* MEM */
			dprintf("MEM: %#x, %#x\n", tags[2], tags[3]);
			break;
		case 4:			/* RAMDISK */
			dprintf("RAMDISK: %#x, %#x, %#x\n", tags[2], tags[3],
					tags[4]);
			break;
		case 5:
			dprintf("INITRD: %#x, %#x\n", tags[2], tags[3]);
			break;
		case 6:
			dprintf("SERIAL: %#x, %#x\n", tags[2], tags[3]);
			break;
		case 7:
			dprintf("REVISION: %#x\n", tags[2]);
			break;
		case 9:
			dprintf("CMDLINE: cmdline=\"%s\"\n", (char *)&tags[2]);
			break;
		default:
			if ((t >> 16) != 0x5441)
				return;

			dprintf("TAG%04X: ?\n", t & 0xFFFF);
			break;
		}
	}
}

void
buildtags(uInt *tags)
{
	uInt *t = tags;

	t = buildcoretag(t, 0x0, 0x1000, 0x0);
#if 0
	/* need to build a kernel without meminfo before we can use this. */
	t = buildmemtag(t, 0xC0000000, 0x1000000);
#endif

	if (g_initrdsize)
	{
		t = buildramdisktag(t, 0, 8192, 0);
		t = buildinitrdtag(t, g_initrd, g_initrdsize);
	}
#if 0
	t = buildserialnumtag(t, 0, 0);
	t = buildboardrevtag(t, 6);
#endif
	t = buildcmdlinetag(t, mkcmdline());
	t = buildnonetag(t);
	showtags(tags);
}

/* BUILD TAGS */

/* PARSE COMMANDS */
void
parse_args(char *args)
{
	char *s = args;
	char *opt;
	char *arg;

	while (*s && *s != ' ' && *s != '\t')
		s++;

	while (*s == ' ' || *s == '\t')
		s++;

	opt = s;

	while (*s && *s != ' ' && *s != '\t')
		s++;

	if (*s != '\0')
		*s++ = '\0';

	while (*s == ' ' || *s == '\t')
		s++;

	arg = s;

#ifdef DEBUG
	dprintf("options = \"%s\", arg = \"%s\"\n", opt, arg);
#endif

	if (sf_strncmp(opt, "nfs", 4) == 0)
	{
		/* NFS options */
		g_boot_options = BOOTOPT_NFS;
		g_nfs_root_path = arg;
#ifdef DEBUG
		dprintf("NFS root path = %s (%p)\n", g_nfs_root_path, g_nfs_root_path);
#endif
		g_server_ip_string = get_server_ip_string();
		g_client_ip_string = get_client_ip_string();
	}
	else if (sf_strncmp(opt, "initrd", 7) == 0)
	{
		g_boot_options = BOOTOPT_INITRD;
		g_initrd_name = arg;
	}
	else if (sf_strncmp(opt, "dev", 4) == 0)
	{
		g_boot_options = BOOTOPT_DEV;
		g_root_dev_path = arg;
	}
	else if (sf_strncmp(opt, "zimage", 7) == 0)
		g_boot_options = BOOTOPT_ZIMAGE;
	else if (*opt != '\0')
	{
		dprintf("unrecognized options \"%s\"\n", opt);
		sf_exit();
	}
}
/* PARSE COMMANDS */

/* MAIN */
int
main(int argc, const char *argv[], const char *envv[])
{
	uInt size;
	uLong entrypoint;
	uInt loc = IMAGE_LOC;
	static char bootpath[1024];
	static char bootargs[1024];

	dprintf("EDB7312 Linux loader (v0.2)...\n");
#ifdef DEBUG
	dprintf("C00FC000 = %08X\n", *(uInt *)0xC00FC000);
	dprintf("C00FE000 = %08X\n", *(uInt *)0xC00FE000);
	dprintf("C0090C00 = %08X\n", *(uInt *)0xC0090C00);
	dprintf("C00C0C00 = %08X\n", *(uInt *)0xC00C0C00);
#endif

	/* get bootpath and bootargs */
	get_string_property("/chosen", "bootpath", bootpath, sizeof bootpath);
	get_string_property("/chosen", "bootargs", bootargs, sizeof bootargs);

	dprintf("bootpath <%s>, bootargs <%s>\n", bootpath, bootargs);

	parse_args(bootargs);

	if (g_boot_options == BOOTOPT_ZIMAGE)
	{
		dprintf("Loading zImage: ");
		size = download_image("zImage", loc);
	}
	else
	{
		dprintf("Loading vmlinux: ");
		size = download_image("vmlinux", loc);
	}

	dprintf(" \r\n");

	if (size == 0)
	{
		dprintf("Unable to load vmlinux via TFTP\r\n");
		return R_OK;
	}

	if (g_boot_options == BOOTOPT_INITRD)
	{
		if (g_initrd_name == '\0')
			g_initrd_name = "ramdisk.gz";

		dprintf("Loading %s: ", g_initrd_name);
		g_initrd = INITRD_LOC;
		g_initrdsize = download_image(g_initrd_name, g_initrd);
		dprintf(" \r\n");
	}

	dprintf("Linux image loaded\r\n");

	/* turn off interrupts */
	disable_interrupts();
	set_sp(0xC0FF0000);

	/* flush cache */
	flush_cache();
#ifdef DEBUG
	dprintf("cache flushed\r\n");
#endif

	/* flush TLB */
	flush_tlb();
#ifdef DEBUG
	dprintf("TLB flushed\r\n");
#endif

	/* disable MMU */
	disable_mmu();
#ifdef DEBUG
	dprintf("MMU now disabled\r\n");
#endif

	/* flush cache */
	flush_cache();
#ifdef DEBUG
	dprintf("cache flushed\r\n");
#endif

	/* flush TLB */
	flush_tlb();
#ifdef DEBUG
	dprintf("TLB flushed\r\n");
#endif

	/* move image into position */
	if (g_boot_options == BOOTOPT_ZIMAGE)
	{
		memcpy((void *)ZIMAGE_LOC, (void *)loc, size);
		dprintf("zImage moved into place, entrypoint=%#x\n", ZIMAGE_LOC);
		entrypoint = (uLong)ZIMAGE_LOC;
	}
	else
	{
		elf_load((uByte *)loc, &entrypoint);
		dprintf("ELF image moved into place, entrypoint=%#x\n", entrypoint);
	}

	buildtags((uInt *)0xC0020100);

	dprintf("About to launch\r\n");
#if 0
	waitanykey(0x3F);
#endif

	/* branch to entry point */
	launch(0, 131, 0, 0, entrypoint);
	return R_OK;
}
/* MAIN */
