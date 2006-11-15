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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* Convert an FCode image into a PCI ROM image complete with headers.
   Generate TEKHEX format output suitable for most PROM programmers.
   Does not support VPD structure yet.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

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


char *g_prog = NULL;
unsigned long g_addr = 0;		/* current output address */
unsigned long g_vendid = 0;
unsigned long g_devid = 0;
unsigned long g_classcode = 0;
unsigned long g_version = 0;
char *g_copyright = NULL;
char *g_bios = NULL;
unsigned long g_entry = 0;
int g_rawbin = 0;


/* for dump_hex(), dump_zero(), and dump_end() below */
#define MAX 	64		/* max length of line - must be an even number */
#define HEX(i)	((i) < 10 ? (i) + '0' : (i) - 10 + 'A')


/* output the designated hunk of data starting at addr in Tek-HEX format
 */
void
dump_hex(unsigned long addr, char *data, unsigned long size)
{
	char line[MAX + 16];
	int i, n, a, csum;

	if (g_rawbin)
	{
		static unsigned long lastaddr = 0;

		while (lastaddr < g_addr)
		{
			putc('\0', stdout);
			lastaddr++;
		}

		/* assuming addrs are sequential */
		fwrite(data, sizeof *data, size, stdout);
		lastaddr += size;
		return;
	}

	line[0] = '%';		/* header */
	line[3] = '6';		/* data */
	line[6] = '8';		/* length of load addr */

	while (size > 0)
	{
		n = ((size << 1) > MAX ) ? MAX : (size << 1);
		csum = n + 14;
		line[1] = HEX((csum >> 4) & 0xF);
		line[2] = HEX(csum & 0xF);
		csum = ((csum >> 4) & 0xF) + (csum & 0xF) + 8 + 6;

		a = addr;
		size -= (n >> 1);
		addr += (n >> 1);

		for (i = 14; i >= 7; i--)
		{
			line[i] = HEX(a & 0xF);
			csum += (a & 0xF);
			a >>= 4;
		}

		for (i = 0;	i < n; i += 2)
		{
			line[15 + i] = HEX((*data >> 4) & 0xF);
			line[16 + i] = HEX(*data & 0xF);
			csum += ((*data >> 4) & 0xF) + (*data & 0xF);
			data++;
		}

		line[15 + i] = '\0';

		line[4] = HEX((csum >> 4) & 0xF);
		line[5] = HEX(csum & 0xF);

		fputs(line, stdout);
		putc('\n', stdout);
	}
}

/* dump a bunch of zero records to make sure that segments like bss
   are initialized correctly
 */
void
dump_zero(unsigned long addr, unsigned long size)
{
	char line[MAX + 16];
	int i, n, a, csum;

	line[0] = '%';		/* header */
	line[3] = '6';		/* data */
	line[6] = '8';		/* length of load addr */

	while (size > 0)
	{
		n = ((size << 1) > MAX ) ? MAX : (size << 1);
		csum = n + 14;
		line[1] = HEX((csum >> 4) & 0xF);
		line[2] = HEX(csum & 0xF);
		csum = ((csum >> 4) & 0xF) + (csum & 0xF) + 8 + 6;

		a = addr;
		size -= (n >> 1);
		addr += (n >> 1);

		for (i = 14; i >= 7; i--)
		{
			line[i] = HEX(a & 0xF);
			csum += (a & 0xF);
			a >>= 4;
		}

		for (i = 0;	i < n; i += 2)
		{
			line[15 + i] = HEX(0);
			line[16 + i] = HEX(0);
		}

		line[15 + i] = '\0';

		line[4] = HEX((csum >> 4) & 0xF);
		line[5] = HEX(csum & 0xF);

		fputs(line, stdout);
		putc('\n', stdout);
	}
}

/* output the final terminating Tek-HEX record -
   gsphex seems to place entrypoint as the final address without
   byte-swapping it, so we do the same
 */
void
dump_end(unsigned long entrypoint)
{
	char line[MAX + 16];
	int i, csum;
	unsigned long a;

	line[0] = '%';		/* header */
	line[3] = '8';		/* end */
	line[6] = '8';		/* length of load addr */

	line[1] = '0';
	line[2] = 'E';
	csum = 14 + 8 + 8;

	a = entrypoint;

	for (i = 14; i >= 7; i--)
	{
		line[i] = HEX(a & 0xF);
		csum += (a & 0xF);
		a >>= 4;
	}

	line[15] = '\0';

	line[4] = HEX((csum >> 4) & 0xF);
	line[5] = HEX(csum & 0xF);

	fputs(line, stdout);
	putc('\n', stdout);
}

void
dump_header(unsigned long imagelen, int imagetype, int lastimage)
{
	struct fcode_header fh;
	struct x86_header xh;
	struct pci_header ph;
	unsigned long l;

	memset((char*)&fh, 0, sizeof fh);
	memset((char*)&xh, 0, sizeof xh);
	memset((char*)&ph, 0, sizeof ph);

	if (imagetype == FCODE_IMAGE)
	{
		fh.id[0] = 0x55;
		fh.id[1] = 0xAA;
		l = sizeof fh + (g_copyright ? strlen(g_copyright) + 1 : 0);
		l = (l + 3) & ~3;
		fh.pcihdr[0] = l & 0xFF;
		fh.pcihdr[1] = l >> 8;
		l += sizeof ph;
		fh.fcode[0] = l & 0xFF;
		fh.fcode[1] = l >> 8;
		dump_hex(g_addr, (char*)&fh, sizeof fh);
		g_addr += sizeof fh;
	}
	else
	{
		xh.id[0] = 0x55;
		xh.id[1] = 0xAA;
		l = sizeof xh + (g_copyright ? strlen(g_copyright) + 1 : 0);
		l = (l + 3) & ~3;
		xh.pcihdr[0] = l & 0xFF;
		xh.pcihdr[1] = l >> 8;
		l += sizeof ph + g_entry - 5;	/* XXX */
		xh.entry[0] = 0xE9;			/* x86 16-bit relative jump */
		xh.entry[1] = l & 0xFF;
		xh.entry[2] = l >> 8;
		dump_hex(g_addr, (char*)&xh, sizeof xh);
		g_addr += sizeof xh;
	}

	/* non-standard insertion of copyright */
	if (g_copyright)
	{
		dump_hex(g_addr, g_copyright, strlen(g_copyright) + 1);
		g_addr += strlen(g_copyright) + 1;
	}

	g_addr = (g_addr + 3) & ~3;		/* align g_addr */

	ph.sig[0] = 'P';
	ph.sig[1] = 'C';
	ph.sig[2] = 'I';
	ph.sig[3] = 'R';
	ph.vendid[0] = g_vendid & 0xFF;
	ph.vendid[1] = g_vendid >> 8;
	ph.devid[0] = g_devid & 0xFF;
	ph.devid[1] = g_devid >> 8;
	ph.vpdptr[0] = 0;			/* no VPD support yet */
	ph.vpdptr[1] = 0;
	ph.hdrlen[0] = sizeof ph & 0xFF;
	ph.hdrlen[1] = sizeof ph >> 8;
	ph.hdrver = 0;				/* zero for v2.0 and v2.1 of PCI spec */
	ph.class[0] = g_classcode & 0xFF;
	ph.class[1] = (g_classcode >> 8) & 0xFF;
	ph.class[2] = g_classcode >> 16;
	imagelen = (imagelen + 511) / 512;
	ph.imagelen[0] = imagelen & 0xFF;
	ph.imagelen[1] = imagelen >> 8;
	ph.imagever[0] = g_version & 0xFF;
	ph.imagever[1] = g_version >> 8;
	ph.type = imagetype;
	ph.flags = lastimage;
	dump_hex(g_addr, (char*)&ph, sizeof ph);
	g_addr += sizeof ph;
}

void
dump_image(const char *fname, int imagetype, int lastimage)
{
	FILE *fp;
	size_t len;
	char *buf;
	
	fp = fopen(fname, "rb+");
	
	if (fp == NULL)
	{
		fprintf(stderr, "cannot open %s for reading\n", fname);
		return;
	}
	
	/* figure out length of file */
	if (fseek(fp, 0, SEEK_END) < 0)
	{
		fclose(fp);
		fprintf(stderr, "cannot seek to end of %s\n", fname);
		return;
	}
	
	len = ftell(fp);
	
	if (len < 0)
	{
		fclose(fp);
		fprintf(stderr, "cannot get length of %s\n", fname);
		return;
	}
		
	/* go back to the beginning of the file */
	if (fseek(fp, 0, SEEK_SET) < 0)
	{
		fclose(fp);
		fprintf(stderr, "cannot seek back to beginning of %s\n", fname);
		return;
	}
	
	/* now we know how much space to allocate, plus a null-byte */
	buf = malloc(len + 1);
	
	if (buf == NULL)
	{
		fclose(fp);
		fprintf(stderr, "cannot allocate %d bytes for file %s\n", len, fname);
		return;
	}
	
	/* finally read in the file */
	if (fread(buf, sizeof *buf, len, fp) != len)
	{
		fclose(fp);
		fprintf(stderr, "cannot read contents of %s\n", fname);
		return;
	}
	
	fclose(fp);
	buf[len] = '\0';

	dump_header(len, imagetype, lastimage);
	dump_hex(g_addr, buf, len);
	g_addr += len;
	free(buf);
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

	while ((o = getopt(argc, argv, "a:b:v:d:c:r:y:R")) != EOF)
		switch (o)
		{
		case 'a':
			g_addr = strtol(optarg, NULL, 0);
			break;

		case 'b':
			g_bios = optarg;
			break;

		case 'e':
			g_entry = strtol(optarg, NULL, 0);
			break;

		case 'v':
			g_vendid = strtol(optarg, NULL, 0);
			break;

		case 'd':
			g_devid = strtol(optarg, NULL, 0);
			break;

		case 'c':
			g_classcode = strtol(optarg, NULL, 0);
			break;

		case 'r':
			g_version = strtol(optarg, NULL, 0);
			break;

		case 'y':
			g_copyright = optarg;
			break;
		
		case 'R':
			g_rawbin = 1;
			break;

		case '?':
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	if (argc < 1)
	{
	usage:
		fprintf(stderr, "usage: %s [opts] fcode-filename\n", g_prog);
		fprintf(stderr, "    -a starting-ROM-address (number)\n");
		fprintf(stderr, "    -b x86 BIOS image (file-name)\n");
		fprintf(stderr, "    -e x86 entry-point for init code (number)\n");
		fprintf(stderr, "    -v vendor-id (number)\n");
		fprintf(stderr, "    -d device-id (number)\n");
		fprintf(stderr, "    -c class-code (number)\n");
		fprintf(stderr, "    -r image-revision (number)\n");
		fprintf(stderr, "    -y copyright-notice (string)\n");
		fprintf(stderr, "    -R raw-binary output instead of Tek-HEX\n");
		return -1;
	}

	if (g_bios)
	{
		dump_image(g_bios, X86_IMAGE, NOT_LAST_IMAGE);
		g_addr = (g_addr + 511) / 512;
	}

	dump_image(argv[0], FCODE_IMAGE, LAST_IMAGE);
	return 0;
}
