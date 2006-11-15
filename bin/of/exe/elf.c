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

/* ELF-format program loader for SmartFirmware.
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

#include "defs.h"
#include "exe.h"

/*
 * These typedefs need to be handled better -
 *  doesn't work on 64-bit machines.  Note:
 *  there currently isn't a 64-bit ABI.
 */
typedef uInt	Elf32_Addr;	/* Unsigned program address */
typedef uInt	Elf32_Off;	/* Unsigned file offset */
typedef Int		Elf32_Sword;	/* Signed large integer */
typedef uInt	Elf32_Word;	/* Unsigned large integer */
typedef uShort	Elf32_Half;	/* Unsigned medium integer */

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
#define	ELFMAG1		'E'			/* e_ident[EI_MAG1] */
#define	ELFMAG2		'L'			/* e_ident[EI_MAG2] */
#define	ELFMAG3		'F'			/* e_ident[EI_MAG3] */
#define	ELFMAG		"\177ELF"	/* magic */
#define	SELFMAG		4			/* size of magic */

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

#ifdef LITTLE_ENDIAN
#define OUR_ELF_ENDIAN	ELFDATA2LSB
#else
#define OUR_ELF_ENDIAN	ELFDATA2MSB
#endif

/* e_ident */
#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                      (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                      (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                      (ehdr).e_ident[EI_MAG3] == ELFMAG3)

/* ELF Header */
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
#define EM_SPARC64		11		/* SPARC v9 64-bit unoffical */
#define EM_PARISC		15		/* HPPA */
#define EM_SPARC32PLUS	18		/* another SPARC */
#define EM_PPC			20		/* PowerPC */
#define EM_ARM		40		/* ARM */
#define EM_NUM			13		/* number of machine types */

/* Version */
#define EV_NONE		0		/* Invalid */
#define EV_CURRENT	1		/* Current */
#define EV_NUM		2		/* number of versions */

/* Section Header */
typedef struct {
	Elf32_Word	sh_name;	/* name - index into section header
					   string table section */
	Elf32_Word	sh_type;	/* type */
	Elf32_Word	sh_flags;	/* flags */
	Elf32_Addr	sh_addr;	/* address */
	Elf32_Off	sh_offset;	/* file offset */
	Elf32_Word	sh_size;	/* section size */
	Elf32_Word	sh_link;	/* section header table index link */
	Elf32_Word	sh_info;	/* extra information */
	Elf32_Word	sh_addralign;	/* address alignment */
	Elf32_Word	sh_entsize;	/* section entry size */
} Elf32_Shdr;

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

/* Symbol table index */
#define STN_UNDEF	0		/* undefined */

/* Extract symbol info - st_info */
#define ELF32_ST_BIND(x)	((x) >> 4)
#define ELF32_ST_TYPE(x)	(((unsigned int) x) & 0xf)
#define ELF32_ST_INFO(b,t)	(((b) << 4) + ((t) & 0xf))

/* Symbol Binding - ELF32_ST_BIND - st_info */
#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* like global - lower precedence */
#define STB_NUM		3		/* number of symbol bindings */
#define STB_LOPROC	13		/* reserved range for processor */
#define STB_HIPROC	15		/*  specific symbol bindings */

/* Symbol type - ELF32_ST_TYPE - st_info */
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
	Elf32_Addr	r_offset;	/* offset of relocation */
	Elf32_Word	r_info;		/* symbol table index and type */
} Elf32_Rel;

/* Relocation entry with explicit addend */
typedef struct 
{
	Elf32_Addr	r_offset;	/* offset of relocation */
	Elf32_Word	r_info;		/* symbol table index and type */
	Elf32_Sword	r_addend;
} Elf32_Rela;

/* Extract relocation info - r_info */
#define ELF32_R_SYM(i)		((i) >> 8)
#define ELF32_R_TYPE(i)		((unsigned char) (i))
#define ELF32_R_INFO(s,t) 	(((s) << 8) + (unsigned char)(t))

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


/* see if this is an ELF image
 */
#ifdef ELF_OUR_MACHINE
Bool
elf_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)load;

	if (loadlen >= sizeof *hdr &&
			IS_ELF(*hdr) &&
			hdr->e_ident[EI_CLASS] == ELFCLASS32 &&
			hdr->e_ident[EI_DATA] == OUR_ELF_ENDIAN &&
			hdr->e_type == ET_EXEC &&
			hdr->e_machine == ELF_OUR_MACHINE)
		return TRUE;

	return FALSE;
}

/* prepare to run the ELF image
 */
Retcode
elf_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)load;
	Elf32_Phdr *phdr;
	/*Elf32_Shdr *shdr;*/
	int i;

	if (!elf_is_exec(e, load, loadlen))		/* sanity check */
		return E_BAD_IMAGE;

	/* copy our headers to temp memory to let us copy over our image */
	i = sizeof *hdr + hdr->e_phoff + sizeof *phdr * hdr->e_phnum;
	DPRINTF(("sizeof *hdr=%d sizeof *phdr= %d phnum=%d len=%d\n",
			sizeof *hdr, sizeof *phdr, hdr->e_phnum, i));

	hdr = (Elf32_Ehdr*)malloc(i);

	if (hdr == NULL)
	{
		cprintf(e, "Cannot allocate enough memory for ELF headers?!?\n");
		return E_OUT_OF_MEMORY;
	}

	memcpy(hdr, load, i);

	phdr = (Elf32_Phdr*)((char*)hdr + hdr->e_phoff);
	/*shdr = (Elf32_Shdr*)(load + hdr->e_shoff);*/
	*entrypoint = hdr->e_entry;		/* MUST return this */

	/* load all programs in this file */
	for (i = 0; i < hdr->e_phnum; i++, phdr++)
	{
		if (phdr->p_type != PT_LOAD)		/* only loadable types */
			continue;

		DPRINTF(("vaddr %#X, load+off %#X, msize %d, fsize %d\n", phdr->p_vaddr,
				load + phdr->p_offset, phdr->p_memsz, phdr->p_filesz));

		/* copy in the entire chunk of program */
		if (phdr->p_filesz > 0)
			memmove((char*)phdr->p_vaddr, load + phdr->p_offset,
					phdr->p_filesz);

		/* if in-memory size is larger, zero out the difference */
		if (phdr->p_memsz > phdr->p_filesz)
			memset((char*)phdr->p_vaddr + phdr->p_filesz, 0,
					phdr->p_memsz - phdr->p_filesz);

	#ifdef MACHINE_CLAIM_MEMORY
		/* claim/map this area if requested */
		MACHINE_CLAIM_MEMORY(e, (char*)phdr->p_vaddr, phdr->p_memsz);
	#endif /* MACHINE_CLAIM_MEMORY */
	}

	/*for (i = 0; i < hdr->e_shnum; i++, shdr++)
		load_section(shdr);*/

	free(hdr);
	return NO_ERROR;
}

/* return expected length of ELF image based on header information
 */
uInt
elf_length(Environ *e, uByte *load, uInt loadlen)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)load;
	Elf32_Shdr *shdr;
	int i;
	uInt len = 0;

	if (loadlen < hdr->e_shoff)
		return 0;

	shdr = (Elf32_Shdr*)(load + hdr->e_shoff);

	/* maximum offset+length in image should be the size of the image */
	for (i = 0; i < hdr->e_shnum; i++, shdr++)
	{
		if (shdr->sh_type != SHT_NOBITS &&
				len < shdr->sh_offset + shdr->sh_size)
			len = shdr->sh_offset + shdr->sh_size;

		if (len > loadlen)
			return 0;
	}

	return len;
}

Sym_table *
elf_symbols(Environ *e, uByte *load, uInt loadlen)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)load;
	Elf32_Shdr *shdr;
	Elf32_Shdr *symtab = NULL;
	Elf32_Shdr *strhdr = NULL;
	char *strtab;
	int i = 0;
	int num, nsyms;
	char *strs;
	Elf32_Sym *syms, *s;
	Sym_table *tab;

	if (!elf_is_exec(e, load, loadlen))		/* sanity check */
		return NULL;

	shdr = (Elf32_Shdr*)(load + hdr->e_shoff);
	strtab = (char*)load + shdr[hdr->e_shstrndx].sh_offset;

	for (i = 0; i < hdr->e_shnum; i++)
	{
		if (strcmp(strtab + shdr[i].sh_name, ELF_SYMTAB) == 0)
			symtab = shdr + i;
		else if (strcmp(strtab + shdr[i].sh_name, ELF_STRTAB) == 0)
			strhdr = shdr + i;
	}

	if (symtab == NULL || strhdr == NULL)
		return NULL;

	if (symtab->sh_entsize != sizeof *syms)
		return NULL;

	strs = (char*)load + strhdr->sh_offset;
	s = syms = (Elf32_Sym*)(load + symtab->sh_offset);
	nsyms = symtab->sh_size / sizeof *syms;
	num = 0;

	/* first count the number of symbols we are interested in */
	for (i = 0; i < nsyms; i++, s++)
	{
		if ((ELF32_ST_BIND(s->st_info) == STB_GLOBAL ||
				ELF32_ST_BIND(s->st_info) == STB_LOCAL) &&
				(ELF32_ST_TYPE(s->st_info) == STT_OBJECT ||
				ELF32_ST_TYPE(s->st_info) == STT_FUNC))
			num++;
	}

	/* allocate space for the symbols */
	tab = (Sym_table*)malloc(sizeof *tab);

	if (tab == NULL)
		return NULL;

	memset(tab, 0, sizeof *tab);
	tab->list = (Sym_ent*)malloc(sizeof *tab->list * num);

	if (tab->list == NULL)
	{
		free(tab);
		return NULL;
	}

	tab->num = num;
	num = 0;
	s = syms = (Elf32_Sym*)(load + symtab->sh_offset);

	/* and load the table */
	for (i = 0; i < nsyms; i++, s++)
	{
		if ((ELF32_ST_BIND(s->st_info) == STB_GLOBAL ||
				ELF32_ST_BIND(s->st_info) == STB_LOCAL) &&
				(ELF32_ST_TYPE(s->st_info) == STT_OBJECT ||
				ELF32_ST_TYPE(s->st_info) == STT_FUNC))
		{
			tab->list[num].name = cstrdup(strs + s->st_name);
			tab->list[num].addr = s->st_value;
			tab->list[num].type = ELF32_ST_TYPE(s->st_info) == STT_FUNC ?
					SYM_TEXT : SYM_DATA;
			DPRINTF(("tab %d: name=%P addr=%#x type=%#x\n", num,
					tab->list[num].name,
					tab->list[num].addr, tab->list[num].type));
			num++;
		}
	}

	return tab;
}

#endif /* ELF_OUR_MACHINE */

const Exec_entry exec_elf =
{
	"ELF",
	#ifdef ELF_OUR_MACHINE
		elf_is_exec,
		elf_load,
		elf_length,
		elf_symbols
	#endif /* ELF_OUR_MACHINE */
};
