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

char *prog;

char *
ltrim(char *str)
{
    while (isspace(str[0]))
	str++;

    return str;
}

void
rtrim(char *str)
{
    int l = strlen(str);

    while (l && isspace(str[l - 1]))
	l--;

    str[l] = '\0';
}

char *
trim(char *str)
{
    rtrim(str);
    return ltrim(str);
}

void
error(int line, char *str)
{
    fprintf(stderr, "%s: %d: %s\n", prog, line, str);
}

void
set(int addr, int val)
{
//    printf("%d: %02XH %d\n", addr, val, val);
}

int
verify_checksum(unsigned char *buf, int len)
{
    int sum = 0;
    int i;

    if (len < 64)
	return 0;

    for (i = 0; i <= 62; i++)
    {
	sum += buf[i];
//	printf("%d: %02XH %d\n", i, buf[i], buf[i]);
    }

//    printf("sum %d, mod 256 %d\n", sum, sum % 256);

    sum &= 0xFF;

    if (sum != buf[63])
	fprintf(stderr, "Checksums: buf %02X: calc %02X\n", buf[63], sum);

    return sum == buf[63];
}

int
parse_num(char *str, char **end, int *valp)
{
    int ishex = 0;
    char *sp;
    int val = 0;

    str = ltrim(str);
//    printf("parse_num: str \"%s\" ch = %X\n", str, str[0]);

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
	/* hex constant */
	ishex = 1;
	str += 2;
    }

    if (!isxdigit(str[0]))
	return 0;

    sp = str;

    while (isxdigit(sp[0]))
	sp++;

//    printf("parse_num: end of num \"%s\" ch = %X\n", sp, sp[0]);

    if (sp[0] == 'h' || sp[0] == 'H')
	ishex = 1;

    sp = str;
//    printf("parse_num: ishex %d\n", ishex);

    if (!ishex)
    {
	if (!isdigit(sp[0]))
	    return 0;

	while (isdigit(sp[0]))
	{
	    val *= 10;
	    val += sp[0] - '0';
	    sp++;
	}
    }
    else
    {
	while (isxdigit(sp[0]))
	{
	    val *= 16;

	    if (sp[0] >= '0' && sp[0] <= '9')
		val += sp[0] - '0';
	    else if (sp[0] >= 'a' && sp[0] <= 'f')
		val += sp[0] - 'a' + 10;
	    else if (sp[0] >= 'A' && sp[0] <= 'F')
		val += sp[0] - 'A' + 10;

	    sp++;
	}

	if (sp[0] == 'h' ||  sp[1] == 'H')
	    sp++;
    }

    if (end)
	*end = sp;

    if (valp)
	*valp = val;

    return 1;
}

int
processfile(char *name, FILE *fp)
{
    char buf[256];
    char tmp[256];
    char *str;
    int line = 0;
    int errs = 0;
    int addr = 0;
    int val;

    memset(buf, 0, 128);
    memset(buf + 128, 0xFF, 128);

    while ((str = fgets(tmp, 256, fp)) != NULL)
    {
	int len = strlen(str);
	int isstr = 0;
	line++;

	while (len && (str[len - 1] == '\n' || str[len - 1] == '\r'))
	    len--;

	str[len] = '\0';
	str = ltrim(str);

	if (str[0] == '#' || str[0] == '\0')
	    continue;

//	printf("processfile: str \"%s\"\n", str);

	if (str[0] == 'S')
	{
	    int l = 0;
	    /* string */
	    str++;

	    if (!isdigit(str[0]))
	    {
		error(line, "Expected decimal constant after initial S");
		errs++;
		continue;
	    }

	    while (isdigit(str[0]))
	    {
		l *= 10;
		l += str[0] - '0';
		str++;
	    }

	    str = ltrim(str);

	    if (str[0] != ':')
	    {
		error(line, "Expected colon after Snn");
		errs++;
		continue;
	    }

	    if (l == 0)
	    {
		error(line, "Minimum string length is 1 character");
		errs++;
		continue;
	    }

	    isstr = l;
	    str = ltrim(str + 1);		/* skip colon */
	}

	if (!parse_num(str, &str, &val))
	{
	    /* !constant */
	    error(line, "Expected numeric constant");
	    errs++;
	    continue;
	}

	str = ltrim(str);

	if (str[0] == ':')
	{
	    str = ltrim(str + 1);		/* skip colon */
	    addr = val;

	    if (isstr)
	    {
		rtrim(str);

		/* parse string */
		while (*str)
		{
		    set(addr, *str);

		    if (addr < 256)
			buf[addr] = *str;
		    else
		    {
			error(line, "String exceeds SPD space");
			errs++;
		    }

		    addr++;
		    str++;
		}
	    }
	    else
	    {
		if (!parse_num(str, &str, &val))
		{
		    /* !constant */
		    error(line, "Expected numeric constant");
		    errs++;
		    continue;
		}

		set(addr, val);

		if (addr < 256)
		    buf[addr] = val;
		else
		{
		    error(line, "Byte after end of SPD space");
		    errs++;
		}

		addr++;
	    }
	}
	else
	{
	    set(addr, val);

	    if (addr < 256)
		buf[addr] = val;
	    else
	    {
		error(line, "Byte after end of SPD space");
		errs++;
	    }

	    addr++;
	}
    }

    if (!verify_checksum(buf, 256))
    {
	error(line, "Checksum incorrect");
//	errs++;
    }

    if (errs || fwrite(buf, sizeof (char), 256, stdout) != 256)
	return 0;

    return 1;
}

int
main(int argc, char **argv)
{
    int ok;

    prog = argv[0];

    /*
     *parse input file and produce a 256 byte output file 
     * input lines are of the form: 
     *
     *	# comment
     *  YY: XX		# YY is the address XX is the value.  Hex value in in H
     *  XX		# or start with 0x.  This form is one byte after prior
     *  Snn: YY: String	# next nn bytes are the ascii characters from String
     */

    if (argc == 2)
    {
	FILE *fp = fopen(argv[1], "r");

	if (fp != NULL)
	{
	    ok = processfile(argv[1], fp);
	    fclose(fp);
	}
	else
	    fprintf(stderr, "%s: cannot open file %s\n", argv[0], argv[1]);
    }
    else
	ok = processfile("", stdin);

    exit(ok ? 0 : 1);
}
