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

/* (c) Copyright 1996 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __BEBOX_H_
#define __BEBOX_H_

											/* PCI addr: */
#define MEMORY_BASE			0x00000000ul
#define MEMORY_END			0x7FFFFFFFul
#define ISA_BASE			0x80000000ul	/* 0x00000000 */
#define ISA_END				0x807FFFFFul	/* 0x007F0000 */
#define PCI_CONFIG_BASE		0x80800000ul	/* 0x00800000 */
#define PCI_CONFIG_END		0x80FFFFFFul	/* 0x00FFFFFF */
#define PCI_IO_BASE			0x81000000ul	/* 0x01000000 */
#define PCI_IO_END			0xBF7FFFFFul	/* 0x3F7FFFFF */
#define PCI_ISA_INT_BASE	0xBFFFFFF0ul	/* 0x3FFFFFF0 */
#define PCI_ISA_INT_END		0xBFFFFFFFul	/* 0x3FFFFFFF */
#define PCI_MEMORY_BASE		0xC0000000ul	/* 0x00000000 */
#define PCI_MEMORY_END		0xFEFFFFFFul	/* 0x3EFFFFFF */


#endif /* __BEBOX_H_ */
