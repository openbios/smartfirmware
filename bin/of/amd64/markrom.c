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


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef unsigned long long uLong;
typedef unsigned int uInt;

#include "updateimage.h"

struct header g_header;
struct segheader g_segheaders[MAX_SEGMENTS];

struct seginfo
{
    char *filename;
    uLong addr;
    unsigned char *buf;
    int len;
};

struct seginfo g_seginfo[MAX_SEGMENTS];

/*
 * This code implements the AUTODIN II polynomial
 * The variable corresponding to the macro argument "crc" should
 * be an unsigned long.
 * Oroginal code  by Spencer Garrett <srg@quick.com>
 */

#define CRC32(crc, ch)	 (crc = (crc >> 8) ^ crc32tab[(crc ^ (ch)) & 0xff])

/* generated using the AUTODIN II polynomial
 *	x^32 + x^26 + x^23 + x^22 + x^16 +
 *	x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
 */

static const unsigned int crc32tab[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

unsigned int
crc32(unsigned char *buf, int len)
{
	unsigned int crc = ~0;
	int i;
	
	for (i = 0; i < len; i++, buf++)
	    CRC32(crc, *buf);

	return ~crc;
}

unsigned int
crc32v(unsigned char *bufs[], int lens[], int num)
{
	unsigned int crc = ~0;
	int i;
	int j;

	for (j = 0; j < num; j++)
	{
		unsigned char *buf = bufs[j];
		int len = lens[j];
		
		for (i = 0; i < len; i++, buf++)
		    CRC32(crc, *buf);
	}

	return ~crc;
}

void
markimage(char *buf, int len, int *newlen)
{
	unsigned int crc;
	unsigned int crc2;

	crc = crc32((unsigned char *)buf, len);

	buf[len + 0] = crc & 0xFF;
	buf[len + 1] = (crc >> 8) & 0xFF;
	buf[len + 2] = (crc >> 16) & 0xFF;
	buf[len + 3] = (crc >> 24) & 0xFF;

	*newlen = len + 4;
}



void
createimage(int num, char *outfile, char *rev, char *build)
{
	int i;
	unsigned char *bufs[MAX_SEGMENTS + 2];
	int lens[MAX_SEGMENTS + 2];
	int offset = 0;
	FILE *fp;

	memset(&g_header, '\0', sizeof g_header);	
	memset(g_segheaders, '\0', sizeof g_segheaders);	

	bufs[0] = (unsigned char *)&g_header;
	lens[0] = sizeof g_header;

	bufs[1] = (unsigned char *)g_segheaders;
	lens[1] = sizeof g_segheaders[0] * num;
	offset = lens[0] + lens[1];

	for (i = 0; i < num; i++)
	{
		g_segheaders[i].offset = offset;
		g_segheaders[i].size = g_seginfo[i].len;
		g_segheaders[i].physaddr = g_seginfo[i].addr;
		g_segheaders[i].crc = crc32(g_seginfo[i].buf, g_seginfo[i].len);
		bufs[i + 2] = (unsigned char *)g_seginfo[i].buf;
		lens[i + 2] = g_seginfo[i].len;
		offset += g_seginfo[i].len;
	}

	g_header.magic = SF_UPDATE_MAGIC_REV0;
	g_header.numsegments = num;
	g_header.hoffset = sizeof g_header;
	g_header.size = offset;
	memset(g_header.builddate, '0', 24);
	memset(g_header.rev, ' ', 16);

	if (build)
		memcpy(g_header.builddate, build, strlen(build));

	if (rev)
		memcpy(g_header.rev, rev, strlen(rev));

	g_header.crc = crc32v(bufs, lens, num + 2);

	if (outfile)
	{
		fp = fopen(outfile, "w");

		if (fp == NULL)
		{
			perror("fopen");
			exit(1);
		}
	}
	else
		fp = stdout;

	if (fwrite(&g_header, sizeof g_header, 1, fp) != 1)
	{
		perror("fwrite");
		exit(1);
	}

	if (fwrite(&g_segheaders, sizeof g_segheaders[0], num, fp) != num)
	{
		perror("fwrite");
		exit(1);
	}

	for (i = 0; i < num; i++)
	{
		if (fwrite(g_seginfo[i].buf, sizeof (char), g_seginfo[i].len, fp) !=
				g_seginfo[i].len)
		{
			perror("fwrite");
			exit(1);
		}
	}

	if (outfile)
		fclose(fp);
}

void
usage()
{
	fprintf(stderr, "Usage: markrom [-v rev] [-b builddate] [-o filename] address filename ...\n");
	exit(2);
}

int
main(int argc, char **argv)
{
	struct stat statbuf;
	int fd;
	char *buf;
	int num = 0;
	char *rev = NULL;
	char *build = NULL;
	char *outfile = NULL;
	int i;
	int ch;

	while ((ch = getopt(argc, argv, "v:b:o:")) != EOF)
	    switch (ch)
	    {
	    case 'v':
		rev = optarg;
		break;
	    case 'b':
		build = optarg;
		break;
	    case 'o':
		outfile = optarg;
		break;
	    default:
		usage();
		break;
	    }

	while (optind < argc)
	{
		g_seginfo[num].addr = strtoll(argv[optind], NULL, 16);
		g_seginfo[num].filename = NULL;
		optind++;

		if (optind < argc)
		{
			g_seginfo[num].filename = argv[optind];
			optind++;
		}
		else
			usage();

		num++;
	}

	if (num < 1)
		usage();

	for (i = 0; i < num; i++)
	{
		if (stat(g_seginfo[i].filename, &statbuf) < 0)
		{
			perror("stat");
			exit(1);
		}

		g_seginfo[i].len = statbuf.st_size;
		buf = (char *)malloc(g_seginfo[i].len);

		if (buf == (char *)0 || buf == (char *)-1)
		{
			perror("malloc");
			exit(1);
		}

		g_seginfo[i].buf = buf;
		fd = open(g_seginfo[i].filename, O_RDONLY);

		if (fd < 0)
		{
			perror("open");
			exit(1);
		}

		if (read(fd, g_seginfo[i].buf, g_seginfo[i].len) != g_seginfo[i].len)
		{
			perror("read");
			exit(1);
		}
	}

	createimage(num, outfile, rev, build);
	exit(0);
}
