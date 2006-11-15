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

/****************************************************************************
* File Name: EpromData.h
* The file contains the data to be burned into EEPROM.
****************************************************************************/
#ifndef EpromData
#define EpromData

#if 0
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#endif

#define IN 1
#define OUT 0

typedef struct
{
    unsigned short ee_addr;
    unsigned short w_count;
} BLOCK;

int EepromTest(void);

int parse_lines(char **StrArray);  
int burn_it(void);
unsigned short get_ee_addr(char *);
int process_directive(char *s);

int get_chksum_type(char *);
unsigned short get_ee_addr(char *);
unsigned short chksum(unsigned short, unsigned short);
unsigned char LFSR_chksum(unsigned short, unsigned short);
unsigned short word_chksum(unsigned short, unsigned short);
void PnP_chksum(unsigned short, unsigned short);
unsigned short byte_swap(unsigned short);
void readBack(void);
int initEEPROM(void);


unsigned short writeEE(unsigned short ,	unsigned short);
unsigned short readEE(unsigned short);

/* Individual Address to be stored in EEPROM */
unsigned short ia_word[3] = {0x0102,0x0304,0x0506};

/* Serial Number to be stored in EEPROM */
unsigned short sn_word[2] = {0x1234,0x5678};

unsigned short chksum_base;
unsigned short crnt_word;
unsigned short EE_data[80];
unsigned short line_cnt;
unsigned char blk_count;
unsigned char chksum_state;
unsigned char ee_size;
BLOCK blocks[3];

/****************************************************************************
* EepromData contains the data to be burned into EEPROM.
****************************************************************************/
char *EepromData[] =
{
    "$addr=0", 
    "$chksum",
    "A10E            ; 14 bytes follow", 
    "2158",           
    "$ieee           ; 3 words of individual address",
    "0020            ; 1 word at PP_020",
    "0300            ; IO base = 300h",
    "$chksum=b       ; 8-bit checksum on config data",   
    "$addr=1C",
    "$chksum",
    "$ieee   ; 3 words of ieee address",
    "11B0    ; ISA flags (IO Mode,IOCHRDY,UseSA,DMA disable,DMA burst,IRQ10)",
    "0000    ; PacketPage Memory base",
    "0000    ; Boot PROM base",
    "0000    ; Boot PROM mask",
    "0040    ; Transmission Control (ignore missing media)",
    "0021    ; Adapter config (10Base-T circuitry only)",
    "0000    ; Reserved - set to 0",
    "0000    ; Product ID",
    "BF0F    ; Mfg Date - 8.15.95",
    "$ieee   ; 3 words of ieee address",
    "0000    ; Reserved - set to 0",
    "0000    ; Reserved - set to 0",   
    "0000    ; Reserved - set to 0",
    "0000    ; Reserved - set to 0",
    "$chksum=w",
    "$chksum",
    "630E    ; EISA ID (low word)",
    "4000    ; EISA ID (high word)",
    "$snum   ; 2 words of serial number",
    "$chksum=L",
    0	
};

#endif
