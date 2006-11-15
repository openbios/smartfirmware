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


#include "defs.h"
#include "pci.h"

struct bitvalues
{
	char *name;
	int value;
};

struct bitfield
{
	char *name;
	int bit;
	int width;
	struct bitvalues *values;
};

static char *
bitfield_value(struct bitvalues *values, int value)
{
	static char buf[20];

	for (; values->name; values++)
		if (values->value == value)
			return values->name;

	bprintf(buf, "%x", value);
	return buf;
}

void
dump_bits(Environ *e, char *name, int offset, struct bitfield *bits,
		uLong value)
{
	int first = 1;

	if (bits == NULL)
	{
		cprintf(e, "%#x %s = %#Lx\n", offset, name, value);
		return;
	}

	cprintf(e, "%#x %s = %#Lx [", offset, name, value);

	for (; bits->name; bits++)
		if (bits->width || (value & (1 << bits->bit)))
		{
			if (!first)
				cprintf(e, ", ");
			else
				first = 0;

			if (bits->width == 0)
				cprintf(e, "%s", bits->name);
			else if (bits->values)
				cprintf(e, "%s=%s", bits->name, bitfield_value(bits->values,
						(value >> bits->bit) & ((1 << bits->width) - 1)));
			else
				cprintf(e, "%s=%x", bits->name, 
						(value >> bits->bit) & ((1 << bits->width) - 1));
		}

	cprintf(e, "]\n");
}

struct register_set
{
	int offset;
	char *name;
	struct bitfield *fields;
};

void
dump_pci_config_set(Environ *e, int base, struct register_set *regs)
{
	for (; regs->name; regs++)
		dump_bits(e, regs->name, regs->offset, regs->fields,
				pci_config_read(0, base >> 16, (base >> 11) & 0x1F,
				(base >> 8) & 7, (base & 0xFF) + regs->offset));
}

uLong
msr_read(int offset)
{
	uInt valhi;
	uInt vallo;
	__asm __volatile("rdmsr" : "=d" (valhi),	
		"=a" (vallo) : "c" (offset));
	return ((uLong)valhi << 32) | vallo;
}

void
dump_msrs(Environ *e, int base, struct register_set *regs)
{
	for (; regs->name; regs++)
		dump_bits(e, regs->name, regs->offset, regs->fields,
				msr_read(base + regs->offset));
}

struct bitvalues pci_devvend_id_vend_values[] =
{
	{ "AMD (1022)", 0x1022 },
	{ NULL },
};

struct bitfield pci_devvend_id[] =
{
	{ "Device", 16, 16 },
	{ "Vendor", 0, 16, pci_devvend_id_vend_values },
	{ NULL },
};

struct bitfield pci_cmdstat[] =
{
	{ "Status", 16, 16 },
	{ "Command", 0, 16 },
	{ NULL },
};

struct bitfield pci_class[] =
{
	{ "Class", 24, 8 },
	{ "Sub-Class", 16, 8 },
	{ "Interface", 8, 8 },
	{ "Revision", 0, 8 },
	{ NULL },
};

struct bitvalues pci_bist_header_values[] =
{
	{ "NormalDev (0)", 0 },
	{ "PCI-PCI Bridge (1)", 1 },
	{ "PCI-CardBus Bridge (2)", 2 },
	{ NULL },
};

struct bitfield pci_bist[] =
{
	{ "BIST", 24, 8 },
	{ "MultiFunc", 23 },
	{ "HeaderType", 16, 7, pci_bist_header_values },
	{ "LatencyTimer", 8, 8 },
	{ "CacheLineSize", 0, 8 },
	{ NULL },
};

struct bitfield pci_bar[] =
{
	{ "Address", 4, 28 },
	{ "MemType", 0, 4 },
	{ "Type", 0, 2 },
	{ NULL },
};

struct bitfield pci_cis[] =
{
	{ "CIS Pointer", 0, 32 },
	{ NULL },
};

struct bitfield pci_subsys[] =
{
	{ "Device", 16, 16 },
	{ "Vendor", 0, 16, pci_devvend_id_vend_values },
	{ NULL },
};

struct bitfield pci_rombase[] =
{
	{ "Address", 10, 22 },
	{ "Enable", 0, 1 },
	{ NULL },
};

struct bitfield pci_capptr[] =
{
	{ "CapPtr", 0, 8 },
	{ NULL },
};

struct bitfield pci_latgnt[] =
{
	{ "MaxLat", 24, 8 },
	{ "MinGnt", 16, 8 },
	{ "IntPin", 8, 8 },
	{ "IntLine", 0, 8 },
	{ NULL },
};

struct bitvalues claw_htroute_linkid[] =
{
	{ "This node (1)", 1 },
	{ "Link 0 (2)", 2 },
	{ "Link 1 (4)", 4 },
	{ "Link 2 (8)", 8 },
	{ NULL },
};

struct bitfield claw_htroute[] =
{
	{ "BCRte", 16, 4, claw_htroute_linkid, },
	{ "RPRte", 8, 4, claw_htroute_linkid, },
	{ "RQRte", 0, 4, claw_htroute_linkid, },
	{ NULL },
};

struct bitfield claw_nodeid[] =
{
	{ "CPUCnt", 16, 3, },
	{ "LkNode", 12, 3, },
	{ "SbNode", 8, 3, },
	{ "NodeCnt", 4, 3, },
	{ "NodeId", 0, 3, },
	{ NULL },
};

struct bitfield claw_unitid[] =
{
	{ "SbUnit", 8, 2, },
	{ "HbUnit", 6, 2, },
	{ "McUnit", 4, 2, },
	{ "C1Unit", 2, 2, },
	{ "C0Unit", 0, 2, },
	{ NULL },
};

struct bitfield claw_httransctl[] =
{
	{ "DisIsocWrMedPri", 31 },
	{ "DisIsocWrHiPri", 30 },
	{ "DisWrLoPri", 29 },
	{ "EnCpuRdHiPri", 28 },
	{ "HiPriBypCnt", 26, 2 },
	{ "MedPriBypCnt", 24, 2 },
	{ "DsNpReqLmt", 21, 2 },
	{ "SeqIdSrcNodeEn", 20 },
	{ "ApicExtSpur", 19 },
	{ "ApicExtId", 18 },
	{ "ApicExtBrdCst", 17 },
	{ "LintEn", 16 },
	{ "LimitCldtCfg", 15 },
	{ "BufRelPri", 13, 2 },
	{ "ChgIsocToOrd", 12 },
	{ "RspPassPW", 11 },
	{ "DisFillP", 10 },
	{ "DisRmtPMemC", 9 },
	{ "DisPMemC", 8 },
	{ "CPURdRspPassPW", 7 },
	{ "CPUReqPassPW", 6 },
	{ "Cpu1En", 5 },
	{ "DisMTS", 4 },
	{ "DisWrDwP", 3 },
	{ "DisWrBP", 2 },
	{ "DisRdDwP", 1 },
	{ "DisRdBP", 0 },
	{ NULL },
};

struct bitfield claw_htinitctl[] =
{
	{ NULL },
};

struct bitfield claw_htcap[] =
{
	{ NULL },
};

struct bitfield claw_htlinkctl[] =
{
	{ NULL },
};

struct bitfield claw_htlinkfreq[] =
{
	{ NULL },
};

struct bitfield claw_htfeature[] =
{
	{ NULL },
};

struct bitfield claw_htbufcnt[] =
{
	{ NULL },
};

struct bitfield claw_htbusnum[] =
{
	{ NULL },
};

struct bitfield claw_httype[] =
{
	{ NULL },
};


struct register_set claw_htctl[] = 
{
	{ 0x00, "Device/Vendor ID", pci_devvend_id, },
	{ 0x04, "Command/Status", pci_cmdstat, },
	{ 0x08, "Class", pci_class, },
	{ 0x0C, "BIST/Header Type", pci_bist, },
	{ 0x10, "BAR 0", pci_bar, },
	{ 0x14, "BAR 1", pci_bar, },
	{ 0x18, "BAR 2", pci_bar, },
	{ 0x1C, "BAR 3", pci_bar, },
	{ 0x20, "BAR 4", pci_bar, },
	{ 0x24, "BAR 5", pci_bar, },
	{ 0x28, "CIS", pci_cis, },
	{ 0x2C, "Subsystem", pci_subsys, },
	{ 0x30, "ROM Base", pci_rombase, },
	{ 0x34, "Capabilities", pci_capptr, },
	{ 0x38, "Reserved 38", },
	{ 0x3C, "LAT/GNT", pci_latgnt, },
	{ 0x40, "Route Node 0", claw_htroute, },
	{ 0x44, "Route Node 1", claw_htroute, },
	{ 0x48, "Route Node 2", claw_htroute, },
	{ 0x4C, "Route Node 3", claw_htroute, },
	{ 0x50, "Route Node 4", claw_htroute, },
	{ 0x54, "Route Node 5", claw_htroute, },
	{ 0x58, "Route Node 6", claw_htroute, },
	{ 0x5C, "Route Node 7", claw_htroute, },
	{ 0x60, "Node ID", claw_nodeid, },
	{ 0x64, "Unit ID", claw_unitid, },
	{ 0x68, "HT Trans Ctl", claw_httransctl, },
	{ 0x6C, "HT Init Ctl", claw_htinitctl, },
	{ 0x70, "Reserved 70", },
	{ 0x74, "Reserved 74", },
	{ 0x78, "Reserved 78", },
	{ 0x7C, "Reserved 7C", },
	{ 0x80, "HT0 Capabilities", claw_htcap, },
	{ 0x84, "HT0 Link Ctl", claw_htlinkctl, },
	{ 0x88, "HT0 Link Freq", claw_htlinkfreq, },
	{ 0x8C, "HT0 Feature", claw_htfeature, },
	{ 0x90, "HT0 Buf Cnt", claw_htbufcnt, },
	{ 0x94, "HT0 Bus Num", claw_htbusnum, },
	{ 0x98, "HT0 Type", claw_httype, },
	{ 0x9C, "Reserved 9C", },
	{ 0xA0, "HT1 Capabilities", claw_htcap, },
	{ 0xA4, "HT1 Link Ctl", claw_htlinkctl, },
	{ 0xA8, "HT1 Link Freq", claw_htlinkfreq, },
	{ 0xAC, "HT1 Feature", claw_htfeature, },
	{ 0xB0, "HT1 Buf Cnt", claw_htbufcnt, },
	{ 0xB4, "HT1 Bus Num", claw_htbusnum, },
	{ 0xB8, "HT1 Type", claw_httype, },
	{ 0xBC, "Reserved BC", },
	{ 0xC0, "HT2 Capabilities", claw_htcap, },
	{ 0xC4, "HT2 Link Ctl", claw_htlinkctl, },
	{ 0xC8, "HT2 Link Freq", claw_htlinkfreq, },
	{ 0xCC, "HT2 Feature", claw_htfeature, },
	{ 0xD0, "HT2 Buf Cnt", claw_htbufcnt, },
	{ 0xD4, "HT2 Bus Num", claw_htbusnum, },
	{ 0xD8, "HT2 Type", claw_httype, },
	{ 0xDC, "Reserved DC", },
	{ 0xE0, "Reserved E0", },
	{ 0xE4, "Reserved E4", },
	{ 0xE8, "Reserved E8", },
	{ 0xEC, "Reserved EC", },
	{ 0xF0, "Reserved F0", },
	{ 0xF4, "Reserved F4", },
	{ 0xF8, "Reserved F8", },
	{ 0xFC, "Reserved FC", },
	{ 0x00, NULL, },
};

struct bitfield claw_dram_base[] =
{
	{ "Base", 16, 16, },
	{ "Interleave Enb", 8, 3, },
	{ "WE", 1, },
	{ "RE", 0, },
	{ NULL },
};

struct bitfield claw_dram_limit[] =
{
	{ "Limit", 16, 16, },
	{ "Interleave Sel", 8, 3, },
	{ "Dst Node", 0, 3, },
	{ NULL },
};

struct bitfield claw_mmio_base[] =
{
	{ "Base", 8, 24 },
	{ "Lock", 3 },
	{ "CpuDis", 2 },
	{ "WE", 1 },
	{ "RE", 0 },
	{ NULL },
};

struct bitfield claw_mmio_limit[] =
{
	{ "Limit", 8, 24 },
	{ "NP", 7 },
	{ "DstLink", 4, 2 },
	{ "DstNode", 0, 3 },
	{ NULL },
};

struct bitfield claw_pci_base[] =
{
	{ "Base", 12, 12 },
	{ "IE", 5 },
	{ "VE", 4 },
	{ "WE", 1 },
	{ "RE", 0 },
	{ NULL },
};

struct bitfield claw_pci_limit[] =
{
	{ "Limit", 12, 12 },
	{ "DstLink", 4, 2 },
	{ "DstNode", 0, 3 },
	{ NULL },
};

struct bitfield claw_config_base[] =
{
	{ "Bus Limit", 24, 8 },
	{ "Bus Base", 16, 8 },
	{ "DstLink", 8, 2 },
	{ "DstNode", 4, 3 },
	{ "WE", 1 },
	{ "RE", 0 },
	{ NULL },
};

struct register_set claw_addrctl[] =
{
	{ 0x00, "Device/Vendor ID", pci_devvend_id, },
	{ 0x04, "Command/Status", pci_cmdstat, },
	{ 0x08, "Class", pci_class, },
	{ 0x0C, "BIST/Header Type", pci_bist, },
	{ 0x10, "BAR 0", pci_bar, },
	{ 0x14, "BAR 1", pci_bar, },
	{ 0x18, "BAR 2", pci_bar, },
	{ 0x1C, "BAR 3", pci_bar, },
	{ 0x20, "BAR 4", pci_bar, },
	{ 0x24, "BAR 5", pci_bar, },
	{ 0x28, "CIS", pci_cis, },
	{ 0x2C, "Subsystem", pci_subsys, },
	{ 0x30, "ROM Base", pci_rombase, },
	{ 0x34, "Capabilities", pci_capptr, },
	{ 0x38, "Reserved 38", },
	{ 0x3C, "LAT/GNT", pci_latgnt, },
	{ 0x40, "DRAM Base 0", claw_dram_base },
	{ 0x44, "DRAM Limit 0", claw_dram_limit },
	{ 0x48, "DRAM Base 1", claw_dram_base },
	{ 0x4C, "DRAM Limit 1", claw_dram_limit },
	{ 0x50, "DRAM Base 2", claw_dram_base },
	{ 0x54, "DRAM Limit 2", claw_dram_limit },
	{ 0x58, "DRAM Base 3", claw_dram_base },
	{ 0x5C, "DRAM Limit 3", claw_dram_limit },
	{ 0x60, "DRAM Base 4", claw_dram_base },
	{ 0x64, "DRAM Limit 4", claw_dram_limit },
	{ 0x68, "DRAM Base 5", claw_dram_base },
	{ 0x6C, "DRAM Limit 5", claw_dram_limit },
	{ 0x70, "DRAM Base 6", claw_dram_base },
	{ 0x74, "DRAM Limit 6", claw_dram_limit },
	{ 0x78, "DRAM Base 7", claw_dram_base },
	{ 0x7C, "DRAM Limit 7", claw_dram_limit },
	{ 0x80, "Memory-Mapped I/O Base 0", claw_mmio_base },
	{ 0x84, "Memory-Mapped I/O Limit 0", claw_mmio_limit },
	{ 0x88, "Memory-Mapped I/O Base 1", claw_mmio_base },
	{ 0x8C, "Memory-Mapped I/O Limit 1", claw_mmio_limit },
	{ 0x90, "Memory-Mapped I/O Base 2", claw_mmio_base },
	{ 0x94, "Memory-Mapped I/O Limit 2", claw_mmio_limit },
	{ 0x98, "Memory-Mapped I/O Base 3", claw_mmio_base },
	{ 0x9C, "Memory-Mapped I/O Limit 3", claw_mmio_limit },
	{ 0xA0, "Memory-Mapped I/O Base 4", claw_mmio_base },
	{ 0xA4, "Memory-Mapped I/O Limit 4", claw_mmio_limit },
	{ 0xA8, "Memory-Mapped I/O Base 5", claw_mmio_base },
	{ 0xAC, "Memory-Mapped I/O Limit 5", claw_mmio_limit },
	{ 0xB0, "Memory-Mapped I/O Base 6", claw_mmio_base },
	{ 0xB4, "Memory-Mapped I/O Limit 6", claw_mmio_limit },
	{ 0xB8, "Memory-Mapped I/O Base 7", claw_mmio_base },
	{ 0xBC, "Memory-Mapped I/O Limit 7", claw_mmio_limit },
	{ 0xC0, "PCI I/O Base 0", claw_pci_base },
	{ 0xC4, "PCI I/O Limit 0", claw_pci_limit },
	{ 0xC8, "PCI I/O Base 1", claw_pci_base },
	{ 0xCC, "PCI I/O Limit 1", claw_pci_limit },
	{ 0xD0, "PCI I/O Base 2", claw_pci_base },
	{ 0xD4, "PCI I/O Limit 2", claw_pci_limit },
	{ 0xD8, "PCI I/O Base 3", claw_pci_base },
	{ 0xDC, "PCI I/O Limit 3", claw_pci_limit },
	{ 0xE0, "Config Base and Limit 0", claw_config_base },
	{ 0xE4, "Config Base and Limit 1", claw_config_base },
	{ 0xE8, "Config Base and Limit 2", claw_config_base },
	{ 0xEC, "Config Base and Limit 3", claw_config_base },
	{ 0xF0, "Reserved F0", },
	{ 0xF4, "Reserved F4", },
	{ 0xF8, "Reserved F8", },
	{ 0xFC, "Reserved F8", },
	{ 0x00, NULL, },
};

struct bitfield claw_dram_cs_base[] =
{
	{ "Addr[35:25]", 21, 11 },
	{ "Addr[19:13]", 9, 7 },
	{ NULL },
};

struct bitfield claw_dram_cs_mask[] =
{
	{ "Mask[35:25]", 21, 11 },
	{ "Mask[19:13]", 9, 7 },
	{ NULL },
};

struct bitfield claw_dram_mapping[] =
{
	{ "CS7/6", 12, 3 },
	{ "CS5/4", 8, 3 },
	{ "CS3/2", 4, 3 },
	{ "CS1/0", 0, 3 },
	{ NULL },
};

struct bitfield claw_dram_time_low[] =
{
	{ "Twr", 27, 2 },
	{ "Trp", 24, 3 },
	{ "Tras", 20, 4 },
	{ "Trrd", 16, 3 },
	{ "Trcd", 12, 3 },
	{ "Trfc", 8, 4 },
	{ "Trc", 4, 4 },
	{ "Tcl", 0, 3 },
	{ NULL },
};

struct bitfield claw_dram_time_high[] =
{
	{ "Twcl", 20, 3 },
	{ "Tref", 8, 5 },
	{ "Trwt", 4, 3 },
	{ "Twtr", 0, 1 },
	{ NULL },
};

struct bitfield claw_dram_config_low[] =
{
	{ "BypMax", 25, 3 },
	{ "DisInRcv", 24, 1 },
	{ "x4DIMM", 20, 4 },
	{ "32ByteEn", 19 },
	{ "UnBuffDIMM", 18 },
	{ "DIMMEcEn", 17 },
	{ "128/64", 16 },
	{ "RdWrQByp", 14, 2 },
	{ "SR_S", 13 },
	{ "ESR", 13 },
	{ "DRAM_init", 8, 1 },
	{ "DisDqsHys", 3 },
	{ "QFC_EN", 2 },
	{ "D_DRV", 1 },
	{ "DLL_dis", 0 },
	{ NULL },
};

struct bitfield claw_dram_config_high[] =
{
	{ "MC3_EN", 29 },
	{ "MC2_EN", 28 },
	{ "MC1_EN", 27 },
	{ "MC0_EN", 26 },
	{ "MCR", 25 },
	{ "Mclk", 20, 3 },
	{ "DCC_EN", 19 },
	{ "IDL_lmt", 16, 3 },
	{ "RdPreamble", 8, 3 },
	{ "AsyncLat", 0, 4 },
	{ NULL },
};

struct bitfield claw_dram_delay[] =
{
	{ "AdjFaster", 25 },
	{ "AdjSlower", 24 },
	{ "Adj", 16, 8 },
	{ NULL },
};

struct register_set claw_tracectl[] =
{
	{ 0x00, "Device/Vendor ID", pci_devvend_id, },
	{ 0x04, "Command/Status", pci_cmdstat, },
	{ 0x08, "Class", pci_class, },
	{ 0x0C, "BIST/Header Type", pci_bist, },
	{ 0x10, "BAR 0", pci_bar, },
	{ 0x14, "BAR 1", pci_bar, },
	{ 0x18, "BAR 2", pci_bar, },
	{ 0x1C, "BAR 3", pci_bar, },
	{ 0x20, "BAR 4", pci_bar, },
	{ 0x24, "BAR 5", pci_bar, },
	{ 0x28, "CIS", pci_cis, },
	{ 0x2C, "Subsystem", pci_subsys, },
	{ 0x30, "ROM Base", pci_rombase, },
	{ 0x34, "Capabilities", pci_capptr, },
	{ 0x38, "Reserved 38", },
	{ 0x3C, "LAT/GNT", pci_latgnt, },
	{ 0x40, "DRAM CS Base 0", claw_dram_cs_base },
	{ 0x44, "DRAM CS Base 1", claw_dram_cs_base },
	{ 0x48, "DRAM CS Base 2", claw_dram_cs_base },
	{ 0x4C, "DRAM CS Base 3", claw_dram_cs_base },
	{ 0x50, "DRAM CS Base 4", claw_dram_cs_base },
	{ 0x54, "DRAM CS Base 5", claw_dram_cs_base },
	{ 0x58, "DRAM CS Base 6", claw_dram_cs_base },
	{ 0x5C, "DRAM CS Base 7", claw_dram_cs_base },
	{ 0x60, "DRAM CS Mask 0", claw_dram_cs_mask },
	{ 0x64, "DRAM CS Mask 1", claw_dram_cs_mask },
	{ 0x68, "DRAM CS Mask 2", claw_dram_cs_mask },
	{ 0x6C, "DRAM CS Mask 3", claw_dram_cs_mask },
	{ 0x70, "DRAM CS Mask 4", claw_dram_cs_mask },
	{ 0x74, "DRAM CS Mask 5", claw_dram_cs_mask },
	{ 0x78, "DRAM CS Mask 6", claw_dram_cs_mask },
	{ 0x7C, "DRAM CS Mask 7", claw_dram_cs_mask },
	{ 0x80, "DRAM Bank Address Mapping", claw_dram_mapping },
	{ 0x84, "Reserved 84" },
	{ 0x88, "DRAM Timing Low", claw_dram_time_low },
	{ 0x8C, "DRAM Timing High", claw_dram_time_high },
	{ 0x90, "DRAM Config Low", claw_dram_config_low },
	{ 0x94, "DRAM Config High", claw_dram_config_high },
	{ 0x98, "DRAM Delay Line", claw_dram_delay },
	{ 0x9C, "Reserved 9C" },
	{ 0xA0, "Reserved A0" },
	{ 0xA4, "Reserved A4" },
	{ 0xA8, "Reserved A8" },
	{ 0xAC, "Reserved AC" },
	{ 0xB0, "Time Stamp Counter Low" },
	{ 0xB4, "Time Stamp Counter High" },
	{ 0xB8, "Trace Buffer Base/Limit Address" },
	{ 0xBC, "Trace Buffer Address Pointer" },
	{ 0xC0, "Trace Control" },
	{ 0xC4, "Trace Start" },
	{ 0xC8, "Trace Stop" },
	{ 0xCC, "Trace Capture" },
	{ 0xD0, "Trigger Event 0 Command Low" },
	{ 0xD4, "Trigger Event 0 Command High" },
	{ 0xD8, "Trigger Event 0 Mask Low" },
	{ 0xDC, "Trigger Event 0 Mask High" },
	{ 0xE0, "Trigger Event 1 Command Low" },
	{ 0xE4, "Trigger Event 1 Command High" },
	{ 0xE8, "Trigger Event 1 Mask Low" },
	{ 0xEC, "Trigger Event 1 Mask High" },
	{ 0xF0, "Reserved F0" },
	{ 0xF4, "Reserved F4" },
	{ 0xF8, "Reserved F8" },
	{ 0xFC, "Reserved FC" },
	{ 0x00, NULL, },
};

struct register_set claw_miscctl[] =
{
	{ 0x00, "Device/Vendor ID", pci_devvend_id, },
	{ 0x04, "Command/Status", pci_cmdstat, },
	{ 0x08, "Class", pci_class, },
	{ 0x0C, "BIST/Header Type", pci_bist, },
	{ 0x10, "BAR 0", pci_bar, },
	{ 0x14, "BAR 1", pci_bar, },
	{ 0x18, "BAR 2", pci_bar, },
	{ 0x1C, "BAR 3", pci_bar, },
	{ 0x20, "BAR 4", pci_bar, },
	{ 0x24, "BAR 5", pci_bar, },
	{ 0x28, "CIS", pci_cis, },
	{ 0x2C, "Subsystem", pci_subsys, },
	{ 0x30, "ROM Base", pci_rombase, },
	{ 0x34, "Capabilities", pci_capptr, },
	{ 0x38, "Reserved 38", },
	{ 0x3C, "LAT/GNT", pci_latgnt, },
	{ 0x40, "MCA Northbridge Control" },
	{ 0x44, "MCA Northbridge Configuration" },
	{ 0x48, "MCA Northbridge Status Low" },
	{ 0x4C, "MCA Northbridge Status High" },
	{ 0x50, "MCA Northbridge Address Low" },
	{ 0x54, "MCA Northbridge Address High" },
	{ 0x58, "Scrub Control" },
	{ 0x5C, "DRAM Scrub Address Low" },
	{ 0x60, "DRAM Scrub Address High" },
	{ 0x64, "Reserved 64" },
	{ 0x68, "Reserved 68" },
	{ 0x6C, "Reserved 6C" },
	{ 0x70, "SRI-to-XBAR Buffer Counts" },
	{ 0x74, "XBAR-to-SRI Buffer Counts" },
	{ 0x78, "MCT-to-XBAR Buffer Counts" },
	{ 0x7C, "Free List Buffer Counts" },
	{ 0x80, "Power Management Control Low" },
	{ 0x84, "Power Management Control High" },
	{ 0x90, "GART Aperture Control" },
	{ 0x94, "GART Aperture Base" },
	{ 0x98, "GART Table Base" },
	{ 0x9C, "GART Cache Control" },
	{ 0xB0, "Reserved B0" },
	{ 0xB8, "Northbridge Array Address" },
	{ 0xBC, "Northbridge Array Data" },
	{ 0xC0, "FID/VID Control Low" },
	{ 0xC4, "FID/VID Control High" },
	{ 0xC8, "FID/VID Status Low" },
	{ 0xCC, "FID/VID Status High" },
	{ 0xD0, "Reserved D0" },
	{ 0xD4, "Clock Power/Timing Low" },
	{ 0xD8, "Clock Power/Timing High" },
	{ 0xDC, "HyperTransport Read Pointer Optimization" },
	{ 0xE4, "Thermtrip Status" },
	{ 0xE8, "Northbridge Capabilities" },
	{ 0xF0, "Reserved F0" },
	{ 0xF4, "Reserved F4" },
	{ 0xF8, "Reserved F8" },
	{ 0xFC, "Reserved FC" },
	{ 0x00, NULL, },
};

struct register_set claw_msrs[] =
{
	{ 0x0010, "TSC" },
	{ 0x001B, "APIC_BASE" },
	{ 0x002A, "EBL_CR_POWERON" },
	{ 0x008B, "PATCH_LEVEL" },
	{ 0x00FE, "MTRRcap" },
	{ 0x0174, "SYSENTER_CS" },
	{ 0x0175, "SYSENTER_ESP" },
	{ 0x0176, "SYSENTER_EIP" },
	{ 0x0179, "MCG_CAP" },
	{ 0x017A, "MCG_STATUS" },
	{ 0x017B, "MCG_CTL" },
	{ 0x01D9, "DebugCtl" },
	{ 0x01DB, "LastBranchFromIP" },
	{ 0x01DC, "LastBranchToIP" },
	{ 0x01DD, "LastExceptionFromIP" },
	{ 0x01DE, "LastExceptionToIP" },
	{ 0x0200, "MTRRphysBase0" },
	{ 0x0201, "MTRRphysMask0" },
	{ 0x0202, "MTRRphysBase1" },
	{ 0x0203, "MTRRphysMask1" },
	{ 0x0204, "MTRRphysBase2" },
	{ 0x0205, "MTRRphysMask2" },
	{ 0x0206, "MTRRphysBase3" },
	{ 0x0207, "MTRRphysMask3" },
	{ 0x0208, "MTRRphysBase4" },
	{ 0x0209, "MTRRphysMask4" },
	{ 0x020A, "MTRRphysBase5" },
	{ 0x020B, "MTRRphysMask5" },
	{ 0x020C, "MTRRphysBase6" },
	{ 0x020D, "MTRRphysMask6" },
	{ 0x020E, "MTRRphysBase7" },
	{ 0x020F, "MTRRphysMask7" },
	{ 0x0250, "MTRRfix64K_00000" },
	{ 0x0258, "MTRRfix16K_80000" },
	{ 0x0259, "MTRRfix16K_A0000" },
	{ 0x0268, "MTRRfix4K_C0000" },
	{ 0x0269, "MTRRfix4K_C8000" },
	{ 0x026A, "MTRRfix4K_D0000" },
	{ 0x026B, "MTRRfix4K_D8000" },
	{ 0x026C, "MTRRfix4K_E0000" },
	{ 0x026D, "MTRRfix4K_E8000" },
	{ 0x026E, "MTRRfix4K_F0000" },
	{ 0x026F, "MTRRfix4K_F8000" },
	{ 0x0277, "PAT" },
	{ 0x02FF, "MTRRdefType" },
	{ 0x0400, "MC0_CTL" },
	{ 0x0401, "MC0_STATUS" },
	{ 0x0402, "MC0_ADDR" },
	{ 0x0403, "MC0_MISC" },
	{ 0x0404, "MC1_CTL" },
	{ 0x0405, "MC1_STATUS" },
	{ 0x0406, "MC1_ADDR" },
	{ 0x0407, "MC1_MISC" },
	{ 0x0408, "MC2_CTL" },
	{ 0x0409, "MC2_STATUS" },
	{ 0x040A, "MC2_ADDR" },
	{ 0x040B, "MC2_MISC" },
	{ 0x040C, "MC3_CTL" },
	{ 0x040D, "MC3_STATUS" },
	{ 0x040E, "MC3_ADDR" },
	{ 0x040F, "MC3_MISC" },
	{ 0x0410, "MC4_CTL" },
	{ 0x0411, "MC4_STATUS" },
	{ 0x0412, "MC4_ADDR" },
	{ 0x0413, "MC4_MISC" },

	{ 0xC0000080, "EFER" },
	{ 0xC0000081, "STAR" },
	{ 0xC0000082, "LSTAR" },
	{ 0xC0000083, "CSTAR" },
	{ 0xC0000084, "SF_MASK" },
	{ 0xC0000100, "FS.Base" },
	{ 0xC0000101, "GS.Base" },
	{ 0xC0000102, "KernelGSbase" },
	{ 0xC0010000, "PerfEvtSel0" },
	{ 0xC0010001, "PerfEvtSel1" },
	{ 0xC0010002, "PerfEvtSel2" },
	{ 0xC0010003, "PerfEvtSel3" },
	{ 0xC0010004, "PerfCtr0" },
	{ 0xC0010005, "PerfCtr1" },
	{ 0xC0010006, "PerfCtr2" },
	{ 0xC0010007, "PerfCtr3" },
	{ 0xC0010010, "SYSCFG" },
	{ 0xC0010016, "IORRBase0" },
	{ 0xC0010017, "IORRMask0" },
	{ 0xC0010018, "IORRBase1" },
	{ 0xC0010019, "IORRMask1" },
	{ 0xC001001A, "TOP_MEM" },
	{ 0xC001001D, "TOP_MEM2" },

	{ 0x00, NULL, },
};

C(f_dump_hammer)
{
	/* dump the contents of various ClawHammer registers */
	cprintf(e, "PCI Config registers 24,0\n");
	dump_pci_config_set(e, 0x00C000, claw_htctl);
	cprintf(e, "\nPCI Config registers 24,1\n");
	dump_pci_config_set(e, 0x00C100, claw_addrctl);
	cprintf(e, "\nPCI Config registers 24,2\n");
	dump_pci_config_set(e, 0x00C200, claw_tracectl);
	cprintf(e, "\nPCI Config registers 24,3\n");
	dump_pci_config_set(e, 0x00C300, claw_miscctl);
	cprintf(e, "\nHammer MSRs\n");
	dump_msrs(e, 0, claw_msrs);
	return NO_ERROR;
}

uInt
superio_read(int addr)
{
    uByte v;
    pci_io_write(0, 0x2E, addr, 1);
    pci_io_read(0, 0x2F, &v, 1);
    return v;
}

void
superio_write(int addr, int data)
{
    pci_io_write(0, 0x2E, addr, 1);
    pci_io_write(0, 0x2F, data, 1);
}

void
dump_superio_set(Environ *e, int logdevnum, struct register_set *regs)
{
	superio_write(7, logdevnum);

	for (; regs->name; regs++)
		dump_bits(e, regs->name, regs->offset, regs->fields,
				superio_read(regs->offset));
}

struct register_set superio_global[] = 
{
	{ 0x20, "CR20" },
	{ 0x21, "CR21" },
	{ 0x22, "CR22" },
	{ 0x23, "CR23" },
	{ 0x24, "CR24" },
	{ 0x25, "CR25" },
	{ 0x26, "CR26" },
	{ 0x28, "CR28" },
	{ 0x29, "CR29" },
	{ 0x2A, "CR2A" },
	{ 0x2B, "CR2B" },
	{ 0x00, NULL, },
};

struct register_set superio_uart[] = 
{
	{ 0x30, "Enable (CR30)" },
	{ 0x60, "Base Addr Low (CR60)" },
	{ 0x61, "Base Addr High (CR61)" },
	{ 0x70, "IRQ Select (CR70)" },
	{ 0xF0, "Clock Source (CRF0)" },
	{ 0xF1, "IR CTL (CRF1)" },
	{ 0x00, NULL, },
};

C(f_dump_superio)
{
	/* enable the fucker */
	pci_io_write(0, 0x2E, 0x87, 1);
	pci_io_write(0, 0x2E, 0x87, 1);

	/* dump the contents of various SuperIO registers */
	dump_superio_set(e, 0, superio_global);
	dump_superio_set(e, 2, superio_uart);		/* UART A */
	dump_superio_set(e, 3, superio_uart);		/* UART B */

	/* disable the fucker */
	pci_io_write(0, 0x2E, 0xAA, 1);
	return NO_ERROR;
}

/* Hammer resources to dump */
/*
 *	HT registers		PCI config 	24,0
 *				PCI config 	24,1
 *				PCI config 	24,3
 *				PCI config 	24,4
 *	MSR			MSR		
 *	Debug 			DR
 *	Control			CR
 *	Segment			
 *	Segment GDTR/LDTR/IDTR
 *	AGP			
 *	    AGP dev		PCI config	1,0
 *	    	Graphics Mem	Memory		1,0,10
 *	    	GART Block	Memory		1,0,B8:1000
 *	    AGP bridge		PCI config	2,0
 *	I/O Hub fixed
 *	    Slave DMA ctl	I/O		0:10
 *	    8259		I/O		20:2
 *	    Prog Intval Tmr	I/O		40:4
 *	    PS/2		I/O		60,64
 *	    AT compat reg	I/O		61
 *	    RTC			I/O		70:4
 *	    DMA Page		I/O		80:10
 *	    Sys Ctl Reg		I/O		92
 *	    Slave 8250		I/O		A0:2
 *	    Master DMA ctl	I/O		C0:20
 *	    FP err ctl 		I/O		F0:2
 *	    Sec. IDE		I/O		170:8,376
 *	    Pri. IDE		I/O		1F0:8,3F6
 *	    EISA int ctl	I/O		4D0:2
 *	    Reset Reg		I/O		CF9
 *	    IO APIC		Mem		FEC00000:100
 *	I/O Hub relocatable
 *	    PCI bridge		PCI config	4,0
 *	    ISA Bridge		PCI Config	5,0
 *	    NVRAM		I/O		5,0,74:100
 *	    HPET 		Mem		5,0,A0:400
 *	    IDE			PCI config	5,1
 *		Pri Cmd		I/O		5,1,10:8
 *		Pri Ctl		I/O		5,1,14:4
 *		Sec Cmd		I/O		5,1,18:8
 *		Sec Ctl		I/O		5,1,1C:4
 *		Bus Mast	I/O		5,1,20:10
 *	    SMBus		PCI config	5,2
 *	    	SC		I/O		5,2,10:20
 *	    System Management	PCI config	5,3
 *	    	PM		I/O		5,3,58:100
 *	    ???			PCI config	5,4
 *	    AC'97 Audio		PCI config	5,5
 *		Mixer		I/O		5,5,10:100
 *		Bus Mast	I/O		5,5,14:40
 *	    AC'97 Modem		PCI config	5,6
 *		Mixer		I/O		5,6,10:100
 *		Bus Mast	I/O		5,6,14:40
 *	    HPET 		Mem		5,0,A0:400
 *	I2C
 *	    Clock Gen		I2C		???
 *	    DIMM1		I2C		???
 *	    DIMM2		I2C		???
 *	Super I/O
 *	    Config		I/O		2E
 *	    COM1		I/O		3F8
 */

#if 0
    free_block(IO, 0x0, 0x10000);

    for (base = 0x0; base < 0x10000; base += 0x400)
	 alloc_fixed(IO, base + 0x100, 0x300);		/* ISA plugin N */

    alloc_fixed(IO, 0x000, 0x100);		/* motherboard regs */

Alternatively

    for (base = 0x400; base < 0x10000; base += 0x400)
	free_block(IO, base, 0x100);

    alloc_fixed(IO, 0x4D0, 2);		/* EISA Int Ctl */
    alloc_fixed(IO, 0xCF8, 8);		/* PCI Config */
#endif
