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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __MACHDEP_H_
#define __MACHDEP_H_

/* machdep.h for Peritek AGP graphics board with MIPS R4700.
*/


/* macros which control the switching on of certain features: */

/* If your machine is little-endian, and system header files do not define
   this macro, it MUST be defined here (or in the Makefile).  The default
   is big-endian, such as MC680x0 */
/*#define LITTLE_ENDIAN*/		/* little-endian byte-ordering, such as i386 */

/* define the __LONGLONG macro if your compiler supports "long long"
   or some other type of 64-bit integers */
/*#define __LONGLONG*/

/* define the macro SF_64BIT if you want Cells to be 64-bits, which may break
   the embedded FCode in some current plug-in cards */
/*#define SF_64BIT*/

/* this describes which font to be used as the default system font */
/*#define LARGE_FONT*/		/* use 16x23 font instead of smaller 8x16 font */

/* define this macro to turn on a very basic debugging malloc, mostly for
   standalone use */
#define DEBUG_MALLOC

/* define this macro to run SmartFirmware as a standalone application on
   a host */
/*#define STANDALONE*/


/* macros which define certain important values */

/* this defines the maximum amount of memory to be allocated for use by
   malloc - all subsequent memory requests are through malloc, including
   for Forth environment */
#define MALLOC_POOL		(512 * 1024)	/* total number of bytes available
											for everything */

/* these macros specify size limits on various data structures - again,
   these are fairly conservative and are unlikely to need to be changed */
#define STR_SIZE		256		/* max string length - minimum value required
									by OF spec is 256 */
#define MEM_SIZE		(MALLOC_POOL / 2)	/* total number of bytes malloced
												for Forth environment */
#define STACK_SIZE		128		/* max size of data stack */
#define RET_STACK_SIZE	64		/* max size of return stack */
#define MAX_ADDR_CELLS	4		/* max value for "#address-cells" property -
									min reqired by OF spec is 4 */
#define MAX_ALARMS		16		/* maximum number of alarms that may be
									pending at one time */

/* these identify the firmware - no commas are allowed in the following
   strings */
#define MANUFACTURER	"CodeGenInc"	/* name of manufacturer */
#define MACHINE_TYPE	"BeBox"			/* machine-specific name */
#define FIRMWARE_REV	"1.00"			/* current version of firmware */
#define STANDARD_REV	0x00030000		/* currently version 3.0 of the
										OpenFirmware standard (0x0003.0000) */

/* for any builtin Ethernet, the MAC hardware address should be defined here
   or pointed to some static buffer that is filled in during initialization */
#define MAC_ADDRESS		"Bogus!"		/* 6-bytes, or 48-bits */


/* These should be a newline-terminated string containing any
   machine-dependent help for the "help" word and the title. Help should
   be no more than 23 lines of text. */
#define MACHDEP_HELP_TITLE	"bebox"
#define MACHDEP_HELP	\
	"CodeGen, Inc. BeBox version of SmartFirmware(tm).\n" \
	"This area under construction.\n"


/* declare a bunch of types for particular sizes of integers, their alignment
   requirements and their matching bitmasks */

typedef char Char;				/* at least 8-bit signed integer/character */
typedef unsigned char uChar;	/* at least 8-bit unsigned integer/character */
typedef short Short;			/* at least 16-bit signed integer */ 
typedef unsigned short uShort;	/* at least 16-bit unsigned integer */
typedef long Int;				/* at least 32-bit signed integer */
typedef unsigned long uInt;		/* at least 32-bit unsigned integer - size_t */
#ifdef __LONGLONG
typedef __LONGLONG Long;				/* at least 32-bit (preferably 64-bit)
										signed integer */
typedef unsigned __LONGLONG uLong;	/* at least 32-bit (preferably 64-bit)
										unsigned integer */
#else
typedef long Long;				/* at least 32-bit (preferably 64-bit)
										signed integer */
typedef unsigned long uLong;	/* at least 32-bit (preferably 64-bit)
										unsigned integer */
#endif
typedef long Ptr;				/* must be able to hold any pointer */
typedef unsigned long uPtr;		/* must be able to hold any pointer */
typedef int Bool;		/* fastest integer on machine - not much reason
							to change this */

/* Forth types below - depend upon the definitions above */
typedef Char Byte;		/* a Forth byte - 8-bits */
typedef uChar uByte;	/* an unsigned Forth byte - 8-bits - used for
							pointers to memory */
typedef Short Word;		/* a Forth word - 16-bits */

#ifdef SF_64BIT
#ifndef __LONGLONG
#  error __LONGLONG must be defined and type Long must be a 64-bit integer.
#endif
typedef Long Cell;		/* a Forth cell - at least 64-bits, also must
							hold any pointer */
typedef uLong uCell;	/* unsigned Forth cell */
#else
typedef Int Cell;		/* a Forth cell - must be able to hold any pointer */
typedef uInt uCell;		/* unsigned Forth cell */
#endif

typedef Short Doublet;		/* Forth doublet - 16-bits */
typedef uShort uDoublet;	/* unsigned Forth doublet - 16-bits */
typedef Int Quadlet;		/* Forth quadlet - 32-bits */
typedef uInt uQuadlet;		/* unsigned Forth quadlet - 32-bits */

/* these specify alignment requirements for pointers to the specified types -
   the defaults are conservative and should work fine on many hosts */
#define ALIGNMENT	(sizeof (Cell))			/* align Cell* to this */
#define QALIGNMENT	(sizeof (Quadlet))		/* align Quadlet* to this */
#define DALIGNMENT	(sizeof (Doublet))		/* align Doublet* to this */
#define BALIGNMENT	(sizeof (Byte))			/* align Byte* to this */

/* these macros make assumptions about the sizes selected above and must
   be correct! */
#define BYTE_SIZE		8				/* size of a Byte */
#define BYTE_MASK		0xFF			/* a Byte with all bits turned on */
#define DOUBLET_SIZE	16				/* size of a Doublet */
#define DOUBLET_MASK	0xFFFF			/* a Doublet with all bits turned on */
#define QUADLET_SIZE	32				/* size of a Quadlet */
#define QUADLET_MASK	0xFFFFFFFF		/* a Quadlet with all bits turned on */

#ifdef __LONGLONG
typedef Long Octlet;		/* Forth octlet - two Quadlets, or 64-bits */
typedef uLong uOctlet;		/* Forth octlet - two Quadlets, or 64-bits */
#define OALIGNMENT	(sizeof (Octlet))		/* align Octlet* to this */
#define OCTLET_SIZE	64				/* size of a Quadlet */
#define OCTLET_MASK	(uOctlet)0xFFFFFFFFFFFFFFFF		/* an Octlet with all
														bits turned on */
#endif


#ifdef LARGE_FONT
#define FONT_FILE		"cour16x23.font"
#define FONT_WIDTH		16
#define FONT_HEIGHT		25		/* add one for the 1st zero row plus
									one for last virtual zero row */
#define FONT_ADVANCE	2
#define FONT_FIRST		' '
#define FONT_LAST		'~'
#define FONT_COUNT		(FONT_LAST - FONT_FIRST + 1)
#endif

#ifndef FONT_FILE
#define FONT_FILE		"cour8x16.font"
#define FONT_WIDTH		8
#define FONT_HEIGHT		18		/* add one for the 1st zero row plus
									one for last virtual zero row */
#define FONT_ADVANCE	1
#define FONT_FIRST		' '
#define FONT_LAST		'~'
#define FONT_COUNT		(FONT_LAST - FONT_FIRST + 1)
#endif


/* platform-specific stuff goes here */
#define CPU0_INTR_MASK		(*(uInt*)0x7FFFF0F0ul)
#define CPU1_INTR_MASK		(*(uInt*)0x7FFFF1F0ul)
#define INTR_SOURCES		(*(uInt*)0x7FFFF2F0ul)
#define PROC_INTR_ID		(*(uInt*)0x7FFFF3F0ul)
#define PROC_RESETS			(*(uInt*)0x7FFFF4F0ul)

/* bit 6 in PROC_INTR_ID to identify processor: 0==CPU0 1==CPU1 */
#define CPIR_ID				0x02000000ul

extern void do_read(uInt addr, void *value, int size);
extern void do_write(uInt addr, Int value, int size);


#endif /* __MACHDEP_H_ */
