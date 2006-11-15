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

//
//
// LIB7312.H - Function prototype defintions for the library routines
//
// Copyright (c) 1998-1999 Cirrus Logic, Inc.
//
//

//#define LCD_X_SIZE		320
//#define LCD_Y_SIZE		240	;

typedef void (*PFNISR)(void);

//
// Definition of ColorPixel structure
//

typedef struct CLRPixel{
  char r;
  char g;
  char b;
} CPixel;

//
// From adc.c:
//
extern void ADCEnable(void);
extern void ADCDisable(void);
extern long ADCGetData(long lChannel);

//
// From codec.c:
//
extern void CodecEnable(void);
extern void CodecDisable(void);
extern void CodecPlay(char *pcBuffer, long lLength);
extern void CodecPlayBg(char *pcBuffer, long lLength);
extern int CodecPlaySpaceAvailable(void);
extern int CodecPlayIsDone(void);
extern void CodecPlayWaitTilDone(void);
extern void CodecRecord(char *pcBuffer, long lLength);
extern void CodecRecordBg(char *pcBuffer, long lLength);
extern int CodecRecordSpaceAvailable(void);
extern int CodecRecordIsDone(void);
extern void CodecRecordWaitTilDone(void);

//
// From dai.c
//
extern void DAIEnable(void);
extern void DAIDisable(void);
extern int DAIPlayBuffer(short *psBuffer, long lBufferLength, long *plDone);
extern int DAIPlaySpaceAvailable(void);
extern int DAIPlayIsDone(void);
extern int DAIRecordBuffer(short *psBuffer, long lBufferLength, long *plDone);
extern int DAIRecordSpaceAvailable(void);
extern int DAIRecordIsDone(void);

//
// From ir.c:
//
extern long IREnable(long lDataRate);
extern void IRDisable(void);
extern void IRSendChar(char cChar);
extern char IRReceiveChar(void);
extern long IRCharReady(void);

//
// From isr.c:
//
extern void InterruptInstallIRQ(void);
extern void InterruptRemoveIRQ(void);
extern void InterruptInstallFIQ(void);
extern void InterruptRemoveFIQ(void);
extern void InterruptSetCodecHandler(PFNISR pfnCodec);
extern void InterruptSetADCHandler(PFNISR pfnADC);
extern void InterruptSetDAIHandler(PFNISR pfnDAI);
extern void InterruptSetUART1Handler(PFNISR pfnUART1);
extern void InterruptSetUART2Handler(PFNISR pfnUART2);
extern void InterruptSetKbdHandler(PFNISR pfnKbd);
extern void InterruptSetSSI2Handler(PFNISR pfnSSI2);
extern void InterruptSetParallelHandler(PFNISR pfnParallel);

//
// From kbd.c:
//
extern void KbdRead(char *pcKeysPressed);
extern char KbdGetButton(void);
extern void KbdNoButton(void);

//
// From lcd.c:
//
extern void LCDLine(long lX1, long lY1, long lX2, long lY2, char cColor);
extern void LCDCircle(long lX, long lY, long lRadius, char cColor);
extern void LCDFillCircle(long lX, long lY, long lRadius, char cColor);
extern void LCDPrintChar(char cChar, long lX, long lY, char cColor);
extern void LCDPrintString(char *pcBuffer, long lX, long lY, char cColor);
extern void LCDPrintCharX2(char cChar, long lX, long lY, char cColor);
extern void LCDPrintStringX2(char *pcBuffer, long lX, long lY, char cColor);

//
// From lcd_clr.c
//
extern void LCDColorLine(long lX1, long lY1, long lX2, long lY2, CPixel sColor);
extern void LCDColorCircle(long lX, long lY, long lRadius, CPixel sColor);
extern void LCDColorFillCircle(long lX, long lY, long lRadius, CPixel sColor);
extern void LCDColorPrintChar(char cChar, long lX, long lY, CPixel sColor);
extern void LCDColorPrintString(char *pcBuffer, long lX, long lY, CPixel Color);
extern void LCDColorPrintCharX2(char cChar, long lX, long lY, CPixel sColor);
extern void LCDColorPrintStringX2(char *pcBuffer, long lX, long lY, CPixel Color);


//
// From lcd_alps.c:
//
extern void LCDEnable(void);
extern void LCDOn(void);
extern void LCDOff(void);
extern void LCDCls(void);
extern void LCDSetPixel(long lX, long lY, char cColor);
extern char LCDGetPixel(long lX, long lY);
extern void LCDBacklightOn(void);
extern void LCDBacklightOff(void);

//
// From lcd_color.c
//
extern void LCDColorEnable(void);
extern void LCDColorOn(void);
extern void LCDColorOff(void);
extern void LCDColorCls(void);
extern void LCDColorSetPixel(long lX, long lY, CPixel sColor);
extern void LCDCSetPixel(long lX, long lY, char cColor);
extern char LCDGetPixel(long lX, long lY);
extern void LCDColorBacklightOn(void);
extern void LCDColorBacklightOff(void);
extern void LCDColorContrastEnable(void);



//
// From nandflsh.c:
//
extern long NANDGetSize(unsigned long *pulDeviceSize,
                        unsigned long *pulPageSize,
                        unsigned long *pulPagesPerBlock,
                        unsigned long *pulNumBlocks);
extern void NANDReadPage(unsigned long ulPage, int bReadSpare,
                         unsigned long *pulBuffer);
extern void NANDEraseBlock(unsigned long ulBlock);
extern void NANDWritePage(unsigned long ulPage, int bWriteSpare,
                          unsigned long *pulBuffer);
extern long SMIsPresent(void);
extern long SMGetSize(unsigned long *pulDeviceSize, unsigned long *pulPageSize,
                      unsigned long *pulPagesPerBlock,
                      unsigned long *pulNumBlocks);
extern long SMReadPage(unsigned long ulPage, int bReadSpare,
                       unsigned char *pucBuffer);
extern long SMEraseBlock(unsigned long ulBlock);
extern long SMWritePage(unsigned long ulPage, int bWriteSpare,
                        unsigned char *pucBuffer);

//
// From norflsh.c:
//
extern long FlashNumSectors(void);
extern long FlashSectorInfo(long lSector, long *plSectorOffset,
                            long *plSectorLength);
extern void FlashEraseChip(unsigned long ulFlashBase);
extern void FlashEraseSector(unsigned long ulFlashBase, long lSector);
extern void FlashProgramBlock(unsigned long ulFlashBase, long lOffset,
                              unsigned char *pucData, long lNumBytes);

//
// From parallel.c:
//
extern void ParallelEnable(void);
extern void ParallelDisable(void);
extern void ParallelSendChar(char ucChar);
extern char ParallelReceiveChar(void);
extern long ParallelCharReady(void);

//
// From ssi2.c:
//
extern void SSI2Enable(int bMaster);
extern void SSI2Disable(void);
extern void SSI2SendChar(char cChar);
extern char SSI2ReceiveChar(void);

//
// From uart.c:
//
extern long UARTEnable(long lPort, long lDataRate, long lDataBits,
                       long lStopBits, long lParity, long lEvenParity);
extern void UARTDisable(long lPort);
extern void UARTSendChar(long lPort, char cChar);
extern char UARTReceiveChar(long lPort);
extern long UARTCharReady(long lPort);
