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

/* WARNING: This code may assume a little endian machine */
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

int swapflag;

#define SWAPCHAR(value) (value)
#define SWAPSHORT(value) \
	((swapflag) ? ((((value) >> 8) & 0xFF) | (((value) << 8) & 0xFF00)) :	\
			(value))
#define SWAPINT(value) \
	((swapflag) ? ((((value) >> 24) & 0xFF) | (((value) >> 8) & 0xFF00) | \
	(((value) << 8) & 0xFF0000) | (((value) << 24) & 0xFF000000)) : (value))


struct fileheader
{
    unsigned short magic;
    unsigned short sections;
    int ctime;
    int symtab;
    int nsyms;
    unsigned short opthdrlen;
    unsigned short flags;
};

#define F_RELFLG	0x0001
#define F_EXEC		0x0002
#define F_LNNO		0x0004
#define F_LSYMS		0x0008
#define F_GSP10		0x0010
#define F_GSP20		0x0020
#define F_LENDIAN	0x0100
#define F_BENDIAN	0x0200


void
dumpheader(struct fileheader *header)
{
	int flags;

	printf("File header:\n");
	printf("\tmagic = 0x%.4X\n", SWAPSHORT(header->magic));
	printf("\tsections = %d\n", SWAPSHORT(header->sections));
	printf("\tctime = %d\n", SWAPINT(header->ctime));
	printf("\tsymtab = %d\n", SWAPINT(header->symtab));
	printf("\tnsyms = %d\n", SWAPINT(header->nsyms));
	printf("\topthdrlen = %d\n", SWAPSHORT(header->opthdrlen));
	flags = SWAPSHORT(header->flags);
	printf("\tflags = 0x%X\n", flags);

	if (flags & F_RELFLG)
		printf("\t\tF_RELFLG\n");

	if (flags & F_EXEC)
		printf("\t\tF_EXEC\n");

	if (flags & F_LNNO)
		printf("\t\tF_LNNO\n");

	if (flags & F_LSYMS)
		printf("\t\tF_LSYMS\n");

	if (flags & F_GSP10)
		printf("\t\tF_GSP10\n");

	if (flags & F_GSP20)
		printf("\t\tF_GSP20\n");

	if (flags & F_LENDIAN)
		printf("\t\tF_LENDIAN\n");

	if (flags & F_BENDIAN)
		printf("\t\tF_BENDIAN\n");
}

struct optfileheader
{
    short magic;
    short version;
    int sztext;
    int szdata;
    int szbss;
    int entrypoint;
    int textaddr;
    int dataaddr;
};

void
dumpoptheader(struct optfileheader *header)
{
    int size;

    printf("Optional file header:\n");
    printf("\tmagic = 0x%.4X\n", SWAPSHORT(header->magic));
    printf("\tversion = %d\n", SWAPSHORT(header->version));
    size = SWAPINT(header->sztext);
    printf("\tsztext = %d bits, %d bytes\n", size, size / 8);
    size = SWAPINT(header->szdata);
    printf("\tszdata = %d bits, %d bytes\n", size, size / 8);
    size = SWAPINT(header->szbss);
    printf("\tszbss = %d bits, %d bytes\n", size, size / 8);
    printf("\tentrypoint = 0x%X\n", SWAPINT(header->entrypoint));
    printf("\ttextaddr = 0x%X\n", SWAPINT(header->textaddr));
    printf("\tdataaddr = 0x%X\n", SWAPINT(header->dataaddr));
}

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

void
dumpsectionheader(int num, struct sectionheader *header)
{
	int size;
	int flags;

	printf("Section #%d, \"%s\":\n", num, header->name);
	printf("\tphysaddr = 0x%X\n", SWAPINT(header->physaddr));
	printf("\tvirtaddr = 0x%X\n", SWAPINT(header->virtaddr));
	size = SWAPINT(header->size);
	printf("\tsize = %d bits, %d bytes\n", size, size / 8);
	printf("\trawdata = 0x%X\n", SWAPINT(header->rawdata));
	printf("\treldata = 0x%X\n", SWAPINT(header->reldata));
	printf("\tlndata = %d\n", SWAPINT(header->lndata));
	printf("\tnumreloc = %d\n", SWAPSHORT(header->numreloc));
	printf("\tnumln = %d\n", SWAPSHORT(header->numln));
	printf("\treserved = 0x%X\n", SWAPCHAR(header->reserved));
	printf("\tmempage = %d\n", SWAPCHAR(header->mempage));
	flags = SWAPSHORT(header->flags);
	printf("\tflags = 0x%X\n", flags);

	if (flags & S_DSECT)
		printf("\t\tS_DSECT\n");

	if (flags & S_NOLOAD)
		printf("\t\tS_NOLOAD\n");

	if (flags & S_GROUP)
		printf("\t\tS_GROUP\n");

	if (flags & S_PAD)
		printf("\t\tS_PAD\n");

	if (flags & S_COPY)
		printf("\t\tS_COPY\n");

	if (flags & S_TEXT)
		printf("\t\tS_TEXT\n");

	if (flags & S_DATA)
		printf("\t\tS_DATA\n");

	if (flags & S_BSS)
		printf("\t\tS_BSS\n");

	switch (flags & S_ALIGNMSK)
	{
	case S_ALIGN10:
		printf("\t\tS_ALIGN10\n");
		break;
	case S_ALIGN20:
		printf("\t\tS_ALIGN20\n");
		break;
	case S_EVEN10:
		printf("\t\tS_EVEN10\n");
		break;
	case S_EVEN20:
		printf("\t\tS_EVEN20\n");
		break;
	default:
		printf("\t\tS_ALIGN_%X\n", (flags & S_ALIGNMSK) >> 8);
		break;
	}
}

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

#define	SYMENT	struct syment
/*#define	SYMESZ	sizeof(SYMENT) */
#define	SYMESZ	18

#define n_name		_n._n_name
#define n_ptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset


void
dumpsyms(unsigned char *image, int nsyms)
{
	struct syment *sym;
	char *strtab = (char*)(image + nsyms * SYMESZ);
	int i;

	printf("SYMTAB (%d) [%d]:\n", nsyms, SYMESZ);

    for (i = 0; i < nsyms; i++)
	{
		/* sym may be mis-sized by the compiler, so this should always work */
		sym = (struct syment*)(image);
		printf("%3d:", i);
		printf(" z=0x%X", SWAPINT(sym->n_zeroes));
		printf(" o=0x%X", SWAPINT(sym->n_offset));
		printf(" v=0x%X", SWAPINT(sym->n_value));
		printf(" scnum=0x%X", SWAPSHORT(sym->n_scnum));
		printf(" type=0x%X", SWAPINT(sym->n_type));
		printf(" sclass=%d", sym->n_sclass);
		printf(" naux=%d", sym->n_numaux);
		printf("\n");
		image += SYMESZ;
	}
}

void
dumpimage(unsigned char *imagestart, int imagelen)
{
	unsigned char *image = imagestart;
	struct fileheader *header;
	int sections, olen;
	int i, len = 0;

	/* dump the file headers */
	header = (struct fileheader *)image;
	swapflag = (header->magic == 0x5001);
	image += 20;
	printf("swapflag=%d\n", swapflag);
	dumpheader(header);

	if (SWAPSHORT(header->opthdrlen) >= 28)
	{
		struct optfileheader *optheader = (struct optfileheader *)image;
		dumpoptheader(optheader);
		image += 28;
		olen = SWAPSHORT(header->opthdrlen);
		olen -= 28;
	}
	else
		olen = SWAPSHORT(header->opthdrlen);


	if (olen)
	{
		printf("xtra opthdr (%d):", olen);

		for (i = 0; i < olen; i++)
			printf(" %.2X", *(unsigned char *)image++);

		printf("\n");
	}

	len = sizeof *header;
	len += SWAPSHORT(header->opthdrlen);

	/* dump the section headers */
	sections = SWAPSHORT(header->sections);
	len += sections * sizeof (struct sectionheader);

	for (i = 0; i < sections; i++)
	{
		struct sectionheader *secheader = (struct sectionheader *)image;
		image += 40;
		dumpsectionheader(i + 1, secheader);

		if (!(SWAPSHORT(secheader->flags) & (S_BSS | S_NOLOAD)) &&
				strcmp(secheader->name, ".bss") != 0)
			len += SWAPINT(secheader->size);
	}

	printf("file length=%d\n", len);

	dumpsyms(imagestart + SWAPINT(header->symtab), SWAPINT(header->nsyms));
}

int
main(int argc, char **argv)
{
	FILE *fp;
	unsigned char *image;
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
	image = (unsigned char *)malloc(imagelen);

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
