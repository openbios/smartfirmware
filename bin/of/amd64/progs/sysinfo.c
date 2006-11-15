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


#ifdef STANDALONE
#include <stdio.h>
#endif

#include "sfclient.h"
extern int bprintf(char *buf, const char *fmt, ...);
#define sprintf bprintf

#ifndef STANDALONE
#define PCI_CONFIG_ADDR		0xCF8ul
#define PCI_CONFIG_DATA		0xCFCul

#define printf sf_printf
#define exit(r) sf_exit()
#endif

#define MAX_NODES	8
#define MAX_CPUS	16
#define MAX_HT		3
#define MAX_LINKS	(MAX_NODES*MAX_HT)

#define STEPPING_A0	0x00
#define STEPPING_A2	0x02
#define STEPPING_B0	0x10
#define STEPPING_C0	0x20

char g_stepping = STEPPING_A2;
int numnodes;
char nodepresent[MAX_NODES];
char cpupresent[MAX_CPUS];
uByte linkpresent[MAX_NODES][MAX_HT];

#ifndef STANDALONE
void
outl(int addr, unsigned int val)
{
    __asm __volatile("outl %0,%%dx" : : "a" (val), "d" (addr));
}

unsigned int
inl(int addr)
{
    unsigned val;
    __asm __volatile("inl %%dx,%0" : "=a" (val) : "d" (addr));
    return val;
}

uInt
pci_config_read(uInt physhi)
{
    unsigned int value;
    outl(PCI_CONFIG_ADDR, physhi | 0x80000000);
    value = inl(PCI_CONFIG_DATA);
    outl(PCI_CONFIG_ADDR, 0);
    return value;
}

void
pci_config_write(uInt physhi, uInt value)
{
    outl(PCI_CONFIG_ADDR, physhi | 0x80000000);
    outl(PCI_CONFIG_DATA, value);
    outl(PCI_CONFIG_ADDR, 0);
}
#else

#include "data.c"

unsigned int *nodes[] =
{
	node0,
	node1,
	node2,
	node3,
	node4,
	node5,
	node6,
	node7,
};

uInt
pci_config_read(uInt physhi)
{
    int n = (physhi >> 11) & 0x7;
    int w = (physhi >> 2) & 0xFF;

    return nodes[n][w];
}

void
pci_config_write(uInt physhi, uInt value)
{
}
#endif

/*
Solo Example
                                            Route to node
                               |------------------------------------------|
Node    HT0     HT1     HT2    N0    N1    N2    N3    N4    N5    N6    N7
================================================================================
N0       IO       -       -     -     -     -     -     -     -     -     -

8-way example
                                            Route to node
                               |------------------------------------------|
Node    HT0     HT1     HT2    N0    N1    N2    N3    N4    N5    N6    N7
================================================================================
N0       IO      N1      N2     -   HT1   HT2   HT1   HT2   HT1   HT2   HT1
N1       N3      N4      N2   HT2     -   HT2   HT0   HT1     -     -     -
N2       N0      N4      IO   HT0     -     -     -   HT1     -     -     -
N3       N5      N6      N1   HT2   HT2     -     -     -   HT0   HT1     -
N4       N2      N1      N6     -   HT1   HT0     -     -     -   HT2     -
N5       N7      N3      IO     -     -     -   HT1     -     -     -   HT0
N6       N4      N3      N7     -     -     -   HT1   HT0     -     -   HT2
N7       IO      N6      N5     -     -     -     -     -   HT2   HT1     -

0 - 1 - 3 - 5
|   |   |   |
2 - 4 - 6 - 7
*/

static int
get_neighbor(int node, int link)
{
    return -1;
}

#define RQ	1
#define RP	2
#define BC	3

static int
get_route(int src, int dest, int packettype)
{
    uInt physhi = 0x00C040 | (src << 11) | (dest << 2);
    uInt reg = pci_config_read(physhi);
    int linkset;

    switch (packettype)
    {
    case RQ:
	linkset = reg & 0xF;
	break;
    case RP:
	linkset = (reg >> 8) & 0xF;
	break;
    case BC:
	linkset = (reg >> 16) & 0xF;
	break;
    default:
	linkset = 0;
	break;
    }

    switch (linkset)
    {
    case 0x1:
	return -1;
    case 0x2:
	return 0;
    case 0x4:
	return 1;
    case 0x8:
	return 2;
    }

    return -2;
}

void
show_nodes()
{
    int n;
    int i;

    numnodes = ((pci_config_read(0xC060) >> 4) & 0x7) + 1;

    printf("                                            Route to node\n");
    printf("                               |------------------------------------------|\n");
    printf("Node    HT0     HT1     HT2    N0    N1    N2    N3    N4    N5    N6    N7\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt capptr = pci_config_read(0xC034 | (n << 11)) & 0xFF;

	nodepresent[n] = n < numnodes;

	if (!nodepresent[n])
	{
	    for (i = 0; i < MAX_HT; i++)
		linkpresent[n][i] = 0;

	    continue;
	}

	printf("N%d ", n);

	for (i = 0; i < MAX_HT; i++)
	{
	    int node;

	    if (capptr)
	    {
		uInt cap = pci_config_read(0xC000 | (n << 11) | capptr);

		while (((cap >> 8) & 0xFF) != 0 && 
			((cap >> 8) & 0xFF) != 0xFF && (cap & 0xFF) != 8)
		{
		    capptr = (cap >> 8) & 0xFF;
		    cap = pci_config_read(0xC000 | (n << 11) | capptr);
		}


		if ((cap & 0xFF) == 8)
		{
		    linkpresent[n][i] = capptr;
		    capptr = (cap >> 8) & 0xFF;
		}
		else
		{
		    linkpresent[n][i] = 0;
		    capptr = 0;
		}
	    }
	    else
		linkpresent[n][i] = 0;

	    node = get_neighbor(n, i);

	    if (node < 0)
		printf("      IO");
	    else
		printf("      N%d", node);
	}

	for (i = 0; i < MAX_NODES; i++)
	{
	    int ht = get_route(n, i, RQ);

	    if (ht < 0)
		printf("     -");
	    else
		printf("   HT%d", ht);
	}

	printf("\n");
    }

    printf("\n");
}

static void
show_links()
{
    int n;
    int l;

/*
Node  Link  --cap---  -linkctl  --freq--  --feat--  -bufcnt-  -busnum-  --type--
================================================================================
N0     HT0  XXXXXXXX  XXXXXXXX  XXXXXXXX  XXXXXXXX  XXXXXXXX  XXXXXXXX  XXXXXXXX
*/
    printf("Node  Link  --cap---  -linkctl  --freq--  --feat--  -bufcnt-  -busnum-  --type--\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	if (!nodepresent[n])
	    continue;

	for (l = 0; l < MAX_HT; l++)
	{
	    uInt physhi = 0xC000 | (n << 11) | linkpresent[n][l];
	    uInt cap = pci_config_read(physhi + 0x00);
	    uInt linkctl = pci_config_read(physhi + 0x04);
	    uInt freq = pci_config_read(physhi + 0x08);
	    uInt feat = pci_config_read(physhi + 0x0C);
	    uInt bufcnt = pci_config_read(physhi + 0x10);
	    uInt busnum = pci_config_read(physhi + 0x14);
	    uInt type = pci_config_read(physhi + 0x18);

	    if (!linkpresent[n][l])
		continue;

	    printf("N%d     HT%d", n, l);
	    printf("  %08X", cap);
	    printf("  %08X", linkctl);
	    printf("  %08X", freq);
	    printf("  %08X", feat);
	    printf("  %08X", bufcnt);
	    printf("  %08X", busnum);
	    printf("  %08X", type);
	    printf("\n");
	}
    }

    printf("\n");


/*
Node  Link   Rev  Freq  Width  SecBus  SubBus
================================================================================
N0     HT0  1.01   800     16       0       3
*/
    printf("Node  Link   Rev  Freq  Width  SecBus  SubBus\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	if (!nodepresent[n])
	    continue;

	for (l = 0; l < MAX_HT; l++)
	{
	    uInt physhi = 0xC000 | (n << 11) | linkpresent[n][l];
//	    uInt cap = pci_config_read(physhi + 0x00);
	    uInt linkctl = pci_config_read(physhi + 0x04);
	    uInt freq = pci_config_read(physhi + 0x08);
//	    uInt feat = pci_config_read(physhi + 0x0C);
//	    uInt bufcnt = pci_config_read(physhi + 0x10);
	    uInt busnum = pci_config_read(physhi + 0x14);
//	    uInt type = pci_config_read(physhi + 0x18);

	    if (!linkpresent[n][l])
		continue;

	    printf("N%d     HT%d", n, l);

	    printf("  %d.%02d", (freq >> 5) & 0x7, freq & 0x1F);

	    switch ((freq >> 8) & 0xF)
	    {
	    case 0x0: printf("   200"); break;
	    case 0x2: printf("   400"); break;
	    case 0x4: printf("   600"); break;
	    case 0x5: printf("   800"); break;
	    case 0x6: printf("  1000"); break;
	    case 0xF: printf("   100"); break;
	    default: printf("   (%X)", (freq >> 8) & 0xF); break;
	    }

	    switch ((linkctl >> 28) & 0x7)
	    {
	    case 0: printf("      8"); break;
	    case 1: printf("     16"); break;
	    case 2: printf("    (2)"); break;
	    case 3: printf("     32"); break;
	    case 4: printf("      2"); break;
	    case 5: printf("      4"); break;
	    case 6: printf("    (6)"); break;
	    case 7: printf("    N/C"); break;
	    }

	    printf("     %3d", (busnum >> 8) & 0xFF);
	    printf("     %3d", (busnum >> 16) & 0xFF);
	    printf("\n");
	}
    }

    printf("\n");
}

void
show_nodemap()
{
    int n;
    int i;

    // show DRAM ranges, IO ranges, and config ranges in Function 1

/*
Node  Decoder  Start      End         DstNode   R/W    Interleave
================================================================================
N0    DRAM0    0000000000-001FFFFFFF  N0        Write  4 nodes (A14,A13,A12=7)
*/
    printf("Node  Decoder  Start      End         DstNode   R/W    Interleave\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt physhi = 0xC140 | (n << 11);

	if (!nodepresent[n])
	    continue;

	for (i = 0; i < 8; i++)
	{
	    uInt base = pci_config_read(physhi + i * 8);
	    uInt limit = pci_config_read(physhi + i * 8 + 4);
	    char *interleave = NULL;
	    char *rw = NULL;

	    switch ((base >> 8) & 7)
	    {
	    case 0:
		interleave = "None";
		break;
	    case 1:
		switch ((limit >> 8) & 7)
		{
		case 0: interleave = "2 nodes (A12=0)"; break;
		case 1: interleave = "2 nodes (A12=1)"; break;
		case 2: interleave = "2 nodes (2)"; break;
		case 3: interleave = "2 nodes (3)"; break;
		case 4: interleave = "2 nodes (4)"; break;
		case 5: interleave = "2 nodes (5)"; break;
		case 6: interleave = "2 nodes (6)"; break;
		case 7: interleave = "2 nodes (7)"; break;
		}

		break;
	    case 2:
		interleave = "(2)";
		break;
	    case 3:
		switch ((limit >> 8) & 7)
		{
		case 0: interleave = "4 nodes (A13,A12=0)"; break;
		case 1: interleave = "4 nodes (A13,A12=1)"; break;
		case 2: interleave = "4 nodes (A13,A12=2)"; break;
		case 3: interleave = "4 nodes (A13,A12=3)"; break;
		case 4: interleave = "4 nodes (4)"; break;
		case 5: interleave = "4 nodes (5)"; break;
		case 6: interleave = "4 nodes (6)"; break;
		case 7: interleave = "4 nodes (7)"; break;
		}

		break;
	    case 4:
		interleave = "(4)";
		break;
	    case 5:
		interleave = "(5)";
		break;
	    case 6:
		interleave = "(6)";
		break;
	    case 7:
		switch ((limit >> 8) & 7)
		{
		case 0: interleave = "4 nodes (A14,A13,A12=0)"; break;
		case 1: interleave = "4 nodes (A14,A13,A12=1)"; break;
		case 2: interleave = "4 nodes (A14,A13,A12=2)"; break;
		case 3: interleave = "4 nodes (A14,A13,A12=3)"; break;
		case 4: interleave = "4 nodes (A14,A13,A12=4)"; break;
		case 5: interleave = "4 nodes (A14,A13,A12=5)"; break;
		case 6: interleave = "4 nodes (A14,A13,A12=6)"; break;
		case 7: interleave = "4 nodes (A14,A13,A12=7)"; break;
		}

		break;
	    }

	    switch (base & 3)
	    {
	    case 0: rw = "None "; break;
	    case 1: rw = "Read "; break;
	    case 2: rw = "Write"; break;
	    case 3: rw = "R/W  "; break;
	    }

	    printf("N%d    DRAM%d    %010qX-%010qX  N%d        %s  %s\n",
		    n, i, (uLong)(base & 0xFFFF0000) << 8,
		    ((uLong)limit << 8) | 0xFFFFFF, limit & 7, rw, interleave);
	}

	physhi += 0x40;

	for (i = 0; i < 8; i++)
	{
	    uInt base = pci_config_read(physhi + i * 8);
	    uInt limit = pci_config_read(physhi + i * 8 + 4);
	    char *attr = NULL;
	    char *rw = NULL;
	    char *np = NULL;
	    char *link = NULL;

	    switch ((base >> 2) & 3)
	    {
	    case 0: attr = ""; break;
	    case 1: attr = "CpuDis "; break;
	    case 2: attr = "Lock "; break;
	    case 3: attr = "Lock CpuDis "; break;
	    }

	    switch (base & 3)
	    {
	    case 0: rw = "None "; break;
	    case 1: rw = "Read "; break;
	    case 2: rw = "Write"; break;
	    case 3: rw = "R/W  "; break;
	    }

	    if (limit & 0x80)
		np = "NP ";
	    else
		np = "";

	    switch ((limit >> 4) & 3)
	    {
	    case 0: link = "Link0"; break;
	    case 1: link = "Link1"; break;
	    case 2: link = "Link2"; break;
	    case 3: link = ""; break;
	    }

	    printf("N%d    MMIO%d    %010qX-%010qX  N%d        %s  %s%s%s\n",
		    n, i, (uLong)(base & 0xFFFFFF00) << 8,
		    ((uLong)limit << 8) | 0xFFFF, limit & 7, rw, attr, np, link);
	}

	physhi += 0x40;

	for (i = 0; i < 4; i++)
	{
	    uInt base = pci_config_read(physhi + i * 8);
	    uInt limit = pci_config_read(physhi + i * 8 + 4);
	    char *attr = NULL;
	    char *rw = NULL;
	    char *link = NULL;

	    switch ((base >> 4) & 3)
	    {
	    case 0: attr = ""; break;
	    case 1: attr = "VGAEnb "; break;
	    case 2: attr = "ISAEnb "; break;
	    case 3: attr = "ISAEnb VGAEnb "; break;
	    }

	    switch (base & 3)
	    {
	    case 0: rw = "None "; break;
	    case 1: rw = "Read "; break;
	    case 2: rw = "Write"; break;
	    case 3: rw = "R/W  "; break;
	    }

	    switch ((limit >> 4) & 3)
	    {
	    case 0: link = "Link0"; break;
	    case 1: link = "Link1"; break;
	    case 2: link = "Link2"; break;
	    case 3: link = ""; break;
	    }

	    printf("N%d    IO%d          %06X-    %06X  N%d        %s  %s%s\n",
		    n, i, base & 0x00FFF000,
		    (limit & 0x0FFF000) | 0xFFF, limit & 7, rw, attr, link);
	}

	physhi += 0x20;

	for (i = 0; i < 4; i++)
	{
	    uInt base = pci_config_read(physhi + i * 4);
	    char *attr = NULL;
	    char *rw = NULL;
	    char *link = NULL;

	    switch ((base >> 2) & 1)
	    {
	    case 0: attr = "DevCmp "; break;
	    case 1: attr = "BusCmp "; break;
	    }

	    switch (base & 3)
	    {
	    case 0: rw = "None "; break;
	    case 1: rw = "Read "; break;
	    case 2: rw = "Write"; break;
	    case 3: rw = "R/W  "; break;
	    }

	    switch ((base >> 8) & 3)
	    {
	    case 0: link = "Link0"; break;
	    case 1: link = "Link1"; break;
	    case 2: link = "Link2"; break;
	    case 3: link = ""; break;
	    }

	    printf("N%d    CONF%d            %02X-        %02X  N%d        %s  %s%s\n",
		    n, i, (base >> 16) & 0xFF,
		    (base >> 24) & 0xFF, (base >> 4) & 7, rw, attr, link);
	}
    }

    printf("\n");
}

void
show_drambanks()
{
    int n;
    int i;

    // show DRAM CS in Function 2
    // Only 36 bits decoded (ie 64GB per node, 512GB per system)
    // 2G Max per chip select
    // 8 chip selects per node 128GB per system
    // 16GB per node max

    printf("Node  CS   Base       Mask       Enable  Type\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt physhi = 0xC240 | (n << 11);
	uInt bankmap = pci_config_read(physhi + 0x40);

	if (!nodepresent[n])
	    continue;

	for (i = 0; i < 8; i++)
	{
	    uInt base = pci_config_read(physhi + i * 4);
	    uInt mask = pci_config_read(physhi + i * 4 + 0x20);
	    int map = (bankmap >> ((i >> 1) * 4)) & 0x7;
	    char *type = NULL;

	    switch (map)
	    {
	    case 0:
		type = "32MB (R12C8)";
		break;
	    case 1:
		type = "64MB (R12C9)";
		break;
	    case 2:
		type = "128MB (R13C9|R12C10)";
		break;
	    case 3:
		type = "256MB (R13C10|R12C11)";
		break;
	    case 4:
		type = "512MB (R13C11|R14C10)";
		break;
	    case 5:
		type = "1GB (R14C11|R13C12)";
		break;
	    case 6:
		type = "2GB (R14C12)";
		break;
	    case 7:
		type = "reserved";
		break;
	    }

	    printf("N%d    CS%d  %09qX  %09qX  %d       %s\n", n, i, 
		    (uLong)(base & 0xFFE0FE00) << 4,
		    (uLong)(mask & 0x3FE0FE00) << 4, base & 1, type);
	}
    }

    printf("\n");
}

void
show_dramtiming()
{
    int n;

    printf("Node    Clk  Twr  Trp  Tras  Trrd  Trcd  Trfc  Trc  Tcl  Twcl  Trwt  Twtr  Tref\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt physhi = 0xC280 | (n << 11);
	uInt timelow = pci_config_read(physhi + 0x8);
	uInt timehigh = pci_config_read(physhi + 0xC);
	uInt confhigh = pci_config_read(physhi + 0x14);
	char *clk = NULL;
	int clkf = 0;
	int twr;
	char *trp = NULL;
	char *tras = NULL;
	char *trrd = NULL;
	char *trcd = NULL;
	int trfc;
	int trc;
	char *tcl = NULL;
	char *twcl = NULL;
	char *trwt = NULL;
	int twtr;
	char *tref = NULL;

	if (!nodepresent[n])
	    continue;

	switch ((confhigh >> 20) & 7)
	{
	case 0: clk = "100MHz"; clkf = 100; break;
	case 1: clk = "(1)"; clkf = 0; break;
	case 2: clk = "133MHz"; clkf = 133; break;
	case 3: clk = "(3)"; clkf = 0; break;
	case 4: clk = "(4)"; clkf = 0; break;
	case 5: clk = "166MHz"; clkf = 166; break;
	case 6: clk = "(6)"; clkf = 0; break;
	case 7: clk = "200MHz"; clkf = 200; break;
	}

	switch (timelow & 7)
	{
	case 0: tcl = "(0)"; break;
	case 1: tcl = "2"; break;
	case 2: tcl = "3"; break;
	case 3: tcl = "(3)"; break;
	case 4: tcl = "(4)"; break;
	case 5: tcl = "2.5"; break;
	case 6: tcl = "(6)"; break;
	case 7: tcl = "(7)"; break;
	}

	trc = ((timelow >> 4) & 0xF) + 7;
	trfc = ((timelow >> 8) & 0xF) + 9;

	switch ((timelow >> 12) & 7)
	{
	case 0: trcd = "(0)"; break;
	case 1: trcd = "(1)"; break;
	case 2: trcd = "2"; break;
	case 3: trcd = "3"; break;
	case 4: trcd = "4"; break;
	case 5: trcd = "5"; break;
	case 6: trcd = "6"; break;
	case 7: trcd = "(7)"; break;
	}

	switch ((timelow >> 16) & 7)
	{
	case 0: trrd = "(0)"; break;
	case 1: trrd = "(1)"; break;
	case 2: trrd = "2"; break;
	case 3: trrd = "3"; break;
	case 4: trrd = "4"; break;
	case 5: trrd = "(5)"; break;
	case 6: trrd = "(6)"; break;
	case 7: trrd = "(7)"; break;
	}

	switch ((timelow >> 20) & 0xF)
	{
	case 0: tras = "(0)"; break;
	case 1: tras = "(1)"; break;
	case 2: tras = "(2)"; break;
	case 3: tras = "(3)"; break;
	case 4: tras = "(4)"; break;
	case 5: tras = "5"; break;
	case 6: tras = "6"; break;
	case 7: tras = "7"; break;
	case 8: tras = "8"; break;
	case 9: tras = "9"; break;
	case 10: tras = "10"; break;
	case 11: tras = "11"; break;
	case 12: tras = "12"; break;
	case 13: tras = "13"; break;
	case 14: tras = "14"; break;
	case 15: tras = "15"; break;
	}

	switch ((timelow >> 24) & 7)
	{
	case 0: trp = "(0)"; break;
	case 1: trp = "(1)"; break;
	case 2: trp = "2"; break;
	case 3: trp = "3"; break;
	case 4: trp = "4"; break;
	case 5: trp = "5"; break;
	case 6: trp = "6"; break;
	case 7: trp = "(7)"; break;
	}

	twr = ((timelow >> 28) & 1) + 2;
	twtr = (timehigh & 1) + 1;

	switch ((timehigh >> 4) & 7)
	{
	case 0: trwt = "1"; break;
	case 1: trwt = "2"; break;
	case 2: trwt = "3"; break;
	case 3: trwt = "4"; break;
	case 4: trwt = "5"; break;
	case 5: trwt = "6"; break;
	case 6: trwt = "(6)"; break;
	case 7: trwt = "(7)"; break;
	}

	switch ((timehigh >> 8) & 0x1F)
	{
	case 0: if (clkf == 100) tref = "4K"; else tref = "100MHz 4K"; break;
	case 1: if (clkf == 133) tref = "4K"; else tref = "133MHz 4K"; break;
	case 2: if (clkf == 166) tref = "4K"; else tref = "166MHz 4K"; break;
	case 3: if (clkf == 200) tref = "4K"; else tref = "200MHz 4K"; break;
	case 4: tref = "(4)"; break;
	case 5: tref = "(5)"; break;
	case 6: tref = "(6)"; break;
	case 7: tref = "(7)"; break;
	case 8: if (clkf == 100) tref = "8/16K"; else tref = "100MHz 8/16K"; break;
	case 9: if (clkf == 133) tref = "8/16K"; else tref = "133MHz 8/16K"; break;
	case 10: if (clkf == 166) tref = "8/16K"; else tref = "166MHz 8/16K"; break;
	case 11: if (clkf == 200) tref = "8/16K"; else tref = "200MHz 8/16K"; break;
	case 12: tref = "(12)"; break;
	case 13: tref = "(13)"; break;
	case 14: tref = "(14)"; break;
	case 15: tref = "(15)"; break;
	case 16: tref = "(16)"; break;
	case 17: tref = "(17)"; break;
	case 18: tref = "(18)"; break;
	case 19: tref = "(19)"; break;
	case 20: tref = "(20)"; break;
	case 21: tref = "(21)"; break;
	case 22: tref = "(22)"; break;
	case 23: tref = "(23)"; break;
	case 24: tref = "(24)"; break;
	case 25: tref = "(25)"; break;
	case 26: tref = "(26)"; break;
	case 27: tref = "(27)"; break;
	case 28: tref = "(28)"; break;
	case 29: tref = "(29)"; break;
	case 30: tref = "(30)"; break;
	case 31: tref = "(31)"; break;
	}

	switch ((timehigh >> 20) & 7)
	{
	case 0: twcl = "1"; break;
	case 1: twcl = "2"; break;
	case 2: twcl = "(2)"; break;
	case 3: twcl = "(3)"; break;
	case 4: twcl = "(4)"; break;
	case 5: twcl = "(5)"; break;
	case 6: twcl = "(6)"; break;
	case 7: twcl = "(7)"; break;
	}

	printf("N%d   %6s    %d  %3s   %3s   %3s   %3s   %3d  %3d  %3s   %3s   %3s     %d  %4s\n", n,
		clk, twr, trp, tras, trrd, trcd, trfc, trc, tcl, twcl, trwt, twtr, tref);

    }

    printf("\n");

    printf("Node  Width ECC DLL Drv QFC DQS Clr SRS QByp Buf 32b  x4 Rcv Byp Asyn Pre Idle\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt physhi = 0xC280 | (n << 11);
	uInt conflow = pci_config_read(physhi + 0x10);
	uInt confhigh = pci_config_read(physhi + 0x14);
	int width;
	char *ecc;
	char *dll;
	char *drv;
	char *qfc;
	char *dqs;
	char *clr;
	char *srs;
	int qbyp;
	char *buf;
	char *b32;
	int x4;
	char *rcv;
	int byp;
	int asyn;
	char *pre = NULL;
	int idle;

	if (!nodepresent[n])
	    continue;

	width = (conflow & 0x10000) ? 128 : 64;
	ecc = (conflow & 0x20000) ? "Yes" : "No";
	dll = (conflow & 0x1) ? "Dis" : "Enb";
	drv = (conflow & 0x2) ? "Wk" : "Nrm";
	qfc = (conflow & 0x4) ? "Enb" : "Dis";
	dqs = (conflow & 0x8) ? "Dis" : "Enb";
	clr = g_stepping >= STEPPING_C0 ? ((conflow & 0x800) ? "Cmp" : "Inc") : "-";
	srs = (conflow & 0x2000) ? "Act" : "Off";
	qbyp = 1 << (((conflow >> 14) & 3) + 1);
	buf = (conflow & 0x40000) ? "No" : "Yes";
	b32 = (conflow & 0x80000) ? (width == 64 ? "32/4" : "64/4") : (width == 64 ? "64/8" : "64/4");
	x4 = (conflow >> 20) & 0xF;
	rcv = (conflow & 0x1000000) ? "Dis" : "Enb";
	byp = (conflow >> 25) & 7;
	asyn = confhigh & 0xF;

	switch ((confhigh >> 4) & 0xF)
	{
	case 0: pre = "2.0"; break;
	case 1: pre = "2.5"; break;
	case 2: pre = "3.0"; break;
	case 3: pre = "3.5"; break;
	case 4: pre = "4.0"; break;
	case 5: pre = "4.5"; break;
	case 6: pre = "5.0"; break;
	case 7: pre = "5.5"; break;
	case 8: pre = "6.0"; break;
	case 9: pre = "6.5"; break;
	case 10: pre = "7.0"; break;
	case 11: pre = "7.5"; break;
	case 12: pre = "8.0"; break;
	case 13: pre = "8.5"; break;
	case 14: pre = "9.0"; break;
	case 15: pre = "9.5"; break;
	}

	idle = 1 << (((confhigh >> 16) & 7) + 1);

	if (idle == 2)
	    idle = 0;

#if 0
Bits Mnemonic Function R/W Reset
31–30 reserved R 0
29 MC3_EN Memory Clock 3 Enabled R/W 0
28 MC2_EN Memory Clock 2 Enabled R/W 0
27 MC1_EN Memory Clock 1 Enabled R/W 0
26 MC0_EN Memory Clock 0 Enabled R/W 0
25 MCR Memory Clock Ratio Valid R/W 0
24–23 reserved R 0
22–20 MemClk DRAM MEMCLK Frequency R/W 0
19 DCC_EN Dynamic Idle Cycle Center Enable R/W 0
18–16 ILD_lmt Idle Cycle Limit R/W 0
15–12 reserved R 0
11–8 RdPreamble Read Preamble R/W 0
7–4 reserved R 0
3–0 AsyncLat Maximum Asynchronous Latency R/W 0
#endif

#if 0
Bits Mnemonic Function R/W Reset
31–26 reserved R 0
25 AdjFaster Adjust Faster R/W 0
24 AdjSlower Adjust Slower R/W 0
23–16 Adj Delay Line Adjust R/W 0
15–0 reserved R 0
#endif

//Node  Width ECC DLL Drv QFC DQS Clr SRS QByp Buf 32b  x4 Rcv Byp Asyn Pre Idle
//================================================================================
//N0      128 Yes Enb Nrm Dis Dis Cmp Act   16 Yes 32/4  F Enb   7   15 5.0 256

	printf("N%d      %3d %3s %3s %3s %3s %3s %3s %3s   %2d %3s %4s  %X %3s   %d   %2d %3s  %3d\n", n, width, ecc, dll, drv, qfc, dqs, clr, srs, qbyp, buf, b32, x4, rcv, byp, asyn, pre, idle);
    }

    printf("\n");

    printf("Node  DynIdle  ClkRatio  Clk0  Clk1  Clk2  Clk3  DLAdj  DLDir\n");
    printf("================================================================================\n");

    for (n = 0; n < MAX_NODES; n++)
    {
	uInt physhi = 0xC280 | (n << 11);
	uInt confhigh = pci_config_read(physhi + 0x14);
	uInt delayline = pci_config_read(physhi + 0x18);
	char *dynidle;
	char *clkratio;
	char *clk0;
	char *clk1;
	char *clk2;
	char *clk3;
	int dladj;
	char *dldir = NULL;

	if (!nodepresent[n])
	    continue;

	dynidle = (confhigh & 0x80000) ? "Enb" : "Dis";
	clkratio = (confhigh & 0x2000000) ? "Valid" : "Not Yet";
	clk0 = (confhigh & 0x4000000) ? "Enb" : "Dis";
	clk1 = (confhigh & 0x8000000) ? "Enb" : "Dis";
	clk2 = (confhigh & 0x10000000) ? "Enb" : "Dis";
	clk3 = (confhigh & 0x20000000) ? "Enb" : "Dis";
	dladj = (delayline >> 16) & 0xFF;

	switch ((delayline >> 24) & 3)
	{
	case 0: dldir = "None"; break;
	case 1: dldir = "Slow"; break;
	case 2: dldir = "Fast"; break;
	case 3: dldir = "Undef"; break;
	}

/*
Node  DynIdle  ClkRatio  Clk0  Clk1  Clk2  Clk3  DLAdj  DLDir
================================================================================
N0        Enb     Valid   Enb   Enb   Dis   Dis    256  Undef          
*/
	printf("N%d        %3s   %7s   %3s   %3s   %3s   %3s    %3d  %5s\n", n, dynidle, clkratio, clk0, clk1, clk2, clk3, dladj, dldir);
    }

    printf("\n");
}

struct dimm_info
{
    int ctltype;
    uInt physhi;
    uByte devaddr;
    char node;
    char fbank;		/* Front side bank */
    char rbank;		/* Read side bank */
};

#ifndef STANDALONE
int g_amd8111_device_id = 5;

volatile unsigned int CSR_BASE;

#define SMB_BASE		CSR_BASE	/* XXX */
#define SMB_STATUS		(SMB_BASE + 0xE0)
#define SMB_CONTROL		(SMB_BASE + 0xE2)
#define SMB_ADDR		(SMB_BASE + 0xE4)
#define SMB_DATA		(SMB_BASE + 0xE6)
#define SMB_CMD			(SMB_BASE + 0xE8) /* byte */
#define SMB_FIFO		(SMB_BASE + 0xE9) /* byte - for blk cmds */

/* PME0 */
#define SMB_STAT_SMB_BUSY	0x0800	/* RO */
#define SMB_STAT_HOST_BUSY	0x0008	/* RO */
#define SMB_STAT_HOST_DONE	0x0010	/* write to clear */
#define SMB_STAT_ERROR		0x0027	/* write to clear */

/* PME2 */
#define SMB_CTRL_START		0x0008
#define SMB_CTRL_CYC_BYTE	0x0002
#define SMB_CTRL_CYC_WORD	0x0003
#define SMB_CTRL_CYC_BLOCK	0x0005

/* PME3 */
#define SMB_ADDR_READ_CYCLE	0x0001	/* read==high write==low */
#define SMB_ADDR_WRITE_CYCLE	0x0000	/* read==high write==low */

#define SMB_WRITE_BYTE	0x06
#define SMB_READ_BYTE	0x07
#define SMB_WRITE_BLOCK	0x0A
#define SMB_READ_BLOCK	0x0B


uShort
inw(uInt addr)
{
    uShort val;

    __asm __volatile("inw %%dx,%0" : "=a" (val) : "d" (addr));
    return val;
}

void
outw(uInt addr, uShort val)
{
    uShort v;

    v = val;
    __asm __volatile("outw %0,%%dx" : : "a" (v), "d" (addr));
}

static int
inb(int addr)
{
    uByte val;

    __asm __volatile("inb %%dx,%0" : "=a" (val) : "d" (addr));
    return val;
}

static void
outb(int addr, int val)
{
    uByte v;

    v = val;
    __asm __volatile("outb %0,%%dx" : : "a" (v), "d" (addr));
}

static int
wait(void)
{
    uShort status = inw(SMB_STATUS);

    while (status  & (SMB_STAT_SMB_BUSY | SMB_STAT_HOST_BUSY))
	status = inw(SMB_STATUS);

    while ((status & (SMB_STAT_HOST_DONE | SMB_STAT_ERROR)) == 0)
	status = inw(SMB_STATUS);

    if (status & SMB_STAT_HOST_DONE)
	outw(SMB_STATUS, SMB_STAT_HOST_DONE);

    if (status & SMB_STAT_ERROR)
    {
	outw(SMB_STATUS, SMB_STAT_ERROR);
	return 0;
    }

    return 1;
}

int
smb_read_byte(unsigned char addr, unsigned char cmd, int *data)
{
    outw(SMB_ADDR, (addr << 1) | SMB_ADDR_READ_CYCLE);
    outb(SMB_CMD, cmd);
    outw(SMB_CONTROL, SMB_CTRL_CYC_BYTE);
    outw(SMB_CONTROL, SMB_CTRL_CYC_BYTE | SMB_CTRL_START);

    if (!wait())
	return 0;

    *data = inw(SMB_DATA);
    return 1;
}

uByte
seeprom_read0(uInt physhi, uByte dev, uByte addr)
{
    int data;

    CSR_BASE = pci_config_read(physhi | 0x58);
    CSR_BASE &= 0xFF00;

    if (!smb_read_byte(0x50 | dev, addr, &data))
	return 0xFF;

    return data;
}
#else
uByte
seeprom_read0(uInt physhi, uByte dev, uByte addr)
{
    if (dev == 0)
	return dimm0[addr];

    if (dev == 1)
	return dimm1[addr];

    return 0xFF;
}
#endif

typedef uByte (*seepromreadfunc)(uInt physhi, uByte dev, uByte addr);

seepromreadfunc seepromfuncs[] =
{
    NULL,
    seeprom_read0,
};

#define SEEPROM_TYPE1		1

struct dimm_info dimms[] =
{
    { SEEPROM_TYPE1, 0x002B00, 0, 0, 0, 1 },
    { SEEPROM_TYPE1, 0x002B00, 1, 0, 2, 3 },
};

#if 0
void
show_dimms()
{
    int i;
/*
DIMM   Node  FBank  RBank Addr   Type  Banks  CAS  ....
================================================================================
*/
    printf("DIMM   Node  FBank  RBank Addr   Type  Rev  Banks  Width  CAS  ....\n");
    printf("================================================================================\n");

    for (i = 0; i < sizeof dimms / sizeof dimms[0]; i++)
    {
	int ctltype = dimms[i].ctltype;
	seepromreadfunc readfunc;
	uInt physhi;
	uByte devaddr;
	uByte dramtype;
	uByte rev;
	uByte banks;
	uShort width;
	uByte cfg;

	printf("DIMM%d  N%d    %5d  %5d %4d", i, dimms[i].node,
		dimms[i].fbank, dimms[i].rbank, dimms[i].devaddr);

	if (ctltype < 0 ||
		ctltype >= sizeof seepromfuncs / sizeof seepromfuncs[0] ||
		seepromfuncs[ctltype] == NULL)
	{
	    printf("\n");
	    continue;
	}

	readfunc = seepromfuncs[ctltype];
	physhi = dimms[i].physhi;
	devaddr = dimms[i].devaddr;

	dramtype = (*readfunc)(physhi, devaddr, 2);

	if (dramtype != 7)
	{
	    printf("   (%02X)\n", dramtype);
	    continue;
	}

	printf("   DDR ");

	rev = (*readfunc)(physhi, devaddr, 62);
	printf("   %02X", rev);

	banks = (*readfunc)(physhi, devaddr, 5);
	printf("  %5d", banks);

	width = (*readfunc)(physhi, devaddr, 6);
	width |= (*readfunc)(physhi, devaddr, 7) << 8;
	printf("  %5d", width);

	cfg = (*readfunc)(physhi, devaddr, 11);

	switch (cfg)
	{
	case 0: printf("  None  "); break;
	case 1: printf("  Parity"); break;
	case 2: printf("  ECC   "); break;
	default: printf("  (%02X)  ", cfg); break;
	}

	printf("\n");

	{
	    uByte tmp;
	    tmp = (*readfunc)(physhi, devaddr, 1);
	    printf("\tSPD Size = %d\n", 1 << tmp);
	    tmp = (*readfunc)(physhi, devaddr, 0);
	    printf("\tSPD User = %d\n", tmp);
	    tmp = (*readfunc)(physhi, devaddr, 3);
	    printf("\tRow Addresses (Bank 1) = %d\n", tmp & 0xF);
	    printf("\tRow Addresses (Bank 2) = %d\n", (tmp >> 4) & 0xF);
	    tmp = (*readfunc)(physhi, devaddr, 4);
	    printf("\tColumn Addresses (Bank 1) = %d\n", tmp & 0xF);
	    printf("\tColumn Addresses (Bank 2) = %d\n", (tmp >> 4) & 0xF);
	    tmp = (*readfunc)(physhi, devaddr, 8);

	    switch (tmp)
	    {
	    case 0: printf("\tVoltage Level = TTL\n"); break;
	    case 1: printf("\tVoltage Level = LVTTL\n"); break;
	    case 2: printf("\tVoltage Level = HSTL 1.5\n"); break;
	    case 3: printf("\tVoltage Level = SSTL 3.3\n"); break;
	    case 4: printf("\tVoltage Level = SSTL 2.5\n"); break;
	    default: printf("\tVoltage Level = (%02X)\n", tmp); break;
	    }

	    tmp = (*readfunc)(physhi, devaddr, 9);
	    printf("\ttCKmin = %d.%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);
	    tmp = (*readfunc)(physhi, devaddr, 10);
	    printf("\ttAC = .%d%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);

	    tmp = (*readfunc)(physhi, devaddr, 12);

	    switch (tmp & 0x7F)
	    {
	    case 0: printf("\tRefresh Rate = Normal (1x) 15.625us"); break;
	    case 1: printf("\tRefresh Rate = Reduced (.25x) 3.906us"); break;
	    case 2: printf("\tRefresh Rate = Reduced (.5x) 7.813us"); break;
	    case 3: printf("\tRefresh Rate = Extended (2x) 31.25us"); break;
	    case 4: printf("\tRefresh Rate = Extended (4x) 62.5us"); break;
	    case 5: printf("\tRefresh Rate = Extended (8x) 125us"); break;
	    default: printf("\tRefresh Rate = (%02X)", tmp & 0x7F); break;
	    }

	    printf("%s\n", tmp & 0x80 ? "   (Self Refresh)" : "");

	    tmp = (*readfunc)(physhi, devaddr, 13);
	    printf("\tPrimary SDRAM width (bank 1) = %d\n", tmp & 0x7F); 
	    printf("\tPrimary SDRAM width (bank 2) = %d\n", (tmp & 0x7F) * (tmp & 0x80 ? 2 : 1)); 

	    tmp = (*readfunc)(physhi, devaddr, 14);
	    printf("\tError Checking SDRAM width (bank 1) = %d\n", tmp & 0x7F); 
	    printf("\tError Checking SDRAM width (bank 2) = %d\n", (tmp & 0x7F) * (tmp & 0x80 ? 2 : 1)); 

	    tmp = (*readfunc)(physhi, devaddr, 15);
	    printf("\tMinimum Clock Delay, Back-to-Back Random Column Access = %d\n", tmp);

	    tmp = (*readfunc)(physhi, devaddr, 16);
	    printf("\tBurst Lengths = (%02X)%s%s%s%s%s\n", tmp,
		    tmp & 0x80 ? " Page" : "", tmp & 0x08 ? " 8" : "",
		    tmp & 0x04 ? " 4" : "", tmp & 0x02 ? " 2" : "",
		    tmp & 0x01 ? " 1" : "");

	    tmp = (*readfunc)(physhi, devaddr, 17);
	    printf("\tNumber of Banks on SDRAM Device = %d\n", tmp);

	    tmp = (*readfunc)(physhi, devaddr, 18);
	    printf("\tCAS Latency = (%02X)%s%s%s%s%s%s\n", tmp,
		    tmp & 0x20 ? " 3.5" : "", tmp & 0x10 ? " 3" : "",
		    tmp & 0x08 ? " 2.5" : "", tmp & 0x04 ? " 2" : "",
		    tmp & 0x02 ? " 1.5" : "", tmp & 0x01 ? " 1" : "");

	    tmp = (*readfunc)(physhi, devaddr, 19);
	    printf("\tCS Latency = (%02X)%s%s%s%s%s%s%s\n", tmp,
		    tmp & 0x40 ? " 6" : "", tmp & 0x20 ? " 5" : "",
		    tmp & 0x10 ? " 4" : "", tmp & 0x08 ? " 3" : "",
		    tmp & 0x04 ? " 2" : "", tmp & 0x02 ? " 1" : "",
		    tmp & 0x01 ? " 0" : "");

	    tmp = (*readfunc)(physhi, devaddr, 20);
	    printf("\tWrite Latency = (%02X)%s%s%s%s\n", tmp,
		    tmp & 0x08 ? " 3" : "", tmp & 0x04 ? " 2" : "",
		    tmp & 0x02 ? " 1" : "", tmp & 0x01 ? " 0" : "");

	    tmp = (*readfunc)(physhi, devaddr, 21);
	    printf("\tSDRAM Module Attributes = (%02X)%s%s%s%s%s%s\n", tmp,
		    tmp & 0x20 ? " DiffClkIn" : "", tmp & 0x10 ? " FETExEnb" : "",
		    tmp & 0x08 ? " FETEnb" : "", tmp & 0x04 ? " PLL" : "",
		    tmp & 0x02 ? " Reg" : "", tmp & 0x01 ? " Buf" : "");

	    tmp = (*readfunc)(physhi, devaddr, 22);
	    printf("\tSDRAM Device Attributes = (%02X)%s%s%s%s%s\n", tmp,
		    tmp & 0x80 ? " FastAP" : "", tmp & 0x40 ? " AutoPre" : "",
		    tmp & 0x20 ? " UppVDDtol=0.2V" : "", tmp & 0x10 ? " LowVDDtol=0.2" : "",
		    tmp & 0x01 ? " WeakDrv" : "");

	    tmp = (*readfunc)(physhi, devaddr, 23);
	    printf("\ttCKmin @CL-.5 = %d.%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);
	    tmp = (*readfunc)(physhi, devaddr, 24);
	    printf("\ttAC @CL-.5 = .%d%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);

	    tmp = (*readfunc)(physhi, devaddr, 25);
	    printf("\ttCKmin @CL-1 = %d.%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);
	    tmp = (*readfunc)(physhi, devaddr, 26);
	    printf("\ttAC @CL-1 = .%d%dns\n", (tmp >> 4) & 0xF, tmp & 0xF);

	    tmp = (*readfunc)(physhi, devaddr, 27);
	    printf("\ttRP = %d.%dns\n", (tmp >> 2) & 0x3F, tmp & 0x3 ? (100 / (tmp & 0x3)) : 0);

	    tmp = (*readfunc)(physhi, devaddr, 28);
	    printf("\ttRRD = %d.%dns\n", (tmp >> 2) & 0x3F, tmp & 0x3 ? (100 / (tmp & 0x3)) : 0);

	    tmp = (*readfunc)(physhi, devaddr, 29);
	    printf("\ttRCD = %d.%dns\n", (tmp >> 2) & 0x3F, tmp & 0x3 ? (100 / (tmp & 0x3)) : 0);

	    tmp = (*readfunc)(physhi, devaddr, 30);
	    printf("\ttRAS = %dns\n", tmp);

	    tmp = (*readfunc)(physhi, devaddr, 31);
	    printf("\tModule Bank Density = (%02X)%s%s%s%s%s%s%s%s\n", tmp,
		    tmp & 0x80 ? " 512MB" : "", tmp & 0x40 ? " 256MB" : "",
		    tmp & 0x20 ? " 128MB" : "", tmp & 0x10 ? " 64MB" : "",
		    tmp & 0x08 ? " 32MB" : "", tmp & 0x04 ? " 16MB" : "",
		    tmp & 0x02 ? " 2GB" : "", tmp & 0x01 ? " 1GB" : "");
	}
    }

    printf("\n");
}
#endif

uByte
spd_byte(int i, int off)
{
    int ctltype;
    seepromreadfunc readfunc;
    uInt physhi;
    uByte devaddr;

    if (i < 0 || i >= sizeof dimms / sizeof dimms[0])
	return 0xFF;

    ctltype = dimms[i].ctltype;

    if (ctltype < 0 ||
	    ctltype >= sizeof seepromfuncs / sizeof seepromfuncs[0] ||
	    seepromfuncs[ctltype] == NULL)
	return 0xFF;

    readfunc = seepromfuncs[ctltype];
    physhi = dimms[i].physhi;
    devaddr = dimms[i].devaddr;

    return (*readfunc)(physhi, devaddr, off);
}

int
spd_count()
{
    return sizeof dimms / sizeof dimms[0];
}

char *
spd_name(int n)
{
    static char names[2][20];
    static char set[2] = { 0, 0 };
    int i;

    if (n >= 0 && n < 2 && set[n])
	return names[n];

    /* read the part number out of the DIMM */
    for (i = 0; i < 18; i++)
	names[n][i] = spd_byte(n, 73 + i);

    while (i > 0 && names[n][i - 1] == ' ')
	i--;

    names[n][i] = '\0';
    return names[n];
}

/* Code for dumping SPD Serial EEPROMs */
int
strcmp(char const *s1, char const *s2)
{
    while (*s1 == *s2 && *s1)
    {
	s1++;
	s2++;
    }

    return *s1 - *s2;
}

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
    char *x = NULL;

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
    int buf[4];
    int i;

    if (dimms == NULL)
    {
	num = spd_count();
	dimms = buf;

	if (num > 4)
	    num = 4;

	for (i = 0; i < num; i++)
	    dimms[i] = i;

//	qsort(dimms, num, sizeof (int), dimmcmp);
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

void
show_dimms()
{
    dump_spd(NULL, -1);
}

#ifndef STANDALONE
uLong
rdmsr(int num)
{
    uInt valhi;
    uInt vallo;
    __asm __volatile("rdmsr" : "=d" (valhi), "=a" (vallo) : "c" (num));
    return ((uLong)valhi << 32) | vallo;
}

void
cpuid(int num, uInt *eaxp, uInt *ebxp, uInt *ecxp, uInt *edxp)
{
    uInt eax;
    uInt ebx;
    uInt ecx;
    uInt edx;
    __asm __volatile("cpuid" : "=d" (edx), "=c" (ecx), "=b" (ebx), "=a" (eax) : "a" (num));

    *eaxp = eax;
    *ebxp = ebx;
    *ecxp = ecx;
    *edxp = edx;
}
#else
uLong
rdmsr(int num)
{
    int i;

    for (i = 0; i + 2 < sizeof msrs / sizeof msrs[0]; i += 3)
	if (msrs[i] == num)
	    return (((uLong)msrs[i + 2]) << 32) | msrs[i + 1];

    printf("\nrdmsr(0x%X)\n", num);
    return 0xFFFFFFFFFFFFFFFFULL;
}

void
cpuid(int num, uInt *eaxp, uInt *ebxp, uInt *ecxp, uInt *edxp)
{
    int i;

    for (i = 0; i + 4 < sizeof cpuids / sizeof cpuids[0]; i += 5)
	if (cpuids[i] == num)
	{
	    *eaxp = cpuids[i + 1];
	    *ebxp = cpuids[i + 2];
	    *ecxp = cpuids[i + 3];
	    *edxp = cpuids[i + 4];
	    return;
	}

    printf("\ncpuid(0x%X)\n", num);
    *eaxp = 0xFFFFFFFF;
    *ebxp = 0xFFFFFFFF;
    *ecxp = 0xFFFFFFFF;
    *edxp = 0xFFFFFFFF;
}
#endif

struct fixed_mtrr_info
{
    uInt msr;
    int shift;
    uInt mask;
    uInt base;
    uInt size;
};

struct fixed_mtrr_info fixed_mtrrs[] =
{
    { 0x250,  0, 0xFF, 0x00000, 0x10000 },
    { 0x250,  8, 0xFF, 0x10000, 0x10000 },
    { 0x250, 16, 0xFF, 0x20000, 0x10000 },
    { 0x250, 24, 0xFF, 0x30000, 0x10000 },
    { 0x250, 32, 0xFF, 0x40000, 0x10000 },
    { 0x250, 40, 0xFF, 0x50000, 0x10000 },
    { 0x250, 48, 0xFF, 0x60000, 0x10000 },
    { 0x250, 56, 0xFF, 0x70000, 0x10000 },
    { 0x258,  0, 0xFF, 0x80000, 0x04000 },
    { 0x258,  8, 0xFF, 0x84000, 0x04000 },
    { 0x258, 16, 0xFF, 0x88000, 0x04000 },
    { 0x258, 24, 0xFF, 0x8C000, 0x04000 },
    { 0x258, 32, 0xFF, 0x90000, 0x04000 },
    { 0x258, 40, 0xFF, 0x94000, 0x04000 },
    { 0x258, 48, 0xFF, 0x98000, 0x04000 },
    { 0x258, 56, 0xFF, 0x9C000, 0x04000 },
    { 0x259,  0, 0xFF, 0xA0000, 0x04000 },
    { 0x259,  8, 0xFF, 0xA4000, 0x04000 },
    { 0x259, 16, 0xFF, 0xA8000, 0x04000 },
    { 0x259, 24, 0xFF, 0xAC000, 0x04000 },
    { 0x259, 32, 0xFF, 0xB0000, 0x04000 },
    { 0x259, 40, 0xFF, 0xB4000, 0x04000 },
    { 0x259, 48, 0xFF, 0xB8000, 0x04000 },
    { 0x259, 56, 0xFF, 0xBC000, 0x04000 },

    { 0x268,  0, 0xFF, 0xC0000, 0x01000 },
    { 0x268,  8, 0xFF, 0xC1000, 0x01000 },
    { 0x268, 16, 0xFF, 0xC2000, 0x01000 },
    { 0x268, 24, 0xFF, 0xC3000, 0x01000 },
    { 0x268, 32, 0xFF, 0xC4000, 0x01000 },
    { 0x268, 40, 0xFF, 0xC5000, 0x01000 },
    { 0x268, 48, 0xFF, 0xC6000, 0x01000 },
    { 0x268, 56, 0xFF, 0xC7000, 0x01000 },

    { 0x269,  0, 0xFF, 0xC8000, 0x01000 },
    { 0x269,  8, 0xFF, 0xC9000, 0x01000 },
    { 0x269, 16, 0xFF, 0xCA000, 0x01000 },
    { 0x269, 24, 0xFF, 0xCB000, 0x01000 },
    { 0x269, 32, 0xFF, 0xCC000, 0x01000 },
    { 0x269, 40, 0xFF, 0xCD000, 0x01000 },
    { 0x269, 48, 0xFF, 0xCE000, 0x01000 },
    { 0x269, 56, 0xFF, 0xCF000, 0x01000 },

    { 0x26A,  0, 0xFF, 0xD0000, 0x01000 },
    { 0x26A,  8, 0xFF, 0xD1000, 0x01000 },
    { 0x26A, 16, 0xFF, 0xD2000, 0x01000 },
    { 0x26A, 24, 0xFF, 0xD3000, 0x01000 },
    { 0x26A, 32, 0xFF, 0xD4000, 0x01000 },
    { 0x26A, 40, 0xFF, 0xD5000, 0x01000 },
    { 0x26A, 48, 0xFF, 0xD6000, 0x01000 },
    { 0x26A, 56, 0xFF, 0xD7000, 0x01000 },

    { 0x26B,  0, 0xFF, 0xD8000, 0x01000 },
    { 0x26B,  8, 0xFF, 0xD9000, 0x01000 },
    { 0x26B, 16, 0xFF, 0xDA000, 0x01000 },
    { 0x26B, 24, 0xFF, 0xDB000, 0x01000 },
    { 0x26B, 32, 0xFF, 0xDC000, 0x01000 },
    { 0x26B, 40, 0xFF, 0xDD000, 0x01000 },
    { 0x26B, 48, 0xFF, 0xDE000, 0x01000 },
    { 0x26B, 56, 0xFF, 0xDF000, 0x01000 },

    { 0x26C,  0, 0xFF, 0xE0000, 0x01000 },
    { 0x26C,  8, 0xFF, 0xE1000, 0x01000 },
    { 0x26C, 16, 0xFF, 0xE2000, 0x01000 },
    { 0x26C, 24, 0xFF, 0xE3000, 0x01000 },
    { 0x26C, 32, 0xFF, 0xE4000, 0x01000 },
    { 0x26C, 40, 0xFF, 0xE5000, 0x01000 },
    { 0x26C, 48, 0xFF, 0xE6000, 0x01000 },
    { 0x26C, 56, 0xFF, 0xE7000, 0x01000 },

    { 0x26D,  0, 0xFF, 0xE8000, 0x01000 },
    { 0x26D,  8, 0xFF, 0xE9000, 0x01000 },
    { 0x26D, 16, 0xFF, 0xEA000, 0x01000 },
    { 0x26D, 24, 0xFF, 0xEB000, 0x01000 },
    { 0x26D, 32, 0xFF, 0xEC000, 0x01000 },
    { 0x26D, 40, 0xFF, 0xED000, 0x01000 },
    { 0x26D, 48, 0xFF, 0xEE000, 0x01000 },
    { 0x26D, 56, 0xFF, 0xEF000, 0x01000 },

    { 0x26E,  0, 0xFF, 0xF0000, 0x01000 },
    { 0x26E,  8, 0xFF, 0xF1000, 0x01000 },
    { 0x26E, 16, 0xFF, 0xF2000, 0x01000 },
    { 0x26E, 24, 0xFF, 0xF3000, 0x01000 },
    { 0x26E, 32, 0xFF, 0xF4000, 0x01000 },
    { 0x26E, 40, 0xFF, 0xF5000, 0x01000 },
    { 0x26E, 48, 0xFF, 0xF6000, 0x01000 },
    { 0x26E, 56, 0xFF, 0xF7000, 0x01000 },

    { 0x26F,  0, 0xFF, 0xF8000, 0x01000 },
    { 0x26F,  8, 0xFF, 0xF9000, 0x01000 },
    { 0x26F, 16, 0xFF, 0xFA000, 0x01000 },
    { 0x26F, 24, 0xFF, 0xFB000, 0x01000 },
    { 0x26F, 32, 0xFF, 0xFC000, 0x01000 },
    { 0x26F, 40, 0xFF, 0xFD000, 0x01000 },
    { 0x26F, 48, 0xFF, 0xFE000, 0x01000 },
    { 0x26F, 56, 0xFF, 0xFF000, 0x01000 },
};

char *
get_typename(int type)
{
    static char buf[20];

    switch (type)
    {
    case 0:
	return "UC";
    case 1:
	return "WC";
    case 4:
	return "WT";
    case 5:
	return "WP";
    case 6:
	return "WB";
    }

    bprintf(buf, "(%02X)", type);
    return buf;
}

void
show_mtrrs()
{
    int i;
    uLong mtrrcap = rdmsr(0x0FE);
    uLong mtrrdeftype = rdmsr(0x2FF);
    int fixen = (mtrrdeftype >> 10) & 1;
    int enb = (mtrrdeftype >> 11) & 1;
    int deftype = mtrrdeftype & 0xFF;
    char *deftypename = get_typename(deftype);

    printf("MTRR        Start         End            V  Type\n");
    printf("=================================================\n");

    for (i = 0; i < sizeof fixed_mtrrs / sizeof fixed_mtrrs[0]; i++)
    {
	uLong msrval = rdmsr(fixed_mtrrs[i].msr);
	int type = (msrval >> fixed_mtrrs[i].shift) & fixed_mtrrs[i].mask;
	char *typename = get_typename(type);

	printf("MTRR_%05X  00000%08X-00000%08X  %d  %s", fixed_mtrrs[i].base,
		fixed_mtrrs[i].base,
		fixed_mtrrs[i].base + fixed_mtrrs[i].size - 1, fixen, typename);

	if (fixed_mtrrs[i].shift == 0)
	    printf("      %016qX\n", msrval);
	else
	    printf("\n");
    }

    for (i = 0; i < 8; i++)
    {
	uLong phys = rdmsr(0x200 + i * 2);
	uLong mask = rdmsr(0x201 + i * 2);
	uLong sz = (~mask & 0x000FFFFFFFFFF000UL) + 0x1000;
	int type = phys & 0xFF;
	char *typename = get_typename(type);

	printf("MTRR%d       %013qX-%013qX  %d  %s\n", i, phys, phys + sz - 1,
		(mask >> 11) & 1, typename);
    }

    printf("\n");
}

int
find_fixed_mtrr(uLong *base, uLong *size, uByte *typep)
{
    uLong b = *base;
    uLong msr;
    int type;
    int type2;
    int i;
    int j;

    if (b >= 0x100000)
	return 0;

    for (i = 0; i < sizeof fixed_mtrrs / sizeof fixed_mtrrs[0]; i++)
	if (b >= fixed_mtrrs[i].base &&
		b < fixed_mtrrs[i].base + fixed_mtrrs[i].size)
	    break;

    msr = rdmsr(fixed_mtrrs[i].msr);
    type = (msr >> fixed_mtrrs[i].shift) & fixed_mtrrs[i].mask;

    for (j = i + 1; j < sizeof fixed_mtrrs / sizeof fixed_mtrrs[0]; j++)
    {
	msr = rdmsr(fixed_mtrrs[j].msr);
	type2 = (msr >> fixed_mtrrs[j].shift) & fixed_mtrrs[j].mask;

	if (type != type2)
		break;
    }

    *size = (fixed_mtrrs[j - 1].base + fixed_mtrrs[j - 1].size) - b;
    *typep = type;
    return 1;
}

#if 0
int
find_variable_mtrr(uLong *base, uLong *size, uByte *typep)
{
    uLong b = *base;
    int msrnum = 0x200;
    int i;
    uLong maxsz;
    uLong start;
    int mintype;
    int found = 0;

    for (i = 0x200; i < 0x210; i += 2)
    {
	uLong phys = rdmsr(i);
	uLong mask = rdmsr(i + 1);
	uLong sz;

	if ((mask & 0x800) == 0)
	    continue;

	phys &= 0x000FFFFFFFFFF000UL;
	sz = (~mask & 0x000FFFFFFFFFF000UL) + 0x1000;

	if (b >= phys && b < phys + size)
	{
	    if (found)
	    {
		if (phys + size - b < maxsz)
		    maxsz = phys + size - b;

		if (phys & 0xFF) < mintype)
		    mintype = phys & 0xFF;
	    }
	    else
	    {
		maxsz = phys + size - b;
		mintype = phys & 0xFF;
		found = 1;
	    }
	}
    }

    if (found)
    {
	*size = maxsz;
	*typep = mintype;
	return 1;
    }

    for (i = 0x200; i < 0x210; i += 2)
    {
	uLong phys = rdmsr(i);
	uLong mask = rdmsr(i + 1);
	uLong sz;

	if ((mask & 0x800) == 0)
	    continue;

	phys &= 0x000FFFFFFFFFF000UL;
	sz = (~mask & 0x000FFFFFFFFFF000UL) + 0x1000;

	if (b < phys)
	{
	    if (found)
	    {
		if (phys < start)
			if (start - phys phys
		maxsz = phys + size - b;
		
		if (phys + sz - b < maxsz)
		    maxsz = phys + size - b;

		if ((phys & 0xFF) < mintype)
		    mintype = phys & 0xFF;
	    }
	    else
	    {
		start = phys;
		maxsz = size;
		mintype = phys & 0xFF;
		found = 1;
	    }
	}
    }
}
#endif

int
find_iorr(uLong *base, uLong *size, uByte *typep)
{
    return 0;
}

int
find_top_mem(uLong *base, uLong *size, uByte *typep)
{
    return 0;
}

void
show_mtrrs2()
{
    uLong base = 0UL;
    uLong size; 
    uByte type;

    printf("MTRR        Start         End            V  Type\n");
    printf("=================================================\n");

    while (find_fixed_mtrr(&base, &size, &type))
    {
	printf("            %013qX-%013qX  -  %s\n", base, base + size - 1,
		get_typename(type));
	base += size;
    }

    printf("\n");
}

void
show_pa()
{
/*
Start        End          Node Type  Attributes
================================================================================
FF.FFFF.FFFF-FF.FFFF.FFFF   N0  RAM  Bank=0
FF.FFFF.FFFF-FF.FFFF.FFFF   N0  RAM  Bank=1
FF.FFFF.FFFF-FF.FFFF.FFFF   N0  RAM  Bank=2
FF.FFFF.FFFF-FF.FFFF.FFFF   N0  RAM  Bank=3
*/
}

void
show_va()
{
/*
Start          End             Physical                  Attributes
================================================================================
FFFF.FFFF.FFFF-FFFF.FFFF.FFFF  FF.FFFF.FFFF-FF.FFFF.FFFF US RW P
*/
}

void
dump_msr(int num)
{
    uLong val = rdmsr(num);

    printf("    0x%08X, 0x%08X, 0x%08X,\n", num, val & 0xFFFFFFFF,
	    (val >> 32) & 0xFFFFFFFF);
}

void
dump_cpuid(int num)
{
    uInt eax;
    uInt ebx;
    uInt ecx;
    uInt edx;
    cpuid(num, &eax, &ebx, &ecx, &edx);

    printf("    0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X,\n", num, eax, ebx, ecx, edx);
}

void
dump_data()
{
    int n;
    int i;

    for (n = 0; n < MAX_NODES; n++)
    {
	printf("unsigned int node%d[] =\n{\n",n );

	for (i = 0; i < 4*256; i += 4)
	{
	    if ((i & 0xF) == 0x0)
		printf("   ");

	    printf(" 0x%08X,", pci_config_read(0xC000 | (n << 11) | i));

	    if ((i & 0xF) == 0xC)
		printf("\n");
	}

	printf("};\n\n");
    }

    for (i = 0; i < 2; i++)
    {
	int j;

	printf("unsigned char dimm%d[] =\n{\n",i );

	for (j = 0; j < 256; j++)
	{
	    if ((j & 7) == 0x0)
		printf("   ");

	    printf(" 0x%02X,", seeprom_read0(0x002B00, i, j));

	    if ((j & 7) == 0x7)
		printf("\n");
	}

	printf("};\n\n");
    }

    printf("unsigned int msrs[] =\n{\n",i );

    dump_msr(0x200);
    dump_msr(0x201);
    dump_msr(0x202);
    dump_msr(0x203);
    dump_msr(0x204);
    dump_msr(0x205);
    dump_msr(0x206);
    dump_msr(0x207);
    dump_msr(0x208);
    dump_msr(0x209);
    dump_msr(0x20A);
    dump_msr(0x20B);
    dump_msr(0x20C);
    dump_msr(0x20D);
    dump_msr(0x20E);
    dump_msr(0x20F);
    dump_msr(0x250);
    dump_msr(0x258);
    dump_msr(0x259);
    dump_msr(0x268);
    dump_msr(0x269);
    dump_msr(0x26A);
    dump_msr(0x26B);
    dump_msr(0x26C);
    dump_msr(0x26D);
    dump_msr(0x26E);
    dump_msr(0x26F);

    printf("};\n\n");

    printf("unsigned int cpuids[] =\n{\n",i );

    dump_cpuid(0);
    dump_cpuid(1);
    dump_cpuid(0x80000000);
    dump_cpuid(0x80000001);
    dump_cpuid(0x80000002);
    dump_cpuid(0x80000003);
    dump_cpuid(0x80000004);
    dump_cpuid(0x80000005);
    dump_cpuid(0x80000006);
    dump_cpuid(0x80000007);

    printf("};\n\n");
}

int
main(int argc, char **argv)
{
//    dump_data();
    show_nodes();
    show_links();
    show_nodemap();
    show_drambanks();
    show_dramtiming();
    show_dimms();
    show_mtrrs2();
    show_pa();
    show_va();
//    exit(0);
    return 0;
}

