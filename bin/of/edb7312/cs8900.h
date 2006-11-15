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


/* Defines for CS8900A */

#ifndef __CS8900A_H__
#define __CS8900A_H__

#define CS8900A_BASE 		0x20000000

/* Address translation for 73xx cogent board */
#define TX_CMD 			0x0004
#define TX_LENGTH 		0x0006
#define PP_POINTER 		0x000A
#define PP_DATA 		0x000C
#define SKIP 			0040

#define SUCCESS 		1
#define FAILURE 		0

#define PP_EISA_SIG		0x0000
#define ID_EISA_SIG		0x630E
#define PP_CHIPID		0x0002
#define ID_CS8900		0x0000
#define ID_CS8920		0x4000
#define ID_CS8920M		0x6000
#define PP_IOBASE		0x0020
#define PP_INTNUM		0x0022
#define PP_DMACHNUM		0x0024
#define PP_RXCFG		0x0102
#define PP_RXCTL		0x0104
#define PP_TXCFG		0x0106
#define PP_TXCMD		0x0108
#define PP_BUFCFG		0x010A
#define PP_LINECTL		0x0112
#define PP_SELFCTL		0x0114
#define PP_BUSCTL		0x0116
#define PP_TESTCTL		0x0118
#define PP_SELFST		0x0136

#endif /* __CS8900A_H__ */
