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

/* Program to dump out headers from an ELF binary to aid in building
   an executable/loader.
   ELF definitions copied from OpenBSD.  Original copyrights follow:
 */

/*	$OpenBSD: exec_elf.h,v 1.10 1996/12/11 05:55:33 imp Exp $	*/
/*
 * Copyright (c) 1995, 1996 Erik Theisen.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * These typedefs need to be handled better -
 *  doesn't work on 64-bit machines.  Note:
 *  there currently isn't a 64-bit ABI.
	[However, there appears to be ad-hoc support for 64-bits, so this
	file has been carefully hacked to be compiled for elf64.  --Parag]
 */
typedef unsigned int	Elf32_Addr;		/* Unsigned program address */
typedef unsigned int	Elf32_Off;		/* Unsigned file offset */
typedef int				Elf32_Sword;	/* Signed large integer */
typedef unsigned int	Elf32_Word;		/* Unsigned large integer */
typedef unsigned short	Elf32_Half;		/* Unsigned medium integer */

#ifdef INT64
/* 64-bit extras */
typedef unsigned long long	Elf64_Addr;	/* Unsigned program address */
typedef unsigned long long	Elf64_Off;	/* Unsigned file offset */
typedef long long		Elf64_Sword;	/* Signed large integer */
typedef unsigned long long	Elf64_Word;	/* Unsigned large integer */
typedef Elf64_Addr	Elf_Addr;		/* Unsigned program address */
typedef Elf64_Off	Elf_Off;		/* Unsigned file offset */
typedef Elf64_Word	Elf_Word;		/* Unsigned large integer */
typedef Elf64_Sword	Elf_Sword;		/* Unsigned large integer */
#define X "qX"
#define D "qd"
#else
typedef Elf32_Addr	Elf_Addr;		/* Unsigned program address */
typedef Elf32_Off	Elf_Off;		/* Unsigned file offset */
typedef Elf32_Word	Elf_Word;		/* Unsigned large integer */
typedef Elf32_Sword	Elf_Sword;		/* Unsigned large integer */
#define X "X"
#define D "d"
#endif

/* e_ident[] identification indexes */
#define EI_MAG0		0		/* file ID */
#define EI_MAG1		1		/* file ID */
#define EI_MAG2		2		/* file ID */
#define EI_MAG3		3		/* file ID */
#define EI_CLASS	4		/* file class */
#define EI_DATA		5		/* data encoding */
#define EI_VERSION	6		/* ELF header version */
#define EI_PAD		7		/* start of pad bytes */
#define EI_NIDENT	16		/* Size of e_ident[] */

/* e_ident[] magic number */
#define	ELFMAG0		0x7f		/* e_ident[EI_MAG0] */
#define	ELFMAG1		'E'		/* e_ident[EI_MAG1] */
#define	ELFMAG2		'L'		/* e_ident[EI_MAG2] */
#define	ELFMAG3		'F'		/* e_ident[EI_MAG3] */
#define	ELFMAG		"\177ELF"	/* magic */
#define	SELFMAG		4		/* size of magic */

/* e_ident[] file class */
#define	ELFCLASSNONE	0		/* invalid */
#define	ELFCLASS32		1		/* 32-bit objs */
#define	ELFCLASS64		2		/* 64-bit objs */
#define	ELFCLASSNUM		3		/* number of classes */

/* e_ident[] data encoding */
#define ELFDATANONE	0		/* invalid */
#define ELFDATA2LSB	1		/* Little-Endian */
#define ELFDATA2MSB	2		/* Big-Endian */
#define ELFDATANUM	3		/* number of data encode defines */

/* e_ident */
#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                      (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                      (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                      (ehdr).e_ident[EI_MAG3] == ELFMAG3)

/* ELF Header */
typedef struct elfhdr{
	unsigned char	e_ident[EI_NIDENT]; /* ELF Identification */
	Elf32_Half	e_type;			/* object file type */
	Elf32_Half	e_machine;		/* machine */
	Elf32_Word	e_version;		/* object file version */
	Elf_Addr	e_entry;		/* virtual entry point */
	Elf_Off		e_phoff;		/* program header table offset */
	Elf_Off		e_shoff;		/* section header table offset */
	Elf32_Word	e_flags;		/* processor-specific flags */
	Elf32_Half	e_ehsize;		/* ELF header size */
	Elf32_Half	e_phentsize;	/* program header entry size */
	Elf32_Half	e_phnum;		/* number of program header entries */
	Elf32_Half	e_shentsize;	/* section header entry size */
	Elf32_Half	e_shnum;		/* number of section header entries */
	Elf32_Half	e_shstrndx;		/* section header table's "section 
								   header string table" entry offset */
} Elf_Ehdr;

/* e_type */
#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* relocatable file */
#define ET_EXEC		2		/* executable file */
#define ET_DYN		3		/* shared object file */
#define ET_CORE		4		/* core file */
#define ET_NUM		5		/* number of types */
#define ET_LOPROC	0xff00		/* reserved range for processor */
#define ET_HIPROC	0xffff		/*  specific e_type */

/* e_machine */
#define EM_NONE		0		/* No Machine */
#define EM_M32		1		/* AT&T WE 32100 */
#define EM_SPARC	2		/* SPARC */
#define EM_386		3		/* Intel 80386 */
#define EM_68K		4		/* Motorola 68000 */
#define EM_88K		5		/* Motorola 88000 */
#define EM_486		6		/* Intel 80486 - unused? */
#define EM_860		7		/* Intel 80860 */
#define EM_MIPS		8		/* MIPS R3000 Big-Endian only */
/* 
 * Don't know if EM_MIPS_RS4_BE,
 * EM_SPARC64, EM_PARISC,
 * or EM_PPC are ABI compliant
 */
#define EM_MIPS_RS4_BE	10		/* MIPS R4000 Big-Endian */
#define EM_SPARC64	11		/* SPARC v9 64-bit unoffical */
#define EM_PARISC	15		/* HPPA */
#define EM_PPC		20		/* PowerPC */
#define EM_NUM		13		/* number of machine types */

/* Version */
#define EV_NONE		0		/* Invalid */
#define EV_CURRENT	1		/* Current */
#define EV_NUM		2		/* number of versions */

/* Section Header */
typedef struct {
	Elf32_Word	sh_name;		/* name - index into section header
								   string table section */
	Elf32_Word	sh_type;		/* type */
	Elf_Word	sh_flags;		/* flags */
	Elf_Addr	sh_addr;		/* address */
	Elf_Off		sh_offset;		/* file offset */
	Elf_Word	sh_size;		/* section size */
	Elf32_Word	sh_link;		/* section header table index link */
	Elf32_Word	sh_info;		/* extra information */
	Elf_Word	sh_addralign;	/* address alignment */
	Elf_Word	sh_entsize;		/* section entry size */
} Elf_Shdr;

/* Special Section Indexes */
#define SHN_UNDEF	0		/* undefined */
#define SHN_LORESERVE	0xff00		/* lower bounds of reserved indexes */
#define SHN_LOPROC	0xff00		/* reserved range for processor */
#define SHN_HIPROC	0xff1f		/*   specific section indexes */
#define SHN_ABS		0xfff1		/* absolute value */
#define SHN_COMMON	0xfff2		/* common symbol */
#define SHN_HIRESERVE	0xffff		/* upper bounds of reserved indexes */

/* sh_type */
#define SHT_NULL	0		/* inactive */
#define SHT_PROGBITS	1		/* program defined information */
#define SHT_SYMTAB	2		/* symbol table section */
#define SHT_STRTAB	3		/* string table section */
#define SHT_RELA	4		/* relocation section with addends*/
#define SHT_HASH	5		/* symbol hash table section */
#define SHT_DYNAMIC	6		/* dynamic section */
#define SHT_NOTE	7		/* note section */
#define SHT_NOBITS	8		/* no space section */
#define SHT_REL		9		/* relation section without addends */
#define SHT_SHLIB	10		/* reserved - purpose unknown */
#define SHT_DYNSYM	11		/* dynamic symbol table section */
#define SHT_NUM		12		/* number of section types */
#define SHT_LOPROC	0x70000000	/* reserved range for processor */
#define SHT_HIPROC	0x7fffffff	/*  specific section header types */
#define SHT_LOUSER	0x80000000	/* reserved range for application */
#define SHT_HIUSER	0xffffffff	/*  specific indexes */

/* Section names */
#define ELF_BSS         ".bss"		/* uninitialized data */
#define ELF_DATA        ".data"		/* initialized data */
#define ELF_DEBUG       ".debug"	/* debug */
#define ELF_DYNAMIC     ".dynamic"	/* dynamic linking information */
#define ELF_DYNSTR      ".dynstr"	/* dynamic string table */
#define ELF_DYNSYM      ".dynsym"	/* dynamic symbol table */
#define ELF_FINI        ".fini"		/* termination code */
#define ELF_GOT         ".got"		/* global offset table */
#define ELF_HASH        ".hash"		/* symbol hash table */
#define ELF_INIT        ".init"		/* initialization code */
#define ELF_REL_DATA    ".rel.data"	/* relocation data */
#define ELF_REL_FINI    ".rel.fini"	/* relocation termination code */
#define ELF_REL_INIT    ".rel.init"	/* relocation initialization code */
#define ELF_REL_DYN     ".rel.dyn"	/* relocaltion dynamic link info */
#define ELF_REL_RODATA  ".rel.rodata"	/* relocation read-only data */
#define ELF_REL_TEXT    ".rel.text"	/* relocation code */
#define ELF_RODATA      ".rodata"	/* read-only data */
#define ELF_SHSTRTAB    ".shstrtab"	/* section header string table */
#define ELF_STRTAB      ".strtab"	/* string table */
#define ELF_SYMTAB      ".symtab"	/* symbol table */
#define ELF_TEXT        ".text"		/* code */


/* Section Attribute Flags - sh_flags */
#define SHF_WRITE	0x1		/* Writable */
#define SHF_ALLOC	0x2		/* occupies memory */
#define SHF_EXECINSTR	0x4		/* executable */
#define SHF_MASKPROC	0xf0000000	/* reserved bits for processor */
					/*  specific section attributes */

/* Symbol Table Entry */
typedef struct elf32_sym {
	Elf32_Word	st_name;	/* name - index into string table */
	Elf32_Addr	st_value;	/* symbol value */
	Elf32_Word	st_size;	/* symbol size */
	unsigned char	st_info;	/* type and binding */
	unsigned char	st_other;	/* 0 - no defined meaning */
	Elf32_Half	st_shndx;	/* section header index */
} Elf32_Sym;

typedef struct elf64_sym {
	Elf32_Word	st_name;		/* name - index into string table */
	unsigned char	st_info;	/* type and binding */
	unsigned char	st_other;	/* 0 - no defined meaning */
	Elf32_Half	st_shndx;		/* section header index */
	Elf_Addr	st_value;		/* symbol value */
	Elf_Word	st_size;		/* symbol size */
} Elf64_Sym;

/* Symbol table index */
#define STN_UNDEF	0		/* undefined */

/* Extract symbol info - st_info */
#define ELF_ST_BIND(x)	((x) >> 4)
#define ELF_ST_TYPE(x)	(((unsigned int) x) & 0xf)
#define ELF_ST_INFO(b,t)	(((b) << 4) + ((t) & 0xf))

/* Symbol Binding - ELF_ST_BIND - st_info */
#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* like global - lower precedence */
#define STB_NUM		3		/* number of symbol bindings */
#define STB_LOPROC	13		/* reserved range for processor */
#define STB_HIPROC	15		/*  specific symbol bindings */

/* Symbol type - ELF_ST_TYPE - st_info */
#define STT_NOTYPE	0		/* not specified */
#define STT_OBJECT	1		/* data object */
#define STT_FUNC	2		/* function */
#define STT_SECTION	3		/* section */
#define STT_FILE	4		/* file */
#define STT_NUM		5		/* number of symbol types */
#define STT_LOPROC	13		/* reserved range for processor */
#define STT_HIPROC	15		/*  specific symbol types */

/* Relocation entry with implicit addend */
typedef struct 
{
	Elf_Addr	r_offset;	/* offset of relocation */
	Elf_Word	r_info;		/* symbol table index and type */
} Elf_Rel;

/* Relocation entry with explicit addend */
typedef struct 
{
	Elf_Addr	r_offset;	/* offset of relocation */
	Elf_Word	r_info;		/* symbol table index and type */
	Elf_Sword	r_addend;
} Elf_Rela;

/* Extract relocation info - r_info */
#define ELF32_R_SYM(i)		((i) >> 8)
#define ELF32_R_TYPE(i)		((i) & 0xFF)
#define ELF32_R_INFO(s,t) 	(((s) << 8) + ((t) & 0xFF))

/* 64-bit Extract relocation info - r_info */
#define ELF64_R_SYM(i)		((i) >> 32)
#define ELF64_R_TYPE(i)		((i) & 0xffffffff)
#define ELF64_R_INFO(s,t) 	(((s) << 32) + ((t) & 0xFFFFFFFF))

#ifdef INT64
/* Program Header */
typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Word	p_flags;	/* flags */
	Elf_Off		p_offset;	/* segment offset */
	Elf_Addr	p_vaddr;	/* virtual address of segment */
	Elf_Addr	p_paddr;	/* physical address - ignored? */
	Elf_Word	p_filesz;	/* number of bytes in file for seg. */
	Elf_Word	p_memsz;	/* number of bytes in mem. for seg. */
	Elf_Word	p_align;	/* memory alignment */
} Elf_Phdr;
#else
/* Program Header */
typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* segment offset */
	Elf_Addr	p_vaddr;	/* virtual address of segment */
	Elf_Addr	p_paddr;	/* physical address - ignored? */
	Elf_Word	p_filesz;	/* number of bytes in file for seg. */
	Elf_Word	p_memsz;	/* number of bytes in mem. for seg. */
	Elf_Word	p_flags;	/* flags */
	Elf_Word	p_align;	/* memory alignment */
} Elf_Phdr;
#endif

/* Segment types - p_type */
#define PT_NULL		0		/* unused */
#define PT_LOAD		1		/* loadable segment */
#define PT_DYNAMIC	2		/* dynamic linking section */
#define PT_INTERP	3		/* the RTLD */
#define PT_NOTE		4		/* auxiliary information */
#define PT_SHLIB	5		/* reserved - purpose undefined */
#define PT_PHDR		6		/* program header */
#define PT_NUM		7		/* Number of segment types */
#define PT_LOPROC	0x70000000	/* reserved range for processor */
#define PT_HIPROC	0x7fffffff	/*  specific segment types */

/* Segment flags - p_flags */
#define PF_X		0x1		/* Executable */
#define PF_W		0x2		/* Writable */
#define PF_R		0x4		/* Readable */
#define PF_MASKPROC	0xf0000000	/* reserved bits for processor */
					/*  specific segment flags */

/* Dynamic structure */
typedef struct 
{
	Elf_Sword	d_tag;		/* controls meaning of d_val */
	union 
	{
		Elf_Word	d_val;	/* Multiple meanings - see d_tag */
		Elf_Addr	d_ptr;	/* program virtual address */
	} d_un;
} Elf_Dyn;

extern Elf_Dyn	_DYNAMIC[];

/* Dynamic Array Tags - d_tag */
#define DT_NULL		0		/* marks end of _DYNAMIC array */
#define DT_NEEDED	1		/* string table offset of needed lib */
#define DT_PLTRELSZ	2		/* size of relocation entries in PLT */
#define DT_PLTGOT	3		/* address PLT/GOT */
#define DT_HASH		4		/* address of symbol hash table */
#define DT_STRTAB	5		/* address of string table */
#define DT_SYMTAB	6		/* address of symbol table */
#define DT_RELA		7		/* address of relocation table */
#define DT_RELASZ	8		/* size of relocation table */
#define DT_RELAENT	9		/* size of relocation entry */
#define DT_STRSZ	10		/* size of string table */
#define DT_SYMENT	11		/* size of symbol table entry */
#define DT_INIT		12		/* address of initialization func. */
#define DT_FINI		13		/* address of termination function */
#define DT_SONAME	14		/* string table offset of shared obj */
#define DT_RPATH	15		/* string table offset of library
					   search path */
#define DT_SYMBOLIC	16		/* start sym search in shared obj. */
#define DT_REL		17		/* address of rel. tbl. w addends */
#define DT_RELSZ	18		/* size of DT_REL relocation table */
#define DT_RELENT	19		/* size of DT_REL relocation entry */
#define DT_PLTREL	20		/* PLT referenced relocation entry */
#define DT_DEBUG	21		/* bugger */
#define DT_TEXTREL	22		/* Allow rel. mod. to unwritable seg */
#define DT_JMPREL	23		/* add. of PLT's relocation entries */
#define DT_NUM		24		/* Number used. */
#define DT_LOPROC	0x70000000	/* reserved range for processor */
#define DT_HIPROC	0x7fffffff	/*  specific dynamic array tags */

/* Standard ELF hashing function */
unsigned long elf_hash(const unsigned char *name);

#define ELF_TARG_VER	1	/* The ver for which this code is intended */


static char *strtab;

static void
swap(char *mem, int len)
{
	int t, i;

	for (i = 0; i < len / 2; i++)
	{
		t = mem[i];
		mem[i] = mem[len - i - 1];
		mem[len - i - 1] = t;
	}
}

int is64 = 0;
int do_swap = 0;

static void
dump_header(Elf_Ehdr *hdr)
{
	int i;

	printf("ELF header:\n");
	printf("ident=0x");

	for (i = 0; i < EI_NIDENT; i++)
		printf("%2X.", hdr->e_ident[i]);

	printf("\n");

#ifdef LITTLE_ENDIAN
	do_swap = (hdr->e_ident[EI_DATA] == ELFDATA2MSB);
#else
	do_swap = (hdr->e_ident[EI_DATA] == ELFDATA2LSB);
#endif

	if (do_swap)
	{
		swap((char*)&hdr->e_type, sizeof hdr->e_type);
		swap((char*)&hdr->e_machine, sizeof hdr->e_machine);
		swap((char*)&hdr->e_version, sizeof hdr->e_version);
		swap((char*)&hdr->e_entry, sizeof hdr->e_entry);
		swap((char*)&hdr->e_phoff, sizeof hdr->e_phoff);
		swap((char*)&hdr->e_shoff, sizeof hdr->e_shoff);
		swap((char*)&hdr->e_flags, sizeof hdr->e_flags);
		swap((char*)&hdr->e_ehsize, sizeof hdr->e_ehsize);
		swap((char*)&hdr->e_phentsize, sizeof hdr->e_phentsize);
		swap((char*)&hdr->e_phnum, sizeof hdr->e_phnum);
		swap((char*)&hdr->e_shentsize, sizeof hdr->e_shentsize);
		swap((char*)&hdr->e_shnum, sizeof hdr->e_shnum);
		swap((char*)&hdr->e_shstrndx, sizeof hdr->e_shstrndx);
	}

	printf("type=0x%X machine=0x%X version=0x%X entry=0x%" X "\n",
			hdr->e_type, hdr->e_machine, hdr->e_version, hdr->e_entry);
	printf("phoff=0x%" X " shoff=0x%" X " flags=0x%X ehsize=0x%X\n",
			hdr->e_phoff, hdr->e_shoff, hdr->e_flags, hdr->e_ehsize);
	printf(
		"phentsize=0x%X phunm=0x%X shentsize=0x%X shnum=0x%X shstrndx=0x%X\n",
			hdr->e_phentsize, hdr->e_phnum, hdr->e_shentsize,
			hdr->e_shnum, hdr->e_shstrndx);
}

static void
dump_program_header(Elf_Phdr *p, Elf_Word i)
{
	printf("program header %d:\n", i);
	p += i;

	if (do_swap)
	{
		swap((char*)&p->p_type, sizeof p->p_type);
		swap((char*)&p->p_offset, sizeof p->p_offset);
		swap((char*)&p->p_vaddr, sizeof p->p_vaddr);
		swap((char*)&p->p_paddr, sizeof p->p_paddr);
		swap((char*)&p->p_filesz, sizeof p->p_filesz);
		swap((char*)&p->p_memsz, sizeof p->p_memsz);
		swap((char*)&p->p_flags, sizeof p->p_flags);
		swap((char*)&p->p_align, sizeof p->p_align);
	}

	printf("type=0x%X offset=0x%" X " vaddr=0x%" X " paddr=0x%" X "\n",
			p->p_type, p->p_offset, p->p_vaddr, p->p_paddr);
	printf("filesz=0x%" X " memsz=0x%" X " flags=0x%X align=0x%" X "\n",
			p->p_filesz, p->p_memsz, p->p_flags, p->p_align);
}

static void
dump_section_header(Elf_Shdr *s, Elf_Word i)
{
	printf("section header %d:\n", i);
	s += i;

	if (do_swap)
	{
		swap((char*)&s->sh_name, sizeof s->sh_name);
		swap((char*)&s->sh_type, sizeof s->sh_type);
		swap((char*)&s->sh_flags, sizeof s->sh_flags);
		swap((char*)&s->sh_addr, sizeof s->sh_addr);
		swap((char*)&s->sh_offset, sizeof s->sh_offset);
		swap((char*)&s->sh_size, sizeof s->sh_size);
		swap((char*)&s->sh_link, sizeof s->sh_link);
		swap((char*)&s->sh_info, sizeof s->sh_info);
		swap((char*)&s->sh_addralign, sizeof s->sh_addralign);
		swap((char*)&s->sh_entsize, sizeof s->sh_entsize);
	}

	printf("name=%s (0x%X) type=0x%X flags=0x%" X " addr=0x%" X " offset=0x%" X "\n",
			strtab + s->sh_name,
			s->sh_name, s->sh_type, s->sh_flags, s->sh_addr, s->sh_offset);
	printf("size=0x%" X " link=0x%X info=0x%X addralign=0x%" X " entsize=0x%" X "\n",
			s->sh_size, s->sh_link, s->sh_info, s->sh_addralign, s->sh_entsize);
}

static void
dump_symbols(char *image, Elf_Ehdr *hdr, Elf_Shdr *scn)
{
	Elf_Shdr *symtab = NULL;
	Elf_Shdr *strhdr = NULL;
	Elf_Word i = 0;
	Elf_Word num;
	char *strs;
#ifdef INT64
	Elf64_Sym *syms, *s;
#else
	Elf32_Sym *syms, *s;
#endif

	for (i = 0; i < hdr->e_shnum; i++)
	{
		if (strcmp(strtab + scn[i].sh_name, ELF_SYMTAB) == 0)
			symtab = scn + i;
		else if (strcmp(strtab + scn[i].sh_name, ELF_STRTAB) == 0)
			strhdr = scn + i;
	}

	if (symtab == NULL || strhdr == NULL)
	{
		printf("NULL symtab and/or strhdr?!?\n");
		return;
	}

	if (symtab->sh_entsize != sizeof *syms)
	{
		printf("symtab->sh_entsize = " D " != sizeof *syms = %d?!?\n",
				symtab->sh_entsize, sizeof *syms);
		return;
	}

	strs = image + strhdr->sh_offset;
#if INT64
	s = syms = (Elf64_Sym*)(image + symtab->sh_offset);
#else
	s = syms = (Elf32_Sym*)(image + symtab->sh_offset);
#endif
	num = symtab->sh_size / sizeof *syms;

	for (i = 0; i < num; i++, s++)
	{
		if (do_swap)
		{
			swap((char*)&s->st_name, sizeof s->st_name);
			swap((char*)&s->st_value, sizeof s->st_value);
			swap((char*)&s->st_size, sizeof s->st_size);
			swap((char*)&s->st_shndx, sizeof s->st_shndx);
		}

		if ((ELF_ST_BIND(s->st_info) == STB_GLOBAL ||
				ELF_ST_BIND(s->st_info) == STB_LOCAL) &&
				(ELF_ST_TYPE(s->st_info) == STT_OBJECT ||
				ELF_ST_TYPE(s->st_info) == STT_FUNC))
			printf("%*" D ": %s value=0x%" X " size=0x%" X " info=0x%X shndx=0x%X\n",
					sizeof i, i, strs + s->st_name, s->st_value, s->st_size,
					s->st_info, s->st_shndx);
	}
}

static void
dumpimage(char *image, Elf_Word imagelen)
{
	Elf_Ehdr *hdr;
	Elf_Shdr *scnhdr;
	Elf_Phdr *proghdr;
	Elf_Off	sh_offset;
	Elf_Word i, len = 0;

	hdr = (Elf_Ehdr*)image;

	if (!IS_ELF(*hdr))
	{
		printf("not ELF binary\n");
		return;
	}

	dump_header(hdr);
	proghdr = (Elf_Phdr*)(image + hdr->e_phoff);
	scnhdr = (Elf_Shdr*)(image + hdr->e_shoff);
	sh_offset = scnhdr[hdr->e_shstrndx].sh_offset;

	if (do_swap)
		swap((char*)&sh_offset, sizeof sh_offset);

	strtab = image + sh_offset;
	printf("\n");

	for (i = 0; i < hdr->e_phnum; i++)
		dump_program_header(proghdr, i);

	printf("\n");

	for (i = 0; i < hdr->e_shnum; i++)
	{
		dump_section_header(scnhdr, i);

		if (scnhdr[i].sh_type != SHT_NOBITS &&
				len < scnhdr[i].sh_offset + scnhdr[i].sh_size)
			len = scnhdr[i].sh_offset + scnhdr[i].sh_size;
	}

	printf("\nimage len=%d\n\n", len);

	dump_symbols(image, hdr, scnhdr);
}


int
main(int argc, char *argv[])
{
	FILE *fp;
	char *image;
	int imagelen;
	struct stat statbuf;

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s file\n", argv[0]);
		exit(-1);
	}

	fp = fopen(argv[1], "r");

	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open %s!\n", argv[1]);
		exit(1);
	}

	if (fstat(fileno(fp), &statbuf) < 0)
	{
		fprintf(stderr, "Cannot stat %s!\n", argv[1]);
		exit(2);
	}

	printf("%s is %d bytes long\n", argv[1], statbuf.st_size);
	imagelen = statbuf.st_size;
	image = (char *)malloc(imagelen);

	if (image == NULL)
	{
		fprintf(stderr, "Cannot allocate space for %s!\n", argv[1]);
		exit(3);
	}

	if (fread(image, sizeof (char), imagelen, fp) != imagelen)
	{
		fprintf(stderr, "Cannot read %s!\n", argv[1]);
		exit(2);
	}

	printf("%s image is in memory at 0x%X\n", argv[1], image);
	dumpimage(image, imagelen);
	exit(0);
}
