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

// Defines for H6542B Keyboard/Mouse Controller
//
 
// note that the auxillary port is used to control the mouse

// 6542B Commands (Sent to the Command Port)
#define HT6542_CMD_SET_BYTE		0x60	// Set the command byte
#define HT6542_CMD_GET_BYTE		0x20	// Get the command byte
#define HT6542_CMD_KBD_OBUFF	0xD2	// Write to HT6542 Keyboard Output Buffer
#define HT6542_CMD_AUX_OBUFF	0xD3	// Write to HT6542 Mouse Output Buffer
#define HT6542_CMD_AUX_WRITE	0xD4	// Write to Mouse Port
#define HT6542_CMD_AUX_OFF		0xA7	// Disable Mouse Port
#define HT6542_CMD_AUX_ON		0xA8	// Re-Enable Mouse Port
#define HT6542_CMD_AUX_TEST		0xA9	// Test for the presence of a Mouse
#define HT6542_CMD_DIAG			0xAA	// Start Diagnostics
#define HT6542_CMD_KBD_TEST		0xAB	// Test for presence of a keyboard
#define HT6542_CMD_KBD_OFF		0xAD	// Disable Keyboard Port - best to use KBD_DAT_ON instead
#define HT6542_CMD_KBD_ON		0xAE	// Re-Enable Keyboard Port - best to use KBD_DAT_OFF instead

// HT6542B command byte set by KBD_CMD_SET_BYTE and retrieved by KBD_CMD_GET_BYTE
#define HT6542_CMD_BYTE_TRANS	0x40
#define HT6542_CMD_BYTE_AUX_OFF	0x20	// 1 = mouse port disabled, 0 = enabled
#define HT6542_CMD_BYTE_KBD_OFF	0x10	// 1 = keyboard port disabled, 0 = enabled
#define HT6542_CMD_BYTE_OVER	0x08	// 1 = override keyboard lock
#define HT6542_CMD_BYTE_RES		0x04	// reserved
#define HT6542_CMD_BYTE_AUX_INT	0x02	// 1 = enable mouse interrupt
#define HT6542_CMD_BYTE_KBD_INT	0x01	// 1 = enable keyboard interrupt

// Keyboard Commands (Sent to the Data Port)
#define KBD_CMD_LED				0xED	// Set Keyboard LEDS with next byte
#define KBD_CMD_ECHO			0xEE	// Echo - we get 0xFA, 0xEE back
#define KBD_CMD_MODE			0xF0	// set scan code mode with next byte
#define KBD_CMD_ID				0xF2	// get keyboard/mouse ID
#define KBD_CMD_RPT				0xF3	// Set Repeat Rate and Delay with Second Byte
#define KBD_CMD_ON				0xF4	// Enable keyboard
#define KBD_CMD_OFF				0xF5	// Disables Scanning and Resets to Defaults
#define KBD_CMD_DEF				0xF6	// Reverts keyboard to default settings
#define KBD_CMD_RST				0xFF	// Reset - we should get 0xFA, 0xAA back

// Set LED second bit defines
#define KBD_CMD_LED_SCROLL		0x01	// Set SCROLL LOCK LED on
#define KBD_CMD_LED_NUM			0x02	// Set NUM LOCK LED on
#define KBD_CMD_LED_CAPS		0x04	// Set CAPS LOCK LED on

// Set Mode second byte defines
#define KBD_CMD_MODE_STAT		0x00	// get current scan code mode
#define KBD_CMD_MODE_SCAN1		0x01	// set mode to scan code 1
#define KBD_CMD_MODE_SCAN2		0x02	// set mode to scan code 2
#define KBD_CMD_MODE_SCAN3		0x03	// set mode to scan code 3

// Keyboard/Mouse ID Codes
#define KBD_CMD_ID_1ST			0xAB	// first byte is 0xAB, second is actual ID
#define KBD_CMD_ID_KBD			0x83	// Keyboard
#define KBD_CMD_ID_MOUSE		0x00	// Mouse

// Keyboard Data Return Defines
#define KBD_STAT_OVER			0x00	// Buffer Overrun
#define KBD_STAT_DIAG_OK		0x55	// Internal Self Test OK
#define KBD_STAT_RST_OK			0xAA	// Reset Complete
#define KBD_STAT_ECHO			0xEE	// Echo Command Return
#define KBD_STAT_BRK			0xF0	// Prefix for Break Key Code
#define KBD_STAT_ACK			0xFA	// Received after all commands
#define KBD_STAT_DIAG_FAIL		0xFD	// Internal Self Test Failed
#define KBD_STAT_RESEND			0xFE	// Resend Last Command

// HT6542B Status Register Bit Defines
#define HT6542_STAT_OBF			0x01	// 1 = output buffer is full
#define HT6542_STAT_IBF			0x02	// 1 = input buffer is full
#define HT6542_STAT_SYS			0x04	// system flag - unused
#define HT6542_STAT_CMD			0x08	// 1 = command in input buffer, 0 = data
#define HT6542_STAT_INH			0x10	// 1 = Inhibit - unused
#define HT6542_STAT_TX			0x20	// 1 = Transmit Timeout has occured
#define HT6542_STAT_RX			0x40	// 1 = Receive Timeout has occured
#define HT6542_STAT_PERR		0x80	// 1 = Parity Error from Keyboard

// Create arrays to hold the strings to display for each scan code.  We need
// one for caps, one for non-caps and one for shift.  We allow up to eight characters
// per key.  To look up, just multiply the scan code by 8 (or left shift by 3).  

const unsigned char	scan_code[] = {
    "        F9      F7      F5      F3      F1      F2      F12             F10     F8      F6      F4      TAB     `               "
    "        ALT     LT SHIFT        CTL     q       1                                       s       a       w       2               "
    "        c       x       d       e       4       3                               v       f       t       r       5               "
    "        n       b       h       g       y       6                               m       j       u       7       8               "
    "        ,       k       i       o       0       9                       .       /       l       ;       p       -               "
    "                \'               [       =                       CAP LOCKRT SHIFTENTER   ]               \\                       "
    "                                                BS                      End             LT ARROWHOME                            "
    "INS     DEL     DN ARROWPAD 5   RT ARROWUP ARROWESC     NUM LOCKF11     PAD +   PAGE DN PAD -   PAD *   PAGE UP SCR LOCK        "};
																										     
const unsigned char	scan_code_caps[] = {
    "        F9      F7      F5      F3      F1      F2      F12             F10     F8      F6      F4      TAB     `               "
    "        ALT     LT SHIFT        CTL     Q       1                                       S       A       W       2               "
    "        C       X       D       E       4       3                               V       F       T       R       5               "
    "        N       B       H       G       Y       6                               M       J       U       7       8               "
    "        ,       K       I       O       0       9                       .       /       L       ;       P       -               "
    "                \'               [       =                       CAP LOCKRT SHIFTENTER   ]               \\                       "
    "                                                BS                      END             LT ARROWHOME                            "
    "INS     DEL     DN ARROWPAD 5   RT ARROWUP ARROWESC     NUM LOCKF11     PAD +   PAGE DN PAD -   PAD *   PAGE UP SCR LOCK        "};
																										     
const unsigned char	scan_code_shift[] = {
    "        F9      F7      F5      F3      F1      F2      F12             F10     F8      F6      F4      TAB     ~               "
    "        ALT     LT SHIFT        CTL     Q       1                                       S       A       W       @               "
    "        C       X       D       E       $       #                       SPACE   V       F       T       R       %               "
    "        N       B       H       G       Y       ^                               M       J       U       &       *               "
    "        <       K       I       O       )       (                       >       ?       L       :       P       _               "
    "                \"              [       +                       CAP LOCKRT SHIFTENTER   ]               |                       "
    "                                                BS                      END             LT ARROWHOME                            "
    "INS     DEL     DN ARROWPAD 5   RT ARROWUP ARROWESC     NUM LOCKF11     PAD +   PAGE DN PAD -   PRTSC   PAGE UP SCR LOCK        "};

