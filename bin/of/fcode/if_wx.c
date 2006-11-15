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

/* (c) Copyright 2000-2001 by CodeGen, Inc.  All Rights Reserved. */

/* On-board Fcode ethernet driver for Intel 82543 PRO/1000 gigabit cards.
   Compile with "cc-fcode" then tokenize with "of -t".
   Much code is copied and hacked from Intel's Linux e1000 driver sources.
 */

#include "fcode.h"
#include "fcpci.h"

//#define DEBUG

/* BEGIN HACKED INTEL CODE */

/*****************************************************************************
 *****************************************************************************
 Copyright (c) 1999-2000, Intel Corporation 

 All rights reserved.

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, 
 this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation 
 and/or other materials provided with the distribution.

 3. Neither the name of Intel Corporation nor the names of its contributors 
 may be used to endorse or promote products derived from this software 
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

 *****************************************************************************
****************************************************************************/
 
/*
 *
 * e1000.h
 * 
 */

/* Unix Typedefs */
typedef unsigned char uchar_t;
typedef unsigned int uint_t;
typedef unsigned short ushort_t;
typedef unsigned long ulong_t;
typedef unsigned long paddr_t;
typedef ulong_t major_t;
typedef unsigned char boolean_t;
typedef unsigned long uintptr_t;

#define B_TRUE          TRUE
#define B_FALSE         FALSE


/*
 * e1000_defines.h
 * 
 */

/* Supported RX Buffer Sizes */
#define E1000_RXBUFFER_2048		2048
#define E1000_RXBUFFER_4096		4096
#define E1000_RXBUFFER_8192		8192
#define E1000_RXBUFFER_16384	16384

/* Max and Min Number of Supported TX and RX Descriptors */ 
#define E1000_MAX_TXD	256
#define E1000_MIN_TXD	80
#define E1000_MAX_RXD	256
#define E1000_MIN_RXD	80
 
#define ETH_LENGTH_OF_ADDRESS 6
#define MAC_ADDR	ETH_LENGTH_OF_ADDRESS

/* The number of entries in the CAM. The CAM (Content Addressable
* Memory) holds the directed and multicast addresses that we
* monitor. We reserve one of these spots for our directed address,
* allowing us E1000_CAM_ENTRIES - 1 multicast addresses.
*/


#define RET_STATUS_SUCCESS 0
#define RET_STATUS_FAILURE 1

/* E1000 specific hardware defines */
#define e1000regs E1000_REGISTERS
#define PE1000_ADAPTER bdd_t*


#ifdef DO_ASSERT

#define ASSERT(expr) assert("\$" __FILE__, __LINE__, "\$" #expr, (expr))

void
assert(Fstr file, int line, Fstr exstr, int expr)
{
	if (!expr)
	{
		cr();
		type("\$assertion failure: ");
		type(file);
		type("\$:");
		dot(line);
		type("\$: ");
		type(exstr);
		cr();
	}
}

#else
#	define ASSERT(expr)		/* expr */
#endif

void
udelay(int n)
{
	while (n-- > 0)
		noop();
}

#define DelayInMicroseconds(x) noop()
#define DelayInMilliseconds(x) ms(x)

typedef struct uint64
{
	volatile uQuadlet lo;
	volatile uQuadlet hi;
} uint64_t;

typedef uByte UCHAR, UINT8, *PUCHAR, *PUINT8, BOOLEAN;
typedef uDoublet USHORT, UINT16, *PUSHORT, *PUINT16;
typedef uQuadlet UINT, ULONG, UINT32, *PUINT, *PULONG, *PUINT32;
typedef void VOID, *PVOID;
typedef uint64_t E1000_64_BIT_PHYSICAL_ADDRESS,
		*PE1000_64_BIT_PHYSICAL_ADDRESS;

#define IN
#define OUT
#define STATIC static

#ifdef DEBUG
void
out(Fstr str)
{
	type(str);
	cr();
}

void
out1(Fstr str, UINT a)
{
	type(str);
	udot(a);
	cr();
}

void
out2(Fstr str, UINT a, UINT b)
{
	type(str);
	udot(a);
	udot(b);
	cr();
}
#endif

#define MSGOUT(S, A, B)		out2("\$wx: " S, A, B)
#define DEBUGFUNC(F)		DEBUGOUT(F);

#ifdef DEBUG
#	define DEBUGOUT(S)			out("\$wx: " S)
#	define DEBUGOUT1(S, A)		out1("\$wx: " S, (UINT)A)
#	define DEBUGOUT2(S, A, B)	out2("\$wx: " S, (UINT)A, (UINT)B)
#else
#	define DEBUGOUT(S)
#	define DEBUGOUT1(S, A)
#	define DEBUGOUT2(S, A, B)
#endif // DEBUG

#define MAC_DECODE_SIZE (128 * 1024)

typedef enum MAC_TYPE {
    MAC_WISEMAN_2_0 = 0,
    MAC_WISEMAN_2_1,
    MAC_LIVENGOOD,
    NUM_MACS
} MAC_TYPE, *PMAC_TYPE;

#define WISEMAN_2_0_REV_ID               2
#define WISEMAN_2_1_REV_ID               3

typedef enum GIGABIT_MEDIA_TYPE {
    MEDIA_TYPE_COPPER = 0,
    MEDIA_TYPE_FIBER = 1,
    NUM_MEDIA_TYPES
} GIGABIT_MEDIA_TYPE, *PGIGABIT_MEDIA_TYPE;

typedef enum SPEED_DUPLEX_TYPE {
    HALF_10 = 10,
    FULL_10 = 11,
    HALF_100 = 100,
    FULL_100 = 101,
    FULL_1000 = 1001
} SPEED_DUPLEX_TYPE, *PSPEED_DUPLEX_TYPE;

#define SPEED_10                        10
#define SPEED_100                      100
#define SPEED_1000                    1000
#define HALF_DUPLEX                      1
#define FULL_DUPLEX                      2

#define ENET_HEADER_SIZE                14
#define MAXIMUM_ETHERNET_PACKET_SIZE  1514
#define MINIMUM_ETHERNET_PACKET_SIZE    60
#define CRC_LENGTH                       4

#define MAX_JUMBO_FRAME_SIZE    (0x3F00)

#define ISL_CRC_LENGTH                         4

#define MAXIMUM_VLAN_ETHERNET_PACKET_SIZE   1514
#define MINIMUM_VLAN_ETHERNET_PACKET_SIZE     60
#define VLAN_TAG_SIZE                          4

#define ETHERNET_IEEE_VLAN_TYPE     0x8100
#define ETHERNET_IP_TYPE            0x0800
#define ETHERNET_IPX_TYPE           0x8037
#define ETHERNET_IPX_OLD_TYPE       0x8137
#define MAX_802_3_LEN_FIELD         0x05DC

#define ETHERNET_ARP_TYPE           0x0806
#define ETHERNET_XNS_TYPE           0x0600
#define ETHERNET_X25_TYPE           0x0805
#define ETHERNET_BANYAN_TYPE        0x0BAD
#define ETHERNET_DECNET_TYPE        0x6003
#define ETHERNET_APPLETALK_TYPE     0x809B
#define ETHERNET_SNA_TYPE           0x80D5
#define ETHERNET_SNMP_TYPE          0x814C

#define IP_OFF_MF_BIT               0x0002
#define IP_OFF_OFFSET_MASK          0xFFF8
#define IP_PROTOCOL_ICMP                 1
#define IP_PROTOCOL_IGMP                 2
#define IP_PROTOCOL_TCP                  6
#define IP_PROTOCOL_UDP               0x11
#define IP_PROTOCOL_IPRAW             0xFF

#define POLL_IMS_ENABLE_MASK (E1000_IMS_PCIE | E1000_IMS_RXDMT0 | E1000_IMS_RXSEQ)

#define IMS_ENABLE_MASK (E1000_IMS_RXT0 | E1000_IMS_TXDW | E1000_IMS_PCIE | E1000_IMS_RXDMT0 | E1000_IMS_RXSEQ | E1000_IMS_LSC | E1000_IMS_GPI_EN1)

#define E1000_RAR_ENTRIES 16

typedef struct E1000_RECEIVE_DESCRIPTOR {
    volatile E1000_64_BIT_PHYSICAL_ADDRESS BufferAddress;

    volatile USHORT Length;
    volatile USHORT Csum;
    volatile UCHAR ReceiveStatus;
    volatile UCHAR Errors;
    volatile USHORT Special;

} E1000_RECEIVE_DESCRIPTOR, *PE1000_RECEIVE_DESCRIPTOR;

#define MIN_NUMBER_OF_DESCRIPTORS (8)
#define MAX_NUMBER_OF_DESCRIPTORS (0xFFF8)

#define E1000_RXD_STAT_DD        (0x01)
#define E1000_RXD_STAT_EOP       (0x02)

#define E1000_RXD_STAT_ISL       (0x04)
#define E1000_RXD_STAT_IXSM      (0x04)
#define E1000_RXD_STAT_VP        (0x08)
#define E1000_RXD_STAT_BPDU      (0x10)
#define E1000_RXD_STAT_TCPCS     (0x20)
#define E1000_RXD_STAT_IPCS      (0x40)

#define E1000_RXD_STAT_PIF       (0x80)

#define E1000_RXD_ERR_CE         (0x01)
#define E1000_RXD_ERR_SE         (0x02)
#define E1000_RXD_ERR_SEQ        (0x04)

#define E1000_RXD_ERR_ICE        (0x08)

#define E1000_RXD_ERR_CXE        (0x10)

#define E1000_RXD_ERR_TCPE       (0x20)
#define E1000_RXD_ERR_IPE        (0x40)

#define E1000_RXD_ERR_RXE        (0x80)

#define E1000_RXD_ERR_FRAME_ERR_MASK (E1000_RXD_ERR_CE | E1000_RXD_ERR_SE | E1000_RXD_ERR_SEQ | E1000_RXD_ERR_CXE | E1000_RXD_ERR_RXE)

#define E1000_RXD_SPC_VLAN_MASK  (0x0FFF)
#define E1000_RXD_SPC_PRI_MASK   (0xE000)
#define E1000_RXD_SPC_PRI_SHIFT  (0x000D)
#define E1000_RXD_SPC_CFI_MASK   (0x1000)
#define E1000_RXD_SPC_CFI_SHIFT  (0x000C)

#define E1000_TXD_DTYP_D        (0x00100000)
#define E1000_TXD_DTYP_C        (0x00000000)
#define E1000_TXD_POPTS_IXSM    (0x01)
#define E1000_TXD_POPTS_TXSM    (0x02)

typedef struct E1000_TRANSMIT_DESCRIPTOR {
    E1000_64_BIT_PHYSICAL_ADDRESS BufferAddress;

    union {
        volatile ULONG DwordData;
        struct TXD_FLAGS {
            volatile USHORT Length;
            volatile UCHAR Cso;
            volatile UCHAR Cmd;
        } Flags;
    } Lower;

    union {
        volatile ULONG DwordData;
        struct TXD_FIELDS {
            volatile UCHAR TransmitStatus;
            volatile UCHAR Css;
            volatile USHORT Special;
        } Fields;
    } Upper;

} E1000_TRANSMIT_DESCRIPTOR, *PE1000_TRANSMIT_DESCRIPTOR;

typedef struct E1000_TCPIP_CONTEXT_TRANSMIT_DESCRIPTOR {
    union {
        ULONG IpXsumConfig;
        struct IP_XSUM_FIELDS {
            UCHAR Ipcss;
            UCHAR Ipcso;
            USHORT Ipcse;
        } IpFields;
    } LowerXsumSetup;

    union {
        ULONG TcpXsumConfig;
        struct TCP_XSUM_FIELDS {
            UCHAR Tucss;
            UCHAR Tucso;
            USHORT Tucse;
        } TcpFields;
    } UpperXsumSetup;

    ULONG CmdAndLength;

    union {
        ULONG DwordData;
        struct TCP_SEG_FIELDS {
            UCHAR Status;
            UCHAR HdrLen;
            USHORT Mss;
        } Fields;
    } TcpSegSetup;

} E1000_TCPIP_CONTEXT_TRANSMIT_DESCRIPTOR,
    *PE1000_TCPIP_CONTEXT_TRANSMIT_DESCRIPTOR;

typedef struct E1000_TCPIP_DATA_TRANSMIT_DESCRIPTOR {
    E1000_64_BIT_PHYSICAL_ADDRESS BufferAddress;

    union {
        ULONG DwordData;
        struct TXD_OD_FLAGS {
            USHORT Length;
            UCHAR TypLenExt;
            UCHAR Cmd;
        } Flags;
    } Lower;

    union {
        ULONG DwordData;
        struct TXD_OD_FIELDS {
            UCHAR TransmitStatus;
            UCHAR Popts;
            USHORT Special;
        } Fields;
    } Upper;

} E1000_TCPIP_DATA_TRANSMIT_DESCRIPTOR,
    *PE1000_TCPIP_DATA_TRANSMIT_DESCRIPTOR;

#define E1000_TXD_CMD_EOP   (0x01000000)
#define E1000_TXD_CMD_IFCS  (0x02000000)

#define E1000_TXD_CMD_IC    (0x04000000)

#define E1000_TXD_CMD_RS    (0x08000000)
#define E1000_TXD_CMD_RPS   (0x10000000)

#define E1000_TXD_CMD_DEXT  (0x20000000)
#define E1000_TXD_CMD_ISLVE (0x40000000)

#define E1000_TXD_CMD_IDE   (0x80000000)

#define E1000_TXD_STAT_DD   (0x00000001)
#define E1000_TXD_STAT_EC   (0x00000002)
#define E1000_TXD_STAT_LC   (0x00000004)
#define E1000_TXD_STAT_TU   (0x00000008)

#define E1000_TXD_CMD_TCP   (0x01000000)
#define E1000_TXD_CMD_IP    (0x02000000)
#define E1000_TXD_CMD_TSE   (0x04000000)

#define E1000_TXD_STAT_TC   (0x00000004)

#define E1000_NUM_UNICAST          (16)
#define E1000_MC_TBL_SIZE          (128)

#define E1000_VLAN_FILTER_TBL_SIZE (128)

enum {
    FLOW_CONTROL_NONE = 0,
    FLOW_CONTROL_RECEIVE_PAUSE = 1,
    FLOW_CONTROL_TRANSMIT_PAUSE = 2,
    FLOW_CONTROL_FULL = 3,
    FLOW_CONTROL_HW_DEFAULT = 0xFF
};

typedef struct {
    volatile unsigned int Low;
    volatile unsigned int High;
} RECEIVE_ADDRESS_REGISTER_PAIR;

typedef struct E1000_REGISTERS {

    volatile unsigned int Ctrl;
    volatile unsigned int Pad1;
    volatile unsigned int Status;
    volatile unsigned int Pad2;
    volatile unsigned int Eecd;
    volatile unsigned int Pad3;
    volatile unsigned int Exct;
    volatile unsigned int Pad4;
    volatile unsigned int Mdic;
    volatile unsigned int Pad5;
    volatile unsigned int Fcal;
    volatile unsigned int Fcah;
    volatile unsigned int Fct;
    volatile unsigned int Pad6;

    volatile unsigned int Vet;
    volatile unsigned int Pad7;

    RECEIVE_ADDRESS_REGISTER_PAIR Rar[16];

    volatile unsigned int Icr;
    volatile unsigned int Pad8;
    volatile unsigned int Ics;
    volatile unsigned int Pad9;
    volatile unsigned int Ims;
    volatile unsigned int Pad10;
    volatile unsigned int Imc;
    volatile unsigned char Pad11[0x24];

    volatile unsigned int Rctl;
    volatile unsigned int Pad12;
    volatile unsigned int PadRdtr0;
    volatile unsigned int Pad13;
    volatile unsigned int PadRdbal0;
    volatile unsigned int PadRdbah0;
    volatile unsigned int PadRdlen0;
    volatile unsigned int Pad14;
    volatile unsigned int PadRdh0;
    volatile unsigned int Pad15;
    volatile unsigned int PadRdt0;
    volatile unsigned int Pad16;
    volatile unsigned int Rdtr1;
    volatile unsigned int Pad17;
    volatile unsigned int Rdbal1;
    volatile unsigned int Rdbah1;
    volatile unsigned int Rdlen1;
    volatile unsigned int Pad18;
    volatile unsigned int Rdh1;
    volatile unsigned int Pad19;
    volatile unsigned int Rdt1;
    volatile unsigned char Pad20[0x0C];
    volatile unsigned int PadFcrth;
    volatile unsigned int Pad21;
    volatile unsigned int PadFcrtl;
    volatile unsigned int Pad22;
    volatile unsigned int Fcttv;
    volatile unsigned int Pad23;
    volatile unsigned int Txcw;
    volatile unsigned int Pad24;
    volatile unsigned int Rxcw;
    volatile unsigned char Pad25[0x7C];
    volatile unsigned int Mta[(128)];

    volatile unsigned int Tctl;
    volatile unsigned int Pad26;
    volatile unsigned int Tqsal;
    volatile unsigned int Tqsah;
    volatile unsigned int Tipg;
    volatile unsigned int Pad27;
    volatile unsigned int Tqc;
    volatile unsigned int Pad28;
    volatile unsigned int PadTdbal;
    volatile unsigned int PadTdbah;
    volatile unsigned int PadTdl;
    volatile unsigned int Pad29;
    volatile unsigned int PadTdh;
    volatile unsigned int Pad30;
    volatile unsigned int PadTdt;
    volatile unsigned int Pad31;
    volatile unsigned int PadTidv;
    volatile unsigned int Pad32;
    volatile unsigned int Tbt;
    volatile unsigned char Pad33[0x0C];

    volatile unsigned int Ait;
    volatile unsigned char Pad34[0xA4];

    volatile unsigned int Ftr[8];
    volatile unsigned int Fcr;
    volatile unsigned int Pad35;
    volatile unsigned int Trcr;

    volatile unsigned char Pad36[0xD4];

    volatile unsigned int Vfta[(128)];
    volatile unsigned char Pad37[0x800];

    volatile unsigned int Pba;
    volatile unsigned char Pad38[0xFFC];

    volatile unsigned char Pad39[0x8];
    volatile unsigned int Ert;
    volatile unsigned char Pad40[0xf4];

    volatile unsigned char Pad41[0x60];
    volatile unsigned int Fcrtl;
    volatile unsigned int Pad42;
    volatile unsigned int Fcrth;
    volatile unsigned char Pad43[0x294];

    volatile unsigned char Pad44[0x10];
    volatile unsigned int Rdfh;
    volatile unsigned int Pad45;
    volatile unsigned int Rdft;
    volatile unsigned char Pad46[0x3e4];

    volatile unsigned int Rdbal0;
    volatile unsigned int Rdbah0;
    volatile unsigned int Rdlen0;
    volatile unsigned int Pad47;
    volatile unsigned int Rdh0;
    volatile unsigned int Pad48;
    volatile unsigned int Rdt0;
    volatile unsigned int Pad49;
    volatile unsigned int Rdtr0;
    volatile unsigned int Pad50;
    volatile unsigned int Rxdctl;
    volatile unsigned int Pad51;
    volatile unsigned int Rddh0;
    volatile unsigned int Pad52;
    volatile unsigned int Rddt0;
    volatile unsigned char Pad53[0x7C4];

    volatile unsigned int Txdmac;
    volatile unsigned int Pad54;
    volatile unsigned int Ett;
    volatile unsigned char Pad55[0x3f4];

    volatile unsigned char Pad56[0x10];
    volatile unsigned int Tdfh;
    volatile unsigned int Pad57;
    volatile unsigned int Tdft;
    volatile unsigned int Pad57a;
    volatile unsigned int Tdfhs;
    volatile unsigned int Pad57b;
    volatile unsigned int Tdfts;
    volatile unsigned int Pad57c;
    volatile unsigned int Tdfpc;
    volatile unsigned char Pad58[0x3cc];

    volatile unsigned int Tdbal;
    volatile unsigned int Tdbah;
    volatile unsigned int Tdl;
    volatile unsigned int Pad59;
    volatile unsigned int Tdh;
    volatile unsigned int Pad60;
    volatile unsigned int Tdt;
    volatile unsigned int Pad61;
    volatile unsigned int Tidv;
    volatile unsigned int Pad62;
    volatile unsigned int Txdctl;
    volatile unsigned int Pad63;
    volatile unsigned int Tddh;
    volatile unsigned int Pad64;
    volatile unsigned int Tddt;
    volatile unsigned char Pad65[0x7C4];

    volatile unsigned int Crcerrs;
    volatile unsigned int Algnerrc;
    volatile unsigned int Symerrs;
    volatile unsigned int Rxerrc;
    volatile unsigned int Mpc;
    volatile unsigned int Scc;
    volatile unsigned int Ecol;
    volatile unsigned int Mcc;
    volatile unsigned int Latecol;
    volatile unsigned int Pad66;
    volatile unsigned int Colc;
    volatile unsigned int Tuc;
    volatile unsigned int Dc;
    volatile unsigned int Tncrs;
    volatile unsigned int Sec;
    volatile unsigned int Cexterr;
    volatile unsigned int Rlec;
    volatile unsigned int Rutec;
    volatile unsigned int Xonrxc;
    volatile unsigned int Xontxc;
    volatile unsigned int Xoffrxc;
    volatile unsigned int Xofftxc;
    volatile unsigned int Fcruc;
    volatile unsigned int Prc64;
    volatile unsigned int Prc127;
    volatile unsigned int Prc255;
    volatile unsigned int Prc511;
    volatile unsigned int Prc1023;
    volatile unsigned int Prc1522;
    volatile unsigned int Gprc;
    volatile unsigned int Bprc;
    volatile unsigned int Mprc;
    volatile unsigned int Gptc;
    volatile unsigned int Pad67;
    volatile unsigned int Gorl;
    volatile unsigned int Gorh;
    volatile unsigned int Gotl;
    volatile unsigned int Goth;
    volatile unsigned char Pad68[8];
    volatile unsigned int Rnbc;
    volatile unsigned int Ruc;
    volatile unsigned int Rfc;
    volatile unsigned int Roc;
    volatile unsigned int Rjc;
    volatile unsigned char Pad69[0xC];
    volatile unsigned int Torl;
    volatile unsigned int Torh;
    volatile unsigned int Totl;
    volatile unsigned int Toth;
    volatile unsigned int Tpr;
    volatile unsigned int Tpt;
    volatile unsigned int Ptc64;
    volatile unsigned int Ptc127;
    volatile unsigned int Ptc255;
    volatile unsigned int Ptc511;
    volatile unsigned int Ptc1023;
    volatile unsigned int Ptc1522;
    volatile unsigned int Mptc;
    volatile unsigned int Bptc;

    volatile unsigned int Tsctc;
    volatile unsigned int Tsctfc;

    volatile unsigned char Pad70[0x0F00];

    volatile unsigned int Rxcsum;
    volatile unsigned char Pad71[0x2FFC];

    volatile unsigned int PadRdfh;
    volatile unsigned int Pad72;
    volatile unsigned int PadRdft;
    volatile unsigned int Pad73;
    volatile unsigned int PadTdfh;
    volatile unsigned int Pad74;
    volatile unsigned int PadTdft;
    volatile unsigned char Pad75[0x7FE4];
    volatile unsigned int Pbm[0x4000];

} E1000_REGISTERS, *PE1000_REGISTERS;

#define E1000_CTRL_FD              (0x00000001)
#define E1000_CTRL_BEM             (0x00000002)
#define E1000_CTRL_PRIOR           (0x00000004)
#define E1000_CTRL_LRST            (0x00000008)
#define E1000_CTRL_TME             (0x00000010)
#define E1000_CTRL_SLE             (0x00000020)
#define E1000_CTRL_ASDE            (0x00000020)
#define E1000_CTRL_SLU             (0x00000040)

#define E1000_CTRL_ILOS            (0x00000080)
#define E1000_CTRL_SPD_SEL         (0x00000300)
#define E1000_CTRL_SPD_10          (0x00000000)
#define E1000_CTRL_SPD_100         (0x00000100)
#define E1000_CTRL_SPD_1000        (0x00000200)
#define E1000_CTRL_BEM32           (0x00000400)
#define E1000_CTRL_FRCSPD          (0x00000800)
#define E1000_CTRL_FRCDPX          (0x00001000)

#define E1000_CTRL_SWDPIN0         (0x00040000)

#define E1000_CTRL_SWDPIN1         (0x00080000)

#define E1000_CTRL_SWDPIN2         (0x00100000)
#define E1000_CTRL_SWDPIN3         (0x00200000)
#define E1000_CTRL_SWDPIO0         (0x00400000)
#define E1000_CTRL_SWDPIO1         (0x00800000)
#define E1000_CTRL_SWDPIO2         (0x01000000)
#define E1000_CTRL_SWDPIO3         (0x02000000)
#define E1000_CTRL_RST             (0x04000000)
#define E1000_CTRL_RFCE            (0x08000000)
#define E1000_CTRL_TFCE            (0x10000000)

#define E1000_CTRL_RTE             (0x20000000)
#define E1000_CTRL_VME             (0x40000000)

#define E1000_STATUS_FD            (0x00000001)
#define E1000_STATUS_LU            (0x00000002)
#define E1000_STATUS_TCKOK         (0x00000004)
#define E1000_STATUS_RBCOK         (0x00000008)
#define E1000_STATUS_TXOFF         (0x00000010)
#define E1000_STATUS_TBIMODE       (0x00000020)
#define E1000_STATUS_SPEED_10      (0x00000000)
#define E1000_STATUS_SPEED_100     (0x00000040)
#define E1000_STATUS_SPEED_1000    (0x00000080)
#define E1000_STATUS_ASDV          (0x00000300)
#define E1000_STATUS_MTXCKOK       (0x00000400)
#define E1000_STATUS_PCI66         (0x00000800)
#define E1000_STATUS_BUS64         (0x00001000)

#define E1000_EESK                 (0x00000001)
#define E1000_EECS                 (0x00000002)
#define E1000_EEDI                 (0x00000004)
#define E1000_EEDO                 (0x00000008)
#define E1000_FLASH_WRITE_DIS      (0x00000010)
#define E1000_FLASH_WRITE_EN       (0x00000020)

#define E1000_EXCTRL_GPI_EN0       (0x00000001)
#define E1000_EXCTRL_GPI_EN1       (0x00000002)
#define E1000_EXCTRL_GPI_EN2       (0x00000004)
#define E1000_EXCTRL_GPI_EN3       (0x00000008)
#define E1000_EXCTRL_SWDPIN4       (0x00000010)
#define E1000_EXCTRL_SWDPIN5       (0x00000020)
#define E1000_EXCTRL_SWDPIN6       (0x00000040)
#define E1000_EXCTRL_SWDPIN7       (0x00000080)
#define E1000_EXCTRL_SWDPIO4       (0x00000100)
#define E1000_EXCTRL_SWDPIO5       (0x00000200)
#define E1000_EXCTRL_SWDPIO6       (0x00000400)
#define E1000_EXCTRL_SWDPIO7       (0x00000800)
#define E1000_EXCTRL_ASDCHK        (0x00001000)
#define E1000_EXCTRL_EE_RST        (0x00002000)
#define E1000_EXCTRL_IPS           (0x00004000)
#define E1000_EXCTRL_SPD_BYPS      (0x00008000)

#define E1000_MDI_WRITE            (0x04000000)
#define E1000_MDI_READ             (0x08000000)
#define E1000_MDI_READY            (0x10000000)
#define E1000_MDI_INT              (0x20000000)
#define E1000_MDI_ERR              (0x40000000)

#define E1000_RAH_RDR              (0x40000000)
#define E1000_RAH_AV               (0x80000000)

#define E1000_ICR_TXDW             (0x00000001)
#define E1000_ICR_TXQE             (0x00000002)
#define E1000_ICR_LSC              (0x00000004)
#define E1000_ICR_RXSEQ            (0x00000008)
#define E1000_ICR_RXDMT0           (0x00000010)
#define E1000_ICR_RXDMT1           (0x00000020)
#define E1000_ICR_RXO              (0x00000040)
#define E1000_ICR_RXT0             (0x00000080)
#define E1000_ICR_RXT1             (0x00000100)
#define E1000_ICR_PCIE             (0x00000200)
#define E1000_ICR_MDIAC            (0x00000200)
#define E1000_ICR_RXCFG            (0x00000400)
#define E1000_ICR_GPI_EN0          (0x00000800)
#define E1000_ICR_GPI_EN1          (0x00001000)
#define E1000_ICR_GPI_EN2          (0x00002000)
#define E1000_ICR_GPI_EN3          (0x00004000)

#define E1000_ICS_TXDW             E1000_ICR_TXDW
#define E1000_ICS_TXQE             E1000_ICR_TXQE
#define E1000_ICS_LSC              E1000_ICR_LSC
#define E1000_ICS_RXSEQ            E1000_ICR_RXSEQ
#define E1000_ICS_RXDMT0           E1000_ICR_RXDMT0
#define E1000_ICS_RXDMT1           E1000_ICR_RXDMT1
#define E1000_ICS_RXO              E1000_ICR_RXO
#define E1000_ICS_RXT0             E1000_ICR_RXT0
#define E1000_ICS_RXT1             E1000_ICR_RXT1
#define E1000_ICS_PCIE             E1000_ICR_PCIE
#define E1000_ICS_MDIAC            E1000_ICR_MDIAC
#define E1000_ICS_RXCFG            E1000_ICR_RXCFG
#define E1000_ICS_GPI_EN0          E1000_ICR_GPI_EN0
#define E1000_ICS_GPI_EN1          E1000_ICR_GPI_EN1
#define E1000_ICS_GPI_EN2          E1000_ICR_GPI_EN2
#define E1000_ICS_GPI_EN3          E1000_ICR_GPI_EN3

#define E1000_IMS_TXDW             E1000_ICR_TXDW
#define E1000_IMS_TXQE             E1000_ICR_TXQE
#define E1000_IMS_LSC              E1000_ICR_LSC
#define E1000_IMS_RXSEQ            E1000_ICR_RXSEQ
#define E1000_IMS_RXDMT0           E1000_ICR_RXDMT0
#define E1000_IMS_RXDMT1           E1000_ICR_RXDMT1
#define E1000_IMS_RXO              E1000_ICR_RXO
#define E1000_IMS_RXT0             E1000_ICR_RXT0
#define E1000_IMS_RXT1             E1000_ICR_RXT1
#define E1000_IMS_PCIE             E1000_ICR_PCIE
#define E1000_IMS_MDIAC            E1000_ICR_MDIAC
#define E1000_IMS_RXCFG            E1000_ICR_RXCFG
#define E1000_IMS_GPI_EN0          E1000_ICR_GPI_EN0
#define E1000_IMS_GPI_EN1          E1000_ICR_GPI_EN1
#define E1000_IMS_GPI_EN2          E1000_ICR_GPI_EN2
#define E1000_IMS_GPI_EN3          E1000_ICR_GPI_EN3

#define E1000_IMC_TXDW             E1000_ICR_TXDW
#define E1000_IMC_TXQE             E1000_ICR_TXQE
#define E1000_IMC_LSC              E1000_ICR_LSC
#define E1000_IMC_RXSEQ            E1000_ICR_RXSEQ
#define E1000_IMC_RXDMT0           E1000_ICR_RXDMT0
#define E1000_IMC_RXDMT1           E1000_ICR_RXDMT1
#define E1000_IMC_RXO              E1000_ICR_RXO
#define E1000_IMC_RXT0             E1000_ICR_RXT0
#define E1000_IMC_RXT1             E1000_ICR_RXT1
#define E1000_IMC_PCIE             E1000_ICR_PCIE
#define E1000_IMC_MDIAC            E1000_ICR_MDIAC
#define E1000_IMC_RXCFG            E1000_ICR_RXCFG
#define E1000_IMC_GPI_EN0          E1000_ICR_GPI_EN0
#define E1000_IMC_GPI_EN1          E1000_ICR_GPI_EN1
#define E1000_IMC_GPI_EN2          E1000_ICR_GPI_EN2
#define E1000_IMC_GPI_EN3          E1000_ICR_GPI_EN3

#define E1000_TINT_RINT_PCI        (E1000_TXDW|E1000_ICR_RXT0|E1000_ICR_PCIE)
#define E1000_CAUSE_ERR            (E1000_ICR_RXSEQ|E1000_ICR_RXO)

#define E1000_RCTL_RST             (0x00000001)
#define E1000_RCTL_EN              (0x00000002)
#define E1000_RCTL_SBP             (0x00000004)
#define E1000_RCTL_UPE             (0x00000008)
#define E1000_RCTL_MPE             (0x00000010)
#define E1000_RCTL_LPE             (0x00000020)
#define E1000_RCTL_LBM_NO          (0x00000000)
#define E1000_RCTL_LBM_MAC         (0x00000040)
#define E1000_RCTL_LBM_SLP         (0x00000080)
#define E1000_RCTL_LBM_TCVR        (0x000000c0)
#define E1000_RCTL_RDMTS0_HALF     (0x00000000)
#define E1000_RCTL_RDMTS0_QUAT     (0x00000100)
#define E1000_RCTL_RDMTS0_EIGTH    (0x00000200)
#define E1000_RCTL_RDMTS1_HALF     (0x00000000)
#define E1000_RCTL_RDMTS1_QUAT     (0x00000400)
#define E1000_RCTL_RDMTS1_EIGTH    (0x00000800)
#define E1000_RCTL_MO_SHIFT        12

#define E1000_RCTL_MO_0            (0x00000000)
#define E1000_RCTL_MO_1            (0x00001000)
#define E1000_RCTL_MO_2            (0x00002000)
#define E1000_RCTL_MO_3            (0x00003000)

#define E1000_RCTL_MDR             (0x00004000)
#define E1000_RCTL_BAM             (0x00008000)

#define E1000_RCTL_SZ_2048         (0x00000000)
#define E1000_RCTL_SZ_1024         (0x00010000)
#define E1000_RCTL_SZ_512          (0x00020000)
#define E1000_RCTL_SZ_256          (0x00030000)

#define E1000_RCTL_SZ_16384        (0x00010000)
#define E1000_RCTL_SZ_8192         (0x00020000)
#define E1000_RCTL_SZ_4096         (0x00030000)

#define E1000_RCTL_VFE             (0x00040000)

#define E1000_RCTL_CFIEN           (0x00080000)
#define E1000_RCTL_CFI             (0x00100000)
#define E1000_RCTL_ISLE            (0x00200000)

#define E1000_RCTL_DPF             (0x00400000)
#define E1000_RCTL_PMCF            (0x00800000)

#define E1000_RCTL_SISLH           (0x01000000)

#define E1000_RCTL_BSEX            (0x02000000)
#define E1000_RDT0_DELAY           (0x0000ffff)
#define E1000_RDT0_FPDB            (0x80000000)

#define E1000_RDT1_DELAY           (0x0000ffff)
#define E1000_RDT1_FPDB            (0x80000000)

#define E1000_RDLEN0_LEN           (0x0007ff80)

#define E1000_RDLEN1_LEN           (0x0007ff80)

#define E1000_RDH0_RDH             (0x0000ffff)

#define E1000_RDH1_RDH             (0x0000ffff)

#define E1000_RDT0_RDT             (0x0000ffff)

#define E1000_FCRTH_RTH            (0x0000FFF8)
#define E1000_FCRTH_XFCE           (0x80000000)

#define E1000_FCRTL_RTL            (0x0000FFF8)
#define E1000_FCRTL_XONE           (0x80000000)

#define E1000_RXDCTL_PTHRESH       0x0000003F
#define E1000_RXDCTL_HTHRESH       0x00003F00
#define E1000_RXDCTL_WTHRESH       0x003F0000
#define E1000_RXDCTL_GRAN          0x01000000

#define E1000_TXDCTL_PTHRESH       0x000000FF
#define E1000_TXDCTL_HTHRESH       0x0000FF00
#define E1000_TXDCTL_WTHRESH       0x00FF0000
#define E1000_TXDCTL_GRAN          0x01000000

#define E1000_TXCW_FD              (0x00000020)
#define E1000_TXCW_HD              (0x00000040)
#define E1000_TXCW_PAUSE           (0x00000080)
#define E1000_TXCW_ASM_DIR         (0x00000100)
#define E1000_TXCW_PAUSE_MASK      (0x00000180)
#define E1000_TXCW_RF              (0x00003000)
#define E1000_TXCW_NP              (0x00008000)
#define E1000_TXCW_CW              (0x0000ffff)
#define E1000_TXCW_TXC             (0x40000000)
#define E1000_TXCW_ANE             (0x80000000)

#define E1000_RXCW_CW              (0x0000ffff)
#define E1000_RXCW_NC              (0x04000000)
#define E1000_RXCW_IV              (0x08000000)
#define E1000_RXCW_CC              (0x10000000)
#define E1000_RXCW_C               (0x20000000)
#define E1000_RXCW_SYNCH           (0x40000000)
#define E1000_RXCW_ANC             (0x80000000)

#define E1000_TCTL_RST             (0x00000001)
#define E1000_TCTL_EN              (0x00000002)
#define E1000_TCTL_BCE             (0x00000004)
#define E1000_TCTL_PSP             (0x00000008)
#define E1000_TCTL_CT              (0x00000ff0)
#define E1000_TCTL_COLD            (0x003ff000)
#define E1000_TCTL_SWXOFF          (0x00400000)
#define E1000_TCTL_PBE             (0x00800000)
#define E1000_TCTL_RTLC            (0x01000000)
#define E1000_TCTL_NRTU            (0x02000000)

#define E1000_TQSAL_TQSAL          (0xffffffc0)
#define E1000_TQSAH_TQSAH          (0xffffffff)

#define E1000_TQC_SQ               (0x00000001)
#define E1000_TQC_RQ               (0x00000002)

#define E1000_TDBAL_TDBAL          (0xfffff000)
#define E1000_TDBAH_TDBAH          (0xffffffff)

#define E1000_TDL_LEN              (0x0007ff80)

#define E1000_TDH_TDH              (0x0000ffff)

#define E1000_TDT_TDT              (0x0000ffff)

#define E1000_RXCSUM_PCSS          (0x000000ff)
#define E1000_RXCSUM_IPOFL         (0x00000100)
#define E1000_RXCSUM_TUOFL         (0x00000200)

#define WX_WRITE_REG(wx, reg, value)	((wx)->regs->reg = (UINT32)(value))
#define WX_READ_REG(wx, reg)			((wx)->regs->reg)

#define E1000_WRITE_REG(reg, value)		WX_WRITE_REG(Adapter, reg, value)
#define E1000_READ_REG(reg)				WX_READ_REG(Adapter, reg)

#define E1000_MDALIGN               (4096)

#define EEPROM_READ_OPCODE          (0x6)
#define EEPROM_WRITE_OPCODE         (0x5)
#define EEPROM_ERASE_OPCODE         (0x7)
#define EEPROM_EWEN_OPCODE          (0x13)
#define EEPROM_EWDS_OPCODE          (0x10)

#define EEPROM_INIT_CONTROL1_REG    (0x000A)
#define EEPROM_INIT_CONTROL2_REG    (0x000F)
#define EEPROM_CHECKSUM_REG         (0x003F)

#define EEPROM_WORD0A_ILOS          (0x0010)
#define EEPROM_WORD0A_SWDPIO        (0x01E0)
#define EEPROM_WORD0A_LRST          (0x0200)
#define EEPROM_WORD0A_FD            (0x0400)
#define EEPROM_WORD0A_66MHZ         (0x0800)

#define EEPROM_WORD0F_PAUSE_MASK    (0x3000)
#define EEPROM_WORD0F_PAUSE         (0x1000)
#define EEPROM_WORD0F_ASM_DIR       (0x2000)
#define EEPROM_WORD0F_ANE           (0x0800)
#define EEPROM_WORD0F_SWPDIO_EXT    (0x00F0)

#define EEPROM_SUM                  (0xBABA)

#define EEPROM_NODE_ADDRESS_BYTE_0  (0)
#define EEPROM_PBA_BYTE_1           (8)

#define EEPROM_WORD_SIZE            (64)

#define NODE_ADDRESS_SIZE           (6)
#define PBA_SIZE                    (4)

#define E1000_COLLISION_THRESHOLD   16
#define E1000_CT_SHIFT              4

#define E1000_FDX_COLLISION_DISTANCE 64
#define E1000_HDX_COLLISION_DISTANCE 64
#define E1000_GB_HDX_COLLISION_DISTANCE 512
#define E1000_COLD_SHIFT            12

#define REQ_TX_DESCRIPTOR_MULTIPLE  8
#define REQ_RX_DESCRIPTOR_MULTIPLE  8

#define DEFAULT_WSMN_TIPG_IPGT      10

#define DEFAULT_LVGD_TIPG_IPGT_FIBER 6

#define DEFAULT_LVGD_TIPG_IPGT_COPPER 8

#define E1000_TIPG_IPGT_MASK        0x000003FF
#define E1000_TIPG_IPGR1_MASK       0x000FFC00
#define E1000_TIPG_IPGR2_MASK       0x3FF00000

#define DEFAULT_WSMN_TIPG_IPGR1     2
#define DEFAULT_LVGD_TIPG_IPGR1     8
#define E1000_TIPG_IPGR1_SHIFT      10

#define DEFAULT_WSMN_TIPG_IPGR2     10
#define DEFAULT_LVGD_TIPG_IPGR2     6
#define E1000_TIPG_IPGR2_SHIFT      20

#define E1000_TXDMAC_DPP            0x00000001

#define FLOW_CONTROL_ADDRESS_LOW    (0x00C28001)
#define FLOW_CONTROL_ADDRESS_HIGH   (0x00000100)
#define FLOW_CONTROL_TYPE           (0x8808)
#define FC_DEFAULT_HI_THRESH        (0x8000)
#define FC_DEFAULT_LO_THRESH        (0x4000)
#define FC_DEFAULT_TX_TIMER         (0x100)

#define PAUSE_SHIFT 5

#define SWDPIO_SHIFT 17

#define SWDPIO__EXT_SHIFT 4

#define ILOS_SHIFT  3

#define MDI_REGADD_SHIFT 16

#define MDI_PHYADD_SHIFT 21

#define RECEIVE_BUFFER_ALIGN_SIZE  (256)

#define LINK_UP_TIMEOUT             500

#define E1000_TX_BUFFER_SIZE ((UINT)1514)

#define E1000_MIN_SIZE_OF_RECEIVE_BUFFERS (2048)

#define E1000_SIZE_OF_RECEIVE_BUFFERS (2048)

#define E1000_SIZE_OF_UNALIGNED_RECEIVE_BUFFERS E1000_SIZE_OF_RECEIVE_BUFFERS+RECEIVE_BUFFER_ALIGN_SIZE

#define COALESCE_BUFFER_SIZE  0x800
#define COALESCE_BUFFER_ALIGN 0x800

#define E1000_WAIT_PERIOD           10

/* number of Tx/Rx descriptors to allocate - must be multiples of 8 */
#define NUM_TX_BUFS		8
#define NUM_RX_BUFS		8
#define BUF_SIZE		E1000_SIZE_OF_RECEIVE_BUFFERS


#define E1000_CTRL_PHY_RESET_DIR  E1000_CTRL_SWDPIO0
#define E1000_CTRL_PHY_RESET      E1000_CTRL_SWDPIN0
#define E1000_CTRL_MDIO_DIR       E1000_CTRL_SWDPIO2
#define E1000_CTRL_MDIO           E1000_CTRL_SWDPIN2
#define E1000_CTRL_MDC_DIR        E1000_CTRL_SWDPIO3
#define E1000_CTRL_MDC            E1000_CTRL_SWDPIN3
#define E1000_CTRL_PHY_RESET_DIR4 E1000_EXCTRL_SWDPIO4
#define E1000_CTRL_PHY_RESET4     E1000_EXCTRL_SWDPIN4

#define PHY_MII_CTRL_REG             0x00
#define PHY_MII_STATUS_REG           0x01
#define PHY_PHY_ID_REG1              0x02
#define PHY_PHY_ID_REG2              0x03
#define PHY_AUTONEG_ADVERTISEMENT    0x04
#define PHY_AUTONEG_LP_BPA           0x05
#define PHY_AUTONEG_EXPANSION_REG    0x06
#define PHY_AUTONEG_NEXT_PAGE_TX     0x07
#define PHY_AUTONEG_LP_RX_NEXT_PAGE  0x08
#define PHY_1000T_CTRL_REG           0x09
#define PHY_1000T_STATUS_REG         0x0A
#define PHY_IEEE_EXT_STATUS_REG      0x0F

#define PXN_PHY_SPEC_CTRL_REG        0x10
#define PXN_PHY_SPEC_STAT_REG        0x11
#define PXN_INT_ENABLE_REG           0x12
#define PXN_INT_STATUS_REG           0x13
#define PXN_EXT_PHY_SPEC_CTRL_REG    0x14
#define PXN_RX_ERROR_COUNTER         0x15
#define PXN_LED_CTRL_REG             0x18

#define MAX_PHY_REG_ADDRESS          0x1F

#define MII_CR_SPEED_SELECT_MSB   0x0040
#define MII_CR_COLL_TEST_ENABLE   0x0080
#define MII_CR_FULL_DUPLEX        0x0100
#define MII_CR_RESTART_AUTO_NEG   0x0200
#define MII_CR_ISOLATE            0x0400
#define MII_CR_POWER_DOWN         0x0800
#define MII_CR_AUTO_NEG_EN        0x1000
#define MII_CR_SPEED_SELECT_LSB   0x2000
#define MII_CR_LOOPBACK           0x4000
#define MII_CR_RESET              0x8000

#define MII_SR_EXTENDED_CAPS      0x0001
#define MII_SR_JABBER_DETECT      0x0002
#define MII_SR_LINK_STATUS        0x0004
#define MII_SR_AUTONEG_CAPS       0x0008
#define MII_SR_REMOTE_FAULT       0x0010
#define MII_SR_AUTONEG_COMPLETE   0x0020
#define MII_SR_PREAMBLE_SUPPRESS  0x0040
#define MII_SR_EXTENDED_STATUS    0x0100
#define MII_SR_100T2_HD_CAPS      0x0200
#define MII_SR_100T2_FD_CAPS      0x0400
#define MII_SR_10T_HD_CAPS        0x0800
#define MII_SR_10T_FD_CAPS        0x1000
#define MII_SR_100X_HD_CAPS       0x2000
#define MII_SR_100X_FD_CAPS       0x4000
#define MII_SR_100T4_CAPS         0x8000

#define NWAY_AR_SELECTOR_FIELD    0x0001
#define NWAY_AR_10T_HD_CAPS       0x0020
#define NWAY_AR_10T_FD_CAPS       0x0040
#define NWAY_AR_100TX_HD_CAPS     0x0080
#define NWAY_AR_100TX_FD_CAPS     0x0100
#define NWAY_AR_100T4_CAPS        0x0200
#define NWAY_AR_PAUSE             0x0400
#define NWAY_AR_ASM_DIR           0x0800
#define NWAY_AR_REMOTE_FAULT      0x2000
#define NWAY_AR_NEXT_PAGE         0x8000

#define NWAY_LPAR_SELECTOR_FIELD  0x0000
#define NWAY_LPAR_10T_HD_CAPS     0x0020
#define NWAY_LPAR_10T_FD_CAPS     0x0040
#define NWAY_LPAR_100TX_HD_CAPS   0x0080
#define NWAY_LPAR_100TX_FD_CAPS   0x0100
#define NWAY_LPAR_100T4_CAPS      0x0200
#define NWAY_LPAR_PAUSE           0x0400
#define NWAY_LPAR_ASM_DIR         0x0800
#define NWAY_LPAR_REMOTE_FAULT    0x2000
#define NWAY_LPAR_ACKNOWLEDGE     0x4000
#define NWAY_LPAR_NEXT_PAGE       0x8000

#define NWAY_ER_LP_NWAY_CAPS      0x0001
#define NWAY_ER_PAGE_RXD          0x0002
#define NWAY_ER_NEXT_PAGE_CAPS    0x0004
#define NWAY_ER_LP_NEXT_PAGE_CAPS 0x0008
#define NWAY_ER_PAR_DETECT_FAULT  0x0100

#define NPTX_MSG_CODE_FIELD       0x0001
#define NPTX_TOGGLE               0x0800

#define NPTX_ACKNOWLDGE2          0x1000

#define NPTX_MSG_PAGE             0x2000
#define NPTX_NEXT_PAGE            0x8000

#define LP_RNPR_MSG_CODE_FIELD    0x0001
#define LP_RNPR_TOGGLE            0x0800

#define LP_RNPR_ACKNOWLDGE2       0x1000

#define LP_RNPR_MSG_PAGE          0x2000
#define LP_RNPR_ACKNOWLDGE        0x4000
#define LP_RNPR_NEXT_PAGE         0x8000

#define CR_1000T_ASYM_PAUSE       0x0080
#define CR_1000T_HD_CAPS          0x0100

#define CR_1000T_FD_CAPS          0x0200

#define CR_1000T_REPEATER_DTE     0x0400

#define CR_1000T_MS_VALUE         0x0800

#define CR_1000T_MS_ENABLE        0x1000

#define CR_1000T_TEST_MODE_NORMAL 0x0000
#define CR_1000T_TEST_MODE_1      0x2000
#define CR_1000T_TEST_MODE_2      0x4000
#define CR_1000T_TEST_MODE_3      0x6000
#define CR_1000T_TEST_MODE_4      0x8000

#define SR_1000T_IDLE_ERROR_CNT   0x0000
#define SR_1000T_ASYM_PAUSE_DIR   0x0100
#define SR_1000T_LP_HD_CAPS       0x0400

#define SR_1000T_LP_FD_CAPS       0x0800

#define SR_1000T_REMOTE_RX_STATUS 0x1000
#define SR_1000T_LOCAL_RX_STATUS  0x2000
#define SR_1000T_MS_CONFIG_RES    0x4000
#define SR_1000T_MS_CONFIG_FAULT  0x8000

#define IEEE_ESR_1000T_HD_CAPS    0x1000

#define IEEE_ESR_1000T_FD_CAPS    0x2000

#define IEEE_ESR_1000X_HD_CAPS    0x4000

#define IEEE_ESR_1000X_FD_CAPS    0x8000

#define PHY_TX_POLARITY_MASK        0x0100
#define PHY_TX_NORMAL_POLARITY      0

#define AUTO_POLARITY_DISABLE       0x0010

#define PXN_PSCR_JABBER_DISABLE         0x0001
#define PXN_PSCR_POLARITY_REVERSAL      0x0002
#define PXN_PSCR_SQE_TEST           0x0004
#define PXN_PSCR_INT_FIFO_DISABLE        0x0008

#define PXN_PSCR_CLK125_DISABLE         0x0010
#define PXN_PSCR_MDI_MANUAL_MODE        0x0000

#define PXN_PSCR_MDIX_MANUAL_MODE       0x0020
#define PXN_PSCR_AUTO_X_1000T           0x0040
#define PXN_PSCR_AUTO_X_MODE            0x0060
#define PXN_PSCR_10BT_EXT_DIST_ENABLE   0x0080
#define PXN_PSCR_MII_5BIT_ENABLE        0x0100
#define PXN_PSCR_SCRAMBLER_DISABLE      0x0200
#define PXN_PSCR_FORCE_LINK_GOOD        0x0400
#define PXN_PSCR_ASSERT_CRS_ON_TX       0x0800
#define PXN_PSCR_RX_FIFO_DEPTH_6        0x0000
#define PXN_PSCR_RX_FIFO_DEPTH_8        0x1000
#define PXN_PSCR_RX_FIFO_DEPTH_10       0x2000
#define PXN_PSCR_RX_FIFO_DEPTH_12       0x3000

#define PXN_PSCR_TXFR_FIFO_DEPTH_6      0x0000
#define PXN_PSCR_TXFR_FIFO_DEPTH_8      0x4000
#define PXN_PSCR_TXFR_FIFO_DEPTH_10     0x8000
#define PXN_PSCR_TXFR_FIFO_DEPTH_12     0xC000

#define PXN_PSSR_JABBER             0x0001
#define PXN_PSSR_REV_POLARITY       0x0002
#define PXN_PSSR_MDIX               0x0020
#define PXN_PSSR_LINK               0x0400
#define PXN_PSSR_SPD_DPLX_RESOLVED  0x0800
#define PXN_PSSR_PAGE_RCVD          0x1000
#define PXN_PSSR_DPLX               0x2000
#define PXN_PSSR_SPEED              0xC000
#define PXN_PSSR_10MBS              0x0000
#define PXN_PSSR_100MBS             0x4000
#define PXN_PSSR_1000MBS            0x8000

#define PXN_IER_JABBER              0x0001
#define PXN_IER_POLARITY_CHANGE     0x0002
#define PXN_IER_MDIX_CHANGE         0x0040
#define PXN_IER_FIFO_OVER_UNDERUN   0x0080
#define PXN_IER_FALSE_CARRIER       0x0100
#define PXN_IER_SYMBOL_ERROR        0x0200
#define PXN_IER_LINK_STAT_CHANGE    0x0400
#define PXN_IER_AUTO_NEG_COMPLETE   0x0800
#define PXN_IER_PAGE_RECEIVED       0x1000
#define PXN_IER_DUPLEX_CHANGED      0x2000
#define PXN_IER_SPEED_CHANGED       0x4000
#define PXN_IER_AUTO_NEG_ERR        0x8000

#define PXN_ISR_JABBER              0x0001
#define PXN_ISR_POLARITY_CHANGE     0x0002
#define PXN_ISR_MDIX_CHANGE         0x0040
#define PXN_ISR_FIFO_OVER_UNDERUN   0x0080
#define PXN_ISR_FALSE_CARRIER       0x0100
#define PXN_ISR_SYMBOL_ERROR        0x0200
#define PXN_ISR_LINK_STAT_CHANGE    0x0400
#define PXN_ISR_AUTO_NEG_COMPLETE   0x0800
#define PXN_ISR_PAGE_RECEIVED       0x1000
#define PXN_ISR_DUPLEX_CHANGED      0x2000
#define PXN_ISR_SPEED_CHANGED       0x4000
#define PXN_ISR_AUTO_NEG_ERR        0x8000

#define PXN_EPSCR_FIBER_LOOPBACK    0x4000
#define PXN_EPSCR_DOWN_NO_IDLE      0x8000

#define PXN_EPSCR_TX_CLK_2_5        0x0060
#define PXN_EPSCR_TX_CLK_25         0x0070
#define PXN_EPSCR_TX_CLK_0          0x0000

#define PXN_LCR_LED_TX          0x0001
#define PXN_LCR_LED_RX          0x0002
#define PXN_LCR_LED_DUPLEX      0x0004
#define PXN_LCR_LINK            0x0008
#define PXN_LCR_BLINK_RATE_42MS     0x0000
#define PXN_LCR_BLINK_RATE_84MS     0x0100
#define PXN_LCR_BLINK_RATE_170MS    0x0200
#define PXN_LCR_BLINK_RATE_340MS    0x0300
#define PXN_LCR_BLINK_RATE_670MS    0x0400

#define PXN_LCR_PULSE_STRETCH_OFF   0x0000
#define PXN_LCR_PULSE_STRETCH_21_42MS   0x1000
#define PXN_LCR_PULSE_STRETCH_42_84MS   0x2000
#define PXN_LCR_PULSE_STRETCH_84_170MS  0x3000
#define PXN_LCR_PULSE_STRETCH_170_340MS 0x4000
#define PXN_LCR_PULSE_STRETCH_340_670MS 0x5000
#define PXN_LCR_PULSE_STRETCH_670_13S   0x6000
#define PXN_LCR_PULSE_STRETCH_13_26S    0x7000

#define PHY_PREAMBLE                    0xFFFFFFFF
#define PHY_SOF                         0x01
#define PHY_OP_READ                     0x02
#define PHY_OP_WRITE                    0x01
#define PHY_TURNAROUND                  0x02

#define PHY_PREAMBLE_SIZE               32

#define MII_CR_SPEED_1000               0x0040
#define MII_CR_SPEED_100                0x2000
#define MII_CR_SPEED_10                 0x0000

#define E1000_PHY_ADDRESS               0x01
#define E1000_10MB_PHY_ADDRESS          0x02

#define PHY_AUTO_NEG_TIME               45

#define PAXSON_PHY_88E1000                0x01410C50
#define PAXSON_PHY_88E1000S               0x01410C40

#define PHY_REVISION_MASK               0xFFFFFFF0
#define AUTONEG_ADVERTISE_SPEED_DEFAULT 0x002F

#define DEVICE_SPEED_MASK               0x00000300

#define REG4_SPEED_MASK                 0x01E0
#define REG9_SPEED_MASK                 0x0300

#define ADVERTISE_10_HALF               0x0001
#define ADVERTISE_10_FULL               0x0002
#define ADVERTISE_100_HALF              0x0004
#define ADVERTISE_100_FULL              0x0008
#define ADVERTISE_1000_HALF             0x0010
#define ADVERTISE_1000_FULL             0x0020


/*
 * e1000_sw.h
 * 
 * This file has all the defines for all the software defined structures
 * that are necessary for the  hardware as well as the DLPI interface 
 */
#define E1000_PCI

/* The size in bytes of a standard ethernet header */
#define ENET_HEADER_SIZE    14
#define MAX_INTS            256
#define CRC_LENGTH 4 

#define MAXIMUM_ETHERNET_PACKET_SIZE    1514

#define MAX_NUM_MULTICAST_ADDRESSES     128
#define MAX_NUM_PHYS_FRAGS_PER_PACKET   16
#define E1000_NUM_MTA_REGISTERS         128
#define RCV_PKT_MUL              5


/* Ethernet Frame Structure */
/*- Ethernet 6-byte Address */
typedef struct eth_address_t {
    uchar_t eth_node_addr[ETH_LENGTH_OF_ADDRESS];
} eth_address_t, *peth_address_t;

typedef enum {
	ANE_ENABLED = 0,
	ANE_DISABLED,
	ANE_SUSPEND
} ANE_state_t;

/* This is the hardware dependant part of the board config structure called
 bdd_t structure, it is very similar to the Adapter structure in the
 driver. The bd_config structure defined by the DLPI interface has 
 pointer to this structure
*/
#define             TX_LOCKUP      0x01
typedef struct ADAPTER_STRUCT {
    /*  Hardware defines for the E1000 */
    PE1000_REGISTERS regs;    /* e1000 registers */

    /* Node Ethernet address */
    uchar_t perm_node_address[ETH_LENGTH_OF_ADDRESS];
    UCHAR CurrentNetAddress[ETH_LENGTH_OF_ADDRESS];
	UINT PartNumber;

    /* command line options */
    BOOLEAN AutoNeg;
    BOOLEAN ForcedSpeedDuplex;
    USHORT AutoNegAdvertised;
    BOOLEAN WaitAutoNegComplete;
    BOOLEAN Promiscuous;

    BOOLEAN GetLinkStatus;

    UCHAR MacType;
    UCHAR MediaType;
    UINT32 PhyId;
	UINT32 PhyAddress;

    BOOLEAN LinkStatusChanged;

    /* Flags for various hardware-software defines */
    UINT e1000_flags;            /* misc. flags */
    UINT32 AdapterStopped;        /* Adapter has been reset */
    UINT32 FullDuplex;            /* current duplex mode */
    UINT32 FlowControl;            /* Enable/Disable flow control */
	UINT32 OriginalFlowControl;
    uint_t flags;                /* Misc board flags    */

    BOOLEAN LongPacket;            /* Long Packet Enable Status */
	UINT	RxBufferLen;

    /* PCI configuration parameters  */
    UINT32 LinkIsActive;            /* Link status */
    UINT AutoNegFailed;            /* Auto-negotiation status flag */
    UINT TxcwRegValue;            /* Original Transmit control word */
    UINT TxIntDelay;            /* Transmit Interrupt delay */
    UINT RxIntDelay;            /* Receive Interrupt delay */

	char *dma_mem;		/* pointer to big chunk of dma-alloc memory */
	char *map_mem;		/* dma-mem mapped into device-space */
	UINT mem_len;		/* length of big chunk of memory */

    /*  Transmit descriptor definitions */
    PE1000_TRANSMIT_DESCRIPTOR FirstTxDescriptor;
    /* transmit descriptor ring start */
    PE1000_TRANSMIT_DESCRIPTOR LastTxDescriptor;
    /* transmit descriptor ring end */
    PE1000_TRANSMIT_DESCRIPTOR NextAvailTxDescriptor;
    /* next free tmd */
    PE1000_TRANSMIT_DESCRIPTOR OldestUsedTxDescriptor;
    /* next tmd to reclaim (used) */
    UINT NumTxDescriptorsAvail;
    UINT NumTxDescriptors;
    PE1000_TRANSMIT_DESCRIPTOR e1000_tbd_data;	/* dma-mapped memory */

	char *tx_bufs;		/* ptr to contiguous transmit buffers */
	char *rx_bufs;		/* ptr to contiguous receive buffers */

    /* Receive definitions */

    UINT NumRxDescriptors;
    UINT MulticastFilterType;

    /*  Receive descriptor definitions  */
    PE1000_RECEIVE_DESCRIPTOR FirstRxDescriptor;
    /* receive descriptor ring 1 start */

    PE1000_RECEIVE_DESCRIPTOR LastRxDescriptor;
    /* receive descriptor ring 1 end */

    PE1000_RECEIVE_DESCRIPTOR NextRxDescriptorToCheck;
    /* next chip rmd ring 1 */

    PE1000_RECEIVE_DESCRIPTOR e1000_rlast1p;
    /* last free rmd ring 1 */

    PE1000_RECEIVE_DESCRIPTOR e1000_rbd_data;
    /* pointer to dma-mapped rx buffer descriptor area */

    UINT32 cur_line_speed;        /* current line speed */

} ADAPTER_STRUCT, *PADAPTER_STRUCT;

/*- Ethernet 14-byte Header */
typedef struct eth_header_t {
    uchar_t eth_dest[ETH_LENGTH_OF_ADDRESS];
    uchar_t eth_src[ETH_LENGTH_OF_ADDRESS];
    ushort_t eth_typelen;
} eth_header_t, *peth_header_t;



STATIC VOID
RaiseMdcClock(IN PADAPTER_STRUCT Adapter, IN OUT UINT32 * CtrlRegValue)
{

    E1000_WRITE_REG(Ctrl, (*CtrlRegValue | E1000_CTRL_MDC));

    DelayInMicroseconds(2);
}

STATIC VOID
LowerMdcClock(IN PADAPTER_STRUCT Adapter, IN OUT UINT32 * CtrlRegValue)
{

    E1000_WRITE_REG(Ctrl, (*CtrlRegValue & ~E1000_CTRL_MDC));

    DelayInMicroseconds(2);
}

STATIC UINT16 MIIShiftInPhyData(IN PADAPTER_STRUCT Adapter)
{
    UINT32 CtrlRegValue;
    UINT16 Data = 0;
    UINT8 i;

    CtrlRegValue = E1000_READ_REG(Ctrl);

    CtrlRegValue &= ~E1000_CTRL_MDIO_DIR;
    CtrlRegValue &= ~E1000_CTRL_MDIO;

    E1000_WRITE_REG(Ctrl, CtrlRegValue);

    RaiseMdcClock(Adapter, &CtrlRegValue);
    LowerMdcClock(Adapter, &CtrlRegValue);

    for (Data = 0, i = 0; i < 16; i++) {
        Data = Data << 1;
        RaiseMdcClock(Adapter, &CtrlRegValue);

        CtrlRegValue = E1000_READ_REG(Ctrl);

        if (CtrlRegValue & E1000_CTRL_MDIO)
            Data |= 1;

        LowerMdcClock(Adapter, &CtrlRegValue);
    }

    RaiseMdcClock(Adapter, &CtrlRegValue);
    LowerMdcClock(Adapter, &CtrlRegValue);

    return (Data);
}

STATIC VOID
MIIShiftOutPhyData(IN PADAPTER_STRUCT Adapter,
                   IN UINT32 Data, IN UINT16 Count)
{
    UINT32 CtrlRegValue;
    UINT32 Mask;

    if (Count > 32)
        ASSERT(0);

    Mask = 0x01 << (Count - 1);

    CtrlRegValue = E1000_READ_REG(Ctrl);

    CtrlRegValue |= (E1000_CTRL_MDIO_DIR | E1000_CTRL_MDC_DIR);

    while (Mask) {

        if (Data & Mask)
            CtrlRegValue |= E1000_CTRL_MDIO;
        else
            CtrlRegValue &= ~E1000_CTRL_MDIO;

        E1000_WRITE_REG(Ctrl, CtrlRegValue);

        DelayInMicroseconds(2);

        RaiseMdcClock(Adapter, &CtrlRegValue);
        LowerMdcClock(Adapter, &CtrlRegValue);

        Mask = Mask >> 1;
    }
}

UINT16
ReadPhyRegister(IN PADAPTER_STRUCT Adapter,
                IN UINT32 RegAddress, IN UINT32 PhyAddress)
 {
    UINT32 Data;
    UINT32 Command;

    ASSERT(RegAddress <= MAX_PHY_REG_ADDRESS);

#if BROKEN_CHIP
	Command = ((RegAddress << MDI_REGADD_SHIFT) |
			   (PhyAddress << MDI_PHYADD_SHIFT) | (E1000_MDI_READ));

	E1000_WRITE_REG(Mdic, Command);

	while (TRUE) {
		DelayInMicroseconds(10);

		Data = E1000_READ_REG(Mdic);

		if (Data & E1000_MDI_READY)
			break;
	}

	ASSERT(!(Data & E1000_MDI_ERR));
#endif

	MIIShiftOutPhyData(Adapter, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

	Command = ((RegAddress) |
			   (PhyAddress << 5) |
			   (PHY_OP_READ << 10) | (PHY_SOF << 12));

	MIIShiftOutPhyData(Adapter, Command, 14);

	Data = (UINT32) MIIShiftInPhyData(Adapter);

	ASSERT(!(Data & E1000_MDI_ERR));

    return ((UINT16) Data);
}

VOID
WritePhyRegister(IN PADAPTER_STRUCT Adapter,
                 IN UINT32 RegAddress,
                 IN UINT32 PhyAddress, IN UINT16 Data)
 {

    UINT32 Command;

    ASSERT(RegAddress <= MAX_PHY_REG_ADDRESS);

#if BROKEN_CHIP

	Command = (((ULONG) Data) |
			   (RegAddress << MDI_REGADD_SHIFT) |
			   (PhyAddress << MDI_PHYADD_SHIFT) | (E1000_MDI_WRITE));

	E1000_WRITE_REG(Mdic, Command);

	DelayInMicroseconds(10);

	while (!(E1000_READ_REG(Mdic) & E1000_MDI_READY))
		DelayInMicroseconds(10);

	ASSERT(!(E1000_READ_REG(Mdic) & E1000_MDI_ERR));
#endif

	MIIShiftOutPhyData(Adapter, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

	Command = (Data |
			   (PHY_TURNAROUND << 16) |
			   (RegAddress << 18) |
			   (PhyAddress << 23) |
			   (PHY_OP_WRITE << 28) | (PHY_SOF << 30));

	MIIShiftOutPhyData(Adapter, Command, 32);
}

VOID PhyHardwareReset(IN PADAPTER_STRUCT Adapter)
 {
    UINT32 ExtCtrlRegValue;

    DEBUGFUNC("PhyHardwareReset")

	DEBUGOUT("Resetting Phy (GoodHope hardware)..");

    ExtCtrlRegValue = E1000_READ_REG(Exct);

    ExtCtrlRegValue |= E1000_CTRL_PHY_RESET_DIR4;

    E1000_WRITE_REG(Exct, ExtCtrlRegValue);

    DelayInMilliseconds(20);

    ExtCtrlRegValue = E1000_READ_REG(Exct);

    ExtCtrlRegValue &= ~E1000_CTRL_PHY_RESET4;

    E1000_WRITE_REG(Exct, ExtCtrlRegValue);

    DelayInMilliseconds(20);

    ExtCtrlRegValue = E1000_READ_REG(Exct);

    ExtCtrlRegValue |= E1000_CTRL_PHY_RESET4;

    E1000_WRITE_REG(Exct, ExtCtrlRegValue);

    DelayInMilliseconds(20);
}

BOOLEAN PhyReset(IN PADAPTER_STRUCT Adapter)
 {
    UINT16 RegData;
    UINT16 i;

    DEBUGFUNC("PhyReset")

        RegData = ReadPhyRegister(Adapter,
                                  PHY_MII_CTRL_REG, Adapter->PhyAddress);

    RegData |= MII_CR_RESET;

    WritePhyRegister(Adapter,
                     PHY_MII_CTRL_REG, Adapter->PhyAddress, RegData);

    i = 0;
    while ((RegData & MII_CR_RESET) && i++ < 500) {
        RegData = ReadPhyRegister(Adapter,
                                  PHY_MII_CTRL_REG, Adapter->PhyAddress);
        DelayInMicroseconds(1);
    }

    if (i >= 500) {
        DEBUGOUT("Timeout waiting for PHY to reset.");
        return FALSE;
    }

    return TRUE;
}

UINT32 AutoDetectGigabitPhy(IN PADAPTER_STRUCT Adapter)
{
    UINT32 PhyAddress = 0;
    UINT32 PhyIDHi;
    UINT16 PhyIDLo;
    BOOLEAN GotOne = FALSE;

    DEBUGFUNC("AutoDetectGigabitPhy")

        while ((!GotOne) && (PhyAddress <= MAX_PHY_REG_ADDRESS)) {

        PhyIDHi = ReadPhyRegister(Adapter, PHY_PHY_ID_REG1, PhyAddress);

        DelayInMicroseconds(2);

        PhyIDLo = ReadPhyRegister(Adapter, PHY_PHY_ID_REG2, PhyAddress);

        Adapter->PhyId = (PhyIDLo | (PhyIDHi << 16)) & PHY_REVISION_MASK;

        if (Adapter->PhyId == PAXSON_PHY_88E1000 ||
            Adapter->PhyId == PAXSON_PHY_88E1000S) {
            DEBUGOUT2("PhyId 0x%x detected at address 0x%x",
                      Adapter->PhyId, PhyAddress);

            GotOne = TRUE;
        } else {
            PhyAddress++;
        }

    }

    if (PhyAddress > MAX_PHY_REG_ADDRESS) {
        DEBUGOUT("Could not auto-detect Phy!");
    }

    return (PhyAddress);
}

VOID
ConfigureMacToPhySettings(IN PADAPTER_STRUCT Adapter,
                          IN UINT16 MiiRegisterData)
 {
    UINT32 DeviceCtrlReg, TctlReg;

    DEBUGFUNC("ConfigureMacToPhySettings")

        TctlReg = E1000_READ_REG(Tctl);
    DEBUGOUT1("TctlReg = %x", TctlReg);

    DeviceCtrlReg = E1000_READ_REG(Ctrl);

    DeviceCtrlReg |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
    DeviceCtrlReg &= ~(DEVICE_SPEED_MASK);

    DEBUGOUT1("MII Register Data = %x\r", MiiRegisterData);

    DeviceCtrlReg &= ~E1000_CTRL_ILOS;

    if (MiiRegisterData & PXN_PSSR_DPLX) {
        DeviceCtrlReg |= E1000_CTRL_FD;

        TctlReg &= ~E1000_TCTL_COLD;
        TctlReg |= (E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);
    } else {
        DeviceCtrlReg &= ~E1000_CTRL_FD;

        if ((MiiRegisterData & PXN_PSSR_SPEED) == PXN_PSSR_1000MBS) {
            TctlReg &= ~E1000_TCTL_COLD;
            TctlReg |=
                (E1000_GB_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);

            TctlReg |= E1000_TCTL_PBE;

        } else {
            TctlReg &= ~E1000_TCTL_COLD;
            TctlReg |= (E1000_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);
        }
    }

    if ((MiiRegisterData & PXN_PSSR_SPEED) == PXN_PSSR_1000MBS)
        DeviceCtrlReg |= E1000_CTRL_SPD_1000;
    else if ((MiiRegisterData & PXN_PSSR_SPEED) == PXN_PSSR_100MBS)
        DeviceCtrlReg |= E1000_CTRL_SPD_100;
    else
        DeviceCtrlReg &= ~(E1000_CTRL_SPD_1000 | E1000_CTRL_SPD_100);

    E1000_WRITE_REG(Tctl, TctlReg);

    E1000_WRITE_REG(Ctrl, DeviceCtrlReg);
}

VOID PxnPhyResetDsp(IN PADAPTER_STRUCT Adapter)
{
    WritePhyRegister(Adapter, 29, Adapter->PhyAddress, 0x1d);
    WritePhyRegister(Adapter, 30, Adapter->PhyAddress, 0xc1);
    WritePhyRegister(Adapter, 30, Adapter->PhyAddress, 0x00);
}

STATIC VOID PhyForceSpeedAndDuplex(IN PADAPTER_STRUCT Adapter)
{
    UINT16 MiiCtrlReg;
    UINT16 MiiStatusReg;
    UINT16 PhyData;
    UINT16 i;
    UINT32 TctlReg;
    UINT32 DeviceCtrlReg;

    DEBUGFUNC("PhyForceSpeedAndDuplex")

	Adapter->FlowControl = FLOW_CONTROL_NONE;

    DEBUGOUT1("Adapter->FlowControl = %d", Adapter->FlowControl);

    DeviceCtrlReg = E1000_READ_REG(Ctrl);

    DeviceCtrlReg |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
    DeviceCtrlReg &= ~(DEVICE_SPEED_MASK);

    DeviceCtrlReg &= ~E1000_CTRL_ASDE;

    MiiCtrlReg = ReadPhyRegister(Adapter,
                                 PHY_MII_CTRL_REG, Adapter->PhyAddress);

    MiiCtrlReg &= ~MII_CR_AUTO_NEG_EN;

    if (Adapter->ForcedSpeedDuplex == FULL_1000 ||
		Adapter->ForcedSpeedDuplex == FULL_100 ||
        Adapter->ForcedSpeedDuplex == FULL_10) {

        DeviceCtrlReg |= E1000_CTRL_FD;
        MiiCtrlReg |= MII_CR_FULL_DUPLEX;

        DEBUGOUT("Full Duplex");
    } else {

        DeviceCtrlReg &= ~E1000_CTRL_FD;
        MiiCtrlReg &= ~MII_CR_FULL_DUPLEX;

        DEBUGOUT("Half Duplex");
    }

    if (Adapter->ForcedSpeedDuplex == FULL_1000) {

        DeviceCtrlReg |= E1000_CTRL_SPD_1000;
        MiiCtrlReg |= MII_CR_SPEED_1000;
        MiiCtrlReg &= ~(MII_CR_SPEED_100 | MII_CR_SPEED_10);

        DEBUGOUT("Forcing 100mb ");
	} 
    else if (Adapter->ForcedSpeedDuplex == FULL_100 ||
        Adapter->ForcedSpeedDuplex == HALF_100) {

        DeviceCtrlReg |= E1000_CTRL_SPD_100;
        MiiCtrlReg |= MII_CR_SPEED_100;
        MiiCtrlReg &= ~(MII_CR_SPEED_1000 | MII_CR_SPEED_10);

        DEBUGOUT("Forcing 100mb ");
    } else {

        DeviceCtrlReg &= ~(E1000_CTRL_SPD_1000 | E1000_CTRL_SPD_100);
        MiiCtrlReg |= MII_CR_SPEED_10;
        MiiCtrlReg &= ~(MII_CR_SPEED_1000 | MII_CR_SPEED_100);

        DEBUGOUT("Forcing 10mb ");
    }

    TctlReg = E1000_READ_REG(Tctl);
    DEBUGOUT1("TctlReg = %x", TctlReg);

    if (!(MiiCtrlReg & MII_CR_FULL_DUPLEX)) {

        TctlReg &= ~E1000_TCTL_COLD;
        TctlReg |= (E1000_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);
    } else {

        TctlReg &= ~E1000_TCTL_COLD;
        TctlReg |= (E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);
    }

    E1000_WRITE_REG(Tctl, TctlReg);

    E1000_WRITE_REG(Ctrl, DeviceCtrlReg);

    PhyData = ReadPhyRegister(Adapter,
                              PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress);

    PhyData &= ~PXN_PSCR_AUTO_X_MODE;

    WritePhyRegister(Adapter,
                     PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress, PhyData);

    DEBUGOUT1("Paxson PSCR: %x", PhyData);

    MiiCtrlReg |= MII_CR_RESET;

    WritePhyRegister(Adapter,
                     PHY_MII_CTRL_REG, Adapter->PhyAddress, MiiCtrlReg);

    if (Adapter->WaitAutoNegComplete) {

        DEBUGOUT("Waiting for forced speed/duplex link.");
        MiiStatusReg = 0;

#define PHY_WAIT_FOR_FORCED_TIME    20

        for (i = 20; i > 0; i--) {

            MiiStatusReg = ReadPhyRegister(Adapter,
                                           PHY_MII_STATUS_REG,
                                           Adapter->PhyAddress);

            MiiStatusReg = ReadPhyRegister(Adapter,
                                           PHY_MII_STATUS_REG,
                                           Adapter->PhyAddress);

            if (MiiStatusReg & MII_SR_LINK_STATUS) {
                i = 0;
            }
			else {
				DelayInMilliseconds(100);
			}
        }

        if (i == 0) {

            PxnPhyResetDsp(Adapter);
        }

        for (i = 20; i > 0; i--) {
            if (MiiStatusReg & MII_SR_LINK_STATUS) {
                i = 0;
            }
			else {
				DelayInMilliseconds(100);

				MiiStatusReg = ReadPhyRegister(Adapter,
											   PHY_MII_STATUS_REG,
											   Adapter->PhyAddress);

				MiiStatusReg = ReadPhyRegister(Adapter,
											   PHY_MII_STATUS_REG,
											   Adapter->PhyAddress);
			}
        }
    }

    PhyData = ReadPhyRegister(Adapter,
                              PXN_EXT_PHY_SPEC_CTRL_REG,
                              Adapter->PhyAddress);

    PhyData |= PXN_EPSCR_TX_CLK_25;

    WritePhyRegister(Adapter,
                     PXN_EXT_PHY_SPEC_CTRL_REG,
                     Adapter->PhyAddress, PhyData);

    PhyData = ReadPhyRegister(Adapter,
                              PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress);

    PhyData |= PXN_PSCR_ASSERT_CRS_ON_TX;

    WritePhyRegister(Adapter,
                     PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress, PhyData);
    DEBUGOUT1("After force, Paxson Phy Specific Ctrl Reg = %4x\r",
              PhyData);

    PxnPhyResetDsp(Adapter);
}

STATIC BOOLEAN PhySetupAutoNegAdvertisement(IN PADAPTER_STRUCT Adapter)
{
    UINT16 MiiAutoNegAdvertiseReg, Mii1000TCtrlReg;

    DEBUGFUNC("PhySetupAutoNegAdvertisement")

        MiiAutoNegAdvertiseReg = ReadPhyRegister(Adapter,
                                                 PHY_AUTONEG_ADVERTISEMENT,
                                                 Adapter->PhyAddress);

    Mii1000TCtrlReg = ReadPhyRegister(Adapter,
                                      PHY_1000T_CTRL_REG,
                                      Adapter->PhyAddress);

    MiiAutoNegAdvertiseReg &= ~REG4_SPEED_MASK;
    Mii1000TCtrlReg &= ~REG9_SPEED_MASK;

    DEBUGOUT1("AutoNegAdvertised %x", Adapter->AutoNegAdvertised);

    if (Adapter->AutoNegAdvertised & ADVERTISE_10_HALF) {
        DEBUGOUT("Advertise 10mb Half duplex");
        MiiAutoNegAdvertiseReg |= NWAY_AR_10T_HD_CAPS;
    }

    if (Adapter->AutoNegAdvertised & ADVERTISE_10_FULL) {
        DEBUGOUT("Advertise 10mb Full duplex");
        MiiAutoNegAdvertiseReg |= NWAY_AR_10T_FD_CAPS;
    }

    if (Adapter->AutoNegAdvertised & ADVERTISE_100_HALF) {
        DEBUGOUT("Advertise 100mb Half duplex");
        MiiAutoNegAdvertiseReg |= NWAY_AR_100TX_HD_CAPS;
    }

    if (Adapter->AutoNegAdvertised & ADVERTISE_100_FULL) {
        DEBUGOUT("Advertise 100mb Full duplex");
        MiiAutoNegAdvertiseReg |= NWAY_AR_100TX_FD_CAPS;
    }

    if (Adapter->AutoNegAdvertised & ADVERTISE_1000_HALF) {
        DEBUGOUT
            ("Advertise 1000mb Half duplex requested, request denied!");
    }

    if (Adapter->AutoNegAdvertised & ADVERTISE_1000_FULL) {
        DEBUGOUT("Advertise 1000mb Full duplex");
        Mii1000TCtrlReg |= CR_1000T_FD_CAPS;
    }

    switch (Adapter->FlowControl) {
    case FLOW_CONTROL_NONE:

        MiiAutoNegAdvertiseReg &= ~(NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);

        break;

    case FLOW_CONTROL_RECEIVE_PAUSE:

        MiiAutoNegAdvertiseReg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);

        break;

    case FLOW_CONTROL_TRANSMIT_PAUSE:

        MiiAutoNegAdvertiseReg |= NWAY_AR_ASM_DIR;
        MiiAutoNegAdvertiseReg &= ~NWAY_AR_PAUSE;

        break;

    case FLOW_CONTROL_FULL:

        MiiAutoNegAdvertiseReg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);

        break;

    default:

        DEBUGOUT("Flow control param set incorrectly");
        ASSERT(0);
        break;
    }

    WritePhyRegister(Adapter,
                     PHY_AUTONEG_ADVERTISEMENT,
                     Adapter->PhyAddress, MiiAutoNegAdvertiseReg);

    DEBUGOUT1("Auto-Neg Advertising %x", MiiAutoNegAdvertiseReg);

    WritePhyRegister(Adapter,
                     PHY_1000T_CTRL_REG,
                     Adapter->PhyAddress, Mii1000TCtrlReg);
    return (TRUE);
}

#ifdef DEBUG

VOID DisplayMiiContents(IN PADAPTER_STRUCT Adapter, IN UINT8 PhyAddress)
{
    UINT16 Data, PhyIDHi, PhyIDLo;
    UINT32 PhyID;

    DEBUGFUNC("DisplayMiiContents")

	DEBUGOUT1("Adapter Base Address = %x", Adapter->regs);

    Data = ReadPhyRegister(Adapter, PHY_MII_CTRL_REG, PhyAddress);

    DEBUGOUT1("MII Ctrl Reg contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_MII_STATUS_REG, PhyAddress);

    Data = ReadPhyRegister(Adapter, PHY_MII_STATUS_REG, PhyAddress);

    DEBUGOUT1("MII Status Reg contents = %x", Data);

    PhyIDHi = ReadPhyRegister(Adapter, PHY_PHY_ID_REG1, PhyAddress);

    DelayInMicroseconds(2);

    PhyIDLo = ReadPhyRegister(Adapter, PHY_PHY_ID_REG2, PhyAddress);

    PhyID = (PhyIDLo | (PhyIDHi << 16)) & PHY_REVISION_MASK;

    DEBUGOUT1("Phy ID = %x", PhyID);

    Data = ReadPhyRegister(Adapter, PHY_AUTONEG_ADVERTISEMENT, PhyAddress);

    DEBUGOUT1("Reg 4 contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_AUTONEG_LP_BPA, PhyAddress);

    DEBUGOUT1("Reg 5 contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_AUTONEG_EXPANSION_REG, PhyAddress);

    DEBUGOUT1("Reg 6 contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_AUTONEG_NEXT_PAGE_TX, PhyAddress);

    DEBUGOUT1("Reg 7 contents = %x", Data);

    Data = ReadPhyRegister(Adapter,
                           PHY_AUTONEG_LP_RX_NEXT_PAGE, PhyAddress);

    DEBUGOUT1("Reg 8 contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_1000T_CTRL_REG, PhyAddress);

    DEBUGOUT1("Reg 9 contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_1000T_STATUS_REG, PhyAddress);

    DEBUGOUT1("Reg A contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PHY_IEEE_EXT_STATUS_REG, PhyAddress);

    DEBUGOUT1("Reg F contents = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_PHY_SPEC_CTRL_REG, PhyAddress);

    DEBUGOUT1("Paxson Specific Control Reg (0x10) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_PHY_SPEC_STAT_REG, PhyAddress);

    DEBUGOUT1("Paxson Specific Status Reg (0x11) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_INT_ENABLE_REG, PhyAddress);

    DEBUGOUT1("Paxson Interrupt Enable Reg (0x12) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_INT_STATUS_REG, PhyAddress);

    DEBUGOUT1("Paxson Interrupt Status Reg (0x13) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_EXT_PHY_SPEC_CTRL_REG, PhyAddress);

    DEBUGOUT1("Paxson Ext. Phy Specific Control (0x14) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_RX_ERROR_COUNTER, PhyAddress);

    DEBUGOUT1("Paxson Receive Error Counter (0x15) = %x", Data);

    Data = ReadPhyRegister(Adapter, PXN_LED_CTRL_REG, PhyAddress);

    DEBUGOUT1("Paxson LED control reg (0x18) = %x", Data);
}

void
wx_dump_regs(ADAPTER_STRUCT *Adapter)
{
	UINT32 reg;
	int i;

	for (i = 0; i < 0x120 >> 3; i++)
	{
		if ((i & 0x3) == 0)
			udot(i << 3);

		reg = ((volatile UINT*)Adapter->regs)[i << 3];
		udot(reg);

		if ((i & 0x3) == 3)
			cr();
	}

	//DisplayMiiContents(Adapter, Adapter->PhyAddress);
}

#endif // DEBUG

STATIC VOID StandBy(PADAPTER_STRUCT Adapter)
{
    UINT32 EecdRegValue;

    EecdRegValue = E1000_READ_REG(Eecd);

    EecdRegValue &= ~(E1000_EECS | E1000_EESK);

    E1000_WRITE_REG(Eecd, EecdRegValue);

    DelayInMicroseconds(5);

    EecdRegValue |= E1000_EECS;

    E1000_WRITE_REG(Eecd, EecdRegValue);
}

STATIC UINT16 WaitEepromCommandDone(PADAPTER_STRUCT Adapter)
{
    UINT32 EecdRegValue;
    UINT i;

    StandBy(Adapter);

    for (i = 0; i < 200; i++) {
        EecdRegValue = E1000_READ_REG(Eecd);

        if (EecdRegValue & E1000_EEDO)
            return (TRUE);

        DelayInMicroseconds(5);
    }

    return (FALSE);
}

STATIC VOID RaiseClock(PADAPTER_STRUCT Adapter, UINT32 * EecdRegValue)
{

    *EecdRegValue = *EecdRegValue | E1000_EESK;

    E1000_WRITE_REG(Eecd, *EecdRegValue);

    DelayInMicroseconds(50);
}

STATIC VOID LowerClock(PADAPTER_STRUCT Adapter, UINT32 * EecdRegValue)
{

    *EecdRegValue = *EecdRegValue & ~E1000_EESK;

    E1000_WRITE_REG(Eecd, *EecdRegValue);

    DelayInMicroseconds(50);
}

STATIC UINT16 ShiftInBits(PADAPTER_STRUCT Adapter)
{
    UINT32 EecdRegValue;
    UINT i;
    UINT16 Data;

    EecdRegValue = E1000_READ_REG(Eecd);

    EecdRegValue &= ~(E1000_EEDO | E1000_EEDI);
    Data = 0;

    for (i = 0; i < 16; i++) {
        Data = Data << 1;
        RaiseClock(Adapter, &EecdRegValue);

        EecdRegValue = E1000_READ_REG(Eecd);

        EecdRegValue &= ~(E1000_EEDI);
        if (EecdRegValue & E1000_EEDO)
            Data |= 1;

        LowerClock(Adapter, &EecdRegValue);
    }

    return (Data);
}

STATIC VOID ShiftOutBits(PADAPTER_STRUCT Adapter, UINT16 Data, UINT16 Count)
{
    UINT32 EecdRegValue;
    UINT32 Mask;

    Mask = 0x01 << (Count - 1);

    EecdRegValue = E1000_READ_REG(Eecd);

    EecdRegValue &= ~(E1000_EEDO | E1000_EEDI);

    do {

        EecdRegValue &= ~E1000_EEDI;

        if (Data & Mask)
            EecdRegValue |= E1000_EEDI;

        E1000_WRITE_REG(Eecd, EecdRegValue);

        DelayInMicroseconds(50);

        RaiseClock(Adapter, &EecdRegValue);
        LowerClock(Adapter, &EecdRegValue);

        Mask = Mask >> 1;

    } while (Mask);

    EecdRegValue &= ~E1000_EEDI;

    E1000_WRITE_REG(Eecd, EecdRegValue);
}

STATIC VOID EepromCleanup(PADAPTER_STRUCT Adapter)
{
    UINT32 EecdRegValue;

    EecdRegValue = E1000_READ_REG(Eecd);

    EecdRegValue &= ~(E1000_EECS | E1000_EEDI);

    E1000_WRITE_REG(Eecd, EecdRegValue);

    RaiseClock(Adapter, &EecdRegValue);
    LowerClock(Adapter, &EecdRegValue);
}

UINT16 ReadEepromWord(PADAPTER_STRUCT Adapter, UINT16 Reg)
{
    UINT16 Data;

    ASSERT(Reg < EEPROM_WORD_SIZE);

    E1000_WRITE_REG(Eecd, E1000_EECS);

    ShiftOutBits(Adapter, EEPROM_READ_OPCODE, 3);
    ShiftOutBits(Adapter, Reg, 6);

    Data = ShiftInBits(Adapter);

    EepromCleanup(Adapter);
    return (Data);
}

BOOLEAN ValidateEepromChecksum(PADAPTER_STRUCT Adapter)
{
    UINT16 Checksum = 0;
    UINT16 Iteration;

    DEBUGOUT("ValidateEepromChecksum");

    for (Iteration = 0; Iteration < (EEPROM_CHECKSUM_REG + 1); Iteration++)
        Checksum += ReadEepromWord(Adapter, Iteration);

    DEBUGOUT1("ValidateEepromChecksum ", Checksum);

    if (Checksum == (UINT16) EEPROM_SUM)
        return (TRUE);
    else
        return (FALSE);
}

BOOLEAN WriteEepromWord(PADAPTER_STRUCT Adapter, UINT16 Reg, UINT16 Data)
{

    E1000_WRITE_REG(Eecd, E1000_EECS);

    ShiftOutBits(Adapter, EEPROM_EWEN_OPCODE, 5);
    ShiftOutBits(Adapter, Reg, 4);

    StandBy(Adapter);

    ShiftOutBits(Adapter, EEPROM_ERASE_OPCODE, 3);
    ShiftOutBits(Adapter, Reg, 6);

    if (!WaitEepromCommandDone(Adapter))
        return (FALSE);

    ShiftOutBits(Adapter, EEPROM_WRITE_OPCODE, 3);
    ShiftOutBits(Adapter, Reg, 6);
    ShiftOutBits(Adapter, Data, 16);

    if (!WaitEepromCommandDone(Adapter))
        return (FALSE);

    ShiftOutBits(Adapter, EEPROM_EWDS_OPCODE, 5);
    ShiftOutBits(Adapter, Reg, 4);

    EepromCleanup(Adapter);

    return (TRUE);
} 

BOOLEAN ReadPartNumber(PADAPTER_STRUCT Adapter, PUINT PartNumber)
{
    UINT16 EepromWordValue;

    DEBUGFUNC("ReadPartNumber")

        if (Adapter->AdapterStopped) {
        *PartNumber = 0;
        return (FALSE);
    }

    EepromWordValue = ReadEepromWord(Adapter,
                                     (UINT16) (EEPROM_PBA_BYTE_1));

    DEBUGOUT("Read first part number word");

    *PartNumber = (UINT32) EepromWordValue;
    *PartNumber = *PartNumber << 16;

    EepromWordValue = ReadEepromWord(Adapter,
                                     (UINT16) (EEPROM_PBA_BYTE_1 + 1));

    DEBUGOUT("Read second part number word");

    *PartNumber |= EepromWordValue;

    DEBUGOUT1("Part number = ", *PartNumber);
    return (TRUE);

}

VOID
GetSpeedAndDuplex(PADAPTER_STRUCT Adapter, PUINT16 Speed, PUINT16 Duplex)
{
    UINT32 DeviceStatusReg;

    DEBUGFUNC("GetSpeedAndDuplex")

        if (Adapter->AdapterStopped) {
        *Speed = 0;
        *Duplex = 0;
        return;
    }

	DEBUGOUT("Livengood MAC");
	DeviceStatusReg = E1000_READ_REG(Status);
	if (DeviceStatusReg & E1000_STATUS_SPEED_1000) {
		*Speed = SPEED_1000;
		DEBUGOUT("   1000 Mbs");
	} else if (DeviceStatusReg & E1000_STATUS_SPEED_100) {
		*Speed = SPEED_100;
		DEBUGOUT("   100 Mbs");
	} else {
		*Speed = SPEED_10;
		DEBUGOUT("   10 Mbs");
	}

	if (DeviceStatusReg & E1000_STATUS_FD) {
		*Duplex = FULL_DUPLEX;
		DEBUGOUT("   Full Duplex\r");
	} else {
		*Duplex = HALF_DUPLEX;
		DEBUGOUT("   Half Duplex\r");
	}
}

VOID ClearHwStatsCounters(PADAPTER_STRUCT Adapter)
{
    volatile UINT32 RegisterContents;

    DEBUGFUNC("ClearHwStatsCounters")

        if (Adapter->AdapterStopped) {
        DEBUGOUT("Exiting because the adapter is stopped!!!");
        return;
    }

    RegisterContents = E1000_READ_REG(Crcerrs);
    RegisterContents = E1000_READ_REG(Symerrs);
    RegisterContents = E1000_READ_REG(Mpc);
    RegisterContents = E1000_READ_REG(Scc);
    RegisterContents = E1000_READ_REG(Ecol);
    RegisterContents = E1000_READ_REG(Mcc);
    RegisterContents = E1000_READ_REG(Latecol);
    RegisterContents = E1000_READ_REG(Colc);
    RegisterContents = E1000_READ_REG(Dc);
    RegisterContents = E1000_READ_REG(Sec);
    RegisterContents = E1000_READ_REG(Rlec);
    RegisterContents = E1000_READ_REG(Xonrxc);
    RegisterContents = E1000_READ_REG(Xontxc);
    RegisterContents = E1000_READ_REG(Xoffrxc);
    RegisterContents = E1000_READ_REG(Xofftxc);
    RegisterContents = E1000_READ_REG(Fcruc);
    RegisterContents = E1000_READ_REG(Prc64);
    RegisterContents = E1000_READ_REG(Prc127);
    RegisterContents = E1000_READ_REG(Prc255);
    RegisterContents = E1000_READ_REG(Prc511);
    RegisterContents = E1000_READ_REG(Prc1023);
    RegisterContents = E1000_READ_REG(Prc1522);
    RegisterContents = E1000_READ_REG(Gprc);
    RegisterContents = E1000_READ_REG(Bprc);
    RegisterContents = E1000_READ_REG(Mprc);
    RegisterContents = E1000_READ_REG(Gptc);
    RegisterContents = E1000_READ_REG(Gorl);
    RegisterContents = E1000_READ_REG(Gorh);
    RegisterContents = E1000_READ_REG(Gotl);
    RegisterContents = E1000_READ_REG(Goth);
    RegisterContents = E1000_READ_REG(Rnbc);
    RegisterContents = E1000_READ_REG(Ruc);
    RegisterContents = E1000_READ_REG(Rfc);
    RegisterContents = E1000_READ_REG(Roc);
    RegisterContents = E1000_READ_REG(Rjc);
    RegisterContents = E1000_READ_REG(Torl);
    RegisterContents = E1000_READ_REG(Torh);
    RegisterContents = E1000_READ_REG(Totl);
    RegisterContents = E1000_READ_REG(Toth);
    RegisterContents = E1000_READ_REG(Tpr);
    RegisterContents = E1000_READ_REG(Tpt);
    RegisterContents = E1000_READ_REG(Ptc64);
    RegisterContents = E1000_READ_REG(Ptc127);
    RegisterContents = E1000_READ_REG(Ptc255);
    RegisterContents = E1000_READ_REG(Ptc511);
    RegisterContents = E1000_READ_REG(Ptc1023);
    RegisterContents = E1000_READ_REG(Ptc1522);
    RegisterContents = E1000_READ_REG(Mptc);
    RegisterContents = E1000_READ_REG(Bptc);

    RegisterContents = E1000_READ_REG(Algnerrc);
    RegisterContents = E1000_READ_REG(Rxerrc);
    RegisterContents = E1000_READ_REG(Tuc);
    RegisterContents = E1000_READ_REG(Tncrs);
    RegisterContents = E1000_READ_REG(Cexterr);
    RegisterContents = E1000_READ_REG(Rutec);

    RegisterContents = E1000_READ_REG(Tsctc);
    RegisterContents = E1000_READ_REG(Tsctfc);

}

VOID ForceMacFlowControlSetting(PADAPTER_STRUCT Adapter)
{
    UINT32 CtrlRegValue;

    DEBUGFUNC("ForceMacFlowControlSetting")

        CtrlRegValue = E1000_READ_REG(Ctrl);

    switch (Adapter->FlowControl) {
    case FLOW_CONTROL_NONE:

        CtrlRegValue &= (~(E1000_CTRL_TFCE | E1000_CTRL_RFCE));
        break;

    case FLOW_CONTROL_RECEIVE_PAUSE:

        CtrlRegValue &= (~E1000_CTRL_TFCE);
        CtrlRegValue |= E1000_CTRL_RFCE;
        break;

    case FLOW_CONTROL_TRANSMIT_PAUSE:

        CtrlRegValue &= (~E1000_CTRL_RFCE);
        CtrlRegValue |= E1000_CTRL_TFCE;
        break;

    case FLOW_CONTROL_FULL:

        CtrlRegValue |= (E1000_CTRL_TFCE | E1000_CTRL_RFCE);
        break;

    default:

        DEBUGOUT("Flow control param set incorrectly");
        ASSERT(0);

        break;
    }

    E1000_WRITE_REG(Ctrl, CtrlRegValue);
}

VOID ConfigFlowControlAfterLinkUp(PADAPTER_STRUCT Adapter)
{
    UINT16 MiiStatusReg, MiiNWayAdvertiseReg, MiiNWayBasePgAbleReg;
    UINT16 Speed, Duplex;

    DEBUGFUNC("ConfigFlowControlAfterLinkUp")

        if (
            ((Adapter->MediaType == MEDIA_TYPE_FIBER)
             && (Adapter->AutoNegFailed))
            || ((Adapter->MediaType == MEDIA_TYPE_COPPER)
                && (!Adapter->AutoNeg))) {
        ForceMacFlowControlSetting(Adapter);
    }

    if ((Adapter->MediaType == MEDIA_TYPE_COPPER) && Adapter->AutoNeg) {

        MiiStatusReg = ReadPhyRegister(Adapter,
                                       PHY_MII_STATUS_REG,
                                       Adapter->PhyAddress);

        MiiStatusReg = ReadPhyRegister(Adapter,
                                       PHY_MII_STATUS_REG,
                                       Adapter->PhyAddress);

        if (MiiStatusReg & MII_SR_AUTONEG_COMPLETE) {

            MiiNWayAdvertiseReg = ReadPhyRegister(Adapter,
                                                  PHY_AUTONEG_ADVERTISEMENT,
                                                  Adapter->PhyAddress);

            MiiNWayBasePgAbleReg = ReadPhyRegister(Adapter,
                                                   PHY_AUTONEG_LP_BPA,
                                                   Adapter->PhyAddress);

            if ((MiiNWayAdvertiseReg & NWAY_AR_PAUSE) &&
                (MiiNWayBasePgAbleReg & NWAY_LPAR_PAUSE)) {

                if (Adapter->OriginalFlowControl == FLOW_CONTROL_FULL) {
                    Adapter->FlowControl = FLOW_CONTROL_FULL;
                    DEBUGOUT("Flow Control = FULL.\r");
                } else {
                    Adapter->FlowControl = FLOW_CONTROL_RECEIVE_PAUSE;
                    DEBUGOUT("Flow Control = RX PAUSE frames only.\r");
                }
            }

            else if (!(MiiNWayAdvertiseReg & NWAY_AR_PAUSE) &&
                     (MiiNWayAdvertiseReg & NWAY_AR_ASM_DIR) &&
                     (MiiNWayBasePgAbleReg & NWAY_LPAR_PAUSE) &&
                     (MiiNWayBasePgAbleReg & NWAY_LPAR_ASM_DIR)) {
                Adapter->FlowControl = FLOW_CONTROL_TRANSMIT_PAUSE;
                DEBUGOUT("Flow Control = TX PAUSE frames only.\r");
            }

            else if ((MiiNWayAdvertiseReg & NWAY_AR_PAUSE) &&
                     (MiiNWayAdvertiseReg & NWAY_AR_ASM_DIR) &&
                     !(MiiNWayBasePgAbleReg & NWAY_LPAR_PAUSE) &&
                     (MiiNWayBasePgAbleReg & NWAY_LPAR_ASM_DIR)) {
                Adapter->FlowControl = FLOW_CONTROL_RECEIVE_PAUSE;
                DEBUGOUT("Flow Control = RX PAUSE frames only.\r");
            }

            else if (Adapter->OriginalFlowControl == FLOW_CONTROL_NONE) {
                Adapter->FlowControl = FLOW_CONTROL_NONE;
                DEBUGOUT("Flow Control = NONE.\r");
            } else {
                Adapter->FlowControl = FLOW_CONTROL_RECEIVE_PAUSE;
                DEBUGOUT("Flow Control = RX PAUSE frames only.\r");
            }

            GetSpeedAndDuplex(Adapter, &Speed, &Duplex);

            if (Duplex == HALF_DUPLEX)
                Adapter->FlowControl = FLOW_CONTROL_NONE;

            ForceMacFlowControlSetting(Adapter);
        } else {
            DEBUGOUT("Copper PHY and Auto Neg has not completed.\r");
        }
    }

}

BOOLEAN PhySetup(IN PADAPTER_STRUCT Adapter, UINT32 DeviceControlReg)
{
    UINT16 MiiCtrlReg, MiiStatusReg;
    UINT16 MiiAutoNegAdvertiseReg, Mii1000TCtrlReg;
    UINT16 i, AutoNegAdDefault, Data;
    UINT16 AutoNegHwSetting;
    BOOLEAN Status = FALSE;
    BOOLEAN RestartAutoNeg = FALSE;

    DEBUGFUNC("PhySetup")

        ASSERT(Adapter->MacType >= MAC_LIVENGOOD);

    DeviceControlReg |=
			(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX | E1000_CTRL_SLU);

    E1000_WRITE_REG(Ctrl, DeviceControlReg);

    PhyHardwareReset(Adapter);

    Adapter->PhyAddress = AutoDetectGigabitPhy(Adapter);

    if (Adapter->PhyAddress > MAX_PHY_REG_ADDRESS) {

        Status = FALSE;
    }

    DEBUGOUT1("Phy ID = %x", Adapter->PhyId);

    MiiCtrlReg = ReadPhyRegister(Adapter,
                                 PHY_MII_CTRL_REG, Adapter->PhyAddress);

    DEBUGOUT1("MII Ctrl Reg contents = %x", MiiCtrlReg);

    MiiCtrlReg &= ~(MII_CR_ISOLATE);

    WritePhyRegister(Adapter,
                     PHY_MII_CTRL_REG, Adapter->PhyAddress, MiiCtrlReg);

    Data = ReadPhyRegister(Adapter,
                           PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress);

    Data |= PXN_PSCR_ASSERT_CRS_ON_TX;

    DEBUGOUT1("Paxson PSCR: %x", Data);

    WritePhyRegister(Adapter,
                     PXN_PHY_SPEC_CTRL_REG, Adapter->PhyAddress, Data);

    Data = ReadPhyRegister(Adapter,
                           PXN_EXT_PHY_SPEC_CTRL_REG, Adapter->PhyAddress);

    Data |= PXN_EPSCR_TX_CLK_25;

    WritePhyRegister(Adapter,
                     PXN_EXT_PHY_SPEC_CTRL_REG, Adapter->PhyAddress, Data);

    MiiAutoNegAdvertiseReg = ReadPhyRegister(Adapter,
                                             PHY_AUTONEG_ADVERTISEMENT,
                                             Adapter->PhyAddress);

    AutoNegHwSetting = (MiiAutoNegAdvertiseReg >> 5) & 0xF;

    Mii1000TCtrlReg = ReadPhyRegister(Adapter,
                                      PHY_1000T_CTRL_REG,
                                      Adapter->PhyAddress);

    AutoNegHwSetting |= ((Mii1000TCtrlReg & 0x0300) >> 4);

    MiiAutoNegAdvertiseReg = ((MiiAutoNegAdvertiseReg & 0x0C00) >> 10);

    Adapter->AutoNegAdvertised &= AUTONEG_ADVERTISE_SPEED_DEFAULT;

    if (Adapter->AutoNegAdvertised == 0)
        Adapter->AutoNegAdvertised = AUTONEG_ADVERTISE_SPEED_DEFAULT;

    AutoNegAdDefault = AUTONEG_ADVERTISE_SPEED_DEFAULT;

    if (Adapter->AutoNeg &&
        (Adapter->AutoNegAdvertised == AutoNegHwSetting) &&
        (Adapter->FlowControl == MiiAutoNegAdvertiseReg)) {
        DEBUGOUT("No overrides - Reading MII Status Reg..");

        MiiStatusReg = ReadPhyRegister(Adapter,
                                       PHY_MII_STATUS_REG,
                                       Adapter->PhyAddress);

        MiiStatusReg = ReadPhyRegister(Adapter,
                                       PHY_MII_STATUS_REG,
                                       Adapter->PhyAddress);

        DEBUGOUT1("MII Status Reg contents = %x", MiiStatusReg);

        if (MiiStatusReg & MII_SR_LINK_STATUS) {
            Data = ReadPhyRegister(Adapter,
                                   PXN_PHY_SPEC_STAT_REG,
                                   Adapter->PhyAddress);
            DEBUGOUT1("Paxson Phy Specific Status Reg contents = %x",
                      Data);

            ConfigureMacToPhySettings(Adapter, Data);

            ConfigFlowControlAfterLinkUp(Adapter);
            return (TRUE);
        }
    }

    if (Adapter->AutoNeg) {
        DEBUGOUT
            ("Livengood - Reconfiguring auto-neg advertisement params");
        RestartAutoNeg = PhySetupAutoNegAdvertisement(Adapter);
    } else {
        DEBUGOUT("Livengood - Forcing speed and duplex");
        PhyForceSpeedAndDuplex(Adapter);
    }

    if (RestartAutoNeg) {
        DEBUGOUT("Restarting Auto-Neg");

        MiiCtrlReg = ReadPhyRegister(Adapter,
                                     PHY_MII_CTRL_REG,
                                     Adapter->PhyAddress);

        MiiCtrlReg |= (MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG);

        WritePhyRegister(Adapter,
                         PHY_MII_CTRL_REG,
                         Adapter->PhyAddress, MiiCtrlReg);

        if (Adapter->WaitAutoNegComplete) {

            DEBUGOUT("Waiting for Auto-Neg to complete.");
            MiiStatusReg = 0;

            for (i = PHY_AUTO_NEG_TIME; i > 0; i--) {

                MiiStatusReg = ReadPhyRegister(Adapter,
                                               PHY_MII_STATUS_REG,
                                               Adapter->PhyAddress);

                MiiStatusReg = ReadPhyRegister(Adapter,
                                               PHY_MII_STATUS_REG,
                                               Adapter->PhyAddress);

                if (MiiStatusReg & MII_SR_AUTONEG_COMPLETE)
                    i = 0;
				else
					DelayInMilliseconds(100);
            }
        }
    }

    MiiStatusReg = ReadPhyRegister(Adapter,
                                   PHY_MII_STATUS_REG,
                                   Adapter->PhyAddress);

    MiiStatusReg = ReadPhyRegister(Adapter,
                                   PHY_MII_STATUS_REG,
                                   Adapter->PhyAddress);

    DEBUGOUT1("Checking for link status - MII Status Reg contents = %x",
              MiiStatusReg);

    for (i = 0; i < 3*1000; i++) {
        if (MiiStatusReg & MII_SR_LINK_STATUS) {
            i = 100*1000;
        }
		else {
			DelayInMicroseconds(10);

			MiiStatusReg = ReadPhyRegister(Adapter,
										   PHY_MII_STATUS_REG,
										   Adapter->PhyAddress);

			MiiStatusReg = ReadPhyRegister(Adapter,
										   PHY_MII_STATUS_REG,
										   Adapter->PhyAddress);
		}
    }

    if (MiiStatusReg & MII_SR_LINK_STATUS) {

        Data = ReadPhyRegister(Adapter,
                               PXN_PHY_SPEC_STAT_REG, Adapter->PhyAddress);

        DEBUGOUT1("Paxson Phy Specific Status Reg contents = %x", Data);

        ConfigureMacToPhySettings(Adapter, Data);
        DEBUGOUT("Valid link established!!!");

        ConfigFlowControlAfterLinkUp(Adapter);
    } else {
        DEBUGOUT("Unable to establish link!!!");
    }

    return (TRUE);
}

VOID CheckForLink(PADAPTER_STRUCT Adapter)
{
    UINT32 RxcwRegValue;
    UINT32 CtrlRegValue;
    UINT32 StatusRegValue;
    UINT16 PhyData;

    DEBUGFUNC("CheckForLink")

        CtrlRegValue = E1000_READ_REG(Ctrl);

    StatusRegValue = E1000_READ_REG(Status);

    RxcwRegValue = E1000_READ_REG(Rxcw);

    if (Adapter->MediaType == MEDIA_TYPE_COPPER && Adapter->GetLinkStatus) {

        PhyData = ReadPhyRegister(Adapter,
                                  PHY_MII_STATUS_REG, Adapter->PhyAddress);

        PhyData = ReadPhyRegister(Adapter,
                                  PHY_MII_STATUS_REG, Adapter->PhyAddress);

        if (PhyData & MII_SR_LINK_STATUS)
            Adapter->GetLinkStatus = FALSE;
        else {
            DEBUGOUT("**** CFL - No link detected. ****\r");
            return;
        }

        if (!Adapter->AutoNeg)
            return;

        switch (Adapter->PhyId) {
        case PAXSON_PHY_88E1000:
        case PAXSON_PHY_88E1000S:

            PhyData = ReadPhyRegister(Adapter,
                                      PXN_PHY_SPEC_STAT_REG,
                                      Adapter->PhyAddress);

            DEBUGOUT1("CFL - Auto-Neg complete.  PhyData = %x\r",
                      PhyData);
            ConfigureMacToPhySettings(Adapter, PhyData);

            ConfigFlowControlAfterLinkUp(Adapter);
            break;

        default:
            DEBUGOUT("CFL - Invalid PHY detected.\r");
			break;
        }
    }

    else

         if ((Adapter->MediaType == MEDIA_TYPE_FIBER) &&
             (!(StatusRegValue & E1000_STATUS_LU)) &&
             (!(CtrlRegValue & E1000_CTRL_SWDPIN1)) &&
             (!(RxcwRegValue & E1000_RXCW_C))) {
        if (Adapter->AutoNegFailed == 0) {
            Adapter->AutoNegFailed = 1;
            return;
        }

        DEBUGOUT("NOT RXing /C/, disable AutoNeg and force link.\r");

        E1000_WRITE_REG(Txcw, (Adapter->TxcwRegValue & ~E1000_TXCW_ANE));

        CtrlRegValue = E1000_READ_REG(Ctrl);
        CtrlRegValue |= (E1000_CTRL_SLU | E1000_CTRL_FD);
        E1000_WRITE_REG(Ctrl, CtrlRegValue);

        ConfigFlowControlAfterLinkUp(Adapter);

    }
        else if ((Adapter->MediaType == MEDIA_TYPE_FIBER) &&
                 (CtrlRegValue & E1000_CTRL_SLU) &&
                 (RxcwRegValue & E1000_RXCW_C)) {

        DEBUGOUT("RXing /C/, enable AutoNeg and stop forcing link.\r");

        E1000_WRITE_REG(Txcw, Adapter->TxcwRegValue);

        E1000_WRITE_REG(Ctrl, (CtrlRegValue & ~E1000_CTRL_SLU));
    }
}

BOOLEAN SetupPcsLink(PADAPTER_STRUCT Adapter, UINT32 DeviceControlReg)
{
    UINT32 i;
    UINT32 StatusContents;
    UINT32 TctlReg;
    UINT32 TransmitConfigWord;

    DEBUGFUNC("SetupPcsLink")

        TctlReg = E1000_READ_REG(Tctl);
    TctlReg |= (E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT);
    E1000_WRITE_REG(Tctl, TctlReg);

    switch (Adapter->FlowControl) {
    case FLOW_CONTROL_NONE:

        TransmitConfigWord = (E1000_TXCW_ANE | E1000_TXCW_FD);

        break;

    case FLOW_CONTROL_RECEIVE_PAUSE:

        TransmitConfigWord =
            (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK);

        break;

    case FLOW_CONTROL_TRANSMIT_PAUSE:

        TransmitConfigWord =
            (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_ASM_DIR);

        break;

    case FLOW_CONTROL_FULL:

        TransmitConfigWord =
            (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK);

        break;

    default:

        DEBUGOUT("Flow control param set incorrectly");
        ASSERT(0);
        break;
    }

    DEBUGOUT("Auto-negotiation enabled");

    E1000_WRITE_REG(Txcw, TransmitConfigWord);
    E1000_WRITE_REG(Ctrl, DeviceControlReg);

    Adapter->TxcwRegValue = TransmitConfigWord;
    DelayInMilliseconds(1);

    if (!(E1000_READ_REG(Ctrl) & E1000_CTRL_SWDPIN1)) {

        DEBUGOUT("Looking for Link");
        for (i = 0; i < (LINK_UP_TIMEOUT / 10); i++) {
            DelayInMilliseconds(10);

            StatusContents = E1000_READ_REG(Status);
            if (StatusContents & E1000_STATUS_LU)
                i = LINK_UP_TIMEOUT;
        }

        if (i == (LINK_UP_TIMEOUT / 10)) {

            DEBUGOUT("Never got a valid link from auto-neg!!!");

            Adapter->AutoNegFailed = 1;
            CheckForLink(Adapter);
            Adapter->AutoNegFailed = 0;
        } else {
            Adapter->AutoNegFailed = 0;
            DEBUGOUT("Valid Link Found");
        }
    } else {
        DEBUGOUT("No Signal Detected");
    }

    return (TRUE);
}

BOOLEAN SetupFlowControlAndLink(PADAPTER_STRUCT Adapter)
{
    UINT32 TempEepromWord;
    UINT32 DeviceControlReg;
    UINT32 ExtDevControlReg;
    BOOLEAN Status = TRUE;

    DEBUGFUNC("SetupFlowControlAndLink")

        TempEepromWord = ReadEepromWord(Adapter, EEPROM_INIT_CONTROL1_REG);

    DeviceControlReg =
        (((TempEepromWord & EEPROM_WORD0A_SWDPIO) << SWDPIO_SHIFT) |
         ((TempEepromWord & EEPROM_WORD0A_ILOS) << ILOS_SHIFT));

    /* DmaFairness */
	/*DeviceControlReg |= E1000_CTRL_PRIOR;*/

    TempEepromWord = ReadEepromWord(Adapter, EEPROM_INIT_CONTROL2_REG);

    if (Adapter->FlowControl > FLOW_CONTROL_FULL) {
        if ((TempEepromWord & EEPROM_WORD0F_PAUSE_MASK) == 0)
            Adapter->FlowControl = FLOW_CONTROL_NONE;
        else if ((TempEepromWord & EEPROM_WORD0F_PAUSE_MASK) ==
                 EEPROM_WORD0F_ASM_DIR) Adapter->FlowControl =
                FLOW_CONTROL_TRANSMIT_PAUSE;
        else
            Adapter->FlowControl = FLOW_CONTROL_FULL;
    }

    Adapter->OriginalFlowControl = Adapter->FlowControl;

    DEBUGOUT1("After fix-ups FlowControl is now = %x",
              Adapter->FlowControl);

	ExtDevControlReg = ((TempEepromWord & EEPROM_WORD0F_SWPDIO_EXT)
						<< SWDPIO__EXT_SHIFT);
	E1000_WRITE_REG(Exct, ExtDevControlReg);

	if (Adapter->MediaType == MEDIA_TYPE_FIBER) {
		Status = SetupPcsLink(Adapter, DeviceControlReg);
	}
	else {
		Status = PhySetup(Adapter, DeviceControlReg);
	}

    DEBUGOUT("Initializing the Flow Control address, type and timer regs");

    E1000_WRITE_REG(Fcal, FLOW_CONTROL_ADDRESS_LOW);
    E1000_WRITE_REG(Fcah, FLOW_CONTROL_ADDRESS_HIGH);
    E1000_WRITE_REG(Fct, FLOW_CONTROL_TYPE);
    E1000_WRITE_REG(Fcttv, FC_DEFAULT_TX_TIMER);

    if (!(Adapter->FlowControl & FLOW_CONTROL_TRANSMIT_PAUSE)) {
        E1000_WRITE_REG(Fcrtl, 0);
        E1000_WRITE_REG(Fcrth, 0);
    } else {
        E1000_WRITE_REG(Fcrtl, (FC_DEFAULT_LO_THRESH | E1000_FCRTL_XONE));
        E1000_WRITE_REG(Fcrth, FC_DEFAULT_HI_THRESH);
    }

    return (Status);
}

VOID InitRxAddresses(PADAPTER_STRUCT Adapter)
{
    UINT i;
    UINT32 HwLowAddress;
    UINT32 HwHighAddress;

    DEBUGFUNC("InitRxAddresses")

    HwLowAddress = (Adapter->CurrentNetAddress[0] |
                    (Adapter->CurrentNetAddress[1] << 8) |
                    (Adapter->CurrentNetAddress[2] << 16) |
                    (Adapter->CurrentNetAddress[3] << 24));

    HwHighAddress = (Adapter->CurrentNetAddress[4] |
                     (Adapter->CurrentNetAddress[5] << 8) | E1000_RAH_AV);

    E1000_WRITE_REG(Rar[0].Low, HwLowAddress);
    E1000_WRITE_REG(Rar[0].High, HwHighAddress);

    for (i = 1; i < E1000_RAR_ENTRIES; i++) {
        E1000_WRITE_REG(Rar[i].Low, 0);
        E1000_WRITE_REG(Rar[i].High, 0);
    }
}

BOOLEAN InitializeHardware(PADAPTER_STRUCT Adapter)
{
    UINT i;
    BOOLEAN Status;
    UINT32 DeviceStatusReg;

    DEBUGFUNC("InitializeHardware");

	DeviceStatusReg = E1000_READ_REG(Status);
	if (DeviceStatusReg & E1000_STATUS_TBIMODE) {
		Adapter->MediaType = MEDIA_TYPE_FIBER;
	} else {
		Adapter->MediaType = MEDIA_TYPE_COPPER;
    }

    DEBUGOUT("Initializing the IEEE VLAN");
    E1000_WRITE_REG(Vet, 0);

    for (i = 0; i < E1000_VLAN_FILTER_TBL_SIZE; i++) {
        E1000_WRITE_REG(Vfta[i], 0);
    }

    InitRxAddresses(Adapter);

    DEBUGOUT("Zeroing the MTA");
    for (i = 0; i < E1000_MC_TBL_SIZE; i++) {
        E1000_WRITE_REG(Mta[i], 0);
    }

    Status = SetupFlowControlAndLink(Adapter);

    ClearHwStatsCounters(Adapter);

    return (Status);
}

VOID AdapterStop(PADAPTER_STRUCT Adapter)
{
    UINT32 IcrContents;

    DEBUGFUNC("AdapterStop")

        if (Adapter->AdapterStopped) {
        DEBUGOUT("Exiting because the adapter is already stopped!!!");
        return;
    }

    Adapter->AdapterStopped = TRUE;

    DEBUGOUT("Masking off all interrupts");
    E1000_WRITE_REG(Imc, 0xffffffff);

    E1000_WRITE_REG(Rctl, 0);
    E1000_WRITE_REG(Tctl, 0);

    DelayInMilliseconds(10);

    DEBUGOUT("Issuing a global reset to MAC");
    E1000_WRITE_REG(Ctrl, E1000_CTRL_RST);

    DelayInMilliseconds(10);

    DEBUGOUT("Masking off all interrupts");
    E1000_WRITE_REG(Imc, 0xffffffff);

    IcrContents = E1000_READ_REG(Icr);
}


#ifdef DEBUG
static int e1000_debug_level = 0;
#endif // DEBUG

/* global defines */


#ifdef INTEL_COPYRIGHTS
/* Global Data structures and variables */
static const char *version =
"Intel(R) PRO/1000 Gigabit Ethernet Adapter - Loadable driver, ver. 2.5.14";
static char e1000_copyright[] = "         Copyright (c) 1999-2000 Intel Corporation";
static char e1000id_string[128] = "Intel(R) PRO/1000 Gigabit Server Adapter";
static char e1000_driver[] = "e1000";
static char e1000_version[] = "2.5.14";
#endif

static boolean_t
e1000_sw_init(PADAPTER_STRUCT Adapter)
{
    int i;
	char *dma, *map;
    PE1000_TRANSMIT_DESCRIPTOR txdesc;
    PE1000_RECEIVE_DESCRIPTOR rxdesc;

	DEBUGFUNC("e1000_sw_init");

	/* allocate all DMA memory in one chunk, then parcel it out */
	Adapter->mem_len = sizeof (E1000_TRANSMIT_DESCRIPTOR) * NUM_TX_BUFS +
			sizeof (E1000_RECEIVE_DESCRIPTOR) * NUM_RX_BUFS +
			BUF_SIZE * NUM_TX_BUFS + BUF_SIZE * NUM_RX_BUFS;
	Adapter->dma_mem = dma_alloc(Adapter->mem_len);

    if (Adapter->dma_mem == NULL)
	{
        typecr("\$e1000_sw_init e1000_rbd_data dma-alloc failed");
        return FALSE;
    }

	Adapter->map_mem = dma_map_in(Adapter->dma_mem, Adapter->mem_len, FALSE);

	if (Adapter->map_mem == NULL)
	{
		dma_free(Adapter->dma_mem, Adapter->mem_len);
		typecr("\$e1000_sw_init: map-in of dma-mem failed");
		return FALSE;
	}

	dma = Adapter->dma_mem;
	map = Adapter->map_mem;
	memset(Adapter->dma_mem, 0, Adapter->mem_len);

    /* Allocate memory for the transmit buffer descriptors */
    Adapter->NumTxDescriptors = NUM_TX_BUFS;
    Adapter->e1000_tbd_data = (PE1000_TRANSMIT_DESCRIPTOR)map;
    Adapter->FirstTxDescriptor = (PE1000_TRANSMIT_DESCRIPTOR)dma;
    Adapter->LastTxDescriptor =
			Adapter->FirstTxDescriptor + (Adapter->NumTxDescriptors - 1);

	dma += sizeof (E1000_TRANSMIT_DESCRIPTOR) * NUM_TX_BUFS;
	map += sizeof (E1000_TRANSMIT_DESCRIPTOR) * NUM_TX_BUFS;

#ifdef DEBUG
    if (e1000_debug_level >= 2)
	{
        type("\$tbd_data ");
		udot((UINT)Adapter->e1000_tbd_data);
		cr();
	}
#endif // DEBUG

    /* Allocate memory for the receive buffer descriptors */
    Adapter->NumRxDescriptors = NUM_RX_BUFS;
    Adapter->e1000_rbd_data = (PE1000_RECEIVE_DESCRIPTOR)map;
    Adapter->FirstRxDescriptor = (PE1000_RECEIVE_DESCRIPTOR)dma;
    Adapter->LastRxDescriptor =
			Adapter->FirstRxDescriptor + (Adapter->NumRxDescriptors - 1);

	dma += sizeof (E1000_RECEIVE_DESCRIPTOR) * NUM_RX_BUFS;
	map += sizeof (E1000_RECEIVE_DESCRIPTOR) * NUM_RX_BUFS;

#ifdef DEBUG
    if (e1000_debug_level >= 2)
	{
        type("\$rbd_data ");
		udot((UINT)Adapter->e1000_rbd_data);
		cr();
	}
#endif // DEBUG

	/* Allocate a buffer for each transmit descriptor. */
	Adapter->tx_bufs = dma;
	DEBUGOUT2("TX dma= map=", dma, map);
	txdesc = Adapter->FirstTxDescriptor;

    for (i = 0; i < Adapter->NumTxDescriptors; i++)
	{
		txdesc->BufferAddress.lo = (UINT)map;
		dma += BUF_SIZE;
		map += BUF_SIZE;
		txdesc++;
	}

	/* Allocate a buffer for each receive descriptor. */
	Adapter->rx_bufs = dma;
	DEBUGOUT2("RX dma= map=", dma, map);
	rxdesc = Adapter->FirstRxDescriptor;

    for (i = 0; i < Adapter->NumRxDescriptors; i++)
	{
		rxdesc->BufferAddress.lo = (UINT)map;
		dma += BUF_SIZE;
		map += BUF_SIZE;
		rxdesc++;
		DEBUGOUT2("rxbuf i map", i, map);
	}

	dma_sync(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);
    return TRUE;
}

static int
ReadNodeAddress(IN PADAPTER_STRUCT Adapter, OUT PUCHAR NodeAddress)
{
    UINT i;
    USHORT EepromWordValue;

    DEBUGFUNC("ReadNodeAddress");

    /* Read our node address from the EEPROM. */
    for (i = 0; i < NODE_ADDRESS_SIZE; i += 2) {
        /* Get word i from EEPROM */
        EepromWordValue = ReadEepromWord(IN Adapter, IN(USHORT)

                                         (EEPROM_NODE_ADDRESS_BYTE_0
                                          + (i / 2)));

        /* Save byte i */
        NodeAddress[i] = (UCHAR) EepromWordValue;

        /* Save byte i+1 */
        NodeAddress[i + 1] = (UCHAR) (EepromWordValue >> 8);
    }


    /* Our IA should not have the Multicast bit set */
    if (NodeAddress[0] & 0x1) {
        return (RET_STATUS_FAILURE);
    }

    memcpy((char*)&Adapter->CurrentNetAddress[0],
           (char*)&Adapter->perm_node_address[0], MAC_ADDR);
    return (RET_STATUS_SUCCESS);

}

static int
e1000_set_mac(PADAPTER_STRUCT Adapter)
{
    UINT32 HwLowAddress = 0;
    UINT32 HwHighAddress = 0;
    UINT32 RctlRegValue;

	DEBUGFUNC("e1000_set_mac()");

    RctlRegValue = E1000_READ_REG(Rctl);

   /******************************************************************
    ** Setup the receive address (individual/node/network address).
    ******************************************************************/

	DEBUGOUT("Programming IA into RAR[0]");

    HwLowAddress = (Adapter->CurrentNetAddress[0] |
                    (Adapter->CurrentNetAddress[1] << 8) |
                    (Adapter->CurrentNetAddress[2] << 16) |
                    (Adapter->CurrentNetAddress[3] << 24));

    HwHighAddress = (Adapter->CurrentNetAddress[4] |
                     (Adapter->CurrentNetAddress[5] << 8) | E1000_RAH_AV);

    E1000_WRITE_REG(Rar[0].Low, HwLowAddress);
    E1000_WRITE_REG(Rar[0].High, HwHighAddress);

    return (0);
}

static void
SetupTransmitStructures(PADAPTER_STRUCT Adapter)
{
	DEBUGFUNC("SetupTransmitStructures");

   /**************************************************
    *        Setup TX Descriptors
    ***************************************************/

    /* Setup TX descriptor pointers */
    Adapter->NextAvailTxDescriptor = Adapter->FirstTxDescriptor;
    Adapter->OldestUsedTxDescriptor = Adapter->FirstTxDescriptor;

    /* Reset the number of descriptors that are available */
    Adapter->NumTxDescriptorsAvail = Adapter->NumTxDescriptors;

    /* Setup the Transmit Control Register (TCTL). */
    if (Adapter->FullDuplex) {
        E1000_WRITE_REG(Tctl, (E1000_TCTL_PSP | E1000_TCTL_EN |
                               (E1000_COLLISION_THRESHOLD <<
                                E1000_CT_SHIFT) |
                               (E1000_FDX_COLLISION_DISTANCE <<
                                E1000_COLD_SHIFT)));
    } else {
        E1000_WRITE_REG(Tctl, (E1000_TCTL_PSP | E1000_TCTL_EN |
                               (E1000_COLLISION_THRESHOLD <<
                                E1000_CT_SHIFT) |
                               (E1000_HDX_COLLISION_DISTANCE <<
                                E1000_COLD_SHIFT)));
    }

   /***********************************************************
    *   Setup Hardware TX Registers 
    ************************************************************/

    /* Setup HW Base and Length of Tx descriptor area */
    E1000_WRITE_REG(Tdbal, Adapter->e1000_tbd_data);
    E1000_WRITE_REG(Tdbah, 0);

    E1000_WRITE_REG(Tdl, (Adapter->NumTxDescriptors *
                          sizeof(E1000_TRANSMIT_DESCRIPTOR)));

    /* Setup our HW Tx Head & Tail descriptor pointers */
    E1000_WRITE_REG(Tdh, 0);
    E1000_WRITE_REG(Tdt, 0);


    /* Set the Transmit IPG register with default values.
     * Three values are used to determine when we transmit a packet on the 
     * wire: IPGT, IPGR1, & IPGR2.  IPGR1 & IPGR2 have no meaning in full 
     * duplex. Due to the state machine implimentation all values are 
     * effectivly 2 higher i.e. a setting of 10 causes the hardware to 
     * behave is if it were set to 1
     *  2. These settigs are in "byte times".  For example, an IPGT setting
     * of 10 plus the state machine delay of 2 is 12 byte time = 96 bit 
     * times. 
     */

    /* Set the Transmit IPG register with default values. */
	if(Adapter->MediaType == MEDIA_TYPE_FIBER) {
		E1000_WRITE_REG(Tipg, DEFAULT_LVGD_TIPG_IPGT_FIBER |
				(DEFAULT_LVGD_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT) |
				(DEFAULT_LVGD_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT));
	} else {
		E1000_WRITE_REG(Tipg, DEFAULT_LVGD_TIPG_IPGT_COPPER |
				(DEFAULT_LVGD_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT) |
				(DEFAULT_LVGD_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT));
	}

    /*
     * Set the Transmit Interrupt Delay register with the vaule for the 
     * transmit interrupt delay
     */
    E1000_WRITE_REG(Tidv, Adapter->TxIntDelay);
}

static int
SetupReceiveStructures(PADAPTER_STRUCT Adapter, boolean_t flag)
{
    UINT32 RegRctl = 0;

	DEBUGFUNC("SetupReceiveStructures");

    /*
     * Set the Receive interrupt delay and also the maximum number of times 
     * the transmit and receive interrupt are going to be processed from the
     * space.c file
     */
    Adapter->RxIntDelay = 0;

    /* Setup our descriptor pointers */
    Adapter->NextRxDescriptorToCheck = Adapter->FirstRxDescriptor;

    /*
     * Set up all the RCTL fields
     */
    E1000_WRITE_REG(Rctl, 0);

    E1000_WRITE_REG(Rdtr0, (Adapter->RxIntDelay | E1000_RDT0_FPDB));

    /* Setup HW Base and Length of Rx descriptor area */
    E1000_WRITE_REG(Rdbal0, Adapter->e1000_rbd_data);
    E1000_WRITE_REG(Rdbah0, 0);

    E1000_WRITE_REG(Rdlen0, (Adapter->NumRxDescriptors *
                             sizeof(E1000_RECEIVE_DESCRIPTOR)));

    /* Setup our HW Rx Head & Tail descriptor pointers */
    E1000_WRITE_REG(Rdh0, 0);
    E1000_WRITE_REG(Rdt0,
			Adapter->LastRxDescriptor - Adapter->FirstRxDescriptor);

    /* Setup the Receive Control Register (RCTL), and ENABLE the receiver.
     * The initial configuration is to: Enable the receiver, accept
     * broadcasts, discard bad packets (and long packets), disable VLAN filter
     * checking, set the receive descriptor minimum threshold size to 1/2,
     * and the receive buffer size to 2k. */

    RegRctl = E1000_RCTL_EN |         /* Enable Receive Unit */
              E1000_RCTL_BAM |        /* Accept Broadcast Packets */
              (Adapter->MulticastFilterType << E1000_RCTL_MO_SHIFT) |
              E1000_RCTL_RDMTS0_EIGTH |
              E1000_RCTL_LBM_NO;      /* Loopback Mode = none */

    switch (Adapter->RxBufferLen) {
    case E1000_RXBUFFER_2048:
    default:
        RegRctl |= E1000_RCTL_SZ_2048;
        break;
    case E1000_RXBUFFER_4096:
        RegRctl |= E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    case E1000_RXBUFFER_8192:
        RegRctl |= E1000_RCTL_SZ_8192 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    case E1000_RXBUFFER_16384:
        RegRctl |= E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
        break;
    }

	/* RegRctl |= E1000_RCTL_SBP; */
    E1000_WRITE_REG(Rctl, RegRctl);
    
    /*
     * Check and report the status of the link
     */
    if (!(E1000_READ_REG(Status) & E1000_STATUS_LU) && (flag == TRUE))
        typecr("\$e1000: NIC Link is Down");
   
    return (0);
}

static int
e1000_set_promisc(PADAPTER_STRUCT Adapter, int flag)
{
    UINT32 RctlRegValue;
    UINT32 TempRctlReg;
    UINT32 NewRctlReg;

    /*
     * Save the original value in the RCTL register
     */
    RctlRegValue = E1000_READ_REG(Rctl);
    TempRctlReg = RctlRegValue;

	DEBUGFUNC("e1000_set_promiscuous");

    /* Check to see if we should set/clear the "Unicast Promiscuous" bit. */
    if (flag == B_TRUE) {
        /*
         * Since the promiscuous flag is set to TRUE, set both the unicast as
         * well as the multicast promiscuous mode on the RCTL
         */
        TempRctlReg |= (E1000_RCTL_UPE | E1000_RCTL_MPE | E1000_RCTL_BAM);
        E1000_WRITE_REG(Rctl, TempRctlReg);
		DEBUGOUT("Promiscuous mode ON");
    } else {
        /*
         * Turn off both the unicast as well as the multicast promiscuous mode
         */
        TempRctlReg &= (~E1000_RCTL_UPE);
        TempRctlReg &= (~E1000_RCTL_MPE);
        E1000_WRITE_REG(Rctl, TempRctlReg);
		DEBUGOUT("Promiscuous mode OFF");
    }


    /* Write the new filter back to the adapter, if it has changed */
    if (TempRctlReg != RctlRegValue) {
        E1000_WRITE_REG(Rctl, TempRctlReg);
    }

    NewRctlReg = E1000_READ_REG(Rctl);

    if (NewRctlReg != TempRctlReg)
        return (B_FALSE);
    else
        return (B_TRUE);
}

static Bool
ProcessTransmitPacket(PADAPTER_STRUCT Adapter)
{
    PE1000_TRANSMIT_DESCRIPTOR TransmitDescriptor;
	Bool done = FALSE;
	int timeout = 10 * 1000;	/* 10 seconds */
    ushort_t icr;

#ifdef DEBUG
    if (e1000_debug_level >= 2)
        typecr("\$ProcessTransmitPacket");
#endif // DEBUG

	/* loop waiting for packet(s) to be transmitted
	 */
    while (!done && timeout > 0)
	{
        /* Get hold of the next descriptor that the E1000 will report status 
         * back within.
         */
		icr = E1000_READ_REG(Icr);
		dma_sync(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);
        TransmitDescriptor = Adapter->OldestUsedTxDescriptor;

        /* Check for wrap case */
        if (TransmitDescriptor > Adapter->LastTxDescriptor)
            TransmitDescriptor -= Adapter->NumTxDescriptors;

        if (TransmitDescriptor->Upper.Fields.TransmitStatus &
				E1000_TXD_STAT_DD)
		{
			TransmitDescriptor->Lower.DwordData = 0;
			TransmitDescriptor->Upper.DwordData = 0;

            /* Free descriptors used by this packet. */
            if (TransmitDescriptor == Adapter->LastTxDescriptor)
                Adapter->OldestUsedTxDescriptor = Adapter->FirstTxDescriptor;
            else
                Adapter->OldestUsedTxDescriptor++;

            /* Make available the descriptor that was previously used */
            Adapter->NumTxDescriptorsAvail++;
			done = TRUE;	/* and we can return */
        }
		else
		{
			ms(10);		/* else wait for a bit */
			timeout -= 10;
		}
    }

#ifdef DEBUG
    if (e1000_debug_level >= 3)
		DEBUGOUT1("ProcessTx: done ", done);
#endif // DEBUG

	return done;
}

static int
ProcessReceivePacket(PADAPTER_STRUCT Adapter, char *buf, int len)
{
    /* Pointer to the receive descriptor being examined. */
    PE1000_RECEIVE_DESCRIPTOR CurrentDescriptor;
    /* The amount of data received in the RBA (will be PacketSize +
     * 4 for the CRC).
     */
    int Length = -2;
    BOOLEAN Status = FALSE;
    ushort_t icr;

#ifdef DEBUG
    if (e1000_debug_level >= 4)
        typecr("\$ProcessReceiveInterrupts");
#endif // DEBUG

    icr = E1000_READ_REG(Icr);
	dma_sync(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);
    CurrentDescriptor = Adapter->NextRxDescriptorToCheck;

    if (!((CurrentDescriptor->ReceiveStatus) & E1000_RXD_STAT_DD)) {
#ifdef DEBUG
        if (e1000_debug_level >= 4)
            typecr("\$Proc_rx_ints: Nothing to do!");
#endif // DEBUG

        /* nothing received to buffer yet */
        return -2;
    }

	/* Make sure this is also the last descriptor in the packet. */
	if (CurrentDescriptor->ReceiveStatus & E1000_RXD_STAT_EOP) 
	{                /* packet has EOP set - begin */
		/* 
		 * Store the packet length. Take the CRC lenght out of the length 
		 *  calculation.   
		 */
		Length = CurrentDescriptor->Length - CRC_LENGTH;

		/*
		 * Only the packets that are valid ethernet packets are processed.
		 * Normally, hardware will discard bad packets . 
		 */
		if ((Length <= (Adapter->LongPacket == TRUE ? MAX_JUMBO_FRAME_SIZE :
											  MAXIMUM_ETHERNET_PACKET_SIZE))
				&& (Length >= MINIMUM_ETHERNET_PACKET_SIZE)
				&& ((CurrentDescriptor->Errors == 0) || 
					(CurrentDescriptor->Errors == E1000_RXD_ERR_CE)))
		{
			Status = TRUE;

			if (Length > len)
				Length = len;

			/* copy the data into the return buffer */
			memcpy(buf, Adapter->rx_bufs + BUF_SIZE *
					(CurrentDescriptor - Adapter->FirstRxDescriptor), Length);

#ifdef DEBUG
			if (e1000_debug_level >= 3)
				DEBUGOUT2("rx buf map-buf ", 
					Adapter->rx_bufs + BUF_SIZE *
						(CurrentDescriptor - Adapter->FirstRxDescriptor), 
					CurrentDescriptor->BufferAddress.lo);
#endif // DEBUG
		}
		else
		{
			typecr("\$e1000: Bad Packet Length");
		}                    /* packet len is good/bad if */
	}                        /* packet has EOP set - end */

	/* Zero out the receive descriptors fields */
	CurrentDescriptor->ReceiveStatus = 0;
	CurrentDescriptor->Length = 0;
    CurrentDescriptor->Csum = 0;
    CurrentDescriptor->Errors = 0;
    CurrentDescriptor->Special = 0;

	/* Advance our pointers to the next descriptor (checking for wrap). */
	if (CurrentDescriptor == Adapter->LastRxDescriptor)
		Adapter->NextRxDescriptorToCheck = Adapter->FirstRxDescriptor;
	else
		Adapter->NextRxDescriptorToCheck++;

#ifdef DEBUG
	if (e1000_debug_level >= 2)
	{
		type("\$Next RX Desc to check is ");
		udot((UINT)CurrentDescriptor);
		cr();
	}
#endif // DEBUG

	dma_sync(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);

	/* Advance the E1000's Receive Queue #0  "Tail Pointer". */
	E1000_WRITE_REG(Rdt0, CurrentDescriptor - Adapter->FirstRxDescriptor);

#ifdef DEBUG
	if (e1000_debug_level >= 3)
	{
		type("\$Rx Tail pointer is now");
		udot(CurrentDescriptor - Adapter->FirstRxDescriptor);
        typecr("\$ProcessReceiveInterrupts: done");
	}
#endif // DEBUG

    return Length;
}

static Bool
SendBuffer(char *buf, int len, PADAPTER_STRUCT Adapter)
{
    PE1000_TRANSMIT_DESCRIPTOR CurrentTxDescriptor;
	char *txbuf;

#ifdef DEBUG
    if (e1000_debug_level >= 3)
		DEBUGFUNC("SendBuffer");
#endif // DEBUG

    if (Adapter->NumTxDescriptorsAvail <= 1) {
		return FALSE;
	}

    CurrentTxDescriptor = Adapter->NextAvailTxDescriptor;
	txbuf = Adapter->tx_bufs + BUF_SIZE *
			(CurrentTxDescriptor - Adapter->FirstTxDescriptor);
	memcpy(txbuf, buf, len);
    CurrentTxDescriptor->Lower.DwordData = len;

    /* zero out the status field in the descriptor. */
    CurrentTxDescriptor->Upper.DwordData = 0;

    /* Check the wrap-around case */
    if (CurrentTxDescriptor == Adapter->LastTxDescriptor)
        Adapter->NextAvailTxDescriptor = Adapter->FirstTxDescriptor;
    else
        Adapter->NextAvailTxDescriptor++;

    Adapter->NumTxDescriptorsAvail--;

    /*
     * If there is a valid value for the transmit interrupt delay, set up the
     * delay in the descriptor field
     */
    if (Adapter->TxIntDelay) {
        CurrentTxDescriptor->Lower.DwordData |=
				(E1000_TXD_CMD_EOP | E1000_TXD_CMD_IDE | E1000_TXD_CMD_IFCS);
    } else {
        CurrentTxDescriptor->Lower.DwordData |=
				(E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS);
    }

    /* Set the RS or the RPS bit by looking at the ReportTxEarly setting */
	CurrentTxDescriptor->Lower.DwordData |= E1000_TXD_CMD_RS;
	//CurrentTxDescriptor->Lower.DwordData |= E1000_TXD_CMD_RPS;

	dma_sync(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);

    /* Advance the Transmit Descriptor Tail (Tdt), this tells the 
     * E1000 that this frame is available to transmit. 
     */
    E1000_WRITE_REG(Tdt,
			Adapter->NextAvailTxDescriptor - Adapter->FirstTxDescriptor);

	/* and wait until the packet is actually sent */
	return ProcessTransmitPacket(Adapter);
}

static int
e1000_init(PADAPTER_STRUCT Adapter)
{
    UINT16 LineSpeed, FullDuplex;
	Fstr str;

	DEBUGFUNC("e1000_init");

    /**** Reset and Initialize the E1000 *******/
    AdapterStop(Adapter);
    Adapter->AdapterStopped = FALSE;

	DEBUGFUNC("e1000_init: about to validate");

    /* Validate the EEPROM */
    if (!ValidateEepromChecksum(Adapter)) {
        typecr("\$e1000: The EEPROM checksum is not valid");
        return (1);
    }

    /* read the ethernet address from the eprom */
    ReadNodeAddress(Adapter, &Adapter->perm_node_address[0]);
	/* Read the part number from the eprom */
	ReadPartNumber(Adapter, &(Adapter->PartNumber));
        
#ifdef DEBUG
    if (e1000_debug_level >= 2) {
        type("\$Node addr is: ");
        udot(Adapter->perm_node_address[0]);
        udot(Adapter->perm_node_address[1]);
        udot(Adapter->perm_node_address[2]);
        udot(Adapter->perm_node_address[3]);
        udot(Adapter->perm_node_address[4]);
        udot(Adapter->perm_node_address[5]);
		cr();
    }
#endif // DEBUG

    /* save off the perm node addr in the local-mac-address property */
	str.str = (char*)&Adapter->perm_node_address[0];
	str.len = MAC_ADDR;
	property(str, "\$local-mac-address");

    /* Scan the chipset and determine whether to set the RS bit or
     * the RPS bit during transmits. On some systems with a fast chipset
     * (450NX), setting the RS bit may cause data corruption. Check the
     * space.c paratemer to see if the user has forced this setting or 
     * has let the software do the detection.
     */

    Adapter->FlowControl = FLOW_CONTROL_FULL;

    /* Do the adapter initialization */
    if (!InitializeHardware(Adapter)) {
        typecr("\$InitializeHardware Failed");
        return (1);
    }

	/* get our MAC address, which may not be our local-mac-address */
	str = mac_address();

	/* use this returned value for our MAC addr */
	if (str.len == MAC_ADDR)
		memcpy((char*)Adapter->CurrentNetAddress, str.str, str.len);

	/* and set it */
	e1000_set_mac(Adapter);

	/* setup everything else that needs setting */
	e1000_set_promisc(Adapter, Adapter->Promiscuous);

    CheckForLink(Adapter);
    if(E1000_READ_REG(Status) & E1000_STATUS_LU) {
        Adapter->LinkIsActive = TRUE;
        GetSpeedAndDuplex(Adapter, &LineSpeed, &FullDuplex);
        Adapter->cur_line_speed = (UINT32) LineSpeed;
        Adapter->FullDuplex = (UINT32) FullDuplex;
    } else {
        Adapter->LinkIsActive = FALSE;
        Adapter->cur_line_speed = 0;
        Adapter->FullDuplex = 0;
    }

#ifdef DEBUG
    if (e1000_debug_level >= 1)
        typecr("\$e1000_init: end");
#endif // DEBUG

    return (0);
}

static int
e1000_runtime_init(PADAPTER_STRUCT Adapter)
{
	DEBUGFUNC("e1000_runtime_init()");

    /* set RxBufferLen to something */
	Adapter->RxBufferLen = E1000_SIZE_OF_RECEIVE_BUFFERS;
	Adapter->LongPacket = FALSE;

    if (!e1000_sw_init(Adapter)) {
        /*  Board is disabled because all memory structures
         *  could not be allocated
         */
        typecr("\$Could not allocate the software mem structures");
        return (1);
    }

    /* Setup and initialize the transmit structures. */
    SetupTransmitStructures(Adapter);

    /* Setup and initialize the receive structures -- we can receive packets
     * off of the wire 
     */
    SetupReceiveStructures(Adapter, TRUE);

    return 0;
}

static int
e1000_close(PADAPTER_STRUCT Adapter)
{
    ushort_t status;

	DEBUGFUNC("e1000_close");

    /* Disable all possible interrupts */
    E1000_WRITE_REG(Imc, (0xffffffff));
    status = E1000_READ_REG(Icr);

    /* Reset the chip */
    AdapterStop(Adapter);

	/* free up all allocated memory */
	if (Adapter->map_mem)
	{
		dma_map_out(Adapter->dma_mem, Adapter->map_mem, Adapter->mem_len);
		dma_free(Adapter->dma_mem, Adapter->mem_len);
	}

	Adapter->dma_mem = NULL;
	Adapter->map_mem = NULL;
    Adapter->mem_len = 0;
    return (0);
}

static int
e1000_open(PADAPTER_STRUCT Adapter)
{
	DEBUGFUNC("e1000_open");

    if (e1000_init(Adapter)) {
        return -1;
    }
    if (e1000_runtime_init(Adapter)) {
        return -1;
    }

    Adapter->AdapterStopped = FALSE;
    return (0);
}


/* END HACKED INTEL CODE */



#ifdef STANDALONE
#pragma expand_inline
static void
setup_device_main(void)
{
	DEV("/pci");
	set_my_self(open_dev("\$/pci"));
	new_device();
}
#endif /* STANDALONE */


/* package variables (globals) */

static ADAPTER_STRUCT g_adapter;
static Bool g_promiscuous;
static int g_speed;
static Bool g_fullduplex;

static volatile unsigned char *g_mmio = NULL;
static int g_mmiosize = 0;
static Instance g_obptftp = NULL;


/* some handy routines */

static Bool
fstreq(Fstr s1, Fstr s2)
{
	Bool ret = FALSE;

	if (s1.len == s2.len)
		ret = comp(s1.str, s2.str, s1.len);

	return ret;
}


int
open(void)			/* open (-- okay?) */
{
	int physhi, physmid, physlo;
	int sizehi, sizelo;
	int cfg, i;
	Font_info fi;
	Prop_err pe;
	Prop_int pi;
	Fstr args;
	Two_fstrs args2;
	Bool diag = diagnostic_mode();
	int addr;

	addr = my_space() & 0xFFFFFF00;

	/* init globals */
	g_promiscuous = FALSE;
	g_fullduplex = TRUE;
	g_speed = 0;	/* auto-negotiate */
	memset((char *)&g_adapter, sizeof g_adapter, 0);

	/* parse our assigned-addresses property to find our register memory-map */
	pe = get_my_property("\$assigned-addresses");

	if (pe.err)
	{
		type("\$cannot find assigned-addresses property\r\n");
		return FALSE;
	}

	pi.prop = pe.prop;

	do
	{
		pi = decode_int(pi.prop);
		physhi = pi.val;
		pi = decode_int(pi.prop);
		physmid = pi.val;
		pi = decode_int(pi.prop);
		physlo = pi.val;
		pi = decode_int(pi.prop);
		sizehi = pi.val;
		pi = decode_int(pi.prop);
		sizelo = pi.val;

	} while (pi.prop.len > 0 && PCI_PHYSHI_SPACE(physhi) != PCI_SPACE_MEM);

	/* assume first memory-space map is for our registers */
	g_mmiosize = sizelo;
	g_mmio = (unsigned char*)map_in(physlo, physmid, physhi, g_mmiosize);

#ifdef DEBUG
	type("\$wx.open: mmio/size=");
	udot((int)g_mmio);
	type("\$/");
	udot(g_mmiosize);
	cr();
#endif

	if (g_mmio == NULL)
	{
		type("\$wx.open: could not map in registers!\r\n");
		return FALSE;
	}

	/* parse our args and set various options */
	args = my_args();
	args2 = left_parse_string(args, ',');

	while (args2.left.len > 0)
	{
		Bool nextarg = TRUE;
		args = args2.left;

		if (fstreq(args, "\$promiscuous"))
			g_promiscuous = TRUE;
		else if (fstreq(args, "\$speed=auto"))
			g_speed = 0;
		else if (fstreq(args, "\$speed=10"))
			g_speed = 10;
		else if (fstreq(args, "\$speed=100"))
			g_speed = 100;
		else if (fstreq(args, "\$speed=1000"))
			g_speed = 1000;
		else if (fstreq(args, "\$duplex=half"))
			g_fullduplex = FALSE;
		else if (fstreq(args, "\$duplex=full"))
			g_fullduplex = TRUE;
		else
			nextarg = FALSE;

		if (nextarg)
		{
			args = args2.right;
			args2 = left_parse_string(args, ',');
		}
	}

	/* create an instance of the obp-tftp package, passing it the
	   any remaining unparsed args */
	push((int)args.str);
	push(args.len);
	g_obptftp = dopen_package(args, "\$obp-tftp");

	if (g_obptftp == NULL)
	{
		type("\$wx.open: could not create instance of obp-tftp!\r\n");
		map_out((char*)g_mmio, g_mmiosize);
		g_mmio = NULL;
		return FALSE;
	}

	/* setup pointer to registers and other fields */
	g_adapter.regs = (E1000_REGISTERS*)g_mmio;
	g_adapter.Promiscuous = g_promiscuous;

	/* setup auto-negotiation or forced speed/duplex */
	if (g_speed == 0)
		g_adapter.AutoNeg = TRUE;
	else
		/* generates the proper enum SPEED_DUPLEX_TYPE */
		g_adapter.ForcedSpeedDuplex = g_speed | (g_fullduplex & 0x1);

	/* enable I/O and memory */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	config_setl((cfg & 0xFFFF) | PCI_COMMAND_IOENABLE |
			PCI_COMMAND_MEMENABLE | PCI_COMMAND_MASTERENABLE,
			addr | PCI_CONFIG_COMMAND);

	/* initialize the adapter */
	if (e1000_open(&g_adapter) != 0)
	{
		type("\$wx.open: could not open adapter!\r\n");
		close_package(g_obptftp);
		g_obptftp = NULL;
		map_out((char*)g_mmio, g_mmiosize);
		g_mmio = NULL;
		return FALSE;
	}

	/* let the user know the MAC addr to help configure BOOTP */
	if (diag)
	{
		type("\$Ethernet MAC addr: ");

		for (i = 0; i < 6; i++)
			udot(g_adapter.CurrentNetAddress[i]);

		cr();
	}

	return TRUE;
}

void
close(void)			/* close (--) */
{
	int cfg;
	int addr;

	e1000_close(&g_adapter);

	addr = my_space() & 0xFFFFFF00;

	/* turn I/O and memory access off */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	config_setl(cfg & 0xFFF8, addr | PCI_CONFIG_COMMAND);

	if (g_obptftp)
		close_package(g_obptftp);
	
	if (g_mmio)
		map_out((char*)g_mmio, g_mmiosize);
	
	g_obptftp = NULL;
	g_mmio = NULL;
}

int
read(char *buf, int len)		/* read (addr len -- actual) */
{
	return ProcessReceivePacket(&g_adapter, buf, len);
}

int
write(char *buf, int len)		/* write (addr len -- actual) */
{
	return (SendBuffer(buf, len, &g_adapter)) ? len : 0;
}

int
load(char *addr)				/* load (addr -- size) */
{
	push((int)addr);
	dcall_method("\$load", g_obptftp);
	return pop();
}

int
ping(char *ipaddr, int msecs)	/* ping (ip-addr msecs -- reply-msecs) */
{
	push((int)ipaddr);
	push(msecs);
	dcall_method("\$ping", g_obptftp);
	return pop();
}

int
selftest(void)					/* selftest (-- 0|error-code) */
{
	Bool diag;
	int err;

	diag = diagnostic_mode();

	if (diag)
		type("\$wx selftest...");

	err = R_OK;		/* TODO */

	if (err)
	{
		if (diag)
			type("\$FAILED!\r\n");
	}
	else
	{
		if (diag)
			type("\$ok\r\n");
	}

	return err;
}

static Fstr
encode_reg(Fstr s1, int pcireg)
{
	int addr, reg, save, set, cfg;
	int t = 0;
	int p = 0;
	int size = 0;
	int ptype = 0;
	int space = 0;
	int physhi = 0;
	Fstr s2;

	addr = my_space() & 0xFFFFFF00;
	reg = pcireg | addr;

	/* disable device before probing BARs for size */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	config_setl(cfg & 0xFFF8, addr | PCI_CONFIG_COMMAND);

	save = config_getl(reg);
	config_setl(0xFFFFFFFF, reg);
	set = config_getl(reg);
	config_setl(save, reg);

	if (set & 1)
	{
		/* i/o space */
		size = (~set | 3) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		ptype = set & 3;
	}
	else
	{
		/* memory space */
		size = (~set | 0xF) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		ptype = set & 0xF;
	}

	if (ptype & 1)
	{
		space = PCI_SPACE_IO;		/* i/o space */
		t = (ptype >> 1) & 1;
	}
	else
	{
		/* memory space */
		p = (ptype >> 3) & 1;
		space = PCI_SPACE_MEM;

		if ((ptype & 6) == 2)
			t = 1;	/* 32 bit BAR in lower 1-meg */
	}

	/* calculate size of expansion ROM BAR */
	if (pcireg == PCI_CONFIG_ROMADDR)
	{
		size = (~set | 0x7FF) + 1;
		size &= -size;		/* clear any un-programable upper bits */
		space = PCI_SPACE_MEM;
	}

	/* restore config reg */
	config_setl(cfg & 0xFFFF, addr | PCI_CONFIG_COMMAND);

	physhi = PCI_PHYSHI_MK(0, p, t, space, /*bus*/ 0, /*dev*/ 0,
			/*func*/ 0, reg);

	if (size)
	{
		s2 = encode_phys(0, 0, physhi);
		s1 = encodeplus(s1, s2);

		s2 = encode_int(0);
		s1 = encodeplus(s1, s2);

		s2 = encode_int(size);
		s1 = encodeplus(s1, s2);
	}

	return s1;
}

void
main(void)
{
	Fstr s1, s2;
	Phys3 phys;

	/* create this device's name and type */
	device_name("\$ethernet");
	device_type("\$network");

#ifdef STANDALONE
	/* setup the basic properties for this device */
	make_properties();
#endif

	/* setup first part of "reg" property for our config addr */
	phys = my_address2();
	phys.hi = my_space();
	s1 = encode_phys(phys.lo, phys.mid, phys.hi);
	s2 = encode_int(0);
	s1 = encodeplus(s1, s2);
	s2 = encode_int(0);
	s1 = encodeplus(s1, s2);

	/* append to "reg" property for BAR[0-5] and ROMADDR config registers */
	s1 = encode_reg(s1, PCI_CONFIG_BAR0);
	s1 = encode_reg(s1, PCI_CONFIG_BAR1);
	s1 = encode_reg(s1, PCI_CONFIG_BAR2);
	s1 = encode_reg(s1, PCI_CONFIG_BAR3);
	s1 = encode_reg(s1, PCI_CONFIG_BAR4);
	s1 = encode_reg(s1, PCI_CONFIG_BAR5);
	s1 = encode_reg(s1, PCI_CONFIG_ROMADDR);

	/* finally save it all as the "reg" property */
	property(s1, "\$reg");

	/* setup some other standard network properties */
	property(encode_int(MAC_ADDR * 8), "\$address-bits");
	property(encode_int(E1000_TX_BUFFER_SIZE), "\$maximum-frame-size");

#ifdef STANDALONE
	// needs to be here so it is the last thing executed
	finish_device();
	close_dev(my_self());
#endif /* STANDALONE */
}
