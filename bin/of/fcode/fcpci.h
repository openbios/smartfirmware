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

/* (c) Copyright 1999-2001 by CodeGen, Inc.  All Rights Reserved. */

/* PCI bus defines and externs for cc-fcode.
 */

#ifndef __FCPCI_H_
#define __FCPCI_H_

#include "fcode.h"

#define PCI_PHYSHI_N(addr)     ((unsigned int)(((addr) >> 31) & 0x01))
#define PCI_PHYSHI_P(addr)     ((unsigned int)(((addr) >> 30) & 0x01))
#define PCI_PHYSHI_T(addr)     ((unsigned int)(((addr) >> 29) & 0x01))
#define PCI_PHYSHI_SPACE(addr) ((unsigned int)(((addr) >> 24) & 0x07))
#define PCI_PHYSHI_BUS(addr)   ((unsigned int)(((addr) >> 16) & 0xFF))
#define PCI_PHYSHI_DEV(addr)   ((unsigned int)(((addr) >> 11) & 0x1F))
#define PCI_PHYSHI_FUNC(addr)  ((unsigned int)(((addr) >> 8) & 0x07))
#define PCI_PHYSHI_REG(addr)   ((unsigned int)((addr) & 0xFF))

#define PCI_PHYSHI_MK(n, p, t, sp, bus, dev, func, reg)  \
		((unsigned int)(((unsigned int)n << 31) | (p << 30) | \
		(t << 29) | (sp << 24) | \
		(bus << 16) | (dev << 11) | (func << 8) | reg))

#define PCI_SPACE_CONFIG		0
#define PCI_SPACE_IO			1
#define PCI_SPACE_MEM			2
#define PCI_SPACE_XMEM			3
#define PCI_SPACE_BUS			4

/* byte offsets to PCI config registers */
#define PCI_CONFIG_ID				0x00
#define PCI_CONFIG_COMMAND			0x04
#define PCI_CONFIG_STATUS			PCI_CONFIG_COMMAND
#define PCI_CONFIG_CLASSREV			0x08
#define PCI_CONFIG_HEADER			0x0C
#define PCI_CONFIG_LATENCY			PCI_CONFIG_HEADER
#define PCI_CONFIG_BIST				PCI_CONFIG_HEADER
#define PCI_CONFIG_CACHE			PCI_CONFIG_HEADER
#define PCI_CONFIG_BAR0				0x10
#define PCI_CONFIG_BAR1				0x14
#define PCI_CONFIG_BAR2				0x18
#define PCI_CONFIG_BAR3				0x1C
#define PCI_CONFIG_BAR4				0x20
#define PCI_CONFIG_BAR5				0x24
#define PCI_CONFIG_CARDBUS			0x28
#define PCI_CONFIG_SUBSYSTEM		0x2C
#define PCI_CONFIG_ROMADDR			0x30
#define PCI_CONFIG_RESERVED_0		0x34
#define PCI_CONFIG_RESERVED_1		0x38
#define PCI_CONFIG_BRIDGE_ROMADDR	0x38
#define PCI_CONFIG_INTERRUPT		0x3C

/* bits in PCI_CONFIG_COMMAND register */
#define PCI_COMMAND_IOENABLE			0x0001
#define PCI_COMMAND_MEMENABLE			0x0002
#define PCI_COMMAND_MASTERENABLE		0x0004
#define PCI_COMMAND_SPECCYCLEENABLE		0x0008
#define PCI_COMMAND_MWIENABLE			0x0010
#define PCI_COMMAND_PALLETESNOOPENABLE	0x0020
#define PCI_COMMAND_PARITYERRENABLE		0x0040
#define PCI_COMMAND_WAITENABLE			0x0080
#define PCI_COMMAND_SYSTEMERRENABLE		0x0100
#define PCI_COMMAND_FASTBACK2BACKENABLE	0x0200

/* bits in PCI_CONFIG_STATUS register */
#define PCI_STATUS_66MHZ				0x0020
#define PCI_STATUS_UDF					0x0040
#define PCI_STATUS_FASTBACK2BACKXFER	0x0080
#define PCI_STATUS_DATAPARITY			0x0100
#define PCI_STATUS_DEVSELTIMING			0x0600
#define PCI_STATUS_SIGTARGETABORT		0x0800
#define PCI_STATUS_RECVTARGETABORT		0x1000
#define PCI_STATUS_RECVMASTERABORT		0x2000
#define PCI_STATUS_SIGSYSTEMERR			0x4000
#define PCI_STATUS_PARITYDETECT			0x8000


/* to be called from main() where currinst is being created */
inline void make_properties(void) { evaluate("\$make-properties"); }
inline void assign_addresses(void) { evaluate("\$assign-addresses"); }

/* to be called from open method where currinst is being opened */
inline char *map_in(int /*lo*/, ... /*...hi, size*/)
		{ asm(" \" map-in\" $call-parent"); }
inline void map_out(char * /*virt*/, int /*size*/) 
		{ asm(" \" map-out\" $call-parent"); }

inline uQuadlet config_getl(int /*addr*/)
		{ asm(" \" config-l@\" $call-parent"); }
inline void config_setl(uQuadlet, int)
		{ asm(" \" config-l!\" $call-parent"); }
inline uDoublet config_getw(int)
		{ asm(" \" config-w@\" $call-parent"); }
inline void config_setw(uDoublet, int)
		{ asm(" \" config-w!\" $call-parent"); }
inline uByte config_getb(int)
		{ asm(" \" config-b@\" $call-parent"); }
inline void config_setb(uByte, int)
		{ asm(" \" config-b!\" $call-parent"); }

/* manage DMA memory */
inline char *dma_alloc(int /*len*/)
		{ asm(" \" dma-alloc\" $call-parent"); }
inline void dma_free(char * /*buf*/, int /*len*/)
		{ asm(" \" dma-free\" $call-parent"); }
inline char *dma_map_in(char * /*virt*/, int /*size*/, int /*cacheable*/)
		{ asm(" \" dma-map-in\" $call-parent"); }
inline void dma_map_out(char * /*virt*/, char * /*devaddr*/, int /*size*/)
		{ asm(" \" dma-map-out\" $call-parent"); }
inline void dma_sync(char * /*virt*/, char * /*devaddr*/, int /*size*/)
		{ asm(" \" dma-sync\" $call-parent"); }

#endif /* __FCPCI_H_ */
