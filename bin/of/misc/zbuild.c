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

/* Build a gzipped executable image suitable for to load using zrun.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "zlib.h"

extern int errno;

int
main(int argc, const char **argv)
{
	Byte *dest;
	uLong destlen;
	Byte *source;
	uLong sourcelen;
	int ret;
	uLong i;
	int raw = 0;
	FILE *fp;

	if (argc <= 1)
	{
		fprintf(stderr, "usage: %s [-r] exe >array\n", argv[0]);
		return -1;
	}

	if (argc > 2 && strcmp(argv[1], "-r") == 0)
	{
		argv++;
		argc--;
		raw = 1;
	}

	fp = fopen(argv[1], "rb+");

	if (fp == NULL)
	{
		fprintf(stderr, "cannot open file %s for reading\n", argv[1]);
		return -2;
	}

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "fseek failure errno=%d\n", errno);
		return -3;
	}

	sourcelen = ftell(fp);
	rewind(fp);
	source = (Byte*)malloc(sourcelen + 1);
	destlen = sourcelen + (sourcelen >> 3);
	dest = (Byte*)malloc(destlen);

	if (source == NULL || dest == NULL)
	{
		fprintf(stderr, "cannot malloc 0x%X bytes\n", sourcelen);
		return -4;
	}

	if (fread(source, sizeof *source, sourcelen, fp) != sourcelen)
	{
		fprintf(stderr, "fread error - errno=%d\n", errno);
		return -5;
	}

	fclose(fp);

	ret = compress(dest, &destlen, source, sourcelen);

	switch (ret)
	{
	case Z_OK:
		break;

	case Z_MEM_ERROR:
		fprintf(stderr, "not enough memory\n");
		return ret;

	case Z_BUF_ERROR:
		fprintf(stderr, "output buffer too small\n");
		return ret;

	default:
		fprintf(stderr, "unknown error %d\n", ret);
		return ret;
	}

	if (raw)
	{
		fwrite(dest, sizeof *dest, destlen, stdout);
		return 0;
	}

	printf("unsigned long gz_orig_length = 0x%X;\n", sourcelen);
	printf("unsigned long gz_length = 0x%X;\n", destlen);
	printf("unsigned char gz_image[] = {");

	for (i = 0; i < destlen; i++)
	{
		if (i % 8 == 0)
			printf("\n");

		printf(" 0x%X,", dest[i]);
	}

	if (i % 8)
		printf("\n");

	printf("};\n");
	return 0;
}
