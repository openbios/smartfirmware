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

/* (c) Copyright 2002 by CodeGen, Inc.  All Rights Reserved. */

/* Dump the headers of a PCI ROM image  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

typedef unsigned char uByte;

#define X86_IMAGE		0
#define FCODE_IMAGE		1
#define LAST_IMAGE		0x80
#define NOT_LAST_IMAGE	0x00

/* all lengths in structs below are little-endian */

struct fcode_header
{
	unsigned char id[2];			/* always 0x55 0xAA */
	unsigned char fcode[2];			/* pointer to FCode */
	unsigned char reserved[20];		/* rest of architecture-specific range */
	unsigned char pcihdr[2];		/* pointer to PCI header */
};

struct x86_header
{
	unsigned char id[2];			/* always 0x55 0xAA */
	unsigned char initsize;			/* initsize in 512-byte chunks */
	unsigned char entry[3];			/* short-jump to initialization code */
	unsigned char appdata[18];		/* reserved for application-unique data */
	unsigned char pcihdr[2];		/* pointer to PCI header */
};

struct pci_header
{
	char sig[4];					/* always 'PCIR' */
	unsigned char vendid[2];
	unsigned char devid[2];
	unsigned char vpdptr[2];
	unsigned char hdrlen[2];		/* length of this struct */
	unsigned char hdrver;			/* always zero */
	unsigned char class[3];
	unsigned char imagelen[2];		/* 512-byte increments */
	unsigned char imagever[2];
	unsigned char type;				/* X86_IMAGE or FCODE_IMAGE */
	unsigned char flags;			/* LAST_IMAGE or NOT_LAST_IMAGE */
	unsigned char reserved[2];
};

char *g_prog;

void
dump_fcode(int off, uByte *image, int len)
{
	struct fcode_header *fcode = (struct fcode_header *)image;

	printf("FCode @ %X\n", off);
	printf("\tid %02X%02X\n", fcode->id[0], fcode->id[1]);
	printf("\tfcode %02X%02X\n", fcode->fcode[1], fcode->fcode[0]);
	printf("\tpcihdr %02X%02X\n", fcode->pcihdr[1], fcode->pcihdr[0]);
}

void
dump_image(uByte *image, int len)
{
	int off = 0;
	int pcioff;
	struct pci_header *pci;

	while (len >= 24)
	{

		if (image[0] != 0x55 || image[1] != 0xAA)
		{
			printf("Nothing at %X -- %02X %02X\n", off, image[0], image[1]);
			off += 512;
			image += 512;
			len -= 512;
			continue;
		}

		pcioff = image[24] | (image[25] << 8);

		if (len < pcioff + 24)
		{
			off += 512;
			image += 512;
			len -= 512;
			continue;
		}

		pci = (struct pci_header *)&image[pcioff];

		printf("Sig '%c%c%c%c' @ %X\n", pci->sig[0], pci->sig[1], pci->sig[2],
				pci->sig[3], off + pcioff);

		if (pci->sig[0] == 'P' && pci->sig[1] == 'C' && pci->sig[2] == 'I' &&
				pci->sig[3] == 'R')
		{
			printf("\tVendor %02X%02X\n", pci->vendid[1], pci->vendid[0]);
			printf("\tDevice %02X%02X\n", pci->devid[1], pci->devid[0]);
			printf("\tVPD Ptr %02X%02X\n", pci->vpdptr[1], pci->vpdptr[0]);
			printf("\tHeader length %02X%02X\n", pci->hdrlen[1], pci->hdrlen[0]);
			printf("\tHeader version %02X\n", pci->hdrver);
			printf("\tClass %02X%02X%02X\n", pci->class[2], pci->class[1], pci->class[0]);
			printf("\tImage length %02X%02X (%X)\n", pci->imagelen[1], pci->imagelen[0],
					((pci->imagelen[1] << 8) | pci->imagelen[0]) * 512);
			printf("\tImage version %02X%02X\n", pci->imagever[1], pci->imagever[0]);
			printf("\tImage type %02X%s\n", pci->type, pci->type == X86_IMAGE ? " (x86)" : pci->type == FCODE_IMAGE ? " (FCode)" : "");
			printf("\tFlags %02X\n", pci->flags);
		}

		if (pci->type == FCODE_IMAGE)
			dump_fcode(off, image, len);

		off += 512;
		image += 512;
		len -= 512;
	}
}

void
dump_rom(char *name)
{
	FILE *fp = fopen(name, "rb");
	off_t size;
	uByte *image;

	if (fp == NULL)
		return;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	image = (uByte *)malloc((size_t)size);

	if (image == NULL)
		return;

	fread((void *)image,  sizeof (uByte), size, fp);
	fclose(fp);
	dump_image(image, (int)size);
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int o;

	g_prog = strrchr(argv[0], '/');

	if (g_prog)
		g_prog++;
	else
		g_prog = argv[0];

	if (argc < 1)
	{
		fprintf(stderr, "usage: %s [opts] rom-image-filename\n", g_prog);
		return -1;
	}

	dump_rom(argv[1]);
	return 0;
}
