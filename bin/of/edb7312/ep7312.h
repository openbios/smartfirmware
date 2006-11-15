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

/* Definitions for the Cirrus Logical EP7312 embedded ARM processor */

//****************************************************************************
//
// CLEP7312.H - Register definitions for the EP7312
//
// Copyright (c) 1998-1999 Cirrus Logic, Inc.
//
//****************************************************************************

//
// The base address of the hardware registers in ARM memory.
//
#define HwBaseAddress                           0x80000000

//
// The offset to each individual register in the EP7312.
//
#define HwPortABCD                              0x00000000
#define HwPortA                                 0x00000000
#define HwPortB                                 0x00000001
#define HwPortD                                 0x00000003
#define HwDdrABCD                               0x00000040
#define HwDdrA                                  0x00000040
#define HwDdrB                                  0x00000041
#define HwDdrD                                  0x00000043
#define HwPortE                                 0x00000080
#define HwDdrE                                  0x000000C0
#define HwControl                               0x00000100
#define HwStatus                                0x00000140
#define HwMemConfig1                            0x00000180
#define HwMemConfig2                            0x000001C0
#define HwDramRefresh                           0x00000200
#define HwIntStatus                             0x00000240
#define HwIntMask                               0x00000280
#define HwLcdControl                            0x000002C0
#define HwTimer1Data                            0x00000300
#define HwTimer2Data                            0x00000340
#define HwRtcData                               0x00000380
#define HwRtcMatch                              0x000003C0
#define HwPumpControl                           0x00000400
#define HwCodecData                             0x00000440
#define HwUartData                              0x00000480
#define HwUartControl                           0x000004C0
#define HwSpiData                               0x00000500
#define HwPaletteLSW                            0x00000540
#define HwPaletteMSW                            0x00000580
#define HwStartFlagClear                        0x000005C0
#define HwBatteryLowEOI                         0x00000600
#define HwMediaChangedEOI                       0x00000640
#define HwTickEOI                               0x00000680
#define HwTimer1EOI                             0x000006C0
#define HwTimer2EOI                             0x00000700
#define HwRtcMatchEOI                           0x00000740
#define HwUartEOI                               0x00000780
#define HwCodecEOI                              0x000007C0
#define HwHalt                                  0x00000800
#define HwStandby                               0x00000840
#define HwLcdFrameBuffer                        0x00001000
#define HwControl2                              0x00001100
#define HwStatus2                               0x00001140
#define HwIntStatus2                            0x00001240
#define HwIntMask2                              0x00001280
#define HwUart2Data                             0x00001480
#define HwUart2Control                          0x000014C0
#define HwSSI2Data                              0x00001500
#define HwSSI2EOF                               0x00001600
#define HwSSI2POP                               0x000016C0
#define HwKeyboardEOI                           0x00001700
#define HwDAIControl                            0x00002000
#define HwDAIData0                              0x00002040
#define HwDAIData1                              0x00002080
#define HwDAIData2                              0x000020C0
#define HwDAIStatus                             0x00002100
#define HwControl3                              0x00002200
#define HwIntStatus3                            0x00002240
#define HwIntMask3                              0x00002280
#define HwLEDFlash                              0x000022C0

//
// Definitions of the bit fields in the HwPortA register for the EP7312
// evaluation board.
//
#define HwPortAKeyboardRow1                     0x00000001
#define HwPortAKeyboardRow2                     0x00000002
#define HwPortAKeyboardRow3                     0x00000004
#define HwPortAKeyboardRow4                     0x00000008
#define HwPortAKeyboardRow5                     0x00000010
#define HwPortAKeyboardRow6                     0x00000020
#define HwPortAKeyboardRow7                     0x00000040
#define HwPortAKeyboardRow8                     0x00000080

//
// Definitions of the bit fields in the HwPortB register for the EP7312
// evaluation board.
//

#define HwPortBSpare1							0x00000001
#define HwPortBRTS                              0x00000002
#define HwPortBRI                               0x00000004
#define HwPortBSpare2                           0x00000008
#define HwPortBCLE                              0x00000010
#define HwPortBALE                              0x00000020
#define HwPortBNANDCS                           0x00000040
#define HwPortBSMCS                             0x00000080

//
// Definitions of the bit fields in the HwPortD register for the EP7312
// evaluation board.
//
#define HwPortDDiagLED                          0x00000001
#define HwPortDLCDDcDcPower                     0x00000002
#define HwPortDLCDPower                         0x00000004
#define HwPortDLCDBacklightPower                0x00000008
#define HwPortDSerialData                       0x00000010
#define HwPortDSerialClock                      0x00000020
#define HwPortDSmartMediaPresent                0x00000040
#define HwPortDSpare3                           0x00000080

//
// Definitions of the bit fields in the HwPortE register for the EP7312
// evaluation board.
//
#define HwPortECodecPower                       0x00000001
#define HwPortESpare4                           0x00000002
#define HwPortEADCPower                         0x00000004

//
// Definitions of the bit fields in the HwControl register.
//
#define HwControlColumnDrive                    0x0000000F
#define HwControlColumnAllHigh                  0x00000000
#define HwControlColumnAllLow                   0x00000001
#define HwControlColumnAllTriState              0x00000002
#define HwControlColumn0High                    0x00000008
#define HwControlColumn1High                    0x00000009
#define HwControlColumn2High                    0x0000000A
#define HwControlColumn3High                    0x0000000B
#define HwControlColumn4High                    0x0000000C
#define HwControlColumn5High                    0x0000000D
#define HwControlColumn6High                    0x0000000E
#define HwControlColumn7High                    0x0000000F
#define HwControlTimer1PreOrFree                0x00000010
#define HwControlTimer1K512OrK2                 0x00000020
#define HwControlTimer2PreOrFree                0x00000040
#define HwControlTimer2K512OrK2                 0x00000080
#define HwControlUartEnable                     0x00000100
#define HwControlBuzzerToggle                   0x00000200
#define HwControlBuzzerTimer1OrToggle           0x00000400
#define HwControlDebugEnable                    0x00000800
#define HwControlLcdEnable                      0x00001000
#define HwControlCodecTxEnable                  0x00002000
#define HwControlCodecRxEnable                  0x00004000
#define HwControlUartSirEnable                  0x00008000
#define HwControlSpiClockSelect                 0x00030000
#define HwControlSpiClock4KHz                   0x00000000
#define HwControlSpiClock16KHz                  0x00010000
#define HwControlSpiClock64KHz                  0x00020000
#define HwControlSpiClock128KHz                 0x00030000
#define HwControlExpClockEnable                 0x00040000
#define HwControlWakeupDisable                  0x00080000
#define HwControlIrDAWidthEnable                0x00100000

//
// Definitions of the bit fields in the HwStatus register.
//
#define HwStatusMediaChangedState               0x00000001
#define HwStatusDcPresent                       0x00000002
#define HwStatusWakeUpState                     0x00000004
#define HwStatusWakeUpFlag                      0x00000008
#define HwStatusLcdType                         0x000000F0
#define HwStatusCts                             0x00000100
#define HwStatusDsr                             0x00000200
#define HwStatusDcd                             0x00000400
#define HwStatusUartTxBusy                      0x00000800
#define HwStatusBatteryChangedFlag              0x00001000
#define HwStatusResetFlag                       0x00002000
#define HwStatusPowerFailFlag                   0x00004000
#define HwStatusColdStartFlag                   0x00008000
#define HwStatusRtcLSB                          0x003F0000
#define HwStatusUartRxFifoEmpty                 0x00400000
#define HwStatusUartTxFifoFull                  0x00800000
#define HwStatusCodecRxFifoEmpty                0x01000000
#define HwStatusCodecTxFifoFull                 0x02000000
#define HwStatusSpiBusy                         0x04000000
#define HwStatusVersionID                       0xC0000000

//
// Definitions of the bit fields in the HwMemConfig1 and HwMemConfig2
// registers.
//
#define HwMemConfigBusWidth                     0x00000003
#define HwMemConfigBusWidth32                   0x00000000
#define HwMemConfigBusWidth16                   0x00000001
#define HwMemConfigBusWidth8                    0x00000002
#define HwMemConfigBusWidthPcmcia               0x00000003
#define HwMemConfigRaWaitState                  0x0000000C
#define HwMemConfigRaWaitState4                 0x00000000
#define HwMemConfigRaWaitState3                 0x00000004
#define HwMemConfigRaWaitState2                 0x00000008
#define HwMemConfigRaWaitState1                 0x0000000C
#define HwMemConfigSaWaitState                  0x00000030
#define HwMemConfigSaWaitState3                 0x00000000
#define HwMemConfigSaWaitState2                 0x00000010
#define HwMemConfigSaWaitState1                 0x00000020
#define HwMemConfigSaWaitState0                 0x00000030
#define HwMemConfigSeqAccessEnable              0x00000040
#define HwMemConfigExpClkEnable                 0x00000080

//
// Definitions of the bit fields in the HwDramRefresh register.
//
#define HwDramRefreshRate                       0x0000007F
#define HwDramRefreshEnable                     0x00000080

//
// Definitions of the bit fields in the HwIntStatus and HwIntMask registers.
//
#define HwFiqExt                                0x00000001
#define HwFiqBatteryLow                         0x00000002
#define HwFiqWatchDog                           0x00000004
#define HwFiqMediaChange                        0x00000008
#define HwIrqCodec                              0x00000010
#define HwIrqExt1                               0x00000020
#define HwIrqExt2                               0x00000040
#define HwIrqExt3                               0x00000080
#define HwIrqTimer1                             0x00000100
#define HwIrqTimer2                             0x00000200
#define HwIrqRtcMatch                           0x00000400
#define HwIrqTick                               0x00000800
#define HwIrqUartTx                             0x00001000
#define HwIrqUartRx                             0x00002000
#define HwIrqUartModem                          0x00004000
#define HwIrqSpi                                0x00008000

//
// Definitions of the bit fields in the HwLcdControl register.
//
#define HwLcdControlBufferSize                  0x00001FFF
#define HwLcdControlLineLength                  0x0007E000
#define HwLcdControlPixelPrescale               0x01F80000
#define HwLcdControlAcPrescale                  0x3E000000
#define HwLcdControlGreyEnable                  0x40000000
#define HwLcdControlGrey4Or2                    0x80000000
#define HwLcdControlBufferSizeShift             0
#define HwLcdControlLineLengthShift             13
#define HwLcdControlPixelPrescaleShift          19
#define HwLcdControlAcPrescaleShift             25

//
// Definitions of the bit fields in the HwPumpControl register.
//
#define HwPumpControlVhBat                      0x0000000F
#define HwPumpControlVhDc                       0x000000F0
#define HwPumpControlLcd                        0x00000F00

//
// Definitions of the bit fields in the HwUartData register.
//
#define HwUartDataFrameErr                      0x00000100
#define HwUartDataParityErr                     0x00000200
#define HwUartDataOverrunErr                    0x00000400

//
// Definitions of the bit fields in the HwUartControl register.
//
#define HwUartControlRate                       0x00000FFF
#define HwUartControlRate115200                 0x00000001
#define HwUartControlRate76800                  0x00000002
#define HwUartControlRate57600                  0x00000003
#define HwUartControlRate38400                  0x00000005
#define HwUartControlRate28800                  0x00000007
#define HwUartControlRate19200                  0x0000000B
#define HwUartControlRate14400                  0x0000000F
#define HwUartControlRate9600                   0x00000017
#define HwUartControlRate4800                   0x0000002F
#define HwUartControlRate2400                   0x0000005F
#define HwUartControlRate1200                   0x000000BF
#define HwUartControlRate600                    0x0000017F
#define HwUartControlRate300                    0x000002FF
#define HwUartControlRate150                    0x000005FF
#define HwUartControlRate110                    0x0000082E
#define HwUartControlBreak                      0x00001000
#define HwUartControlParityEnable               0x00002000
#define HwUartControlParityEven                 0x00004000
#define HwUartControlTwoStopBits                0x00008000
#define HwUartControlFifoEnable                 0x00010000
#define HwUartControlDataLength                 0x00060000
#define HwUartControlDataLength5                0x00000000
#define HwUartControlDataLength6                0x00020000
#define HwUartControlDataLength7                0x00040000
#define HwUartControlDataLength8                0x00060000

//
// Definitions of the bit fields in the HwSpiData register.
//
#define HwSpiDataConfigMask                     0x000000FF
#define HwSpiDataFrameLengthMask                0x00001F00
#define HwSpiDataSampleClockFreerun             0x00002000
#define HwSpiDataTxFrame                        0x00004000
#define HwSpiDataConfigShift                    0
#define HwSpiDataFrameLengthShift               8

//
// Definitions of the bit fields in the HwControl2 register.
//
#define HwControl2CodecEnable                   0x00000001
#define HwControl2Kbd6                          0x00000002
#define HwControl2DramSize                      0x00000004
#define HwControl2KeyboardWakeupEnable          0x00000008
#define HwControl2SSI2TxEnable                  0x00000010
#define HwControl2PCMCIA1Enable                 0x00000020
#define HwControl2PCMCIA2Enable                 0x00000040
#define HwControl2SSI2RxEnable                  0x00000080
#define HwControl2Uart2Enable                   0x00000100
#define HwControl2SSI2MasterEnable              0x00000200
#define HwControl2OSTimerEnable                 0x00001000
#define HwControl2ClockSelect                   0x00002000
#define HwControl2BuzzerSelect                  0x00004000

//
// Definitions of the bit fields in the HwStatus2 register.
//
#define HwStatus2SSI2RxOverflow                 0x00000001
#define HwStatus2SSI2ResVal                     0x00000002
#define HwStatus2SSI2ResFrm                     0x00000004
#define HwStatus2SSI2RxFIFOEmpty                0x00000008
#define HwStatus2SSI2TxFIFOFull                 0x00000010
#define HwStatus2SSI2TxFIFOUnderflow            0x00000020
#define HwStatus2ClockMode                      0x00000040
#define HwStatus2Uart2TxBusy                    0x00000800
#define HwStatus2Uart2RxFifoEmpty               0x00400000
#define HwStatus2Uart2TxFifoFull                0x00800000

//
// Definitions of the bit fields in the HwIntStatus2 and HwIntMask2 registers.
//
#define HwIrqKeyboard                           0x00000001
#define HwIrqSSI2Rx                             0x00000002
#define HwIrqSSI2Tx                             0x00000004
#define HwIrqUart2Tx                            0x00001000
#define HwIrqUart2Rx                            0x00002000

//
// Definitions of the bit fields in the HwDAIControl register.
//
#define HwDAIControlMCE                         0x00010000
#define HwDAIControlECS                         0x00020000
#define HwDAIControlADM                         0x00040000
#define HwDAIControlTTM                         0x00080000
#define HwDAIControlTRM                         0x00100000
#define HwDAIControlATM                         0x00200000
#define HwDAIControlARM                         0x00400000
#define HwDAIControlLBM                         0x00800000

//
// Definitions of the bit fields in the HwDAIData0 register.
//
#define HwDAIData0DataMask                      0x0000FFFF
#define HwDAIData0DataShift                     0

//
// Definitions of the bit fields in the HwDAIData1 register.
//
#define HwDAIData1DataMask                      0x0000FFFF
#define HwDAIData1DataShift                     0

//
// Definitions of the FIFO command for the HwDAIData2 register.
//
#define HwDAIData2RightEnable                   0x000D8000
#define HwDAIData2LeftEnable                    0x00118000

//
// Definitions of the bit fields in the HwDAIStatus register.
//
#define HwDAIStatusATS                          0x00000001
#define HwDAIStatusARS                          0x00000002
#define HwDAIStatusTTS                          0x00000004
#define HwDAIStatusTRS                          0x00000008
#define HwDAIStatusATU                          0x00000010
#define HwDAIStatusARO                          0x00000020
#define HwDAIStatusTTU                          0x00000040
#define HwDAIStatusTRO                          0x00000080
#define HwDAIStatusANF                          0x00000100
#define HwDAIStatusANE                          0x00000200
#define HwDAIStatusTNF                          0x00000400
#define HwDAIStatusTNE                          0x00000800
#define HwDAIStatusFIFO                         0x00001000
#define HwDAIStatusACE                          0x00004000
#define HwDAIStatusTCE                          0x00008000

//
// Definitions of the bit fields in the HwControl3 register.
//
#define HwControl3ADCExtended                   0x00000001
#define HwControl3ClkCtlMask                    0x00000006
#define HwControl3ClkCtl18MHz                   0x00000000
#define HwControl3ClkCtl37MHz                   0x00000002
#define HwControl3ClkCtl49MHz                   0x00000004
#define HwControl3ClkCtl74MHz                   0x00000006
#define HwControl3DAISelect                     0x00000008
#define HwControl3ADCClkEdge                    0x00000010
#define HwControl3VersionMask                   0x000000E0
#define HwControl3FastWakeup                    0x00000100

//
// Definitions of the bit fields in the HwIntStatus3 and HwIntMask3 registers.
//
#define HwFiqDAI                                0x00000001

//
// Definitions of the bit fields in the HwLEDFlash register.
//
#define HwLEDFlashRateMask                      0x00000003
#define HwLEDFlashRate1Sec                      0x00000000
#define HwLEDFlashRate2Sec                      0x00000001
#define HwLEDFlashRate3Sec                      0x00000002
#define HwLEDFlashRate4Sec                      0x00000003
#define HwLEDFlashRatioMask                     0x0000003C
#define HwLEDFlashRatio1_15                     0x00000000
#define HwLEDFlashRatio2_14                     0x00000004
#define HwLEDFlashRatio3_13                     0x00000008
#define HwLEDFlashRatio4_12                     0x0000000C
#define HwLEDFlashRatio5_11                     0x00000010
#define HwLEDFlashRatio6_10                     0x00000014
#define HwLEDFlashRatio7_9                      0x00000018
#define HwLEDFlashRatio8_8                      0x0000001C
#define HwLEDFlashRatio9_7                      0x00000020
#define HwLEDFlashRatio10_6                     0x00000024
#define HwLEDFlashRatio11_5                     0x00000028
#define HwLEDFlashRatio12_4                     0x0000002C
#define HwLEDFlashRatio13_3                     0x00000030
#define HwLEDFlashRatio14_2                     0x00000034
#define HwLEDFlashRatio15_1                     0x00000038
#define HwLEDFlashRatio16_0                     0x0000003C
#define HwLEDFlashEnable                        0x00000040

//
// The base address of the parallel port.
//
#define HwLptAddress                            0x30000000

//
// Definitions of the bit fields in the parallel port register.
//
#define HwLptDataMask                           0x000000FF
#define HwLptStatusMask                         0x00000F00
#define HwLptDataStatusMask                     0x00000FFF
#define HwLptStatusnError                       0x00000100
#define HwLptStatusPaperEnd                     0x00000200
#define HwLptStatusnAck                         0x00000400
#define HwLptStatusBusy                         0x00000800
#define HwLptOutputEnable                       0x00001000
#define HwLptStatusSelect                       0x00002000
#define HwLptControlnStrobe                     0x00001000
#define HwLptControlnAutoFeed                   0x00002000
#define HwLptControlnSelectIn                   0x00004000
#define HwLptControlnInit                       0x00008000

//
// The base address of the NAND FLASH.
//
#define HwNANDAddress                           0x10000000

//
// The base address of the keyboard extended row latch.
//
#define HwKeyboardExRowAddress                  0x30010000

//
// The base address of the external GPIO latch.
//
#define HwExtGPIOAddress                        0x30010000

//
// The base address of the LCD frame buffer in ARM memory.
//
#define HwLcdBaseAddress                        0xC0000000
#define SwLcdBaseAddress                        0xF7000000

//
// Locations of the vectors in the ARM vector table.
//
#define ArmResetVector                          0x00000000
#define ArmUndefInstrVector                     0x00000004
#define ArmSwiVector                            0x00000008
#define ArmPrefAbortVector                      0x0000000C
#define ArmDataAbortVector                      0x00000010
#define ArmReservedVector                       0x00000014
#define ArmIrqVector                            0x00000018
#define ArmFiqVector                            0x0000001C

//
// Locations of the flags in the ARM PSR register.
//
#define ArmMaskMode                             0x0000001F
#define ArmUserMode                             0x00000010
#define ArmFIQMode                              0x00000011
#define ArmIRQMode                              0x00000012
#define ArmSVCMode                              0x00000013
#define ArmAbortMode                            0x00000017
#define ArmUndefMode                            0x0000001B
#define ArmFiqDisable                           0x00000040
#define ArmIrqDisable                           0x00000080
#define ArmMaskCC                               0xF0000000
#define ArmFlagV                                0x10000000
#define ArmFlagC                                0x20000000
#define ArmFlagZ                                0x40000000
#define ArmFlagN                                0x80000000

//
// The coprocessor number of the MMU on the EP7312.
//
#define ArmMmuCP                                0xF

//
// The coprocessor registers in the MMU of the EP7312.
//
#define ArmMmuId                                0x00
#define ArmMmuControl                           0x01
#define ArmMmuPageTableBase                     0x02
#define ArmMmuDomainAccess                      0x03
#define ArmMmuFlushTlb         0x06
#define ArmMmuFlushIdc                          0x07

//
// Definitions of the bit fields in the MMU control register.
//
#define ArmMmuControlMmuEnable                  0x00000001
#define ArmMmuControlAlignFaultEnable           0x00000002
#define ArmMmuControlCacheEnable                0x00000004
#define ArmMmuControlWriteBufferEnable          0x00000008
#define ArmMmuControl32BitCodeEnable            0x00000010
#define ArmMmuControl32BitDataEnable            0x00000020
#define ArmMmuControlMandatory                  0x00000040
#define ArmMmuControlBigEndianEnable            0x00000080
#define ArmMmuControlSystemEnable               0x00000100
#define ArmMmuControlRomEnable                  0x00000200
