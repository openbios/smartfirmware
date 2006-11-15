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
#include <stdarg.h>

#define STEPPING_A0	0x00
#define STEPPING_A2	0x02
#define STEPPING_B0	0x10
#define STEPPING_C0	0x20

#define MAX_DIMMS	4

typedef unsigned char uByte;
typedef unsigned int uInt;

int spd_nums[MAX_DIMMS];

uByte
read_spd(int dimm, int byte)
{
//    printf("read_spd: dimm %d, byte %d, index %d, value %02X\n", dimm, byte, 
//	    spd_nums[dimm], spd_byte(spd_nums[dimm], byte));
    return spd_byte(spd_nums[dimm], byte);
}

uByte
get_stepping()
{
    return STEPPING_A2;
}

void
panic(char *str)
{
    printf("%s\n", str);
    exit(1);
}

void
warn(char *fmt, ...)
{
    char buf[BUFSIZ];
    va_list va;
    va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);
    printf("Warning: %s\n", buf);
}

uByte
ns2mhz(uByte ns)
{
    if (ns == 0)
	return 0;

    if (ns <= 0x50)
	return 200;

    if (ns <= 0x60)
	return 166;

    if (ns <= 0x75)
	return 133;

    if (ns <= 0xA0)
	return 100;

    return 0;
}

uByte
read_clk(int dimm, int byte)
{
    return ns2mhz(read_spd(dimm, byte));
}

char *
dimmtable[] = 
{
    "None",
    "0",
    "1",
    "0,1",

    "2",
    "0,2",
    "1,2",
    "0,1,2",

    "3",
    "0,3",
    "1,3",
    "0,1,3",

    "2,3",
    "0,2,3",
    "1,2,3",
    "0,1,2,3",
};

char *
speedtable[] = 
{
    "None",
    "2",
    "2.5",
    "2.5,2",

    "3",
    "3,2",
    "3,2.5",
    "3,2.5,2",

    "3.5",
    "3.5,2",
    "3.5,2.5",
    "3.5,2.5,2",

    "3.5,3",
    "3.5,3,2",
    "3.5,3,2.5",
    "3.5,3,2.5,2",
};

/*
 * Things that we need to calculate:
 *	Registerd/Not-Registered
 *	Buffered/Unbuffered
 *	ECC/no-ECC
 *	x4/not-x4 for all DIMMS
/*	CS size
 * 	CS base address
 *	tCL
 *	fMEMCLK
 *	tRCD
 *	tRCD
 *	tRAS
 *	tRP
 *	tRC
 *	tRRD
 *	tRFC
 *	tREF
 *	tWCL
 *	AsyncLat
 *	RdPreamble
 *	MemClk Enables
 *	...
 */

int
check_spd()
{
    int dimms = 0xF;
    int i;
    int reg = 0;
    int buf = 0;
    uByte attrib;

    for (i = 0; i < 4; i++)
    {
	/* ignore any DIMMs that are not DDR SDRAM */
	if (read_spd(i, 2) != 7)
	{
	    dimms &= ~(1 << i);

	    if (read_spd(i, 2) != 0 && read_spd(i, 2) != 0xFF)
		warn("Unknown DIMM type in slot %d", i);

	    continue;
	}

	/* get registered DIMMS and buffered DIMMS attributes */
	attrib = read_spd(i, 21);

	if (attrib  & 0x1)
	    buf |= 1 << i;

	if (attrib  & 0x2)
	    reg |= 1 << i;

	/* get CL support attributes */
	attrib = read_spd(i, 18);

	if ((attrib & 0xC0) != 0 || (attrib & 0x3F) == 0)
	{
	    warn("DDR DIMM with unsupport latencies in slot %d", i);
	    dimms &= ~(1 << i);
	}
    }

    reg &= dimms;
//    printf("DIMMs %X\n", dimms);
//    printf("Reg %X\n", reg);

    if (reg != 0 && reg != dimms)
    {
	/* mix of registered and non-registered dimms */
	/* disable the registered dimms */
	warn("Mixed registered and non-registered DIMMs, disabling DIMMs (%s)", dimmtable[reg]);
	dimms &= ~reg;
    }

    buf &= dimms;
//    printf("DIMMs %X\n", dimms);
//    printf("Buf %X\n", buf);

    if (buf != 0 && buf != dimms)
	warn("Mixed buffered and unbuffered DIMMs (%X)", buf);

    if (buf)
	/* set the buffered DIMM bit in conflow */
	;

//    printf("Valid DIMMs %X\n", dimms);
    return dimms;
}

int
prog_timming(int dimms)
{
    uInt timelow = 0;
    uInt timehigh = 0;
    uInt conflow = 0;
    uInt confhigh = 0;
    uByte speeds = 0x3C;
    uByte clk35 = 200;
    uByte clk3 = 200;
    uByte clk25 = 200;
    uByte clk2 = 200;
    uByte clk;
    uByte cl;
    int trcd;
    int tras;
    int trp;
    int trc;
    int trrd;
    int trfc;
    int tref;
    int i;

    uByte stepping = get_stepping();

    if (stepping >= STEPPING_C0)
    {
	/* wait for DRAM controller to become ready */
	/* read config low, If MemClrSatus bit not set repeat */
    }
    else
    {
	/* spin for a while */
    }

//    printf("prog_timing: dimms %X\n", dimms);

    for (i = 0; i < 4; i++)
	if (dimms & (1 << i))
	{
	    int speeds0 = read_spd(i, 18);
	    uByte tclk35 = 0;
	    uByte tclk3 = read_clk(i, 9);
	    uByte tclk25 = read_clk(i, 23);
	    uByte tclk2 = read_clk(i, 25);
//	    printf("DIMM%d: 18: %02X -- 9: %02X -- 23: %02X -- 25: %02X\n", i, speeds0, tclk3, tclk25, tclk2);

	    if (speeds0 & 0x20)
	    {
		tclk35 = tclk3;
		tclk3 = tclk25;
		tclk25 = tclk2;
		tclk2 = 0;
	    }
	    else if (speeds0 & 0x10)
		;
	    else if (speeds0 & 0x08)
	    {
		tclk2 = tclk25;
		tclk25 = tclk3;
		tclk3 = 0;
	    }
	    else if (speeds0 & 0x04)
	    {
		tclk2 = tclk3;
		tclk25 = 0;
		tclk3 = 0;
	    }

	    if (!tclk35)
		speeds0 &= ~0x20;

	    if (!tclk3)
		speeds0 &= ~0x10;

	    if (!tclk25)
		speeds0 &= ~0x08;

	    if (!tclk2)
		speeds0 &= ~0x04;

//	    printf("DIMM%d: 18: %02X -- 9: %02X -- 23: %02X -- 25: %02X\n", i, speeds0, tclk3, tclk25, tclk2);

//	    printf("speeds %X, %s, CL3.5 %dMHz, CL3 %dMHz, CL2.5 %dMHz, CL2 %dMHz\n", speeds0, speedtable[speeds0 >> 2], tclk35, tclk3, tclk25, tclk2);

	    if (speeds0 == 0)
	    {
		// no speeds with clock info
		dimms &= ~(1 << i);
		warn("DIMM in slot %d has no valid clock information", i);
	    }
	    else if ((speeds & speeds0) == 0)
	    {
		// incompatible speeds
		dimms &= ~(1 << i);
		warn("DIMM in slot %d is incompatable with prior DIMMs", i);
	    }
	    else
	    {
		speeds &= speeds0;

		if (tclk35 < clk35)
		    clk35 = tclk35;

		if (tclk3 < clk3)
		    clk3 = tclk3;

		if (tclk25 < clk25)
		    clk25 = tclk25;

		if (tclk2 < clk2)
		    clk2 = tclk2;
	    }
	}

    if (dimms == 0)
	panic("No usable DIMMs");

    /* special case CL3 @166MHZ and CL2 @133MHz to pick CL2 @133MHz */
    if ((speeds & 0x14) == 0x14 && clk3 == 166 && clk2 == 133)
	speeds &= ~0x10;

    if (speeds & 0x10)
    {
	cl = 4;
	clk = clk3;
    }
    else if (speeds & 0x08)
    {
	cl = 3;
	clk = clk25;
    }
    else if (speeds & 0x04)
    {
	cl = 2;
	clk = clk2;
    }

    if (clk == 0)
	panic("No supported clock freq");

    /* OK, at this point we have selected CL3,CL2.5, or CL2 and have selected */
    /* the clock frequency */
    speeds = 1 << cl;
    trcd = 0;
    tras = 0;
    trp = 0;
    trc = 0;
    trrd = 0;
    trfc = 0;
    tref = 0;

    /* get the rest of the parameters from the SPD */
    for (i = 0; i < 4; i++)
	if (dimms & (1 << i))
	{
	    int speeds0 = read_spd(i, 18);
	    int rcd = read_spd(i, 29);
	    int ras = read_spd(i, 30);
	    int rp = read_spd(i, 27);
	    int rc = read_spd(i, 41);
	    int rrd = read_spd(i, 28);
	    int rfc = read_spd(i, 42);
	    int ref = read_spd(i, 3);
	    uByte ns;
	    int x;

	    if ((speeds0 & (speeds << 2)) != 0)
		ns = read_spd(i, 25);
	    else if ((speeds0 & (speeds << 1)) != 0)
		ns = read_spd(i, 23);
	    else
		ns = read_spd(i, 9);

	    /* convert from funky BCD */
	    ns = ((ns >> 4) & 0xF) * 10 + (ns & 0xF);
	    
	    /* compute: x = rcd / ns; */
	    /* rcd is in 1/4ns units ns is in 1/10ns units */
	    /* convert to common units of 1/20ns first */
	    x = (rcd * 5) / (ns * 2);

	    if (x * ns * 2 < rcd * 5)
		x++;

	    if (x > trcd)
		trcd = x;
	    
	    /* compute: x = ras / ns; */
	    /* ras is in 1ns units, ns is in 1/10ns units */
	    /* convert to common units of 1/10ns first */
	    x = (ras * 10) / ns;

	    if (x * ns < ras * 10)
		x++;

	    if (x > tras)
		tras = x;
	    
	    /* compute: x = rp / ns; */
	    /* rp is in 1/4ns units ns is in 1/10ns units */
	    /* convert to common units of 1/20ns first */
	    x = (rp * 5) / (ns * 2);

	    if (x * ns * 2 < rp * 5)
		x++;

	    if (x > trp)
		trp = x;
	    
	    /* compute: x = rc / ns; */
	    /* rc is in 1ns units ns is in 1/10ns units */
	    /* convert to common units of 1/10ns first */
	    /* need to deal with tRC not being set in the SPD */

	    if (rc == 0 || rc == 0xFF)
	    {
		if (clk == 100)
		    x = 7;
		else if (clk == 133)
		    x = 9;
		else if (clk == 166)
		    x = 10;
		else if (clk == 200)
		    x = 11;
	    }
	    else
	    {
		x = (rc * 10) / ns;

		if (x * ns  < rc * 10)
		    x++;
	    }

	    if (x > trc)
		trc = x;
	    
	    /* compute: x = rrd / ns; */
	    /* rrd is in 1/4ns units ns is in 1/10ns units */
	    /* convert to common units of 1/20ns first */
	    x = (rrd * 5) / (ns * 2);

	    if (x * ns * 2 < rrd * 5)
		x++;

	    if (x > trrd)
		trrd = x;
	    
	    /* compute: x = rfc / ns; */
	    /* rfc is in 1ns units ns is in 1/10ns units */
	    /* convert to common units of 1/10ns first */
	    /* need to deal with tRC not being set in the SPD */

	    if (rfc == 0 || rfc == 0xFF)
	    {
		if (clk == 100)
		    x = 8;
		else if (clk == 133)
		    x = 10;
		else if (clk == 166)
		    x = 12;
		else if (clk == 200)
		    x = 14;
	    }
	    else
	    {
		x = (rfc * 10) / ns;

		if (x * ns  < rfc * 10)
		    x++;
	    }

	    if (x > trfc)
		trfc = x;

	    if (ref > tref)
		tref = ref;

	    /* XXX: still need to get reg/non-reg, buf/unbuf, ecc/noecc, and x4 */
	    /* parameter from SPD's */
	}

    /* At this point we have select CL and Clk and have computed */
    /* trcd, tras, trp, trc, trrd, trfc, and tref */
    /* We still need to compute trwt, twcl, AsyncLat, RdPreamble */
    printf("DIMMS %s, CL %s, CLK %dMHz, ", dimmtable[dimms], 
	    cl == 2 ? "2" : cl == 3 ? "2.5" : "3", clk);
    printf("tRCD %d, ", trcd);
    printf("tRAS %d, ", tras);
    printf("tRP %d, ", trp);
    printf("tRC %d, ", trc);
    printf("tRRD %d, ", trrd);
    printf("tRFC %d\n", trfc);
//    printf("tREF %d\n", 1 << tref);
    return dimms;
}

void
prog_chip_selects(int dimms)
{
}

#ifdef MAIN

int 
main(int argc, char **argv)
{
    int i;
    int dimms;

    if (!read_spds())
    {
	fprintf(stderr, "Unable to read SPD values from directory spds/\n");
	exit(1);
    }

    spd_nums[0] = -1;
    spd_nums[1] = -1;
    spd_nums[2] = -1;
    spd_nums[3] = -1;

    for (i = 1; i < argc; i++)
    {
	char *name = argv[i];
	int n = spd_num(name);

	if (strcmp(name, "-") == 0)
	    n = -1;
	else if (n < 0)
	{
	    fprintf(stderr, "Unable to find SPD info for %s\n", name);
	    exit(1);
	}

	spd_nums[i - 1] = n;
    }

    dimms = check_spd();
    dimms = prog_timming(dimms);
    prog_chip_selects(dimms);
}

#endif
