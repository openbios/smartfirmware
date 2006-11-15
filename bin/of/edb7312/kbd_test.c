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

// Diagnostic Test for 6542B Keyboard/Mouse Controller
//
// This will test the interface to the Holtek 8542B by sending a series of
// commands and checking the responses.  If it detects a Keyboard it will
// also go into a test loop.  In this loop all keypresses will be interpreted
// and information will be written to the LCD display.
//

/*#define DEBUG*/

// Includes
#include "defs.h" 
#include "6542b.h"
#include "ep7312.h"

#define HT6542_BASE	EDB7312_VA_PS2				// EDB7312 CS4 + A16 = 1
#define HT6542_CMD	0x01
#define HT6542_STAT	0x01
#define HT6542_DAT	0x00

// Miscellaneous defines and function protos
#define MSECWAIT	800						// number of times around 'while' for each millisec
#define HT6542_MAX_RESENDS	20
#define HT6542_MAX_WAIT		100000

static void noop(Environ *e, char *fmt, ...) { }
#ifdef DEBUG
#define xxcprintf	cprintf	
#else
#define xxcprintf	noop
#endif


// global pointers - defined here since we use them in 
// several functions
static unsigned long *pulPtr = (unsigned long *)EDB7312_VA_EP7312;
static unsigned char *pucPtr = (unsigned char *)HT6542_BASE;

// prototype declarations
int main(void);
unsigned char kbd_read(void);
void kbd_write(unsigned char);
void check_obf(void);
void flush_obf(void);
void check_ibf(void);
unsigned char send_kbd_cmd(unsigned char);

int test_keyboard()
{
	unsigned char data;
	volatile unsigned char test, test1;
	unsigned long testl;

	// set up CS4 for 8-bits, 4 wait states
	// start by clearing the CS4 field
	pulPtr[HwMemConfig2 >> 2] &= 0xffffff00;

	// now get the boot width and set CS4 accordingly
	testl = (pulPtr[HwStatus >> 2] >> 27) & 0x3;		// boot width is in bits 27 and 28 of HwStatus
	switch(testl)
	{
		case 0:	// we booted in 32-bit mode
			pulPtr[HwMemConfig2 >> 2] |= (0x00000090 | 2); 
			break;

		case 1:	// we booted in 8-bit mode
			pulPtr[HwMemConfig2 >> 2] |= (0x00000090 | 0); 
			break;

		case 2:	// we booted in 16-bit mode
			pulPtr[HwMemConfig2 >> 2] |= (0x00000090 | 3); 
			break;
	}

	// flush any pending data
	flush_obf();

	// see if we can run the controller self test
	xxcprintf(g_e, "Starting 6542 Internal Diagnostic Test\n");
	pucPtr[HT6542_CMD] = HT6542_CMD_DIAG;

	// check the status for completion
	// wait until the ibf flag goes false
	check_ibf();

	// now see if we have output data
	check_obf();
	data = pucPtr[HT6542_DAT];
	if(data == KBD_STAT_DIAG_OK)
	{
		xxcprintf(g_e, "HT6542B Internal Diagnostic Test Passed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
	}
	else
	{
		xxcprintf(g_e, "HT6542B Internal Diagnostic Test Failed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
		return -1;
	}

	// next we test the keyboard and mouse ports
	xxcprintf(g_e, "Starting HT6542B Keyboard Port Test\n");
	check_ibf();
	pucPtr[HT6542_CMD] = HT6542_CMD_KBD_TEST;
		
	// check the status for completion
	// wait until the ibf flag goes false
	check_ibf();

	// now see if we have output data
	check_obf();

	data = pucPtr[HT6542_DAT];
	if(data == 0)	// anything but zero is a failure
	{
		xxcprintf(g_e, "HT6542B Keyboard Port Test Passed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
	}
	else
	{
		xxcprintf(g_e, "HT6542B Keyboard Port Test Failed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
	}

	xxcprintf(g_e, "Starting HT6542B Mouse Port Test\n");
	check_ibf();
	pucPtr[HT6542_CMD] = HT6542_CMD_AUX_TEST;
		
	// check the status for completion
	// wait until the ibf flag goes false
	check_ibf();

	// now see if we have output data
	check_obf();
	data = pucPtr[HT6542_DAT];
	if(data == 0)	// anything but zero is a failure
	{
		xxcprintf(g_e, "HT6542B Mouse Port Test Passed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
	}
	else
	{
		xxcprintf(g_e, "HT6542B Mouse Port Test Failed\n");
		xxcprintf(g_e, "Data Port = 0x%02x\n",data);
	}

	// next enable interrupts
	xxcprintf(g_e, "Enable Interrupts\n");
	check_ibf();
	pucPtr[HT6542_CMD] = HT6542_CMD_SET_BYTE;
	check_ibf();

	// send the interrupt enable byte
	pucPtr[HT6542_CMD] = (HT6542_CMD_BYTE_AUX_INT | HT6542_CMD_BYTE_KBD_INT);
	check_ibf();

	// get it back to check it
	pucPtr[HT6542_CMD] = HT6542_CMD_GET_BYTE;
	check_ibf();

	// now see if we have output data
	check_obf();
	data = pucPtr[HT6542_DAT];
	xxcprintf(g_e, "Enable Interrupts Status = %02x\n", data);


	// next we see if we have a keyboard attached.  if we don't
	// we will get a tx timeout status.  if we do, we will get
	// an 0xFA followed by 0xAA.

	xxcprintf(g_e, "Detecting Keyboard Presence\n");
	test = send_kbd_cmd(KBD_CMD_RST);

	if(test != KBD_STAT_ACK)
	{
		xxcprintf(g_e, "No ACK from Keyboard, Received 0x%02x\n", test);
		return 0;
	}

	check_obf();
	test = pucPtr[HT6542_DAT];
	xxcprintf(g_e, "Got %x\n", test);

	if (test != KBD_STAT_RST_OK)
	{
		xxcprintf(g_e, "Keyboard Reset Failed\n");
		return 0;
	}

	xxcprintf(g_e, "Keyboard Detected\n");

	// get the ID of the keyboard

	xxcprintf(g_e, "Detecting Keyboard ID\n");
	test = send_kbd_cmd(KBD_CMD_ID);
	if(test != KBD_STAT_ACK)
	{
		xxcprintf(g_e, "No ACK from Keyboard, Received 0x%02x\n", test);
		return 0;

	}

	check_obf();
	test = pucPtr[HT6542_DAT];
	if(test == KBD_CMD_ID_1ST)	// 1st byte of ID?
	{
		xxcprintf(g_e, "Received Valid ID\n");

		check_obf();
		test1 = pucPtr[HT6542_DAT];
		xxcprintf(g_e, "Keyboard ID = 0x%02x%02x\n",test,test1);
	}
	else
	{
		xxcprintf(g_e, "Bad ID Received, 0x%02x\n", test);
	}

	return 1;
} // main

void check_ibf()
{
	unsigned long temp;
	xxcprintf(g_e, "Waiting for IBF\n");
	temp = pucPtr[HT6542_STAT];
	xxcprintf(g_e, "HT6542B Status = %x\n", temp);

	while(pucPtr[HT6542_STAT] & HT6542_STAT_IBF)
		u_sleep(10);
}

void check_obf()
{
	unsigned long temp;
	int i;

	xxcprintf(g_e, "Waiting for OBF\n");
	temp = pucPtr[HT6542_STAT];
	xxcprintf(g_e, "HT6542B Status = %x\n", temp);

	for (i = 0; i < HT6542_MAX_WAIT; i++)
	{
		if (pucPtr[HT6542_STAT] & HT6542_STAT_OBF)
			break;

		u_sleep(10);
	}

	if (pucPtr[HT6542_STAT] & HT6542_STAT_OBF)
		xxcprintf(g_e, "Got an OBF\n");
	else
		xxcprintf(g_e, "Missed an OBF\n");

	// did we get aninterrupt on eint1?
	temp = pulPtr[HwIntStatus >> 2];
	temp = temp & HwIrqExt1;
	xxcprintf(g_e, "Interrupt Status Register = %x\n", temp);
}

void flush_obf()
{
	unsigned long temp;
	int i;

	xxcprintf(g_e, "Clearing OBF\n");
	temp = pucPtr[HT6542_STAT];
	xxcprintf(g_e, "HT6542B Status = %x\n", temp);

	for (i = 0; i < HT6542_MAX_WAIT; i++)
	{
		if (!(pucPtr[HT6542_STAT] & HT6542_STAT_OBF))
			break;

		temp = pucPtr[HT6542_DAT];
		xxcprintf(g_e, "HT6542B Flush Data = %x\n", temp);
		u_sleep(10);
	}

	if (i >= HT6542_MAX_WAIT)
		xxcprintf(g_e, "HT6542B Flush Failed\n");
}

int check_tx()
{
	int i;
	xxcprintf(g_e, "Waiting for OBF or TX Timeout\n");

	for (i = 0; i < HT6542_MAX_WAIT; i++)
	{
		if (pucPtr[HT6542_STAT] & HT6542_STAT_TX)
		{
			return 1;	// tx means no keyboard present
		}
		if (pucPtr[HT6542_STAT] & HT6542_STAT_OBF)
		{
			return 0;	// obf is good
		}
		u_sleep(10);
	}

	return 1;
}

// send a command to the keyboard.  re-sends if requested.
// returns 0 if the keyboard response is bad, otherwise
// returns the response data.
unsigned char send_kbd_cmd(unsigned char data)
{
	unsigned char temp;
	int i;

	// flush obf
	flush_obf();

	// send the first time
	check_ibf();
	pucPtr[HT6542_DAT] = data;
	u_sleep(10);

	for (i = 0; i < HT6542_MAX_RESENDS; i++)
	{
		if(!check_tx())
		{
			check_obf();
			temp = pucPtr[HT6542_DAT];

			if (temp == 0x30)
			{
				check_obf();
				temp = pucPtr[HT6542_DAT];
			}

			if( temp == KBD_STAT_RESEND)
			{
				pucPtr[HT6542_DAT] = data;
			}
			else	// if not re-send, then see if it's ack
			{
				if(temp == KBD_STAT_ACK)
				{
					return temp;
				}
				else 	// not re-send or ack so return with error
				{
					xxcprintf(g_e, "No Resend or ACK, data %x\n", temp);
					return temp;
				}
			}
		} // if !check_tx
		else
		{
			xxcprintf(g_e, "TX Timeout\n");
			return 1;		
		}
	} // while 1

	xxcprintf(g_e, "Too many resends\n");
	return 1;		
} // main

