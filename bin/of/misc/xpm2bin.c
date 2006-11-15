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

#include <stdio.h>
#include "codegen.xpm"

#define xpmdata noname
#define NUMSTRS (sizeof xpmdata / sizeof xpmdata[0])

int width, height, numcolors, charspp;
unsigned long *idx;
unsigned long *r, *g, *b;
int outcount = 0;

out(unsigned long n)
{
	if (outcount > 11)
	{
		outcount = 0;
		putchar('\n');
	}

	printf("0x%X, ", n);
	outcount++;
}

header()
{
	char *str = xpmdata[0];		/* 1st string contains info */

	if (str && *str)
		width = strtol(str, &str, 0);

	if (str && *str)
		height = strtol(str, &str, 0);

	if (str && *str)
		numcolors = strtol(str, &str, 0);

	if (str && *str)
		charspp = strtol(str, &str, 0);

	if (width <= 0 || height <= 0 || numcolors <= 0 || charspp <= 0)
	{
		fprintf(stderr, "cannot parse xpm header info\n");
		exit(10);
	}

	if (numcolors > 256)
	{
		fprintf(stderr, "sorry - cannot handle anything other than 8-bit pixmaps\n");
		exit(11);
	}

	printf("#define LOGO_DEPTH 8\n");
	printf("#define LOGO_WIDTH %d\n", width);
	printf("#define LOGO_HEIGHT %d\n", height);
	printf("#define LOGO_COLORS %d\n", numcolors);
}

unsigned long
getpix(char *s, int width)
{
	unsigned long ret;

	switch (width)
	{
	case 1:
		ret = *(unsigned char*)s;
		break;

	case 2:
		ret = *(unsigned char*)s << 8;
		s++;
		ret |= *(unsigned char*)s;
		break;

	case 4:
		ret = *(unsigned char*)s << 24;
		s++;
		ret |= *(unsigned char*)s << 16;
		s++;
		ret |= *(unsigned char*)s << 8;
		s++;
		ret |= *(unsigned char*)s;
		break;
	
	default:
		fprintf(stderr, "cannot read xpm pixel info\n");
		exit(20);
		break;
	}
	
	return ret;
}

unsigned
hex2int(unsigned h)
{
	if (h >= '0' && h <= '9')
		return h - '0';
	else if (h >= 'A' && h <= 'F')
		return h - 'A' + 10;
	else if (h >= 'a' && h <= 'f')
		return h - 'a' + 10;
	
	return h;
}

unsigned long
gethex(char *s, int width)
{
	unsigned long ret;

	switch (width)
	{
	case 1:
		ret = hex2int(*(unsigned char*)s);
		break;

	case 2:
		ret = hex2int(*(unsigned char*)s) << 4;
		s++;
		ret |= hex2int(*(unsigned char*)s);
		break;

	case 4:
		ret = hex2int(*(unsigned char*)s) << 12;
		s++;
		ret |= hex2int(*(unsigned char*)s) << 8;
		s++;
		ret |= hex2int(*(unsigned char*)s) << 4;
		s++;
		ret |= hex2int(*(unsigned char*)s);
		break;
	
	default:
		fprintf(stderr, "cannot read xpm hex info\n");
		exit(20);
		break;
	}
	
	return ret;
}

colors()
{
	int i, n;
	char *s;

	/*printf("static const struct { unsigned long r, g, b; }  logo_colors[LOGO_COLORS] = {\n");*/
	printf("static unsigned char /*r,g,b*/ logo_colors[LOGO_COLORS * 3] = {\n");
	outcount = 0;

	idx = (unsigned long*)malloc(numcolors * sizeof idx[0]);
	r = (unsigned long*)malloc(numcolors * sizeof r[0]);
	g = (unsigned long*)malloc(numcolors * sizeof g[0]);
	b = (unsigned long*)malloc(numcolors * sizeof b[0]);

	if (idx == NULL || r == NULL || g == NULL || b == NULL)
	{
		fprintf(stderr, "cannot allocate space for colors\n");
		exit(30);
	}

	for (i = 0; i < numcolors; i++)
	{
		s = xpmdata[1 + i];
		idx[i] = getpix(s, charspp);
/*fprintf(stderr, "i=%d idx[i]=\"%s\" %d (0x%X)\n", i, s, idx[i], idx[i]);*/
		s++;

		while (*s && *s != '#')
			s++;
		
		s++;

		n = strlen(s) / 3;
		
		if (n == 0)
		{
			fprintf(stderr, "color string of wrong length: \"%s\"\n", s);
			exit(31);
		}

		r[i] = gethex(s, n);
		s += n;
		g[i] = gethex(s, n);
		s += n;
		b[i] = gethex(s, n);
		s += n;

		out(r[i]);
		out(g[i]);
		out(b[i]);
	}

	if (outcount)
		putchar('\n');

	printf("};\n");
}

pixmap()
{
	int i, j, k;
	char *s;
	unsigned long pix;

	printf("static const unsigned char logo_pixmap[LOGO_HEIGHT * LOGO_WIDTH] = {\n");
	outcount = 0;

	for (i = 0; i < height; i++)
	{
		s = xpmdata[1 + numcolors + i];

		for (j = 0; j < width; j++)
		{
			pix = getpix(s, charspp);
			s += charspp;

			for (k = 0; k < numcolors; k++)
				if (idx[k] == pix)
					break;
			
			if (k >= numcolors)
			{
				fprintf(stderr, "cannot find pixel 0x%X in index\n", pix);
				exit(50);
			}

			out(k & 0xFF);
		}
	}

	if (outcount)
		putchar('\n');

	printf("};\n");
}

main()
{
	header();
	colors();
	pixmap();
	return 0;
}
