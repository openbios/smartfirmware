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
#include "cs8900.h"
#include "cs8900_eprom.h"
#include "ep7312.h"

/*****************************************************************************/
/*
 * File Name: BnchTest.c
 *
 * Purpose: The LoopTest program performs two tests:
 * 1. EEPROM Test: 
 *    -Write and verifies reset and driver configuration blocks.
 *    -Reset the CS8900
 *    -Verifies that the registers read in by the reset configuration block 
 *     are correct.
 * 2. LoopBack Test:
 *    -Sets up the CS8900 registers for loop back.
 *    -Bids for and transmits a frame.
 *    -Polls the receive register for the receive frame
 *    -Verifies that the data is correct.
 *
 * History:
 *   05/26/00 Melody Lee 	Created the program.
 *
 *	 02/20/2001 Michael J. Kelly, Cogent Computer Systems, Inc.
 *              Modified for use with the EDB7312 by setting up CS2.
 */                                                                            
/*****************************************************************************/

/* The following is the basic flow of the loopback test program */

/* This procedure assumes you are using 10B-T.  Use a "loopback" cable (see
the Q/A on loopback cable fabrication).  A regular 10B-T cable connected to
a hub will not work.*/

/* Configure RxCTL for Promiscious mode, RxOK */
/* (1) Write 0x0104 to IOBase+0xA (PacketPage Pointer)
   (2) Write 0x0180h to IOBase+0xC (PacketPage Data Port)*/

/* Configure TestCTL (FDX, DisableLT) */
/* (3) Write 0x0118 to IOBase+0xA (PacketPage Pointer)
   (4) Write 0x4080 to IOBase+0xC (PacketPage Data Port)*/

/* Set 10BaseT, SerRxOn, SerTxOn in LineCTL */
/* (5) Write 0x0112 to IOBase+0xA (PacketPage Pointer)
   (6) Write 0x00C0 to IOBase+0xC (PacketPage Data Port)*/

/* Write the TX command */
/* (7) Write 00C0h to IOBase+0x4 (PacketPage Pointer)*/

/* Write the frame length  (XX = number of bytes to TX) */
/* (8) Write 00XXh to IOBase+0x6 (PacketPage Pointer)*/

/* Read BusST to verify it is set as 0x0118 (Rdy4TxNow is set) */
/* (9) Write 0x0138 to IOBase+0xA (PacketPage Pointer)
   (10) Read IOBase+0xC  Should read 0x0118*/

/* Copy the TX frame to the on-chip buffer */
/* (11) Write XX/2 words to IOBase+0x0 (TX Data Port)

At this point frame should go out on the wire.

To read the TX'd frame:*/

/* Read RxEvent for RxOK (0x0104) */
/* (12) Write 0x0124 to IOBase+0xA (PacketPage Pointer)
   (13) Read IOBase+0xC  Should read 0x0104 or 0x0504h

Read IOBase+0  This will be the RxStatus (RxEvent) word again.
Read IOBase+0 again.  This will be the RxLength (number of bytes received).
Read IOBase+0 (RxLength/2) times to read complete frame.

*/
/*------------------------------------------
Fabricating a "loopback" cable for testing
------------------------------------------

An easy way to fabricate a "loopback" cable for testing is to take a
PCB 10B-T connector and jumper each row of pins together as shown
in the diagram below.  Insert an ordinary 10B-T cable into the connector.
This will connect the TX pair to the RX pair for a loopback cable.

                    +----+----------+----+
                    |    |          |    |
                    |  o====o====o====o  |
                    |    |          |    |
                    |  o====o====o====o  |
                    |    |          |    |
                    |    +----------+    |
                    |                    |
                    \____________________/


                         Bottom View

*/

#define IO_BASE			CS8900A_BASE
#define NO_FRAMES_TO_SEND	10
#define SIZE_OF_FRAME		100
#define WORD_DATA		0x1234

int entry(void);
void reset(void);
int LoopBackTest(void);

static void
outport(uInt addr, uShort val)
{
	__asm("	strh	%0, [%1, #0]" : : "r"(val), "r"(addr));
}

static uShort
inport(uInt addr)
{
	int val;
	__asm("	ldrh	%0, [%1, #0]" : "=r"(val) : "r"(addr));
	return val & 0xFFFF;
}

/**************************************************************************
*  main()
**************************************************************************/
int
ethernet_test(void)
{

    unsigned long *pulPtr = (unsigned long *)HwBaseAddress;
    unsigned long test;

    // Initialize CS2 for 16-bit 4 wait states.
    // start by clearing the CS2 field
    pulPtr[HwMemConfig1 >> 2] &= 0xff00ffff;

    // now get the boot width and set CS4 accordingly
    // boot width is in bits 27 and 28 of HwStatus
    test = (pulPtr[HwStatus >> 2] >> 27) & 0x3;		// boot width is in bits 27 and 28 of HwStatus

    switch(test)
    {
    case 0:	// we booted in 32-bit mode
	pulPtr[HwMemConfig1 >> 2] |= (0x00900000 | (1 << 16)); 
	break;
    case 1:	// we booted in 8-bit mode
	pulPtr[HwMemConfig1 >> 2] |= (0x00900000 | (3 << 16)); 
	break;
    case 2:	// we booted in 16-bit mode
	pulPtr[HwMemConfig1 >> 2] |= (0x00900000 | (0 << 16)); 
	break;
    }

    EepromTest();
    LoopBackTest();

    return 0;
}

/**************************************************************************
*  LoopBackTest()
**************************************************************************/
int LoopBackTest( void )
{
    int i;
    int j;
    unsigned short receive_frame_size;
    unsigned short temp;
    int errs = 0;

    cprintf(g_e, "Begin LoopBack test..............\n");

    // Initialize the part
    //
    // Configure RxCTL for Promiscious mode, RxOK 
    // (1) Write 0x0104 to IOBase+0xA (PacketPage Pointer)
    // (2) Write 0x0180h to IOBase+0xC (PacketPage Data Port)
    //
    outport(IO_BASE + PP_POINTER, 0x0104);
    outport(IO_BASE + PP_DATA, 0x0180);

    //
    // Configure TestCTL (FDX, DisableLT, ENDEC loop ) */
    // (3) Write 0x0118 to IOBase+0xA (PacketPage Pointer)
    // (4) Write 0x4080 to IOBase+0xC (PacketPage Data Port)*/
    //
    outport(IO_BASE + PP_POINTER, 0x0118);
    outport(IO_BASE + PP_DATA, 0x4000);

    //
    // Set 10BaseT, SerRxOn, SerTxOn in LineCTL 
    // (5) Write 0x0112 to IOBase+0xA (PacketPage Pointer)
    // (6) Write 0x00C0 to IOBase+0xC (PacketPage Data Port)
    //
    outport(IO_BASE + PP_POINTER, 0x0112);
    outport(IO_BASE + PP_DATA, 0x00c0);

    for (i = 0; i < NO_FRAMES_TO_SEND ; i++)
    {
	//
	// Write the TX command 
	// (7) Write 00C0h to IOBase+0x4 
        //
	outport(IO_BASE + TX_CMD, 0x00c0);

	//
	// Write the frame length  (XX = number of bytes to TX)
	//(8) Write 00XXh to IOBase+0x6 
	//
	outport(IO_BASE + TX_LENGTH, SIZE_OF_FRAME * 2); 

	// 
	//(9) Write 0x0138 to IOBase+0xA (PacketPage Pointer)
	//
	outport(IO_BASE + PP_POINTER, 0x0138);

	//
	// delay for a while
	//
	u_sleep(3000);

	//
	// (10) Read IOBase+0xC,  Should read 0x0118 to indicate Ready
	// for a transfer now.
	//  
	if ((temp = inport(IO_BASE + PP_DATA)) !=  0x0118)
	{
	    cprintf(g_e, "Bid for transmit failed\n");
	    cprintf(g_e, "Register PP_DATA = %04x", temp);
	    return FAILURE;
	}

	//
	// Copy the TX frame to the on-chip buffer 
	// (11) Write XX/2 words to IOBase+0x0 (TX Data Port) 
	for (j = 0; j < SIZE_OF_FRAME; j++)
	    outport(IO_BASE, WORD_DATA);

	// delay for a while
	u_sleep(20000);

	//
	// Read RxEvent for RxOK (0x0104) 
	// (12) Write 0x0124 to IOBase+0xA (PacketPage Pointer)
	// (13) Read IOBase+0xC  Should read 0x0104h or 0x0504h
	//
	outport(IO_BASE + PP_POINTER, 0x0124);

	if ((temp = inport(IO_BASE + PP_DATA)) != 0x0104)
	{
	    cprintf(g_e, "Frame not received\n");
	    cprintf(g_e, "Register 0x0124 PP_DATA = %04x\n", temp);
#ifdef RESET_CHIP
	    reset();
#endif
	    return FAILURE;
	}

	//
	// Read IOBase+0  This should be the RxStatus (RxEvent) word again.
	//
	if ((temp = inport(IO_BASE)) != 0x0104)
	{
	    cprintf(g_e, "Frame not received -- reading data port\n");
	    cprintf(g_e, "Register RxStatus = %04x\n", temp);
#ifdef RESET_CHIP
	    reset();
#endif
	    return FAILURE;
	}

	//
	// Read IOBase+0 again.  This should be the RxLength (number of bytes received).
	//
	receive_frame_size = inport(IO_BASE);
	if (SIZE_OF_FRAME < 30)  /*size in words, i.e. pad characters added*/
	    if (receive_frame_size != 60)  /* size in bytes because of pad characters*/
	    {
		cprintf(g_e, "Frame received is not the right size < 60\n");
		cprintf(g_e, "receive_frame_size = %04x\n", receive_frame_size);
#ifdef RESET_CHIP
		reset();
#endif
		return FAILURE;
	    }

	if (SIZE_OF_FRAME >= 30)
	    if (receive_frame_size != (SIZE_OF_FRAME * 2))
	    {
		cprintf(g_e, "Frame received is not the right size >= 60\n");
		cprintf(g_e, "receive_frame_size = %04x ; Should be %04x\n", 
			receive_frame_size, SIZE_OF_FRAME * 2);
#ifdef RESET_CHIP
		reset();
#endif
		return FAILURE;
	    }

	//
	// Read IOBase+0 (RxLength/2) times to read complete frame.
	//
	for (j = 0; j < SIZE_OF_FRAME; j++)
	{
	    if ((temp = inport(IO_BASE)) != WORD_DATA)
	    {
		if (!errs)
		    cprintf(g_e, "Frame data does not match what was sent\n");

		cprintf(g_e, "Received Data  = %04x, j = %d\n", temp, j);
		errs++;
#ifdef RESET_CHIP
		reset();
		return FAILURE;
#endif
	    }
	}

	if (SIZE_OF_FRAME < 30) /* need to issue a skip to flush pad chars*/
	{
	    outport(IO_BASE + PP_POINTER, 0x0102);
	    temp = inport(IO_BASE + PP_DATA);
	    outport(IO_BASE + PP_DATA, temp | SKIP);
	}

	if (errs)
	    break;
    }

#ifdef RESET_CHIP
    reset();
#endif
    if (!errs)
    {
	cprintf(g_e, "Loopback Test Completed Successfully!\n");
	return SUCCESS;
    }

    return FAILURE;
}

/**************************************************************************
*  reset()
**************************************************************************/
void
reset()
{
    unsigned short temp;

    //
    // Reset the CS8900A device
    //
    outport(IO_BASE + PP_POINTER, 0x0114);
    outport(IO_BASE + PP_DATA, 0x0040);

    //
    // delay for a while
    //
    u_sleep(3000);

    //
    // Wait for reset complete
    //
    for (;;)
    {
	outport(IO_BASE + PP_POINTER, 0x0136);

	if (((temp = inport(IO_BASE + PP_DATA)) & 0x0080) != 0)
	    break;
    }
}

/**************************************************************************
*  EepromTest()
**************************************************************************/
int
EepromTest(void)
{
    cprintf(g_e, "Reset CS8900.\n");
    reset();

    cprintf(g_e, "Begin EEPROM test...........\n");

    if (initEEPROM() == FAILURE)
	return FAILURE;

    if (parse_lines(EepromData) == FAILURE)
	return FAILURE;

    cprintf(g_e, "Write data into EEPROM\n");

    if (burn_it() != SUCCESS)
	return FAILURE;

    cprintf(g_e, "Reset CS8900.\n");
    reset();

    cprintf(g_e, "Read back EERPOM data and verify it.\n");
    readBack();

    return SUCCESS;
}

/**************************************************************************
 * initEEPROM(): 
 *************************************************************************/
int
initEEPROM()
{
    unsigned short temp;

    //
    // Check the SelfST register, bit 9, for EEPROM present.
    //
    outport(IO_BASE + PP_POINTER, 0x0136);
    temp = inport(IO_BASE + PP_DATA);

    if ((temp & 0x0200) == 0)
    {
	cprintf(g_e, "CS8900 - EEPROM not present.\n");
	return FAILURE;
    }

    cprintf(g_e, "CS8900 - EEPROM present.\n");

    //
    // Check Self Status register, bit 10, for checksum error.
    // 1 = Checksum ok, 0 = checksum error
    //
    if ((temp & 0x0400) == 0)
	cprintf(g_e, "EEPROM CheckSum Error.\n");
    else
	cprintf(g_e, "EEPROM CheckSum Correct.\n");

    return SUCCESS;
}

void
waitbusyEE()
{
    unsigned short temp;

    //
    // Wait until SIBUSY is clear. Check bit 8 of Self Status register.
    // 1 = busy, 0 = not busy
    //

    for (;;)
    {
	outport(IO_BASE + PP_POINTER, 0x0136);

	if (((temp = inport(IO_BASE + PP_DATA)) & 0x0100) == 0)
	    break;
    }

    cprintf(g_e, "Waitbusy: temp = %#x\n", temp);
}

/**************************************************************************
 * ReadEE(): 
 * Returns word from EEPROM from address EE_word_offset
 *************************************************************************/
unsigned short
readEE(unsigned short EE_word_offset)
{	
    waitbusyEE();

    //
    // Send EEPROM command
    //
    outport(IO_BASE + PP_POINTER, 0x0040);
    outport(IO_BASE + PP_DATA, 0x0200 + EE_word_offset);
    waitbusyEE();

    //
    // delay for a while
    //
    u_sleep(3000);

    //
    // Read data from EEPROM and return.
    //
    outport(IO_BASE + PP_POINTER, 0x0042);
    return inport(IO_BASE + PP_DATA);
}


/**************************************************************************
 * WriteEE(): 
 * Writes outword to the EEPROM @ address EE_Word_offset.  Returns a
 * read back of the word just written.
 *************************************************************************/
unsigned short
writeEE(unsigned short EE_word_offset, unsigned short outword)
{
    waitbusyEE();

    //
    // Enable-write command
    //
    outport(IO_BASE + PP_POINTER, 0x0040);
    outport(IO_BASE + PP_DATA, 0x00F0);
    waitbusyEE();

    // Write the outWord Value to EEPROM
    outport(IO_BASE + PP_POINTER, 0x0042);
    outport(IO_BASE + PP_DATA, outword);

    // 
    // Write command
    //
    outport(IO_BASE + PP_POINTER, 0x0040);
    outport(IO_BASE + PP_DATA, 0x0100 + EE_word_offset);
    waitbusyEE();

    // 
    // Disable-write command
    //
    outport(IO_BASE + PP_POINTER, 0x0040);
    outport(IO_BASE + PP_DATA, 0x0000);
    waitbusyEE();

    //
    // Read back what is written command
    //
    outport(IO_BASE + PP_POINTER, 0x0040);
    outport(IO_BASE + PP_DATA, 0x0200 + EE_word_offset);
    waitbusyEE();

    //
    // Delay for a while
    //
    u_sleep(3000);

    // 
    // Read back and return word
    //
    outport(IO_BASE + PP_POINTER, 0x0042);
    return inport(IO_BASE + PP_DATA);
}

unsigned long
strtoul(char *str, char **end, int base)
{
    unsigned long x = 0;

    if ((base == 16 || base == 0) && str[0] == '0' &&
	    (str[1] == 'x' || str[1] == 'X'))
    {
	str += 2;
	base = 16;
    }
    else if (base == 0 && str[0] == '0')
	base = 8;
    else if (base == 0)
	base = 10;

    while (*str != '\0')
    {
	int digit;

	if (str[0] >= '0' && str[0] <= '9')
	    digit = str[0] - '0';
	else if (str[0] >= 'a' && str[0] <= 'z')
	    digit = str[0] - 'a' + 10;
	else if (str[0] >= 'A' && str[0] <= 'Z')
	    digit = str[0] - 'A' + 10;
	else
	    break;

	if (digit >= base)
	    break;

	x = (x * base) + digit;
	str++;
    }

    if (end)
	*end = str;

    return x;
}


/**************************************************************************
*  parse_lines()
**************************************************************************/
int
parse_lines(char **StrArray)
{
    char *line;
    char *stopstring;
    int c;
    int i;

    line_cnt = 0;
    crnt_word = 0;
    blk_count = 0;

    while (StrArray[line_cnt] != 0)
    {
	line = StrArray[line_cnt];
	line_cnt++;

	for(i = 0; isspace(c = line[i]) && c != '\n'; i++)
	    ;  /* skip white space */

	if (line[i] == '\n')
	    continue;  /* stop processing if end of line */

	if (line[i] == ';')
	    continue; /* ignore rest of line -- it's a commnet */

	if (line[i] == '$')
	{
	    if (process_directive(&line[i + 1]) == FAILURE)
		return FAILURE;

	    continue;
	}

	if (!isxdigit(line[i]))
	{
	    cprintf(g_e, "Error in script file input line %d\n",line_cnt);
	    return FAILURE;
	}

	if (!blk_count)
	{
	    cprintf(g_e, "Error: data word before address specified\n");
	    return FAILURE;
	}

	EE_data[crnt_word++] = strtoul(&line[i], &stopstring, 16);
	blocks[blk_count - 1].w_count += 1;
    }

    return SUCCESS;
}

/**************************************************************************
*  process_directive(()
**************************************************************************/
int
process_directive(char *s)
{
    int c;
    int t;

    c = s[0];
#if 0
    cprintf(g_e, "process_directive: s %#x (%s), c = '%c'\n", s, s, c);
#endif

    //
    // c is the first character of the command 
    //
    switch (c)
    {
    case 'A': case 'a':
	//
	// 'A' is a command to get the address
	//
	blocks[blk_count].ee_addr = get_ee_addr(s);
	blocks[blk_count].w_count = 0;
	blk_count++;
	break;
    case 'C': case 'c':
	//
	// 'C' is a checksum command
	//

	//
	// Find out what type of checksum
	//
	t = get_chksum_type(s);

	if (!t)
	{ 
	    //
	    // Else, returned NULL so it's a start tag
	    if (chksum_state == IN)
	    {
		cprintf(g_e, "Error in script file input line %d\n",line_cnt);
		return FAILURE;
	    }

	    chksum_state = IN;
	    chksum_base = crnt_word;
	    break;
	}
    
	//
	// Call the appropriate checksum routine based on the checksum command.
	//
	switch (t)
	{
	case 'B': case 'b':
	    EE_data[crnt_word] = chksum(chksum_base, crnt_word - chksum_base);
	    blocks[blk_count - 1].w_count += 1;
	    crnt_word++;
	    chksum_state = OUT;
	    break;
	case 'W': case 'w':
	    EE_data[crnt_word] = word_chksum(chksum_base, crnt_word - chksum_base);
	    blocks[blk_count - 1].w_count += 1;
	    crnt_word++;
	    chksum_state = OUT;
	    break;
	case 'L': case 'l':
	    EE_data[crnt_word] = 0x0A00 ^ LFSR_chksum(chksum_base, crnt_word - chksum_base);
	    blocks[blk_count - 1].w_count += 1;
	    crnt_word++;
	    chksum_state = OUT;
	    break;
	case 'P': case 'p':
	    PnP_chksum(chksum_base, crnt_word - chksum_base);
	    chksum_state = OUT;
	    break;
	default:
	    cprintf(g_e, "$CHKSUM command not recognized: line %d\n",line_cnt);
	    return FAILURE;
	}

	break;
    case 'I': case 'i':
	EE_data[crnt_word] = byte_swap(ia_word[0]);
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
	EE_data[crnt_word] = byte_swap(ia_word[1]);
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
	EE_data[crnt_word] = byte_swap(ia_word[2]);
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
	break;
    case 'S': case 's':
	EE_data[crnt_word] = sn_word[1];
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
	EE_data[crnt_word] = sn_word[0];
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
	break;
    default:
	cprintf(g_e, "Error in script file input line %d\n",line_cnt);
	return FAILURE;
    }

    return SUCCESS;
}

char *
strpbrk(char *s, char *set)
{
    int ch;
    int i;

    for (; (ch = *s) != '\0'; s++)
	for (i = 0; set[i]; i++)
	    if (ch == set[i])
		    return s;

    return (char *)0;
}

/**************************************************************************
*  get_chksum_type()
**************************************************************************/
int
get_chksum_type(char *s)
{
    char *c = strpbrk(s,"=");

    //
    // look for = sign -- means it's an end tag
    //
    if (c)
    {   
	c = strpbrk(s, "bBwWlLpP");

	//
	// Return chksum type
	//
	return c[0];
    }

    //
    // "=" sign not found -- assume it's a start tag
    //
    return 0;             
}

/**************************************************************************
*  get_ee_addr()
**************************************************************************/
unsigned short
get_ee_addr(char *s)
{
   int i;

   //
   // Move past directive text
   //
   for (i=0; s[i] != '=' && s[i] != '\0'; i++)
	;

   //
   // Move to first digit
   //
   i++;
   
   while (isspace(s[i]))
	i++;

   return strtoul(&s[i], (char **)0, 16);
}

/**************************************************************************
 * Checksum():
 *
 * Returns a checksum "word" based on all "bytes" in a block.  Blk_lenght is
 * actual number of bytes to sum.  Checksum is returned in high byte of word.
 **************************************************************************/
unsigned short
chksum(unsigned short base, unsigned short word_cnt)
{
    int i;
    unsigned short sum = 0;

    for(i=0; i < word_cnt; i++)
    {
	sum += (EE_data[base] & 0xFF) + (EE_data[base] >> 8);
	++base;
    }

    sum = ~sum + 1;
    return sum << 8;
}


/**************************************************************************
*  PnP_chksum()
**************************************************************************/
void
PnP_chksum(unsigned short base, unsigned short word_cnt)
{
    int i;
    unsigned short sum = 0;

    //
    // don't calc with word that has end tag
    //
    word_cnt--;

    for(i = 0; i < word_cnt; i++)
    {
	sum += (EE_data[base] & 0xFF) + (EE_data[base] >> 8);
	base++;
    }

    //
    // Add byte following the LFSR checksum and end tag
    //
    sum = sum + 0xA + 0x79;   
    if ((EE_data[crnt_word - 1] & 0xFF) == 0x79)
    {
	/* end tag in low byte */
	sum = ~sum + 1;
	i = (sum << 8) ^ 0x79; /* sum79 */
	EE_data[crnt_word - 1] =  i; /* put sum in high byte of prev word */
    }
    else
    { 
	//
	// Else, Assume end tag in high byte of prev word
	//
	sum = sum + (EE_data[crnt_word - 1] & 0xFF);  /* add the low byte */
	sum = ~sum + 1;
	EE_data[crnt_word] = sum & 0xFF;  /* put in low byte of crnt word -- pad with 00 in hb */
	blocks[blk_count - 1].w_count += 1;
	crnt_word++;
    }
}

/**************************************************************************
* ord_chksum Routine
* Returns a checksum word based on all 16-bit words in a block of data.
***************************************************************************/
unsigned short
word_chksum(unsigned short base, unsigned short word_cnt)
{
    int i;
    unsigned short sum = 0;

    for(i = 0; i < word_cnt; i++)
	sum += EE_data[base++];

    return sum = ~sum + 1;
}

/**************************************************************************
* LFSR_chksum Routine
* Returns 8-bit Linear Feedback Shift Register checksum on all bytes 
* in a block of 16-bit words.  Words are assumed to be organized 
* little-endian.
***************************************************************************/
unsigned char
LFSR_chksum(unsigned short base, unsigned short word_cnt)
{
    unsigned char LFSR;
    int byte_ctr;
    unsigned char bits;
    unsigned char XORvalue;
    unsigned char byte;
    int i;
    int j = 0;
    unsigned char bytes[0xFF];

    for(i = 0; i < word_cnt; i++)
    {
	bytes[j++] = EE_data[base]  & 0xFF; /* convert words to byte stream */
	bytes[j++] = EE_data[base++] >> 8;
    }

    LFSR = 0x6A;
    word_cnt = word_cnt << 1; 		/* adjust word count to bytes */

    for (byte_ctr = 0; byte_ctr < word_cnt; byte_ctr++)
    {
	byte = bytes[byte_ctr];

	for (bits = 0; bits < 8; bits++)
	{
	    XORvalue = ((LFSR & 3) == 1 || (LFSR & 3) == 2) ?  1 : 0;
	    LFSR = LFSR >> 1;

	    if (XORvalue ^ (byte & 1))
		LFSR |= 0x80;

	    byte = byte >> 1;
	}
    }

    return LFSR;
}

/**************************************************************************
*  byte_swap()
**************************************************************************/
unsigned short
byte_swap(unsigned short word)
{
   //
   // Byte swap the word
   //
   return (word << 8) + (word  >> 8);
}


/**************************************************************************
*  burn_it()  Burn into EEPROM
**************************************************************************/
int
burn_it(void)
{
    int i, j, k;
    unsigned short temp;

    crnt_word = 0;

    for (i = 0; i < blk_count; i++)
    {
	for(j = 0; j < blocks[i].w_count; j++)
	{
	    k = blocks[i].ee_addr + j;
	    temp = writeEE(k,  EE_data[crnt_word]);

	    if (temp != EE_data[crnt_word])
	    {
		cprintf(g_e, "Error! Burn_it(): EE_Addr=%d, ReadBackData=%x, ShouldBe=%x\n",
			k,temp,EE_data[crnt_word]);
		return FAILURE;
	    }

	    crnt_word++;
	}
    }

    return SUCCESS;
}

/**************************************************************************
*  readBack()
**************************************************************************/
void
readBack(void)
{
    int i, j, k;
    unsigned short temp;

    crnt_word = 0;

    for (i = 0; i < blk_count; i++)
    {
	for(j = 0; j < blocks[i].w_count; j++) 
	{
	    k = blocks[i].ee_addr + j;
	    temp = readEE(k);

	    if (temp != EE_data[crnt_word])
	    {
		cprintf(g_e, "\nError: EEPROM test fail! \n");
		cprintf(g_e, "Error! ReadBack(): EE_Addr=%d, ReadBackData=%x, ShouldBe=%x\n\n", 
			k,temp,EE_data[crnt_word]);
		return;
	    }

	    crnt_word++;
	}
    }

    cprintf(g_e, "EEPROM Test Completed Successfully! \n");
    return ;
}
