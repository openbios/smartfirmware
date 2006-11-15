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

/* (c) Copyright 1997-1998,2000 by CodeGen, Inc.  All Rights Reserved. */

/* COFF-format program loader.
 */

#include "defs.h"
#include "exe.h"


/* for COFF binary files */

/* file header flags */
#define F_RELFLG	0x0001
#define F_EXEC		0x0002
#define F_LNNO		0x0004
#define F_LSYMS		0x0008
#define F_GSP10		0x0010
#define F_GSP20		0x0020
#define F_LENDIAN	0x0100
#define F_BENDIAN	0x0200

struct fileheader
{
    unsigned char magic[2];
    unsigned short sections;
    int ctime;
    int symtab;
    int nsyms;
    unsigned short opthdrlen;
    unsigned short flags;
};
typedef struct fileheader Fileheader;

struct optfileheader
{
    unsigned char magic[2];
    short version;
    int sztext;
    int szdata;
    int szbss;
    int entrypoint;
    int textaddr;
    int dataaddr;
};
typedef struct optfileheader Optfileheader;

/* section header flags */
#define S_DSECT		0x0001
#define S_NOLOAD	0x0002
#define S_GROUP		0x0004
#define S_PAD		0x0008
#define S_COPY		0x0010
#define S_TEXT		0x0020
#define S_DATA		0x0040
#define S_BSS		0x0080
#define S_ALIGNMSK	0x0F00
#define S_ALIGN10	0x0900
#define S_ALIGN20	0x0A00
#define S_EVEN10	0x0400
#define S_EVEN20	0x0500

struct sectionheader 
{
	char name[8];
	int physaddr;
	int virtaddr;
	int size;
	int rawdata;
	int reldata;
	int lndata;
	unsigned short numreloc;
	unsigned short numln;
	unsigned short flags;
	char reserved;
	char mempage;
};
typedef struct sectionheader Sectionheader;


#define SYMNMLEN	8	/* # characters in a symbol name */
#define FILNMLEN	14	/* # characters in a file name */
#define DIMNUM		4	/* # array dimensions in auxiliary entry */

struct syment
{
	union
	{
		char	_n_name[SYMNMLEN];	/* old COFF version */

		struct
		{
			long	_n_zeroes;	/* new == 0 */
			long	_n_offset;	/* offset into string table */
		} _n_n;

		char	*_n_nptr[2];	/* allows for overlaying */
	} _n;

	long		n_value;	/* value of symbol */
	short		n_scnum;	/* section number */
	short		n_type;		/* type and derived type */
	char		n_sclass;	/* storage class */
	char		n_numaux;	/* number of aux. entries */
};
typedef struct syment Syment;

#define	SYMENT	struct syment
/*#define	SYMESZ	sizeof(SYMENT) */
#define	SYMESZ	18

#define n_name		_n._n_name
#define n_ptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

#define SWAPCHAR(flag, value) (value)
#define SWAPSHORT(flag, value) \
	((flag) ? ((((value) >> 8) & 0xFF) | (((value) << 8) & 0xFF00)) : (value))
#define SWAPINT(flag, value) \
	((flag) ? ((((value) >> 24) & 0xFF) | (((value) >> 8) & 0xFF00) | \
	(((value) << 8) & 0xFF0000) | (((value) << 24) & 0xFF000000)) : (value))


#ifdef COFF_MAGIC_0
Bool
coff_is_exec(Environ *e, uByte *load, uInt loadlen)
{
	Fileheader *fhdr = (Fileheader*)load;
	Optfileheader *ofhdr = (Optfileheader*)(fhdr + 1);
	Bool swap = (fhdr->magic[0] == COFF_MAGIC_1);

	DPRINTF(("loading: magic=%#x.%x flags=%#x ctime=%#x opthdrlen=%#x\n", 
			fhdr->magic[0], fhdr->magic[1], SWAPSHORT(swap, fhdr->flags),
			SWAPINT(swap, fhdr->ctime), SWAPSHORT(swap, fhdr->opthdrlen)));

	if (loadlen < sizeof *fhdr)
		return FALSE;

	if (swap)
	{
		uByte b = fhdr->magic[0];
		fhdr->magic[0] = fhdr->magic[1];
		fhdr->magic[1] = b;
	}

	/* verify that it is the right COFF file format */
	if (fhdr->magic[0] == COFF_MAGIC_0 &&
			fhdr->magic[1] == COFF_MAGIC_1 &&
			(SWAPSHORT(swap, fhdr->flags) & F_EXEC) &&
			ofhdr->magic[0] == COFF_OPT_MAGIC_0 &&
			ofhdr->magic[1] == COFF_OPT_MAGIC_1)
		return TRUE;

	return FALSE;
}

/* prepare to run the COFF image already verified to be COFF
 */
Retcode
coff_load(Environ *e, uByte *load, uInt loadlen, uLong *entrypoint)
{
	unsigned char *data = (unsigned char*)load;
	Fileheader *fhdr = (Fileheader*)data;
	Bool swap = (fhdr->magic[0] == COFF_MAGIC_1);
	Optfileheader *ofhdr;
	Sectionheader *scnhdr;
	int numsections = SWAPSHORT(swap, fhdr->sections);
	unsigned char *from, *to;
	int len, i;

	/* sanity check */
	if (!coff_is_exec(e, load, loadlen))
		return E_BAD_IMAGE;

	/* copy the headers into a temp-buffer to support overlapping loads */
	len = sizeof *fhdr + SWAPSHORT(swap, fhdr->opthdrlen) +
			sizeof *scnhdr * numsections;
	fhdr = (Fileheader*)malloc(len);

	if (fhdr == NULL)
	{
		cprintf(e, "Cannot allocate enough memory for COFF headers?!?\n");
		return E_OUT_OF_MEMORY;
	}

	memcpy(fhdr, data, len);
	ofhdr = (Optfileheader*)(fhdr + 1);
	scnhdr = (Sectionheader*)((uPtr)ofhdr + SWAPSHORT(swap, fhdr->opthdrlen));

	*entrypoint = SWAPINT(swap, ofhdr->entrypoint);
	DPRINTF(("entrypoint=%#x\n", *entrypoint));

	/* copy the sections to the right addresses */
	for (i = 0; i < numsections; i++)
	{
		from = (unsigned char*)data + SWAPINT(swap, scnhdr[i].rawdata);
		to = (unsigned char*)(uPtr)SWAPINT(swap, scnhdr[i].physaddr);
		len = SWAPINT(swap, scnhdr[i].size);
		DPRINTF(("section %d from=%#x, to=%#x len=%d\n", i, from, to, len));

		if (len == 0)
			continue;

		if (SWAPSHORT(swap, scnhdr[i].flags) & S_BSS)
			memset(to, 0, len);
		else if (from)
			memmove(to, from, len);

	#ifdef MACHINE_CLAIM_MEMORY
		/* claim/map this area if requested */
		MACHINE_CLAIM_MEMORY(e, (char*)to, len);
	#endif /* MACHINE_CLAIM_MEMORY */
	}

	free(fhdr);
	return NO_ERROR;
}

uInt
coff_length(Environ *e, uByte *load, uInt loadlen)
{
	unsigned char *data = (unsigned char*)load;
	Fileheader *fhdr = (Fileheader*)data;
	Bool swap = (fhdr->magic[0] == COFF_MAGIC_1);
	Optfileheader *ofhdr = (Optfileheader*)(fhdr + 1);
	Sectionheader *scnhdr = (Sectionheader*)((uPtr)ofhdr +
			SWAPSHORT(swap, fhdr->opthdrlen));
	int numsections = SWAPSHORT(swap, fhdr->sections);
	int i, l, len = 0;

	for (i = 0; i < numsections; i++)
	{
		if (SWAPSHORT(swap, scnhdr[i].flags) & (S_BSS | S_NOLOAD))
			continue;
		
		l = SWAPINT(swap, scnhdr[i].rawdata);
		l += SWAPINT(swap, scnhdr[i].size);

		if (l > len)
			len = l;

		if (len > loadlen)
			return 0;
	}

	return len;
}

Sym_table *
coff_symbols(Environ *e, uByte *load, uInt loadlen)
{
	uByte *image = load;
	Fileheader *fhdr;
	Bool swap;
	Optfileheader *ofhdr;
	Sectionheader *scnhdr;
	Syment *sym;
	char *strtab;
	int i, nsyms, num, sect, numsections;
	Sym_table *tab;

	/* sanity check */
	if (!coff_is_exec(e, load, loadlen))
		return NULL;

	fhdr = (Fileheader*)image;
	swap = (fhdr->magic[0] == COFF_MAGIC_1);
	ofhdr = (Optfileheader*)(fhdr + 1);
	scnhdr = (Sectionheader*)((uPtr)ofhdr + SWAPSHORT(swap, fhdr->opthdrlen));
	nsyms = SWAPINT(swap, fhdr->nsyms);
	image += SWAPINT(swap, fhdr->symtab);
	strtab = (char*)(image + nsyms * SYMESZ);
	numsections = SWAPSHORT(swap, fhdr->sections);
	num = 0;
	DPRINTF((
		"coff_symbols: fhdr=%#x swap=%d nsyms=%d strtab=%#x numsections=%d\n",
			fhdr, swap, nsyms, strtab, numsections));

	/* first count the number of symbols we need */
    for (i = 0; i < nsyms; i++)
	{
		/* sym may be mis-sized by the compiler */
		sym = (struct syment*)(image);
		DPRINTF(("coff_symbols: i=%d sym=%#x", i, sym));
		sect = SWAPSHORT(swap, sym->n_scnum);
		DPRINTF((" scnum=%d", sect));

		if (sect >= 0 && sect < numsections)
			num++;

		DPRINTF((" num=%d\n", num));
		image += SYMESZ * (sym->n_numaux + 1);
	}

	DPRINTF(("coff_symbols: num=%d\n", num));

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

	num = 0;
	image = load + SWAPINT(swap, fhdr->symtab);

    for (i = 0; i < nsyms; i++)
	{
		/* sym may be mis-sized by the compiler */
		sym = (struct syment*)(image);
		sect = SWAPSHORT(swap, sym->n_scnum);

		if (sect >= 0 && sect < numsections)
		{
			if (sym->n_zeroes)		/* old-style symbol */
			{
				tab->list[num].name = lstrdup(sym->n_name,
						sym->n_name[7] ?  8 : strlen(sym->n_name));
			}
			else					/* symbol is in string table */
			{
				tab->list[num].name = strtab + SWAPINT(swap, sym->n_offset);
				tab->list[num].name = cstrdup(tab->list[num].name);
			}

			tab->list[num].addr = SWAPINT(swap, sym->n_value);
			tab->list[num].type = (SWAPSHORT(swap, scnhdr[sect].flags) & S_TEXT)
					? SYM_TEXT : SYM_DATA;
			DPRINTF(("tab %d: name=%P addr=%#x type=%#x\n", num,
					tab->list[num].name,
					tab->list[num].addr, tab->list[num].type));

			if (*tab->list[num].name)
				num++;
			else
				free(tab->list[num].name);
		}

		image += SYMESZ * (sym->n_numaux + 1);
	}

	tab->num = num;
	return tab;
}
#endif /* COFF_MAGIC_0 */

const Exec_entry exec_coff =
{
	"COFF",
	#ifdef COFF_MAGIC_0
		coff_is_exec,
		coff_load,
		coff_length,
		coff_symbols
	#endif /* COFF_MAGIC_0 */
};
