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

extern char *spd_name(int);

typedef unsigned char uByte;
typedef unsigned short uShort;

int
dimmcmp(void const *p1, void const *p2)
{
    int i1 = *(int const *)p1;
    int i2 = *(int const *)p2;
    char *n1 = spd_name(i1);
    char *n2 = spd_name(i2);

//    printf("dimmcmp: %d,%d %s,%s\n", i1, i2, n1, n2);

    if (n1 == n2)
	return 0;

    if (n1 == NULL)
	return -1;

    if (n2 == NULL)
	return 1;

    return strcmp(n1, n2);
}

char *
fmtnum(int n)
{
    static char buf[20];

    /* even multiples of the major powers */
    if (n >= (1024*1024*1024) && (n % (1024*1024*1024)) == 0)
	sprintf(buf, "%dG", n / (1024*1024*1024));
    else if (n >= (1024*1024) && (n % (1024*1024)) == 0)
	sprintf(buf, "%dM", n / (1024*1024));
    else if (n >= 1024 && n <= 99999*1024 && (n % 1024) == 0)
	sprintf(buf, "%dK", n / 1024);
    else if (n <= 9999)
	sprintf(buf, "%d", n);
    else if (n < 10*1024)
	sprintf(buf, "%d.%02dK", n / 1024, ((n * 100) / 1024) % 100);
    else if (n < 100*1024)
	sprintf(buf, "%d.%dK", n / 1024, ((n * 10) / 1024) % 10);
    else if (n < 1000*1024)
	sprintf(buf, "%dK", n / 1024);
    else if (n < 10*1024*1024)
	sprintf(buf, "%d.%02dM", n / (1024*1024), (((n / 1024) * 100) / 1024) % 100);
    else if (n < 100*1024*1024)
	sprintf(buf, "%d.%dM", n / (1024*1024), (((n / 1024) * 10) / 1024) % 10);
    else if (n < 1000*1024*1024)
	sprintf(buf, "%dM", n / (1024*1024));
    else
	sprintf(buf, "%d.%02dG", n / (1024*1024*1024), (((n / (1024*1024)) * 100) / 1024) % 100);

    return buf;
}

char *
fmtquarterns(uByte val)
{
    static char buf[6];
    char *x;

    if (val == 0)
	return "    -";

    switch (val & 3)
    {
    case 0: x = "0 "; break;
    case 1: x = "25"; break;
    case 2: x = "5 "; break;
    case 3: x = "75"; break;
    }

    sprintf(buf, "%2d.%s", val >> 2, x);
    return buf;
}

char *
fmthundrethns(uByte val)
{
    static char buf[6];
    int d1;
    int d2;
    int d3;

    if (val == 0)
	return "   -";

    /* d.dd */
    d1 = (val >> 4) / 10;
    d2 = (val >> 4) % 10;
    d3 = val & 0xF;

    if (d3)
	sprintf(buf, "%d.%d%d", d1, d2, d3);
    else
	sprintf(buf, "%d.%d ", d1, d2);

    return buf;
}

char *dramtypetable[] =
{
    "(00)",		// Reserved
    "FPM",
    "EDO",
    "(03)",		// Pipeline Nibble
    "SDRAM",
    "ROM",
    "DDR SGRAM",
    "DDR",
};

#if 0
/*
awk 'func has(x, y)
{
    t = int(x / y);
    return t - int(t / 2) * 2;
}
BEGIN {
    printf("char *caslattable[] =\n{\n");

    for (i = 0; i < 64; i++)
    {
	printf("    \"");
	comma="";

	if (has(i, 32))
	{
	    printf("%s3.5", comma);
	    comma = ",";
	}

	if (has(i, 16))
	{
	    printf("%s3", comma);
	    comma = ",";
	}

	if (has(i, 8))
	{
	    printf("%s2.5", comma);
	    comma = ",";
	}

	if (has(i, 4))
	{
	    printf("%s2", comma);
	    comma = ",";
	}

	if (has(i, 2))
	{
	    printf("%s1.5", comma);
	    comma = ",";
	}

	if (has(i, 1))
	{
	    printf("%s1", comma);
	    comma = ",";
	}

	printf("\",\n");
    }

    printf("};\n\n");
}'
*/
#endif

char *caslattable[] =
{
    "",
    "1",
    "1.5",
    "1.5,1",
    "2",
    "2,1",
    "2,1.5",
    "2,1.5,1",
    "2.5",
    "2.5,1",
    "2.5,1.5",
    "2.5,1.5,1",
    "2.5,2",
    "2.5,2,1",
    "2.5,2,1.5",
    "2.5,2,1.5,1",
    "3",
    "3,1",
    "3,1.5",
    "3,1.5,1",
    "3,2",
    "3,2,1",
    "3,2,1.5",
    "3,2,1.5,1",
    "3,2.5",
    "3,2.5,1",
    "3,2.5,1.5",
    "3,2.5,1.5,1",
    "3,2.5,2",
    "3,2.5,2,1",
    "3,2.5,2,1.5",
    "3,2.5,2,1.5,1",
    "3.5",
    "3.5,1",
    "3.5,1.5",
    "3.5,1.5,1",
    "3.5,2",
    "3.5,2,1",
    "3.5,2,1.5",
    "3.5,2,1.5,1",
    "3.5,2.5",
    "3.5,2.5,1",
    "3.5,2.5,1.5",
    "3.5,2.5,1.5,1",
    "3.5,2.5,2",
    "3.5,2.5,2,1",
    "3.5,2.5,2,1.5",
    "3.5,2.5,2,1.5,1",
    "3.5,3",
    "3.5,3,1",
    "3.5,3,1.5",
    "3.5,3,1.5,1",
    "3.5,3,2",
    "3.5,3,2,1",
    "3.5,3,2,1.5",
    "3.5,3,2,1.5,1",
    "3.5,3,2.5",
    "3.5,3,2.5,1",
    "3.5,3,2.5,1.5",
    "3.5,3,2.5,1.5,1",
    "3.5,3,2.5,2",
    "3.5,3,2.5,2,1",
    "3.5,3,2.5,2,1.5",
    "3.5,3,2.5,2,1.5,1",
};

#if 0
/*
awk 'func has(x, y)
{
    t = int(x / y);
    return t - int(t / 2) * 2;
}
BEGIN {
    printf("char *cslattable[] =\n{\n");

    for (i = 0; i < 128; i++)
    {
	printf("    \"");
	comma="";

	if (has(i, 64))
	{
	    printf("6");
	    comma = ",";
	}

	if (has(i, 32))
	{
	    printf("%s5", comma);
	    comma = ",";
	}

	if (has(i, 16))
	{
	    printf("%s4", comma);
	    comma = ",";
	}

	if (has(i, 8))
	{
	    printf("%s3", comma);
	    comma = ",";
	}

	if (has(i, 4))
	{
	    printf("%s2", comma);
	    comma = ",";
	}

	if (has(i, 2))
	{
	    printf("%s1", comma);
	    comma = ",";
	}

	if (has(i, 1))
	{
	    printf("%s0", comma);
	    comma = ",";
	}

	printf("\",\n");
    }

    printf("};\n\n");
}'
*/
#endif


char *cslattable[] =
{
    "",
    "0",
    "1",
    "1,0",
    "2",
    "2,0",
    "2,1",
    "2,1,0",
    "3",
    "3,0",
    "3,1",
    "3,1,0",
    "3,2",
    "3,2,0",
    "3,2,1",
    "3,2,1,0",
    "4",
    "4,0",
    "4,1",
    "4,1,0",
    "4,2",
    "4,2,0",
    "4,2,1",
    "4,2,1,0",
    "4,3",
    "4,3,0",
    "4,3,1",
    "4,3,1,0",
    "4,3,2",
    "4,3,2,0",
    "4,3,2,1",
    "4,3,2,1,0",
    "5",
    "5,0",
    "5,1",
    "5,1,0",
    "5,2",
    "5,2,0",
    "5,2,1",
    "5,2,1,0",
    "5,3",
    "5,3,0",
    "5,3,1",
    "5,3,1,0",
    "5,3,2",
    "5,3,2,0",
    "5,3,2,1",
    "5,3,2,1,0",
    "5,4",
    "5,4,0",
    "5,4,1",
    "5,4,1,0",
    "5,4,2",
    "5,4,2,0",
    "5,4,2,1",
    "5,4,2,1,0",
    "5,4,3",
    "5,4,3,0",
    "5,4,3,1",
    "5,4,3,1,0",
    "5,4,3,2",
    "5,4,3,2,0",
    "5,4,3,2,1",
    "5,4,3,2,1,0",
    "6",
    "6,0",
    "6,1",
    "6,1,0",
    "6,2",
    "6,2,0",
    "6,2,1",
    "6,2,1,0",
    "6,3",
    "6,3,0",
    "6,3,1",
    "6,3,1,0",
    "6,3,2",
    "6,3,2,0",
    "6,3,2,1",
    "6,3,2,1,0",
    "6,4",
    "6,4,0",
    "6,4,1",
    "6,4,1,0",
    "6,4,2",
    "6,4,2,0",
    "6,4,2,1",
    "6,4,2,1,0",
    "6,4,3",
    "6,4,3,0",
    "6,4,3,1",
    "6,4,3,1,0",
    "6,4,3,2",
    "6,4,3,2,0",
    "6,4,3,2,1",
    "6,4,3,2,1,0",
    "6,5",
    "6,5,0",
    "6,5,1",
    "6,5,1,0",
    "6,5,2",
    "6,5,2,0",
    "6,5,2,1",
    "6,5,2,1,0",
    "6,5,3",
    "6,5,3,0",
    "6,5,3,1",
    "6,5,3,1,0",
    "6,5,3,2",
    "6,5,3,2,0",
    "6,5,3,2,1",
    "6,5,3,2,1,0",
    "6,5,4",
    "6,5,4,0",
    "6,5,4,1",
    "6,5,4,1,0",
    "6,5,4,2",
    "6,5,4,2,0",
    "6,5,4,2,1",
    "6,5,4,2,1,0",
    "6,5,4,3",
    "6,5,4,3,0",
    "6,5,4,3,1",
    "6,5,4,3,1,0",
    "6,5,4,3,2",
    "6,5,4,3,2,0",
    "6,5,4,3,2,1",
    "6,5,4,3,2,1,0",
};

#if 0
/*
awk 'func has(x, y)
{
    t = int(x / y);
    return t - int(t / 2) * 2;
}
BEGIN {
    printf("char *bursttable[] =\n{\n");

    for (i = 0; i < 32; i++)
    {
	printf("    \"");
	comma="";

	if (has(i, 16))
	{
	    printf("%sP", comma);
	    comma = ",";
	}

	if (has(i, 8))
	{
	    printf("%s8", comma);
	    comma = ",";
	}

	if (has(i, 4))
	{
	    printf("%s4", comma);
	    comma = ",";
	}

	if (has(i, 2))
	{
	    printf("%s2", comma);
	    comma = ",";
	}

	if (has(i, 1))
	{
	    printf("%s1", comma);
	    comma = ",";
	}

	printf("\",\n");
    }

    printf("};\n\n");
}'
*/
#endif

char *bursttable[] =
{
    "",
    "1",
    "2",
    "2,1",
    "4",
    "4,1",
    "4,2",
    "4,2,1",
    "8",
    "8,1",
    "8,2",
    "8,2,1",
    "8,4",
    "8,4,1",
    "8,4,2",
    "8,4,2,1",
    "P",
    "P,1",
    "P,2",
    "P,2,1",
    "P,4",
    "P,4,1",
    "P,4,2",
    "P,4,2,1",
    "P,8",
    "P,8,1",
    "P,8,2",
    "P,8,2,1",
    "P,8,4",
    "P,8,4,1",
    "P,8,4,2",
    "P,8,4,2,1",
};

#if 0
/*
awk 'func has(x, y)
{
    t = int(x / y);
    return t - int(t / 2) * 2;
}
BEGIN {
    printf("char *modattribtable[] =\n{\n");

    for (i = 0; i < 64; i++)
    {
	printf("    \"");
	comma="";

	if (has(i, 32))
	{
	    printf("DiffClkIn");
	    comma = ",";
	}

	if (has(i, 16))
	{
	    printf("%sFETExEnb", comma);
	    comma = ",";
	}

	if (has(i, 8))
	{
	    printf("%sFETEnb", comma);
	    comma = ",";
	}

	if (has(i, 4))
	{
	    printf("%sPLL", comma);
	    comma = ",";
	}

	if (has(i, 2))
	{
	    printf("%sReg", comma);
	    comma = ",";
	}

	if (has(i, 1))
	{
	    printf("%sBuf", comma);
	    comma = ",";
	}

	printf("\",\n");
    }

    printf("};\n\n");
}'
*/
#endif

char *modattribtable[] =
{
    "-",
    "Buf",
    "Reg",
    "Reg,Buf",
    "PLL",
    "PLL,Buf",
    "PLL,Reg",
    "PLL,Reg,Buf",
    "FETEnb",
    "FETEnb,Buf",
    "FETEnb,Reg",
    "FETEnb,Reg,Buf",
    "FETEnb,PLL",
    "FETEnb,PLL,Buf",
    "FETEnb,PLL,Reg",
    "FETEnb,PLL,Reg,Buf",
    "FETExEnb",
    "FETExEnb,Buf",
    "FETExEnb,Reg",
    "FETExEnb,Reg,Buf",
    "FETExEnb,PLL",
    "FETExEnb,PLL,Buf",
    "FETExEnb,PLL,Reg",
    "FETExEnb,PLL,Reg,Buf",
    "FETExEnb,FETEnb",
    "FETExEnb,FETEnb,Buf",
    "FETExEnb,FETEnb,Reg",
    "FETExEnb,FETEnb,Reg,Buf",
    "FETExEnb,FETEnb,PLL",
    "FETExEnb,FETEnb,PLL,Buf",
    "FETExEnb,FETEnb,PLL,Reg",
    "FETExEnb,FETEnb,PLL,Reg,Buf",
    "DiffClkIn",
    "DiffClkIn,Buf",
    "DiffClkIn,Reg",
    "DiffClkIn,Reg,Buf",
    "DiffClkIn,PLL",
    "DiffClkIn,PLL,Buf",
    "DiffClkIn,PLL,Reg",
    "DiffClkIn,PLL,Reg,Buf",
    "DiffClkIn,FETEnb",
    "DiffClkIn,FETEnb,Buf",
    "DiffClkIn,FETEnb,Reg",
    "DiffClkIn,FETEnb,Reg,Buf",
    "DiffClkIn,FETEnb,PLL",
    "DiffClkIn,FETEnb,PLL,Buf",
    "DiffClkIn,FETEnb,PLL,Reg",
    "DiffClkIn,FETEnb,PLL,Reg,Buf",
    "DiffClkIn,FETExEnb",
    "DiffClkIn,FETExEnb,Buf",
    "DiffClkIn,FETExEnb,Reg",
    "DiffClkIn,FETExEnb,Reg,Buf",
    "DiffClkIn,FETExEnb,PLL",
    "DiffClkIn,FETExEnb,PLL,Buf",
    "DiffClkIn,FETExEnb,PLL,Reg",
    "DiffClkIn,FETExEnb,PLL,Reg,Buf",
    "DiffClkIn,FETExEnb,FETEnb",
    "DiffClkIn,FETExEnb,FETEnb,Buf",
    "DiffClkIn,FETExEnb,FETEnb,Reg",
    "DiffClkIn,FETExEnb,FETEnb,Reg,Buf",
    "DiffClkIn,FETExEnb,FETEnb,PLL",
    "DiffClkIn,FETExEnb,FETEnb,PLL,Buf",
    "DiffClkIn,FETExEnb,FETEnb,PLL,Reg",
    "DiffClkIn,FETExEnb,FETEnb,PLL,Reg,Buf",
};

#if 0
/*
awk 'func has(x, y)
{
    t = int(x / y);
    return t - int(t / 2) * 2;
}
BEGIN {
    printf("char *devattribtable[] =\n{\n");

    for (i = 0; i < 64; i++)
    {
	printf("    \"");
	comma="";

	if (has(i, 16))
	{
	    printf("%sFastAP", comma);
	    comma = ",";
	}

	if (has(i, 8))
	{
	    printf("%sAutoPre", comma);
	    comma = ",";
	}

	if (has(i, 4))
	{
	    printf("%sPLL", comma);
	    printf("%sUppVDDtol=0.2V", comma);
	    comma = ",";
	}

	if (has(i, 2))
	{
	    printf("%sLowVDDtol=0.2V", comma);
	    comma = ",";
	}

	if (has(i, 1))
	{
	    printf("%sWeakDrv", comma);
	    comma = ",";
	}



	printf("\",\n");
    }

    printf("};\n\n");
}'
*/
#endif

char *devattribtable[] =
{
    "-",
    "WeakDrv",
    "LowVDDtol=0.2V",
    "LowVDDtol=0.2V,WeakDrv",
    "PLLUppVDDtol=0.2V",
    "PLLUppVDDtol=0.2V,WeakDrv",
    "PLLUppVDDtol=0.2V,LowVDDtol=0.2V",
    "PLLUppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "AutoPre",
    "AutoPre,WeakDrv",
    "AutoPre,LowVDDtol=0.2V",
    "AutoPre,LowVDDtol=0.2V,WeakDrv",
    "AutoPre,PLL,UppVDDtol=0.2V",
    "AutoPre,PLL,UppVDDtol=0.2V,WeakDrv",
    "AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "FastAP",
    "FastAP,WeakDrv",
    "FastAP,LowVDDtol=0.2V",
    "FastAP,LowVDDtol=0.2V,WeakDrv",
    "FastAP,PLL,UppVDDtol=0.2V",
    "FastAP,PLL,UppVDDtol=0.2V,WeakDrv",
    "FastAP,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "FastAP,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre",
    "FastAP,AutoPre,WeakDrv",
    "FastAP,AutoPre,LowVDDtol=0.2V",
    "FastAP,AutoPre,LowVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "",
    "WeakDrv",
    "LowVDDtol=0.2V",
    "LowVDDtol=0.2V,WeakDrv",
    "PLLUppVDDtol=0.2V",
    "PLLUppVDDtol=0.2V,WeakDrv",
    "PLLUppVDDtol=0.2V,LowVDDtol=0.2V",
    "PLLUppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "AutoPre",
    "AutoPre,WeakDrv",
    "AutoPre,LowVDDtol=0.2V",
    "AutoPre,LowVDDtol=0.2V,WeakDrv",
    "AutoPre,PLL,UppVDDtol=0.2V",
    "AutoPre,PLL,UppVDDtol=0.2V,WeakDrv",
    "AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "FastAP",
    "FastAP,WeakDrv",
    "FastAP,LowVDDtol=0.2V",
    "FastAP,LowVDDtol=0.2V,WeakDrv",
    "FastAP,PLL,UppVDDtol=0.2V",
    "FastAP,PLL,UppVDDtol=0.2V,WeakDrv",
    "FastAP,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "FastAP,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre",
    "FastAP,AutoPre,WeakDrv",
    "FastAP,AutoPre,LowVDDtol=0.2V",
    "FastAP,AutoPre,LowVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,WeakDrv",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V",
    "FastAP,AutoPre,PLL,UppVDDtol=0.2V,LowVDDtol=0.2V,WeakDrv",
};

void
dump_spd(int *dimms, int num)
{
    int i;

    if (dimms == NULL)
    {
	num = spd_count();
	dimms = (int *)malloc(sizeof (int) * num);

	if (dimms == NULL)
	    return;

	for (i = 0; i < num; i++)
	    dimms[i] = i;

	qsort(dimms, num, sizeof (int), dimmcmp);
    }

    printf("Part Number           Type Rev Banks Width Cfg   Voltage   Cols  Rows  BSize\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte rev;
	uByte banks;
	uShort width;
	uByte cfg;
	uByte tmp;

	printf("%-20s", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    if (dramtype < 7)
		printf("   %s\n", dramtypetable[dramtype]);
	    else
		printf("   (%02X)\n", dramtype);

	    continue;
	}

	printf("  DDR ");

	rev = spd_byte(n, 62);
	printf(" %X.%X", (rev >> 4) & 0xF, rev & 0xF);

	banks = spd_byte(n, 5);
	printf(" %5d", banks);

	width = spd_byte(n, 6);
	width |= spd_byte(n, 7) << 8;
	printf(" %5d", width);

	cfg = spd_byte(n, 11);

	switch (cfg)
	{
	case 0: printf(" None  "); break;
	case 1: printf(" Parity"); break;
	case 2: printf(" ECC   "); break;
	default: printf(" (%02X)  ", cfg); break;
	}

	tmp = spd_byte(n, 8);

	switch (tmp)
	{
	case 0: printf("TTL     "); break;
	case 1: printf("LVTTL   "); break;
	case 2: printf("HSTL 1.5"); break;
	case 3: printf("SSTL 3.3"); break;
	case 4: printf("SSTL 2.5"); break;
	default: printf("(%02X)    ", tmp); break;
	}

	tmp = spd_byte(n, 3);
	printf(" %5s", fmtnum(1 << tmp));

	tmp = spd_byte(n, 4);
	printf(" %5s", fmtnum(1 << tmp));

	tmp = spd_byte(n, 31);

	if (tmp & 0x01)
		printf("     1G");
	else if (tmp & 0x02)
		printf("     2G");
	else if (tmp & 0x04)
		printf("    16M");
	else if (tmp & 0x08)
		printf("    32M");
	else if (tmp & 0x10)
		printf("    64M");
	else if (tmp & 0x20)
		printf("   128M");
	else if (tmp & 0x40)
		printf("   256M");
	else if (tmp & 0x80)
		printf("   512M");

	printf("\n");
    }

    printf("\n");

    printf("Part Number           Type   DWidth EWidth DBnks Bursts   tREF type RB2B    WL\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte tmp;

	printf("%-20s  ", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	tmp = spd_byte(n, 9);

	if (tmp <= 0x50)
	    printf("PC3200");
	else if (tmp <= 0x60)
	    printf("PC2700");
	else if (tmp <= 0x75)
	    printf("PC2100");
	else if (tmp <= 0xA0)
	    printf("PC1600");
	else
	    printf("DDR   ");

/*
Part Number           Type   DWidth EWidth DBanks  Bursts    tREF RB2B   WL
================================================================================
AS1231328123          PC3200   4  8   8  8     16  8,4,2,1 15.625  7.5   15
*/

	tmp = spd_byte(n, 13);
	printf(" %3d", tmp & 0x7F); 
	printf(" %2d", (tmp & 0x7F) * (tmp & 0x80 ? 2 : 1)); 

	tmp = spd_byte(n, 14);
	printf(" %3d", tmp & 0x7F); 
	printf(" %2d", (tmp & 0x7F) * (tmp & 0x80 ? 2 : 1)); 

	tmp = spd_byte(n, 17);
	printf("   %3d", tmp);

	tmp = spd_byte(n, 16);
	printf(" %6s", bursttable[((tmp >> 3) & 0x10) | (tmp & 0xF)]);

	tmp = spd_byte(n, 12);

	switch (tmp & 0x7F)
	{
	case 0: printf(" 15.625"); break;
	case 1: printf("  3.906"); break;
	case 2: printf("  7.813"); break;
	case 3: printf(" 31.25"); break;
	case 4: printf(" 62.5"); break;
	case 5: printf("125"); break;
	default: printf("   (%02X)", tmp & 0x7F); break;
	}

	printf(" %4s", tmp & 0x80 ? "self" : "");

	/* Read Back to Back latency */
	tmp = spd_byte(n, 15);
	printf("  %3d", tmp);

	/* Write Latency */
	tmp = spd_byte(n, 20);
	printf(" %5s", cslattable[tmp & 0xF]);

	printf("\n");
    }

    printf("\n");

    printf("Part Number           Type    Module Attributes        Device Attributes\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte tmp;

	printf("%-20s  ", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	tmp = spd_byte(n, 9);

	if (tmp <= 0x50)
	    printf("PC3200");
	else if (tmp <= 0x60)
	    printf("PC2700");
	else if (tmp <= 0x75)
	    printf("PC2100");
	else if (tmp <= 0xA0)
	    printf("PC1600");
	else
	    printf("DDR   ");

/*
Part Number           Type    Module Attributes        Device Attributes
================================================================================
AS1231328123          PC3200  123456789012345678901234 123456789012345678901234
*/

	tmp = spd_byte(n, 21);
	printf("  %-24s", modattribtable[tmp & 0x3F]);
	tmp = spd_byte(n, 22);
	printf(" %-24s", devattribtable[((tmp >> 3) & 0x1E) | (tmp & 0x1)]);
	printf("\n");
    }

    printf("\n");

    printf("Part Number           Type        CL CSLat   tCK  tAC   tCK  tAC   tCK  tAC\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte tmp;

	printf("%-20s  ", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	tmp = spd_byte(n, 9);

	if (tmp <= 0x50)
	    printf("PC3200");
	else if (tmp <= 0x60)
	    printf("PC2700");
	else if (tmp <= 0x75)
	    printf("PC2100");
	else if (tmp <= 0xA0)
	    printf("PC1600");
	else
	    printf("DDR   ");

	tmp = spd_byte(n, 18);
	printf("%8s", caslattable[tmp & 0x3F]);
	tmp = spd_byte(n, 19);
	printf("%6s", cslattable[tmp & 0x7F]);

	tmp = spd_byte(n, 9);
	printf("  %2d.%d", (tmp >> 4) & 0xF, tmp & 0xF);
	tmp = spd_byte(n, 10);
	printf("  .%d%d", (tmp >> 4) & 0xF, tmp & 0xF);

	tmp = spd_byte(n, 23);

	if (tmp)
	    printf("  %2d.%d", (tmp >> 4) & 0xF, tmp & 0xF);
	else
	    printf("     -");

	tmp = spd_byte(n, 24);

	if (tmp)
	    printf("  .%d%d", (tmp >> 4) & 0xF, tmp & 0xF);
	else
	    printf("    -");

	tmp = spd_byte(n, 25);

	if (tmp)
	    printf("  %2d.%d", (tmp >> 4) & 0xF, tmp & 0xF);
	else
	    printf("     -");

	tmp = spd_byte(n, 26);

	if (tmp)
	    printf("  .%d%d", (tmp >> 4) & 0xF, tmp & 0xF);
	else
	    printf("    -");

	printf("\n");
    }

    printf("\n");

    printf("Part Number           Type     tRP  tRRD  tRCD  tRAS   tCSU tCHLD  tDSU tDHLD\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte tmp;

	printf("%-20s  ", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	tmp = spd_byte(n, 9);

	if (tmp <= 0x50)
	    printf("PC3200");
	else if (tmp <= 0x60)
	    printf("PC2700");
	else if (tmp <= 0x75)
	    printf("PC2100");
	else if (tmp <= 0xA0)
	    printf("PC1600");
	else
	    printf("DDR   ");

/*
Part Number           Type     tRP  tRRD  tRCD tRAS tCSU tCHLD tDSU tDHLD
================================================================================
AS1231328123          PC3200 xx.xx xx.xx xx.xx  xxx  .xx   .xx  .xx   .xx
*/

	/* tRP */
	tmp = spd_byte(n, 27);
	printf("  %5s", fmtquarterns(tmp));

	/* tRRD */
	tmp = spd_byte(n, 28);
	printf(" %5s", fmtquarterns(tmp));

	/* tRCD */
	tmp = spd_byte(n, 29);
	printf(" %5s", fmtquarterns(tmp));

	/* tRAS */
	tmp = spd_byte(n, 29);
	printf("  %3d", tmp);

	/* Addr/Cmd Setup Time */
	tmp = spd_byte(n, 32);
	printf("  %5s", fmthundrethns(tmp));

	/* Addr/Cmd Hold Time */
	tmp = spd_byte(n, 33);
	printf(" %5s", fmthundrethns(tmp));

	/* Data/Mask Setup Time */
	tmp = spd_byte(n, 34);
	printf(" %5s", fmthundrethns(tmp));

	/* Data/Mask Hold Time */
	tmp = spd_byte(n, 35);
	printf(" %5s", fmthundrethns(tmp));
	printf("\n");
    }

    printf("\n");

    printf("Part Number           Type    SPDLen SPDSz ChkSum tRC tRFC maxCK tDQSQ tQHS\n");
    printf("================================================================================\n");

    for (i = 0; i < num; i++)
    {
	int n = dimms[i];
	uByte dramtype;
	uByte tmp;

	printf("%-20s  ", spd_name(n));
	dramtype = spd_byte(n, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	tmp = spd_byte(n, 9);

	if (tmp <= 0x50)
	    printf("PC3200");
	else if (tmp <= 0x60)
	    printf("PC2700");
	else if (tmp <= 0x75)
	    printf("PC2100");
	else if (tmp <= 0xA0)
	    printf("PC1600");
	else
	    printf("DDR   ");

/*
Part Number           Type    SPDLen SPDSz ChkSum  tRC  tRFC  maxCK tDQSQ tQHS
================================================================================
AS1231328123          PC3200     128   256     FF  ddd   ddd  dd.dd  d.dd d.dd
*/

	tmp = spd_byte(n, 0);
	printf("     %3d", tmp);

	tmp = spd_byte(n, 1);
	printf(" %5s", fmtnum(1 << tmp));

	tmp = spd_byte(n, 63);
	printf("   %02X  ", tmp);

	/* tRC */
	tmp = spd_byte(n, 41);
	printf(" %3d", tmp);

	/* tRFC */
	tmp = spd_byte(n, 42);
	printf("  %3d", tmp);

	/* tCK max */
	tmp = spd_byte(n, 43);
	printf("  %5s", fmtquarterns(tmp));

	tmp = spd_byte(n, 44);

	if ((tmp % 10) == 0)
	    printf("  %d.%d ", tmp / 100, (tmp / 10) % 10);
	else
	    printf("  %d.%02d", tmp / 100, tmp % 100);

	tmp = spd_byte(n, 45);
	printf(" %4s", fmthundrethns(tmp));

	printf("\n");
    }

    printf("\n");
}

int 
main(int argc, char **argv)
{
    int i;
    int dimms;
    int set[200];
    int num = 0;

    if (!read_spds())
    {
	fprintf(stderr, "Unable to read SPD values from directory spds/\n");
	exit(1);
    }

    if (argc > 1)
    {
	for (i = 1; i < argc; i++)
	{
	    char *name = argv[i];
	    int n = spd_num(name);

	    if (n < 0)
		fprintf(stderr, "Unable to find SPD info for %s\n", name);
	    else
		set[num++] = n;
	}

	dump_spd(set, num);
    }
    else
	dump_spd(NULL, -1);
}
