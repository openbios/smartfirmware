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

/* (c) Copyright 2003 by CodeGen, Inc.  All Rights Reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

char *prog;

int
readfile(char *name, char **buf, int *buflen)
{
    int fd = open(name, O_RDONLY, 0);
    struct stat statbuf;
    char *b;
    int l;

    if (fd < 0)
	return 0;

    if (fstat(fd, &statbuf) < 0)
    {
	close(fd);
	return 0;
    }

    l = statbuf.st_size;
    b = (char *)malloc(l);

    if (!b)
    {
	close(fd);
	return 0;
    }

    if (read(fd, b, l) != l)
    {
	free(b);
	close(fd);
	return 0;
    }

    if (buf)
	*buf = b;

    if (buflen)
	*buflen = l;

    return 1;
}

int
writefile(char *buf, int buflen)
{
    fwrite(buf, 1, buflen, stdout);
}

int
writepad(int addr, int padlen)
{
    while (padlen--)
    {
	switch (addr & 0xF)
	{
	case 0:
	    putc((addr & 0x10) ? 0x55 : 0xAA, stdout);
	    break;
	case 1:
	    putc((addr >> 16) & 0xFF, stdout);
	    break;
	case 2:
	    putc((addr >> 8) & 0xFF, stdout);
	    break;
	case 3:
	    putc(addr & 0xFC, stdout);
	    break;
	default:
	    putc(addr & 0x0F, stdout);
	    break;
	}

	addr++;
    }
}

void
writepattern(char *padstr, int slen, int len)
{
    if (len % slen)
    {
	int mlen = len % slen;
	writefile(padstr + slen - mlen, mlen);
	len -= mlen;
    }

    for (; len >= slen; len -= slen)
	writefile(padstr, slen);
}

int
parsenum(char *ptr)
{
    int suffix;
    int val;
    char *eptr;

    val = strtoul(ptr, &eptr, 0);

    if  (eptr == NULL)
	return val;

    while (isspace(*eptr))
	eptr++;

    switch (tolower(*eptr))
    {
    case 'b':
	val *= 512;
	break;

    case 'p':
	val *= 4*1024;
	break;

    case 'k':
	val *= 1024;
	break;

    case 'm':
	val *= 1024 * 1024;
	break;

    case 'g':
	val *= 1024 * 1024 * 1024;
	break;

    default:
	break;
    }

    return val;
}

int
usage(void)
{
    fprintf(stderr, "usage: %s [-opts args] output-len [filename]\n", prog);
    fprintf(stderr, "    -S          smart/special padding\n");
    fprintf(stderr, "    -b byteseq  pad with sequence of hex/oct/dec bytes\n");
    fprintf(stderr, "    -p pattern  pad with given text pattern\n");
    fprintf(stderr, "    -l padlen   pad fill sequence to padlen bytes\n");
    fprintf(stderr, "    -i imagelen length of image within output file\n");
    fprintf(stderr, "    -o offset   offset of file within output images\n");
    fprintf(stderr, "    -s skip     skip number of bytes in file\n");
    return 1;
}

int
main(int argc, char **argv)
{
    char *buffer;
    int buflen;
    int i, j;
    int outputlen;
    int ch;
    int imagelen = 0;
    int bufoffset = 0;
    int skip = 0;
    int smartpad = 0;		// use writepad() above to fill
    char *padstr = NULL;	// pad pattern, if specified
    int padstrlen = 0;		// length of pattern string
    int padlen = 0;		// padding length - not output length

    prog = argv[0];

    while ((ch = getopt(argc, argv, "Sb:p:l:i:o:s:")) != -1)
    {
	switch (ch)
	{
	case 'S':
	    smartpad = 1;
	    break;

	case 'p':
	    padstr = optarg;
	    padstrlen = strlen(padstr);
	    break;

	case 'b':
	    padstr = malloc(strlen(optarg) + 1);

	    if (padstr == NULL)
	    {
		fprintf(stderr, "%s: out of memory?!?\n", prog);
		return 4;
	    }

	    buffer = NULL;
	    padstrlen = 0;
	    
	    for (;;)
	    {
		ch = strtoul(optarg, &buffer, 0);

		if (buffer == NULL || buffer == optarg)
		    break;

		padstr[padstrlen++] = ch & 0xFF;

		if (*buffer == '\0')
		    break;

		optarg = buffer + 1;
	    }

	    break;

	case 'l':
	    padlen = parsenum(optarg);
	    break;

	case 'i':
	    imagelen = parsenum(optarg);
	    break;

	case 'o':
	    bufoffset = parsenum(optarg);
	    break;

	case 's':
	    skip = parsenum(optarg);
	    break;

	case '?':
	default:
	    return usage();
	}
    }

    argc -= optind;
    argv += optind;

    if (argc < 1)
	return usage();

    outputlen = parsenum(argv[0]);

    if (padstr == NULL || padstrlen == 0)
    {
	padstr = "\0";
	padstrlen = 1;
    }

    if (argc > 1)
    {
	if (!readfile(argv[1], &buffer, &buflen))
	{
	    fprintf(stderr, "%s: cannot read file, %s\n", prog, argv[1]);
	    return 3;
	}
    }
    else
    {
	buffer = padstr;
	buflen = padstrlen;
    }

    if (skip)
    {
	buflen -= skip;
	buffer += skip;
    }

    if (imagelen)
    {
	if (imagelen < bufoffset)
	{
	    bufoffset = 0;
	    buflen = 0;
	    padlen = imagelen;
	}
	else if (imagelen < bufoffset + buflen)
	{
	    buflen = imagelen - bufoffset;
	    padlen = 0;
	}
	else
	    padlen = imagelen - buflen - bufoffset;
    }
    else
	imagelen = bufoffset + buflen + padlen;

    for (i = 0; i < outputlen; i += imagelen)
    {
	if (bufoffset > outputlen - i)
	    bufoffset = outputlen - i;

	if (bufoffset)
	{
	    if (smartpad)
		writepad(i, bufoffset);
	    else 
		writepattern(padstr, padstrlen, bufoffset);
	}

	if (bufoffset + buflen > outputlen - i)
	    buflen = outputlen - i - bufoffset;

	if (buflen)
	    writefile(buffer, buflen);

	if (bufoffset + buflen + padlen > outputlen - i)
	    padlen = outputlen - i - buflen - bufoffset;

	if (padlen)
	{
	    if (smartpad)
		writepad(i + buflen, padlen);
	    else 
		writepattern(padstr, padstrlen, padlen);
	}
    }

    return 0;
}
