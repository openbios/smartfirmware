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

/* (c) Copyright 2002-2003 by CodeGen, Inc.  All Rights Reserved. */

/* On-board Fcode ethernet driver for Intel 82544 PRO/1000 gigabit cards.
   Compile with "cc-fcode" then tokenize with "of -t".
   Much code is copied and hacked from Intel's Linux e1000 driver sources.
 */

//#define DO_ASSERT
//#define DEBUG

#include "fcode.h"
#include "fcpci.h"

int em_debug_level = 0;

int force_open = 0;

typedef Byte int8_t;
typedef Doublet int16_t;
typedef Quadlet int32_t;
typedef uByte uint8_t, u_int8_t, boolean_t;
typedef uDoublet uint16_t, u_int16_t;
typedef uQuadlet uint32_t, u_int32_t;

typedef struct uint64
{
	volatile uQuadlet lo;
	volatile uQuadlet hi;
} uint64_t, u_int64_t;

typedef unsigned short ushort_t;

#define ETHER_CRC_LEN 4
#define ETHER_HDR_LEN 14
#define ETHERMTU 1500

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

#define usec_delay(x) ms(1)
#define msec_delay(x) ms(x)

#ifdef DEBUG

void
out(Fstr str)
{
    if (em_debug_level >= 1)
    {
	type(str);
	cr();
    }
}

void
out1(Fstr str, uint32_t a)
{
    if (em_debug_level >= 1)
    {
	type(str);
	udot(a);
	cr();
    }
}

void
out2(Fstr str, uint32_t a, uint32_t b)
{
    if (em_debug_level >= 1)
    {
	type(str);
	udot(a);
	udot(b);
	cr();
    }
}

void
out7(Fstr str, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g)
{
    if (em_debug_level >= 1)
    {
	type(str);
	udot(a);
	udot(b);
	udot(c);
	udot(d);
	udot(e);
	udot(f);
	udot(g);
	cr();
    }
}

void
outmac(Fstr str, uint8_t *addr)
{
    if (em_debug_level >= 1)
    {
	int i;

	type(str);
	type("\$ MAC addr: ");

	for (i = 0; i < 6; i++)
		udot((int)addr[i]);

	cr();
    }
}

#	define DEBUGMAC(S, M)			outmac("\$em: " S, M)
#	define DEBUGOUT(S)			out("\$em: " S)
#	define DEBUGOUT1(S, A)		out1("\$em: " S, (uint32_t)A)
#	define DEBUGOUT2(S, A, B)	out2("\$em: " S, (uint32_t)A, (uint32_t)B)
#	define DEBUGOUT7(S, A, B, C, D, E, F, G)	out7("\$em: " S, (uint32_t)A, (uint32_t)B, (uint32_t)C, (uint32_t)D, (uint32_t)E, (uint32_t)F, (uint32_t)G)
#	define DEBUGFUNC(F)		DEBUGOUT(F)
#else
#	define DEBUGMAC(S, M)
#	define DEBUGOUT(S)
#	define DEBUGOUT1(S, A)
#	define DEBUGOUT2(S, A, B)
#	define DEBUGOUT7(S, A, B, C, D, E, F, G)
#	define DEBUGFUNC(F)
#endif // DEBUG

//#define FALSE               0
//#define TRUE                1
#define CMD_MEM_WRT_INVALIDATE          0x0010  /* BIT_4 */
#define PCI_COMMAND_REGISTER		4

#define E1000_READ_REG(a, reg) ((a)->regs[E1000_##reg >> 2])
#define E1000_WRITE_REG(a, reg, value) em_write_reg((a), E1000_##reg, (value))

#define E1000_READ_REG_ARRAY(a, reg, offset) \
				((a)->regs[(E1000_##reg >> 2) + offset])
#define E1000_WRITE_REG_ARRAY(a, reg, offset, value) \
				((a)->regs[(E1000_##reg >> 2) + offset] = (value))

#define em_write_pci_cfg(hw, reg, ptr) \
	config_setl((my_space() & 0xFFFFFF00) | (reg), *(uint16_t *)ptr)

/* BEGIN HACKED INTEL CODE */

/*******************************************************************************

  Copyright (c) 2001-2002 Intel Corporation 
  All rights reserved. 
  
  Redistribution and use in source and binary forms of the Software, with or 
  without modification, are permitted provided that the following conditions 
  are met: 
  
   1. Redistributions of source code of the Software may retain the above 
      copyright notice, this list of conditions and the following disclaimer.
   
   2. Redistributions in binary form of the Software may reproduce the above 
      copyright notice, this list of conditions and the following disclaimer 
      in the documentation and/or other materials provided with the 
      distribution. 
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors shall be used to endorse or promote products derived from 
      this Software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR ITS CONTRIBUTORS BE LIABLE 
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
  SUCH DAMAGE.

*******************************************************************************/

/*$FreeBSD$*/
/* if_em_hw.h
 * Structures, enums, and macros for the MAC
 */


/* Forward declarations of structures used by the shared code */
struct em_hw;
struct em_hw_stats;

/* Enumerated types specific to the e1000 hardware */
/* Media Access Controlers */
typedef enum {
    em_82542_rev2_0 = 0,
    em_82542_rev2_1,
    em_82543,
    em_82544,
    em_82540,
    em_82545,
    em_82546,
    em_num_macs
} em_mac_type;

/* Media Types */
typedef enum {
    em_media_type_copper = 0,
    em_media_type_fiber = 1,
    em_num_media_types
} em_media_type;

typedef enum {
    em_10_half = 0,
    em_10_full = 1,
    em_100_half = 2,
    em_100_full = 3
} em_speed_duplex_type;

/* Flow Control Settings */
typedef enum {
    em_fc_none = 0,
    em_fc_rx_pause = 1,
    em_fc_tx_pause = 2,
    em_fc_full = 3,
    em_fc_default = 0xFF
} em_fc_type;

/* PCI bus types */
typedef enum {
    em_bus_type_unknown = 0,
    em_bus_type_pci,
    em_bus_type_pcix
} em_bus_type;

/* PCI bus speeds */
typedef enum {
    em_bus_speed_unknown = 0,
    em_bus_speed_33,
    em_bus_speed_66,
    em_bus_speed_100,
    em_bus_speed_133,
    em_bus_speed_reserved
} em_bus_speed;

/* PCI bus widths */
typedef enum {
    em_bus_width_unknown = 0,
    em_bus_width_32,
    em_bus_width_64
} em_bus_width;

/* PHY status info structure and supporting enums */
typedef enum {
    em_cable_length_50 = 0,
    em_cable_length_50_80,
    em_cable_length_80_110,
    em_cable_length_110_140,
    em_cable_length_140,
    em_cable_length_undefined = 0xFF
} em_cable_length;

typedef enum {
    em_10bt_ext_dist_enable_normal = 0,
    em_10bt_ext_dist_enable_lower,
    em_10bt_ext_dist_enable_undefined = 0xFF
} em_10bt_ext_dist_enable;

typedef enum {
    em_rev_polarity_normal = 0,
    em_rev_polarity_reversed,
    em_rev_polarity_undefined = 0xFF
} em_rev_polarity;

typedef enum {
    em_polarity_reversal_enabled = 0,
    em_polarity_reversal_disabled,
    em_polarity_reversal_undefined = 0xFF
} em_polarity_reversal;

typedef enum {
    em_auto_x_mode_manual_mdi = 0,
    em_auto_x_mode_manual_mdix,
    em_auto_x_mode_auto1,
    em_auto_x_mode_auto2,
    em_auto_x_mode_undefined = 0xFF
} em_auto_x_mode;

typedef enum {
    em_1000t_rx_status_not_ok = 0,
    em_1000t_rx_status_ok,
    em_1000t_rx_status_undefined = 0xFF
} em_1000t_rx_status;

struct em_phy_info {
    em_cable_length cable_length;
    em_10bt_ext_dist_enable extended_10bt_distance;
    em_rev_polarity cable_polarity;
    em_polarity_reversal polarity_correction;
    em_auto_x_mode mdix_mode;
    em_1000t_rx_status local_rx;
    em_1000t_rx_status remote_rx;
};

struct em_phy_stats {
    uint32_t idle_errors;
    uint32_t receive_errors;
};



/* Error Codes */
#define E1000_SUCCESS    0
#define E1000_ERR_EEPROM 1
#define E1000_ERR_PHY    2
#define E1000_ERR_CONFIG 3
#define E1000_ERR_PARAM  4

/* Function prototypes */
/* Initialization */
void em_reset_hw(struct em_hw *hw);
int32_t em_init_hw(struct em_hw *hw);

/* Link Configuration */
int32_t em_setup_link(struct em_hw *hw);
int32_t em_phy_setup_autoneg(struct em_hw *hw);
void em_config_collision_dist(struct em_hw *hw);
int32_t em_config_fc_after_link_up(struct em_hw *hw);
int32_t em_check_for_link(struct em_hw *hw);
void em_get_speed_and_duplex(struct em_hw *hw, uint16_t * speed, uint16_t * duplex);
int32_t em_wait_autoneg(struct em_hw *hw);

/* PHY */
int32_t em_read_phy_reg(struct em_hw *hw, uint32_t reg_addr, uint16_t *phy_data);
int32_t em_write_phy_reg(struct em_hw *hw, uint32_t reg_addr, uint16_t data);
void em_phy_hw_reset(struct em_hw *hw);
int32_t em_phy_reset(struct em_hw *hw);
int32_t em_detect_gig_phy(struct em_hw *hw);
int32_t em_phy_get_info(struct em_hw *hw, struct em_phy_info *phy_info);
int32_t em_validate_mdi_setting(struct em_hw *hw);

/* EEPROM Functions */
int32_t em_read_eeprom(struct em_hw *hw, uint16_t reg, uint16_t *data);
int32_t em_validate_eeprom_checksum(struct em_hw *hw);
int32_t em_read_part_num(struct em_hw *hw, uint32_t * part_num);
int32_t em_read_mac_addr(struct em_hw * hw);

/* Filters (multicast, vlan, receive) */
void em_init_rx_addrs(struct em_hw *hw);
void em_mc_addr_list_update(struct em_hw *hw, uint8_t * mc_addr_list, uint32_t mc_addr_count, uint32_t pad);
uint32_t em_hash_mc_addr(struct em_hw *hw, uint8_t * mc_addr);
void em_mta_set(struct em_hw *hw, uint32_t hash_value);
void em_rar_set(struct em_hw *hw, uint8_t * mc_addr, uint32_t rar_index);
void em_write_vfta(struct em_hw *hw, uint32_t offset, uint32_t value);
void em_clear_vfta(struct em_hw *hw);

/* LED functions */
int32_t em_setup_led(struct em_hw *hw);
int32_t em_cleanup_led(struct em_hw *hw);
int32_t em_led_on(struct em_hw *hw);
int32_t em_led_off(struct em_hw *hw);

/* Adaptive IFS Functions */

/* Everything else */
void em_clear_hw_cntrs(struct em_hw *hw);
void em_reset_adaptive(struct em_hw *hw);
void em_update_adaptive(struct em_hw *hw);
void em_tbi_adjust_stats(struct em_hw *hw, struct em_hw_stats *stats, uint32_t frame_len, uint8_t * mac_addr);
void em_get_bus_info(struct em_hw *hw);
//void em_write_pci_cfg(struct em_hw *hw, uint32_t reg, uint16_t * value);


/* Macros to handle ID values passed in via -D on command-line.
 * The ID values must be hex numbers without the leading "0x".
 */

#define __glue(s1,s2)	s1 ## s2
#define ID2HEX(s)	__glue(0x0,s)
#define __2str(s)	# s
#define ID2STR(s)	__2str(s)

#ifndef VENDOR_ID
#define VENDOR_ID	8086
#endif

/* this must not match any legal ID below */
#define E1000_DEV_ID_ANY	0x0000


#ifdef DEVICE_ID
    #define EM_DEVICE_ID	ID2HEX(DEVICE_ID)
#endif

/* This should be defined on the command-line to select code support
 * for just a single version or type of a part.  Otherwise it is set
 * to an unsupported ID to turn on code support for every chip.
 */
#ifndef EM_DEVICE_ID
#define EM_DEVICE_ID	E1000_DEV_ID_ANY
#endif

#ifndef MANUFACTURER
    #define MANUFACTURER	"CodeGen"
#endif

#ifndef BOARD_TYPE
    #define BOARD_TYPE		"DEBUG"
#endif

/* this must be used to wrap all device-specific code */
#define CHECK_DEV_ID(expect)	\
	(EM_DEVICE_ID == E1000_DEV_ID_ANY || EM_DEVICE_ID == (expect))

/* PCI Device IDs */
#define E1000_DEV_ID_82542          0x1000
#define E1000_DEV_ID_82543GC_FIBER  0x1001
#define E1000_DEV_ID_82543GC_COPPER 0x1004
#define E1000_DEV_ID_82544EI_COPPER 0x1008
#define E1000_DEV_ID_82544EI_FIBER  0x1009
#define E1000_DEV_ID_82544GC_COPPER 0x100C
#define E1000_DEV_ID_82544GC_LOM    0x100D
#define E1000_DEV_ID_82540EM        0x100E
#define E1000_DEV_ID_82540EM_LOM    0x1015
#define E1000_DEV_ID_82545EM_COPPER 0x100F
#define E1000_DEV_ID_82545EM_FIBER  0x1011
#define E1000_DEV_ID_82546EB_COPPER 0x1010
#define E1000_DEV_ID_82546EB_FIBER  0x1012
#define NUM_DEV_IDS 13

#define NODE_ADDRESS_SIZE 6
#define ETH_LENGTH_OF_ADDRESS 6
#define MAC_ADDR	6

/* MAC decode size is 128K - This is the size of BAR0 */
#define MAC_DECODE_SIZE (128 * 1024)

#define E1000_82542_2_0_REV_ID 2
#define E1000_82542_2_1_REV_ID 3

#define SPEED_10    10
#define SPEED_100   100
#define SPEED_1000  1000
#define HALF_DUPLEX 1
#define FULL_DUPLEX 2

/* The sizes (in bytes) of a ethernet packet */
#define ENET_HEADER_SIZE             14
#define MAXIMUM_ETHERNET_FRAME_SIZE  1518 /* With FCS */
#define MINIMUM_ETHERNET_FRAME_SIZE  64   /* With FCS */
#define ETHERNET_FCS_SIZE            4
#define MAXIMUM_ETHERNET_PACKET_SIZE \
    (MAXIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)
#define MINIMUM_ETHERNET_PACKET_SIZE \
    (MINIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)
#define CRC_LENGTH                   ETHERNET_FCS_SIZE
#define MAX_JUMBO_FRAME_SIZE         0x3F00


/* 802.1q VLAN Packet Sizes */
#define VLAN_TAG_SIZE                     4     /* 802.3ac tag (not DMAed) */

/* Ethertype field values */
#define ETHERNET_IEEE_VLAN_TYPE 0x8100  /* 802.3ac packet */
#define ETHERNET_IP_TYPE        0x0800  /* IP packets */
#define ETHERNET_ARP_TYPE       0x0806  /* Address Resolution Protocol (ARP) */

/* Packet Header defines */
#define IP_PROTOCOL_TCP    6
#define IP_PROTOCOL_UDP    0x11

/* This defines the bits that are set in the Interrupt Mask
 * Set/Read Register.  Each bit is documented below:
 *   o RXDMT0 = Receive Descriptor Minimum Threshold hit (ring 0)
 *   o RXSEQ  = Receive Sequence Error 
 */
#define POLL_IMS_ENABLE_MASK ( \
    E1000_IMS_RXDMT0 |         \
    E1000_IMS_RXSEQ)

/* This defines the bits that are set in the Interrupt Mask
 * Set/Read Register.  Each bit is documented below:
 *   o RXT0   = Receiver Timer Interrupt (ring 0)
 *   o TXDW   = Transmit Descriptor Written Back
 *   o RXDMT0 = Receive Descriptor Minimum Threshold hit (ring 0)
 *   o RXSEQ  = Receive Sequence Error
 *   o LSC    = Link Status Change
 */
#define IMS_ENABLE_MASK ( \
    E1000_IMS_RXT0   |    \
    E1000_IMS_TXDW   |    \
    E1000_IMS_RXDMT0 |    \
    E1000_IMS_RXSEQ  |    \
    E1000_IMS_LSC)

/* The number of high/low register pairs in the RAR. The RAR (Receive Address
 * Registers) holds the directed and multicast addresses that we monitor. We
 * reserve one of these spots for our directed address, allowing us room for
 * E1000_RAR_ENTRIES - 1 multicast addresses. 
 */
#define E1000_RAR_ENTRIES 16

#define MIN_NUMBER_OF_DESCRIPTORS 8
#define MAX_NUMBER_OF_DESCRIPTORS 0xFFF8

/* Receive Descriptor */
struct em_rx_desc {
    volatile uint64_t buffer_addr; /* Address of the descriptor's data buffer */
    volatile uint16_t length;     /* Length of data DMAed into data buffer */
    volatile uint16_t csum;       /* Packet checksum */
    volatile uint8_t status;      /* Descriptor status */
    volatile uint8_t errors;      /* Descriptor Errors */
    volatile uint16_t special;
};

/* Receive Decriptor bit definitions */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */
#define E1000_RXD_STAT_IXSM     0x04    /* Ignore checksum */
#define E1000_RXD_STAT_VP       0x08    /* IEEE VLAN Packet */
#define E1000_RXD_STAT_TCPCS    0x20    /* TCP xsum calculated */
#define E1000_RXD_STAT_IPCS     0x40    /* IP xsum calculated */
#define E1000_RXD_STAT_PIF      0x80    /* passed in-exact filter */
#define E1000_RXD_ERR_CE        0x01    /* CRC Error */
#define E1000_RXD_ERR_SE        0x02    /* Symbol Error */
#define E1000_RXD_ERR_SEQ       0x04    /* Sequence Error */
#define E1000_RXD_ERR_CXE       0x10    /* Carrier Extension Error */
#define E1000_RXD_ERR_TCPE      0x20    /* TCP/UDP Checksum Error */
#define E1000_RXD_ERR_IPE       0x40    /* IP Checksum Error */
#define E1000_RXD_ERR_RXE       0x80    /* Rx Data Error */
#define E1000_RXD_SPC_VLAN_MASK 0x0FFF  /* VLAN ID is in lower 12 bits */
#define E1000_RXD_SPC_PRI_MASK  0xE000  /* Priority is in upper 3 bits */
#define E1000_RXD_SPC_PRI_SHIFT 0x000D  /* Priority is in upper 3 of 16 */
#define E1000_RXD_SPC_CFI_MASK  0x1000  /* CFI is bit 12 */
#define E1000_RXD_SPC_CFI_SHIFT 0x000C  /* CFI is bit 12 */

/* mask to determine if packets should be dropped due to frame errors */
#define E1000_RXD_ERR_FRAME_ERR_MASK ( \
    E1000_RXD_ERR_CE  |                \
    E1000_RXD_ERR_SE  |                \
    E1000_RXD_ERR_SEQ |                \
    E1000_RXD_ERR_CXE |                \
    E1000_RXD_ERR_RXE)

/* Transmit Descriptor */
struct em_tx_desc {
    volatile uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
    union u1 {
        volatile uint32_t data;
        struct {
            volatile uint16_t length;    /* Data buffer length */
            volatile uint8_t cso;        /* Checksum offset */
            volatile uint8_t cmd;        /* Descriptor control */
        } flags;
    } lower;
    union u2 {
        volatile uint32_t data;
        struct {
            volatile uint8_t status;     /* Descriptor status */
            volatile uint8_t css;        /* Checksum start */
            volatile uint16_t special;
        } fields;
    } upper;
};

/* Transmit Descriptor bit definitions */
#define E1000_TXD_DTYP_D     0x00100000 /* Data Descriptor */
#define E1000_TXD_DTYP_C     0x00000000 /* Context Descriptor */
#define E1000_TXD_POPTS_IXSM 0x01       /* Insert IP checksum */
#define E1000_TXD_POPTS_TXSM 0x02       /* Insert TCP/UDP checksum */
#define E1000_TXD_CMD_EOP    0x01000000 /* End of Packet */
#define E1000_TXD_CMD_IFCS   0x02000000 /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_IC     0x04000000 /* Insert Checksum */
#define E1000_TXD_CMD_RS     0x08000000 /* Report Status */
#define E1000_TXD_CMD_RPS    0x10000000 /* Report Packet Sent */
#define E1000_TXD_CMD_DEXT   0x20000000 /* Descriptor extension (0 = legacy) */
#define E1000_TXD_CMD_VLE    0x40000000 /* Add VLAN tag */
#define E1000_TXD_CMD_IDE    0x80000000 /* Enable Tidv register */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_STAT_EC    0x00000002 /* Excess Collisions */
#define E1000_TXD_STAT_LC    0x00000004 /* Late Collisions */
#define E1000_TXD_STAT_TU    0x00000008 /* Transmit underrun */
#define E1000_TXD_CMD_TCP    0x01000000 /* TCP packet */
#define E1000_TXD_CMD_IP     0x02000000 /* IP packet */
#define E1000_TXD_CMD_TSE    0x04000000 /* TCP Seg enable */
#define E1000_TXD_STAT_TC    0x00000004 /* Tx Underrun */

/* Offload Context Descriptor */
struct em_context_desc {
    union {
        uint32_t ip_config;
        struct {
            uint8_t ipcss;      /* IP checksum start */
            uint8_t ipcso;      /* IP checksum offset */
            uint16_t ipcse;     /* IP checksum end */
        } ip_fields;
    } lower_setup;
    union {
        uint32_t tcp_config;
        struct {
            uint8_t tucss;      /* TCP checksum start */
            uint8_t tucso;      /* TCP checksum offset */
            uint16_t tucse;     /* TCP checksum end */
        } tcp_fields;
    } upper_setup;
    uint32_t cmd_and_length;    /* */
    union {
        uint32_t data;
        struct {
            uint8_t status;     /* Descriptor status */
            uint8_t hdr_len;    /* Header length */
            uint16_t mss;       /* Maximum segment size */
        } fields;
    } tcp_seg_setup;
};

/* Offload data descriptor */
struct em_data_desc {
    uint64_t buffer_addr;       /* Address of the descriptor's buffer address */
    union {
        uint32_t data;
        struct {
            uint16_t length;    /* Data buffer length */
            uint8_t typ_len_ext;        /* */
            uint8_t cmd;        /* */
        } flags;
    } lower;
    union {
        uint32_t data;
        struct {
            uint8_t status;     /* Descriptor status */
            uint8_t popts;      /* Packet Options */
            uint16_t special;   /* */
        } fields;
    } upper;
};

/* Filters */
#define E1000_NUM_UNICAST          16   /* Unicast filter entries */
#define E1000_MC_TBL_SIZE          128  /* Multicast Filter Table (4096 bits) */
#define E1000_VLAN_FILTER_TBL_SIZE 128  /* VLAN Filter Table (4096 bits) */


/* Receive Address Register */
struct em_rar {
    volatile uint32_t low;      /* receive address low */
    volatile uint32_t high;     /* receive address high */
};

/* The number of entries in the Multicast Table Array (MTA). */
#define E1000_NUM_MTA_REGISTERS 128

/* IPv4 Address Table Entry */
struct em_ipv4_at_entry {
    volatile uint32_t ipv4_addr;        /* IP Address (RW) */
    volatile uint32_t reserved;
};

/* Four wakeup IP addresses are supported */
#define E1000_WAKEUP_IP_ADDRESS_COUNT_MAX 4
#define E1000_IP4AT_SIZE                  E1000_WAKEUP_IP_ADDRESS_COUNT_MAX
#define E1000_IP6AT_SIZE                  1

/* IPv6 Address Table Entry */
struct em_ipv6_at_entry {
    volatile uint8_t ipv6_addr[16];
};

/* Flexible Filter Length Table Entry */
struct em_fflt_entry {
    volatile uint32_t length;   /* Flexible Filter Length (RW) */
    volatile uint32_t reserved;
};

/* Flexible Filter Mask Table Entry */
struct em_ffmt_entry {
    volatile uint32_t mask;     /* Flexible Filter Mask (RW) */
    volatile uint32_t reserved;
};

/* Flexible Filter Value Table Entry */
struct em_ffvt_entry {
    volatile uint32_t value;    /* Flexible Filter Value (RW) */
    volatile uint32_t reserved;
};

/* Four Flexible Filters are supported */
#define E1000_FLEXIBLE_FILTER_COUNT_MAX 4

/* Each Flexible Filter is at most 128 (0x80) bytes in length */
#define E1000_FLEXIBLE_FILTER_SIZE_MAX  128

#define E1000_FFLT_SIZE E1000_FLEXIBLE_FILTER_COUNT_MAX
#define E1000_FFMT_SIZE E1000_FLEXIBLE_FILTER_SIZE_MAX
#define E1000_FFVT_SIZE E1000_FLEXIBLE_FILTER_SIZE_MAX

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the 
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define E1000_CTRL     0x00000  /* Device Control - RW */
#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_EECD     0x00010  /* EEPROM/Flash Control - RW */
#define E1000_EERD     0x00014  /* EEPROM Read - RW */
#define E1000_CTRL_EXT 0x00018  /* Extended Device Control - RW */
#define E1000_MDIC     0x00020  /* MDI Control - RW */
#define E1000_FCAL     0x00028  /* Flow Control Address Low - RW */
#define E1000_FCAH     0x0002C  /* Flow Control Address High -RW */
#define E1000_FCT      0x00030  /* Flow Control Type - RW */
#define E1000_VET      0x00038  /* VLAN Ether Type - RW */
#define E1000_ICR      0x000C0  /* Interrupt Cause Read - R/clr */
#define E1000_ITR      0x000C4  /* Interrupt Throttling Rate - RW */
#define E1000_ICS      0x000C8  /* Interrupt Cause Set - WO */
#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define E1000_IMC      0x000D8  /* Interrupt Mask Clear - WO */
#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_FCTTV    0x00170  /* Flow Control Transmit Timer Value - RW */
#define E1000_TXCW     0x00178  /* TX Configuration Word - RW */
#define E1000_RXCW     0x00180  /* RX Configuration Word - RO */
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_TBT      0x00448  /* TX Burst Timer - RW */
#define E1000_AIT      0x00458  /* Adaptive Interframe Spacing Throttle - RW */
#define E1000_LEDCTL   0x00E00  /* LED Control - RW */
#define E1000_PBA      0x01000  /* Packet Buffer Allocation - RW */
#define E1000_FCRTL    0x02160  /* Flow Control Receive Threshold Low - RW */
#define E1000_FCRTH    0x02168  /* Flow Control Receive Threshold High - RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RDTR     0x02820  /* RX Delay Timer - RW */
#define E1000_RXDCTL   0x02828  /* RX Descriptor Control - RW */
#define E1000_RADV     0x0282C  /* RX Interrupt Absolute Delay Timer - RW */
#define E1000_RSRPD    0x02C00  /* RX Small Packet Detect - RW */
#define E1000_TXDMAC   0x03000  /* TX DMA Control - RW */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TIDV     0x03820  /* TX Interrupt Delay Value - RW */
#define E1000_TXDCTL   0x03828  /* TX Descriptor Control - RW */
#define E1000_TADV     0x0382C  /* TX Interrupt Absolute Delay Val - RW */
#define E1000_TSPMT    0x03830  /* TCP Segmentation PAD & Min Threshold - RW */
#define E1000_CRCERRS  0x04000  /* CRC Error Count - R/clr */
#define E1000_ALGNERRC 0x04004  /* Alignment Error Count - R/clr */
#define E1000_SYMERRS  0x04008  /* Symbol Error Count - R/clr */
#define E1000_RXERRC   0x0400C  /* Receive Error Count - R/clr */
#define E1000_MPC      0x04010  /* Missed Packet Count - R/clr */
#define E1000_SCC      0x04014  /* Single Collision Count - R/clr */
#define E1000_ECOL     0x04018  /* Excessive Collision Count - R/clr */
#define E1000_MCC      0x0401C  /* Multiple Collision Count - R/clr */
#define E1000_LATECOL  0x04020  /* Late Collision Count - R/clr */
#define E1000_COLC     0x04028  /* Collision Count - R/clr */
#define E1000_DC       0x04030  /* Defer Count - R/clr */
#define E1000_TNCRS    0x04034  /* TX-No CRS - R/clr */
#define E1000_SEC      0x04038  /* Sequence Error Count - R/clr */
#define E1000_CEXTERR  0x0403C  /* Carrier Extension Error Count - R/clr */
#define E1000_RLEC     0x04040  /* Receive Length Error Count - R/clr */
#define E1000_XONRXC   0x04048  /* XON RX Count - R/clr */
#define E1000_XONTXC   0x0404C  /* XON TX Count - R/clr */
#define E1000_XOFFRXC  0x04050  /* XOFF RX Count - R/clr */
#define E1000_XOFFTXC  0x04054  /* XOFF TX Count - R/clr */
#define E1000_FCRUC    0x04058  /* Flow Control RX Unsupported Count- R/clr */
#define E1000_PRC64    0x0405C  /* Packets RX (64 bytes) - R/clr */
#define E1000_PRC127   0x04060  /* Packets RX (65-127 bytes) - R/clr */
#define E1000_PRC255   0x04064  /* Packets RX (128-255 bytes) - R/clr */
#define E1000_PRC511   0x04068  /* Packets RX (255-511 bytes) - R/clr */
#define E1000_PRC1023  0x0406C  /* Packets RX (512-1023 bytes) - R/clr */
#define E1000_PRC1522  0x04070  /* Packets RX (1024-1522 bytes) - R/clr */
#define E1000_GPRC     0x04074  /* Good Packets RX Count - R/clr */
#define E1000_BPRC     0x04078  /* Broadcast Packets RX Count - R/clr */
#define E1000_MPRC     0x0407C  /* Multicast Packets RX Count - R/clr */
#define E1000_GPTC     0x04080  /* Good Packets TX Count - R/clr */
#define E1000_GORCL    0x04088  /* Good Octets RX Count Low - R/clr */
#define E1000_GORCH    0x0408C  /* Good Octets RX Count High - R/clr */
#define E1000_GOTCL    0x04090  /* Good Octets TX Count Low - R/clr */
#define E1000_GOTCH    0x04094  /* Good Octets TX Count High - R/clr */
#define E1000_RNBC     0x040A0  /* RX No Buffers Count - R/clr */
#define E1000_RUC      0x040A4  /* RX Undersize Count - R/clr */
#define E1000_RFC      0x040A8  /* RX Fragment Count - R/clr */
#define E1000_ROC      0x040AC  /* RX Oversize Count - R/clr */
#define E1000_RJC      0x040B0  /* RX Jabber Count - R/clr */
#define E1000_MGTPRC   0x040B4  /* Management Packets RX Count - R/clr */
#define E1000_MGTPDC   0x040B8  /* Management Packets Dropped Count - R/clr */
#define E1000_MGTPTC   0x040BC  /* Management Packets TX Count - R/clr */
#define E1000_TORL     0x040C0  /* Total Octets RX Low - R/clr */
#define E1000_TORH     0x040C4  /* Total Octets RX High - R/clr */
#define E1000_TOTL     0x040C8  /* Total Octets TX Low - R/clr */
#define E1000_TOTH     0x040CC  /* Total Octets TX High - R/clr */
#define E1000_TPR      0x040D0  /* Total Packets RX - R/clr */
#define E1000_TPT      0x040D4  /* Total Packets TX - R/clr */
#define E1000_PTC64    0x040D8  /* Packets TX (64 bytes) - R/clr */
#define E1000_PTC127   0x040DC  /* Packets TX (65-127 bytes) - R/clr */
#define E1000_PTC255   0x040E0  /* Packets TX (128-255 bytes) - R/clr */
#define E1000_PTC511   0x040E4  /* Packets TX (256-511 bytes) - R/clr */
#define E1000_PTC1023  0x040E8  /* Packets TX (512-1023 bytes) - R/clr */
#define E1000_PTC1522  0x040EC  /* Packets TX (1024-1522 Bytes) - R/clr */
#define E1000_MPTC     0x040F0  /* Multicast Packets TX Count - R/clr */
#define E1000_BPTC     0x040F4  /* Broadcast Packets TX Count - R/clr */
#define E1000_TSCTC    0x040F8  /* TCP Segmentation Context TX - R/clr */
#define E1000_TSCTFC   0x040FC  /* TCP Segmentation Context TX Fail - R/clr */
#define E1000_RXCSUM   0x05000  /* RX Checksum Control - RW */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_VFTA     0x05600  /* VLAN Filter Table Array - RW Array */
#define E1000_WUC      0x05800  /* Wakeup Control - RW */
#define E1000_WUFC     0x05808  /* Wakeup Filter Control - RW */
#define E1000_WUS      0x05810  /* Wakeup Status - RO */
#define E1000_MANC     0x05820  /* Management Control - RW */
#define E1000_IPAV     0x05838  /* IP Address Valid - RW */
#define E1000_IP4AT    0x05840  /* IPv4 Address Table - RW Array */
#define E1000_IP6AT    0x05880  /* IPv6 Address Table - RW Array */
#define E1000_WUPL     0x05900  /* Wakeup Packet Length - RW */
#define E1000_WUPM     0x05A00  /* Wakeup Packet Memory - RO A */
#define E1000_FFLT     0x05F00  /* Flexible Filter Length Table - RW Array */
#define E1000_FFMT     0x09000  /* Flexible Filter Mask Table - RW Array */
#define E1000_FFVT     0x09800  /* Flexible Filter Value Table - RW Array */

/* Register Set (82542)
 *
 * Some of the 82542 registers are located at different offsets than they are
 * in more current versions of the 8254x. Despite the difference in location,
 * the registers function in the same manner.
 */
#define E1000_82542_CTRL     E1000_CTRL
#define E1000_82542_STATUS   E1000_STATUS
#define E1000_82542_EECD     E1000_EECD
#define E1000_82542_EERD     E1000_EERD
#define E1000_82542_CTRL_EXT E1000_CTRL_EXT
#define E1000_82542_MDIC     E1000_MDIC
#define E1000_82542_FCAL     E1000_FCAL
#define E1000_82542_FCAH     E1000_FCAH
#define E1000_82542_FCT      E1000_FCT
#define E1000_82542_VET      E1000_VET
#define E1000_82542_RA       0x00040
#define E1000_82542_ICR      E1000_ICR
#define E1000_82542_ITR      E1000_ITR
#define E1000_82542_ICS      E1000_ICS
#define E1000_82542_IMS      E1000_IMS
#define E1000_82542_IMC      E1000_IMC
#define E1000_82542_RCTL     E1000_RCTL
#define E1000_82542_RDTR     0x00108
#define E1000_82542_RDBAL    0x00110
#define E1000_82542_RDBAH    0x00114
#define E1000_82542_RDLEN    0x00118
#define E1000_82542_RDH      0x00120
#define E1000_82542_RDT      0x00128
#define E1000_82542_FCRTH    0x00160
#define E1000_82542_FCRTL    0x00168
#define E1000_82542_FCTTV    E1000_FCTTV
#define E1000_82542_TXCW     E1000_TXCW
#define E1000_82542_RXCW     E1000_RXCW
#define E1000_82542_MTA      0x00200
#define E1000_82542_TCTL     E1000_TCTL
#define E1000_82542_TIPG     E1000_TIPG
#define E1000_82542_TDBAL    0x00420
#define E1000_82542_TDBAH    0x00424
#define E1000_82542_TDLEN    0x00428
#define E1000_82542_TDH      0x00430
#define E1000_82542_TDT      0x00438
#define E1000_82542_TIDV     0x00440
#define E1000_82542_TBT      E1000_TBT
#define E1000_82542_AIT      E1000_AIT
#define E1000_82542_VFTA     0x00600
#define E1000_82542_LEDCTL   E1000_LEDCTL
#define E1000_82542_PBA      E1000_PBA
#define E1000_82542_RXDCTL   E1000_RXDCTL
#define E1000_82542_RADV     E1000_RADV
#define E1000_82542_RSRPD    E1000_RSRPD
#define E1000_82542_TXDMAC   E1000_TXDMAC
#define E1000_82542_TXDCTL   E1000_TXDCTL
#define E1000_82542_TADV     E1000_TADV
#define E1000_82542_TSPMT    E1000_TSPMT
#define E1000_82542_CRCERRS  E1000_CRCERRS
#define E1000_82542_ALGNERRC E1000_ALGNERRC
#define E1000_82542_SYMERRS  E1000_SYMERRS
#define E1000_82542_RXERRC   E1000_RXERRC
#define E1000_82542_MPC      E1000_MPC
#define E1000_82542_SCC      E1000_SCC
#define E1000_82542_ECOL     E1000_ECOL
#define E1000_82542_MCC      E1000_MCC
#define E1000_82542_LATECOL  E1000_LATECOL
#define E1000_82542_COLC     E1000_COLC
#define E1000_82542_DC       E1000_DC
#define E1000_82542_TNCRS    E1000_TNCRS
#define E1000_82542_SEC      E1000_SEC
#define E1000_82542_CEXTERR  E1000_CEXTERR
#define E1000_82542_RLEC     E1000_RLEC
#define E1000_82542_XONRXC   E1000_XONRXC
#define E1000_82542_XONTXC   E1000_XONTXC
#define E1000_82542_XOFFRXC  E1000_XOFFRXC
#define E1000_82542_XOFFTXC  E1000_XOFFTXC
#define E1000_82542_FCRUC    E1000_FCRUC
#define E1000_82542_PRC64    E1000_PRC64
#define E1000_82542_PRC127   E1000_PRC127
#define E1000_82542_PRC255   E1000_PRC255
#define E1000_82542_PRC511   E1000_PRC511
#define E1000_82542_PRC1023  E1000_PRC1023
#define E1000_82542_PRC1522  E1000_PRC1522
#define E1000_82542_GPRC     E1000_GPRC
#define E1000_82542_BPRC     E1000_BPRC
#define E1000_82542_MPRC     E1000_MPRC
#define E1000_82542_GPTC     E1000_GPTC
#define E1000_82542_GORCL    E1000_GORCL
#define E1000_82542_GORCH    E1000_GORCH
#define E1000_82542_GOTCL    E1000_GOTCL
#define E1000_82542_GOTCH    E1000_GOTCH
#define E1000_82542_RNBC     E1000_RNBC
#define E1000_82542_RUC      E1000_RUC
#define E1000_82542_RFC      E1000_RFC
#define E1000_82542_ROC      E1000_ROC
#define E1000_82542_RJC      E1000_RJC
#define E1000_82542_MGTPRC   E1000_MGTPRC
#define E1000_82542_MGTPDC   E1000_MGTPDC
#define E1000_82542_MGTPTC   E1000_MGTPTC
#define E1000_82542_TORL     E1000_TORL
#define E1000_82542_TORH     E1000_TORH
#define E1000_82542_TOTL     E1000_TOTL
#define E1000_82542_TOTH     E1000_TOTH
#define E1000_82542_TPR      E1000_TPR
#define E1000_82542_TPT      E1000_TPT
#define E1000_82542_PTC64    E1000_PTC64
#define E1000_82542_PTC127   E1000_PTC127
#define E1000_82542_PTC255   E1000_PTC255
#define E1000_82542_PTC511   E1000_PTC511
#define E1000_82542_PTC1023  E1000_PTC1023
#define E1000_82542_PTC1522  E1000_PTC1522
#define E1000_82542_MPTC     E1000_MPTC
#define E1000_82542_BPTC     E1000_BPTC
#define E1000_82542_TSCTC    E1000_TSCTC
#define E1000_82542_TSCTFC   E1000_TSCTFC
#define E1000_82542_RXCSUM   E1000_RXCSUM
#define E1000_82542_WUC      E1000_WUC
#define E1000_82542_WUFC     E1000_WUFC
#define E1000_82542_WUS      E1000_WUS
#define E1000_82542_MANC     E1000_MANC
#define E1000_82542_IPAV     E1000_IPAV
#define E1000_82542_IP4AT    E1000_IP4AT
#define E1000_82542_IP6AT    E1000_IP6AT
#define E1000_82542_WUPL     E1000_WUPL
#define E1000_82542_WUPM     E1000_WUPM
#define E1000_82542_FFLT     E1000_FFLT
#define E1000_82542_FFMT     E1000_FFMT
#define E1000_82542_FFVT     E1000_FFVT

/* Statistics counters collected by the MAC */
struct em_hw_stats {
    uint32_t crcerrs;
    uint32_t algnerrc;
    uint32_t symerrs;
    uint32_t rxerrc;
    uint32_t mpc;
    uint32_t scc;
    uint32_t ecol;
    uint32_t mcc;
    uint32_t latecol;
    uint32_t colc;
    uint32_t dc;
    uint32_t tncrs;
    uint32_t sec;
    uint32_t cexterr;
    uint32_t rlec;
    uint32_t xonrxc;
    uint32_t xontxc;
    uint32_t xoffrxc;
    uint32_t xofftxc;
    uint32_t fcruc;
    uint32_t prc64;
    uint32_t prc127;
    uint32_t prc255;
    uint32_t prc511;
    uint32_t prc1023;
    uint32_t prc1522;
    uint32_t gprc;
    uint32_t bprc;
    uint32_t mprc;
    uint32_t gptc;
    uint32_t gorcl;
    uint32_t gorch;
    uint32_t gotcl;
    uint32_t gotch;
    uint32_t rnbc;
    uint32_t ruc;
    uint32_t rfc;
    uint32_t roc;
    uint32_t rjc;
    uint32_t mgprc;
    uint32_t mgpdc;
    uint32_t mgptc;
    uint32_t torl;
    uint32_t torh;
    uint32_t totl;
    uint32_t toth;
    uint32_t tpr;
    uint32_t tpt;
    uint32_t ptc64;
    uint32_t ptc127;
    uint32_t ptc255;
    uint32_t ptc511;
    uint32_t ptc1023;
    uint32_t ptc1522;
    uint32_t mptc;
    uint32_t bptc;
    uint32_t tsctc;
    uint32_t tsctfc;
};

/* Structure containing variables used by the shared code (em_hw.c) */
struct em_hw {
    uint8_t *hw_addr;
    em_mac_type mac_type;
    em_media_type media_type;
    volatile uint32_t *regs;
    em_fc_type fc;
    em_bus_speed bus_speed;
    em_bus_width bus_width;
    em_bus_type bus_type;
    uint32_t phy_id;
    uint32_t phy_addr;
    uint32_t original_fc;
    uint32_t txcw;
    uint32_t autoneg_failed;
    uint32_t max_frame_size;
    uint32_t min_frame_size;
    uint32_t mc_filter_type;
    uint32_t num_mc_addrs;
    uint32_t collision_delta;
    uint32_t tx_packet_delta;
    uint32_t ledctl_default;
    uint32_t ledctl_mode1;
    uint32_t ledctl_mode2;
    uint16_t autoneg_advertised;
    uint16_t pci_cmd_word;
    uint16_t fc_high_water;
    uint16_t fc_low_water;
    uint16_t fc_pause_time;
    uint16_t current_ifs_val;
    uint16_t ifs_min_val;
    uint16_t ifs_max_val;
    uint16_t ifs_step_size;
    uint16_t ifs_ratio;
    uint16_t device_id;
    uint16_t vendor_id;
    uint16_t subsystem_id;
    uint16_t subsystem_vendor_id;
    uint8_t revision_id;
    uint8_t autoneg;
    uint8_t mdix;
    uint8_t forced_speed_duplex;
    uint8_t wait_autoneg_complete;
    uint8_t dma_fairness;
    uint8_t mac_addr[NODE_ADDRESS_SIZE];
    uint8_t perm_mac_addr[NODE_ADDRESS_SIZE];
    boolean_t disable_polarity_correction;
    boolean_t get_link_status;
    boolean_t tbi_compatibility_en;
    boolean_t tbi_compatibility_on;
    boolean_t fc_send_xon;
    boolean_t report_tx_early;
    boolean_t adaptive_ifs;
    boolean_t ifs_params_forced;
    boolean_t in_ifs_mode;
    boolean_t promiscuous;
};


#define E1000_EEPROM_SWDPIN0   0x0001   /* SWDPIN 0 EEPROM Value */
#define E1000_EEPROM_LED_LOGIC 0x0020   /* Led Logic Word */

/* Register Bit Masks */
/* Device Control */
#define E1000_CTRL_FD       0x00000001  /* Full duplex.0=half; 1=full */
#define E1000_CTRL_BEM      0x00000002  /* Endian Mode.0=little,1=big */
#define E1000_CTRL_PRIOR    0x00000004  /* Priority on PCI. 0=rx,1=fair */
#define E1000_CTRL_LRST     0x00000008  /* Link reset. 0=normal,1=reset */
#define E1000_CTRL_TME      0x00000010  /* Test mode. 0=normal,1=test */
#define E1000_CTRL_SLE      0x00000020  /* Serial Link on 0=dis,1=en */
#define E1000_CTRL_ASDE     0x00000020  /* Auto-speed detect enable */
#define E1000_CTRL_SLU      0x00000040  /* Set link up (Force Link) */
#define E1000_CTRL_ILOS     0x00000080  /* Invert Loss-Of Signal */
#define E1000_CTRL_SPD_SEL  0x00000300  /* Speed Select Mask */
#define E1000_CTRL_SPD_10   0x00000000  /* Force 10Mb */
#define E1000_CTRL_SPD_100  0x00000100  /* Force 100Mb */
#define E1000_CTRL_SPD_1000 0x00000200  /* Force 1Gb */
#define E1000_CTRL_BEM32    0x00000400  /* Big Endian 32 mode */
#define E1000_CTRL_FRCSPD   0x00000800  /* Force Speed */
#define E1000_CTRL_FRCDPX   0x00001000  /* Force Duplex */
#define E1000_CTRL_SWDPIN0  0x00040000  /* SWDPIN 0 value */
#define E1000_CTRL_SWDPIN1  0x00080000  /* SWDPIN 1 value */
#define E1000_CTRL_SWDPIN2  0x00100000  /* SWDPIN 2 value */
#define E1000_CTRL_SWDPIN3  0x00200000  /* SWDPIN 3 value */
#define E1000_CTRL_SWDPIO0  0x00400000  /* SWDPIN 0 Input or output */
#define E1000_CTRL_SWDPIO1  0x00800000  /* SWDPIN 1 input or output */
#define E1000_CTRL_SWDPIO2  0x01000000  /* SWDPIN 2 input or output */
#define E1000_CTRL_SWDPIO3  0x02000000  /* SWDPIN 3 input or output */
#define E1000_CTRL_RST      0x04000000  /* Global reset */
#define E1000_CTRL_RFCE     0x08000000  /* Receive Flow Control enable */
#define E1000_CTRL_TFCE     0x10000000  /* Transmit flow control enable */
#define E1000_CTRL_RTE      0x20000000  /* Routing tag enable */
#define E1000_CTRL_VME      0x40000000  /* IEEE VLAN mode enable */
#define E1000_CTRL_PHY_RST  0x80000000  /* PHY Reset */

/* Device Status */
#define E1000_STATUS_FD         0x00000001      /* Full duplex.0=half,1=full */
#define E1000_STATUS_LU         0x00000002      /* Link up.0=no,1=link */
#define E1000_STATUS_FUNC_MASK  0x0000000C      /* PCI Function Mask */
#define E1000_STATUS_FUNC_0     0x00000000      /* Function 0 */
#define E1000_STATUS_FUNC_1     0x00000004      /* Function 1 */
#define E1000_STATUS_TXOFF      0x00000010      /* transmission paused */
#define E1000_STATUS_TBIMODE    0x00000020      /* TBI mode */
#define E1000_STATUS_SPEED_MASK 0x000000C0
#define E1000_STATUS_SPEED_10   0x00000000      /* Speed 10Mb/s */
#define E1000_STATUS_SPEED_100  0x00000040      /* Speed 100Mb/s */
#define E1000_STATUS_SPEED_1000 0x00000080      /* Speed 1000Mb/s */
#define E1000_STATUS_ASDV       0x00000300      /* Auto speed detect value */
#define E1000_STATUS_MTXCKOK    0x00000400      /* MTX clock running OK */
#define E1000_STATUS_PCI66      0x00000800      /* In 66Mhz slot */
#define E1000_STATUS_BUS64      0x00001000      /* In 64 bit slot */
#define E1000_STATUS_PCIX_MODE  0x00002000      /* PCI-X mode */
#define E1000_STATUS_PCIX_SPEED 0x0000C000      /* PCI-X bus speed */

/* Constants used to intrepret the masked PCI-X bus speed. */
#define E1000_STATUS_PCIX_SPEED_66  0x00000000 /* PCI-X bus speed  50-66 MHz */
#define E1000_STATUS_PCIX_SPEED_100 0x00004000 /* PCI-X bus speed  66-100 MHz */
#define E1000_STATUS_PCIX_SPEED_133 0x00008000 /* PCI-X bus speed 100-133 MHz */

/* EEPROM/Flash Control */
#define E1000_EECD_SK        0x00000001 /* EEPROM Clock */
#define E1000_EECD_CS        0x00000002 /* EEPROM Chip Select */
#define E1000_EECD_DI        0x00000004 /* EEPROM Data In */
#define E1000_EECD_DO        0x00000008 /* EEPROM Data Out */
#define E1000_EECD_FWE_MASK  0x00000030 
#define E1000_EECD_FWE_DIS   0x00000010 /* Disable FLASH writes */
#define E1000_EECD_FWE_EN    0x00000020 /* Enable FLASH writes */
#define E1000_EECD_FWE_SHIFT 4
#define E1000_EECD_SIZE      0x00000200 /* EEPROM Size (0=64 word 1=256 word) */
#define E1000_EECD_REQ       0x00000040 /* EEPROM Access Request */
#define E1000_EECD_GNT       0x00000080 /* EEPROM Access Grant */
#define E1000_EECD_PRES      0x00000100 /* EEPROM Present */

/* EEPROM Read */
#define E1000_EERD_START      0x00000001 /* Start Read */
#define E1000_EERD_DONE       0x00000010 /* Read Done */
#define E1000_EERD_ADDR_SHIFT 8
#define E1000_EERD_ADDR_MASK  0x0000FF00 /* Read Address */
#define E1000_EERD_DATA_SHIFT 16
#define E1000_EERD_DATA_MASK  0xFFFF0000 /* Read Data */

/* Extended Device Control */
#define E1000_CTRL_EXT_GPI0_EN   0x00000001 /* Maps SDP4 to GPI0 */ 
#define E1000_CTRL_EXT_GPI1_EN   0x00000002 /* Maps SDP5 to GPI1 */
#define E1000_CTRL_EXT_PHYINT_EN E1000_CTRL_EXT_GPI1_EN
#define E1000_CTRL_EXT_GPI2_EN   0x00000004 /* Maps SDP6 to GPI2 */
#define E1000_CTRL_EXT_GPI3_EN   0x00000008 /* Maps SDP7 to GPI3 */
#define E1000_CTRL_EXT_SDP4_DATA 0x00000010 /* Value of SW Defineable Pin 4 */
#define E1000_CTRL_EXT_SDP5_DATA 0x00000020 /* Value of SW Defineable Pin 5 */
#define E1000_CTRL_EXT_PHY_INT   E1000_CTRL_EXT_SDP5_DATA
#define E1000_CTRL_EXT_SDP6_DATA 0x00000040 /* Value of SW Defineable Pin 6 */
#define E1000_CTRL_EXT_SDP7_DATA 0x00000080 /* Value of SW Defineable Pin 7 */
#define E1000_CTRL_EXT_SDP4_DIR  0x00000100 /* Direction of SDP4 0=in 1=out */
#define E1000_CTRL_EXT_SDP5_DIR  0x00000200 /* Direction of SDP5 0=in 1=out */
#define E1000_CTRL_EXT_SDP6_DIR  0x00000400 /* Direction of SDP6 0=in 1=out */
#define E1000_CTRL_EXT_SDP7_DIR  0x00000800 /* Direction of SDP7 0=in 1=out */
#define E1000_CTRL_EXT_ASDCHK    0x00001000 /* Initiate an ASD sequence */
#define E1000_CTRL_EXT_EE_RST    0x00002000 /* Reinitialize from EEPROM */
#define E1000_CTRL_EXT_IPS       0x00004000 /* Invert Power State */
#define E1000_CTRL_EXT_SPD_BYPS  0x00008000 /* Speed Select Bypass */
#define E1000_CTRL_EXT_LINK_MODE_MASK 0x00C00000
#define E1000_CTRL_EXT_LINK_MODE_GMII 0x00000000
#define E1000_CTRL_EXT_LINK_MODE_TBI  0x00C00000
#define E1000_CTRL_EXT_WR_WMARK_MASK  0x03000000
#define E1000_CTRL_EXT_WR_WMARK_256   0x00000000
#define E1000_CTRL_EXT_WR_WMARK_320   0x01000000
#define E1000_CTRL_EXT_WR_WMARK_384   0x02000000
#define E1000_CTRL_EXT_WR_WMARK_448   0x03000000

/* MDI Control */
#define E1000_MDIC_DATA_MASK 0x0000FFFF
#define E1000_MDIC_REG_MASK  0x001F0000
#define E1000_MDIC_REG_SHIFT 16
#define E1000_MDIC_PHY_MASK  0x03E00000
#define E1000_MDIC_PHY_SHIFT 21
#define E1000_MDIC_OP_WRITE  0x04000000
#define E1000_MDIC_OP_READ   0x08000000
#define E1000_MDIC_READY     0x10000000
#define E1000_MDIC_INT_EN    0x20000000
#define E1000_MDIC_ERROR     0x40000000

/* LED Control */
#define E1000_LEDCTL_LED0_MODE_MASK  0x0000000F
#define E1000_LEDCTL_LED0_MODE_SHIFT 0
#define E1000_LEDCTL_LED0_IVRT       0x00000040
#define E1000_LEDCTL_LED0_BLINK      0x00000080
#define E1000_LEDCTL_LED1_MODE_MASK  0x00000F00
#define E1000_LEDCTL_LED1_MODE_SHIFT 8
#define E1000_LEDCTL_LED1_IVRT       0x00004000
#define E1000_LEDCTL_LED1_BLINK      0x00008000
#define E1000_LEDCTL_LED2_MODE_MASK  0x000F0000
#define E1000_LEDCTL_LED2_MODE_SHIFT 16
#define E1000_LEDCTL_LED2_IVRT       0x00400000
#define E1000_LEDCTL_LED2_BLINK      0x00800000
#define E1000_LEDCTL_LED3_MODE_MASK  0x0F000000
#define E1000_LEDCTL_LED3_MODE_SHIFT 24
#define E1000_LEDCTL_LED3_IVRT       0x40000000
#define E1000_LEDCTL_LED3_BLINK      0x80000000

#define E1000_LEDCTL_MODE_LINK_10_1000  0x0
#define E1000_LEDCTL_MODE_LINK_100_1000 0x1
#define E1000_LEDCTL_MODE_LINK_UP       0x2
#define E1000_LEDCTL_MODE_ACTIVITY      0x3
#define E1000_LEDCTL_MODE_LINK_ACTIVITY 0x4
#define E1000_LEDCTL_MODE_LINK_10       0x5
#define E1000_LEDCTL_MODE_LINK_100      0x6
#define E1000_LEDCTL_MODE_LINK_1000     0x7
#define E1000_LEDCTL_MODE_PCIX_MODE     0x8
#define E1000_LEDCTL_MODE_FULL_DUPLEX   0x9
#define E1000_LEDCTL_MODE_COLLISION     0xA
#define E1000_LEDCTL_MODE_BUS_SPEED     0xB
#define E1000_LEDCTL_MODE_BUS_SIZE      0xC
#define E1000_LEDCTL_MODE_PAUSED        0xD
#define E1000_LEDCTL_MODE_LED_ON        0xE
#define E1000_LEDCTL_MODE_LED_OFF       0xF

/* Receive Address */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

/* Interrupt Cause Read */
#define E1000_ICR_TXDW    0x00000001    /* Transmit desc written back */
#define E1000_ICR_TXQE    0x00000002    /* Transmit Queue empty */
#define E1000_ICR_LSC     0x00000004    /* Link Status Change */
#define E1000_ICR_RXSEQ   0x00000008    /* rx sequence error */
#define E1000_ICR_RXDMT0  0x00000010    /* rx desc min. threshold (0) */
#define E1000_ICR_RXO     0x00000040    /* rx overrun */
#define E1000_ICR_RXT0    0x00000080    /* rx timer intr (ring 0) */
#define E1000_ICR_MDAC    0x00000200    /* MDIO access complete */
#define E1000_ICR_RXCFG   0x00000400    /* RX /c/ ordered set */
#define E1000_ICR_GPI_EN0 0x00000800    /* GP Int 0 */
#define E1000_ICR_GPI_EN1 0x00001000    /* GP Int 1 */
#define E1000_ICR_GPI_EN2 0x00002000    /* GP Int 2 */
#define E1000_ICR_GPI_EN3 0x00004000    /* GP Int 3 */
#define E1000_ICR_TXD_LOW 0x00008000
#define E1000_ICR_SRPD    0x00010000

/* Interrupt Cause Set */
#define E1000_ICS_TXDW    E1000_ICR_TXDW        /* Transmit desc written back */
#define E1000_ICS_TXQE    E1000_ICR_TXQE        /* Transmit Queue empty */
#define E1000_ICS_LSC     E1000_ICR_LSC         /* Link Status Change */
#define E1000_ICS_RXSEQ   E1000_ICR_RXSEQ       /* rx sequence error */
#define E1000_ICS_RXDMT0  E1000_ICR_RXDMT0      /* rx desc min. threshold */
#define E1000_ICS_RXO     E1000_ICR_RXO         /* rx overrun */
#define E1000_ICS_RXT0    E1000_ICR_RXT0        /* rx timer intr */
#define E1000_ICS_MDAC    E1000_ICR_MDAC        /* MDIO access complete */
#define E1000_ICS_RXCFG   E1000_ICR_RXCFG       /* RX /c/ ordered set */
#define E1000_ICS_GPI_EN0 E1000_ICR_GPI_EN0     /* GP Int 0 */
#define E1000_ICS_GPI_EN1 E1000_ICR_GPI_EN1     /* GP Int 1 */
#define E1000_ICS_GPI_EN2 E1000_ICR_GPI_EN2     /* GP Int 2 */
#define E1000_ICS_GPI_EN3 E1000_ICR_GPI_EN3     /* GP Int 3 */
#define E1000_ICS_TXD_LOW E1000_ICR_TXD_LOW
#define E1000_ICS_SRPD    E1000_ICR_SRPD

/* Interrupt Mask Set */
#define E1000_IMS_TXDW    E1000_ICR_TXDW        /* Transmit desc written back */
#define E1000_IMS_TXQE    E1000_ICR_TXQE        /* Transmit Queue empty */
#define E1000_IMS_LSC     E1000_ICR_LSC         /* Link Status Change */
#define E1000_IMS_RXSEQ   E1000_ICR_RXSEQ       /* rx sequence error */
#define E1000_IMS_RXDMT0  E1000_ICR_RXDMT0      /* rx desc min. threshold */
#define E1000_IMS_RXO     E1000_ICR_RXO         /* rx overrun */
#define E1000_IMS_RXT0    E1000_ICR_RXT0        /* rx timer intr */
#define E1000_IMS_MDAC    E1000_ICR_MDAC        /* MDIO access complete */
#define E1000_IMS_RXCFG   E1000_ICR_RXCFG       /* RX /c/ ordered set */
#define E1000_IMS_GPI_EN0 E1000_ICR_GPI_EN0     /* GP Int 0 */
#define E1000_IMS_GPI_EN1 E1000_ICR_GPI_EN1     /* GP Int 1 */
#define E1000_IMS_GPI_EN2 E1000_ICR_GPI_EN2     /* GP Int 2 */
#define E1000_IMS_GPI_EN3 E1000_ICR_GPI_EN3     /* GP Int 3 */
#define E1000_IMS_TXD_LOW E1000_ICR_TXD_LOW
#define E1000_IMS_SRPD    E1000_ICR_SRPD

/* Interrupt Mask Clear */
#define E1000_IMC_TXDW    E1000_ICR_TXDW        /* Transmit desc written back */
#define E1000_IMC_TXQE    E1000_ICR_TXQE        /* Transmit Queue empty */
#define E1000_IMC_LSC     E1000_ICR_LSC         /* Link Status Change */
#define E1000_IMC_RXSEQ   E1000_ICR_RXSEQ       /* rx sequence error */
#define E1000_IMC_RXDMT0  E1000_ICR_RXDMT0      /* rx desc min. threshold */
#define E1000_IMC_RXO     E1000_ICR_RXO         /* rx overrun */
#define E1000_IMC_RXT0    E1000_ICR_RXT0        /* rx timer intr */
#define E1000_IMC_MDAC    E1000_ICR_MDAC        /* MDIO access complete */
#define E1000_IMC_RXCFG   E1000_ICR_RXCFG       /* RX /c/ ordered set */
#define E1000_IMC_GPI_EN0 E1000_ICR_GPI_EN0     /* GP Int 0 */
#define E1000_IMC_GPI_EN1 E1000_ICR_GPI_EN1     /* GP Int 1 */
#define E1000_IMC_GPI_EN2 E1000_ICR_GPI_EN2     /* GP Int 2 */
#define E1000_IMC_GPI_EN3 E1000_ICR_GPI_EN3     /* GP Int 3 */
#define E1000_IMC_TXD_LOW E1000_ICR_TXD_LOW
#define E1000_IMC_SRPD    E1000_ICR_SRPD

/* Receive Control */
#define E1000_RCTL_RST          0x00000001      /* Software reset */
#define E1000_RCTL_EN           0x00000002      /* enable */
#define E1000_RCTL_SBP          0x00000004      /* store bad packet */
#define E1000_RCTL_UPE          0x00000008      /* unicast promiscuous enable */
#define E1000_RCTL_MPE          0x00000010      /* multicast promiscuous enab */
#define E1000_RCTL_LPE          0x00000020      /* long packet enable */
#define E1000_RCTL_LBM_NO       0x00000000      /* no loopback mode */
#define E1000_RCTL_LBM_MAC      0x00000040      /* MAC loopback mode */
#define E1000_RCTL_LBM_SLP      0x00000080      /* serial link loopback mode */
#define E1000_RCTL_LBM_TCVR     0x000000C0      /* tcvr loopback mode */
#define E1000_RCTL_RDMTS_HALF   0x00000000      /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_QUAT   0x00000100      /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_EIGTH  0x00000200      /* rx desc min threshold size */
#define E1000_RCTL_MO_SHIFT     12              /* multicast offset shift */
#define E1000_RCTL_MO_0         0x00000000      /* multicast offset 11:0 */
#define E1000_RCTL_MO_1         0x00001000      /* multicast offset 12:1 */
#define E1000_RCTL_MO_2         0x00002000      /* multicast offset 13:2 */
#define E1000_RCTL_MO_3         0x00003000      /* multicast offset 15:4 */
#define E1000_RCTL_MDR          0x00004000      /* multicast desc ring 0 */
#define E1000_RCTL_BAM          0x00008000      /* broadcast enable */
/* these buffer sizes are valid if E1000_RCTL_BSEX is 0 */
#define E1000_RCTL_SZ_2048      0x00000000      /* rx buffer size 2048 */
#define E1000_RCTL_SZ_1024      0x00010000      /* rx buffer size 1024 */
#define E1000_RCTL_SZ_512       0x00020000      /* rx buffer size 512 */
#define E1000_RCTL_SZ_256       0x00030000      /* rx buffer size 256 */
/* these buffer sizes are valid if E1000_RCTL_BSEX is 1 */
#define E1000_RCTL_SZ_16384     0x00010000      /* rx buffer size 16384 */
#define E1000_RCTL_SZ_8192      0x00020000      /* rx buffer size 8192 */
#define E1000_RCTL_SZ_4096      0x00030000      /* rx buffer size 4096 */
#define E1000_RCTL_VFE          0x00040000      /* vlan filter enable */
#define E1000_RCTL_CFIEN        0x00080000      /* canonical form enable */
#define E1000_RCTL_CFI          0x00100000      /* canonical form indicator */
#define E1000_RCTL_DPF          0x00400000      /* discard pause frames */
#define E1000_RCTL_PMCF         0x00800000      /* pass MAC control frames */
#define E1000_RCTL_BSEX         0x02000000      /* Buffer size extension */

/* Receive Descriptor */
#define E1000_RDT_DELAY 0x0000ffff      /* Delay timer (1=1024us) */
#define E1000_RDT_FPDB  0x80000000      /* Flush descriptor block */
#define E1000_RDLEN_LEN 0x0007ff80      /* descriptor length */
#define E1000_RDH_RDH   0x0000ffff      /* receive descriptor head */
#define E1000_RDT_RDT   0x0000ffff      /* receive descriptor tail */

/* Flow Control */
#define E1000_FCRTH_RTH  0x0000FFF8     /* Mask Bits[15:3] for RTH */
#define E1000_FCRTH_XFCE 0x80000000     /* External Flow Control Enable */
#define E1000_FCRTL_RTL  0x0000FFF8     /* Mask Bits[15:3] for RTL */
#define E1000_FCRTL_XONE 0x80000000     /* Enable XON frame transmission */

/* Receive Descriptor Control */
#define E1000_RXDCTL_PTHRESH 0x0000003F /* RXDCTL Prefetch Threshold */
#define E1000_RXDCTL_HTHRESH 0x00003F00 /* RXDCTL Host Threshold */
#define E1000_RXDCTL_WTHRESH 0x003F0000 /* RXDCTL Writeback Threshold */
#define E1000_RXDCTL_GRAN    0x01000000 /* RXDCTL Granularity */

/* Transmit Descriptor Control */
#define E1000_TXDCTL_PTHRESH 0x000000FF /* TXDCTL Prefetch Threshold */
#define E1000_TXDCTL_HTHRESH 0x0000FF00 /* TXDCTL Host Threshold */
#define E1000_TXDCTL_WTHRESH 0x00FF0000 /* TXDCTL Writeback Threshold */
#define E1000_TXDCTL_GRAN    0x01000000 /* TXDCTL Granularity */
#define E1000_TXDCTL_LWTHRESH 0xFE000000 /* TXDCTL Low Threshold */

/* Transmit Configuration Word */
#define E1000_TXCW_FD         0x00000020        /* TXCW full duplex */
#define E1000_TXCW_HD         0x00000040        /* TXCW half duplex */
#define E1000_TXCW_PAUSE      0x00000080        /* TXCW sym pause request */
#define E1000_TXCW_ASM_DIR    0x00000100        /* TXCW astm pause direction */
#define E1000_TXCW_PAUSE_MASK 0x00000180        /* TXCW pause request mask */
#define E1000_TXCW_RF         0x00003000        /* TXCW remote fault */
#define E1000_TXCW_NP         0x00008000        /* TXCW next page */
#define E1000_TXCW_CW         0x0000ffff        /* TxConfigWord mask */
#define E1000_TXCW_TXC        0x40000000        /* Transmit Config control */
#define E1000_TXCW_ANE        0x80000000        /* Auto-neg enable */

/* Receive Configuration Word */
#define E1000_RXCW_CW    0x0000ffff     /* RxConfigWord mask */
#define E1000_RXCW_NC    0x04000000     /* Receive config no carrier */
#define E1000_RXCW_IV    0x08000000     /* Receive config invalid */
#define E1000_RXCW_CC    0x10000000     /* Receive config change */
#define E1000_RXCW_C     0x20000000     /* Receive config */
#define E1000_RXCW_SYNCH 0x40000000     /* Receive config synch */
#define E1000_RXCW_ANC   0x80000000     /* Auto-neg complete */

/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */

/* Receive Checksum Control */
#define E1000_RXCSUM_PCSS_MASK 0x000000FF   /* Packet Checksum Start */
#define E1000_RXCSUM_IPOFL     0x00000100   /* IPv4 checksum offload */
#define E1000_RXCSUM_TUOFL     0x00000200   /* TCP / UDP checksum offload */
#define E1000_RXCSUM_IPV6OFL   0x00000400   /* IPv6 checksum offload */

/* Definitions for power management and wakeup registers */
/* Wake Up Control */
#define E1000_WUC_APME       0x00000001 /* APM Enable */
#define E1000_WUC_PME_EN     0x00000002 /* PME Enable */
#define E1000_WUC_PME_STATUS 0x00000004 /* PME Status */
#define E1000_WUC_APMPME     0x00000008 /* Assert PME on APM Wakeup */

/* Wake Up Filter Control */
#define E1000_WUFC_LNKC 0x00000001 /* Link Status Change Wakeup Enable */
#define E1000_WUFC_MAG  0x00000002 /* Magic Packet Wakeup Enable */
#define E1000_WUFC_EX   0x00000004 /* Directed Exact Wakeup Enable */
#define E1000_WUFC_MC   0x00000008 /* Directed Multicast Wakeup Enable */
#define E1000_WUFC_BC   0x00000010 /* Broadcast Wakeup Enable */
#define E1000_WUFC_ARP  0x00000020 /* ARP Request Packet Wakeup Enable */
#define E1000_WUFC_IPV4 0x00000040 /* Directed IPv4 Packet Wakeup Enable */
#define E1000_WUFC_IPV6 0x00000080 /* Directed IPv6 Packet Wakeup Enable */
#define E1000_WUFC_FLX0 0x00010000 /* Flexible Filter 0 Enable */
#define E1000_WUFC_FLX1 0x00020000 /* Flexible Filter 1 Enable */
#define E1000_WUFC_FLX2 0x00040000 /* Flexible Filter 2 Enable */
#define E1000_WUFC_FLX3 0x00080000 /* Flexible Filter 3 Enable */
#define E1000_WUFC_ALL_FILTERS 0x000F00FF /* Mask for all wakeup filters */
#define E1000_WUFC_FLX_OFFSET 16       /* Offset to the Flexible Filters bits */
#define E1000_WUFC_FLX_FILTERS 0x000F0000 /* Mask for the 4 flexible filters */

/* Wake Up Status */
#define E1000_WUS_LNKC 0x00000001 /* Link Status Changed */
#define E1000_WUS_MAG  0x00000002 /* Magic Packet Received */
#define E1000_WUS_EX   0x00000004 /* Directed Exact Received */
#define E1000_WUS_MC   0x00000008 /* Directed Multicast Received */
#define E1000_WUS_BC   0x00000010 /* Broadcast Received */
#define E1000_WUS_ARP  0x00000020 /* ARP Request Packet Received */
#define E1000_WUS_IPV4 0x00000040 /* Directed IPv4 Packet Wakeup Received */
#define E1000_WUS_IPV6 0x00000080 /* Directed IPv6 Packet Wakeup Received */
#define E1000_WUS_FLX0 0x00010000 /* Flexible Filter 0 Match */
#define E1000_WUS_FLX1 0x00020000 /* Flexible Filter 1 Match */
#define E1000_WUS_FLX2 0x00040000 /* Flexible Filter 2 Match */
#define E1000_WUS_FLX3 0x00080000 /* Flexible Filter 3 Match */
#define E1000_WUS_FLX_FILTERS 0x000F0000 /* Mask for the 4 flexible filters */

/* Management Control */
#define E1000_MANC_SMBUS_EN      0x00000001 /* SMBus Enabled - RO */
#define E1000_MANC_ASF_EN        0x00000002 /* ASF Enabled - RO */
#define E1000_MANC_R_ON_FORCE    0x00000004 /* Reset on Force TCO - RO */
#define E1000_MANC_RMCP_EN       0x00000100 /* Enable RCMP 026Fh Filtering */
#define E1000_MANC_0298_EN       0x00000200 /* Enable RCMP 0298h Filtering */
#define E1000_MANC_IPV4_EN       0x00000400 /* Enable IPv4 */
#define E1000_MANC_IPV6_EN       0x00000800 /* Enable IPv6 */
#define E1000_MANC_SNAP_EN       0x00001000 /* Accept LLC/SNAP */
#define E1000_MANC_ARP_EN        0x00002000 /* Enable ARP Request Filtering */
#define E1000_MANC_NEIGHBOR_EN   0x00004000 /* Enable Neighbor Discovery 
                                             * Filtering */
#define E1000_MANC_TCO_RESET     0x00010000 /* TCO Reset Occurred */
#define E1000_MANC_RCV_TCO_EN    0x00020000 /* Receive TCO Packets Enabled */
#define E1000_MANC_REPORT_STATUS 0x00040000 /* Status Reporting Enabled */
#define E1000_MANC_SMB_REQ       0x01000000 /* SMBus Request */
#define E1000_MANC_SMB_GNT       0x02000000 /* SMBus Grant */
#define E1000_MANC_SMB_CLK_IN    0x04000000 /* SMBus Clock In */
#define E1000_MANC_SMB_DATA_IN   0x08000000 /* SMBus Data In */
#define E1000_MANC_SMB_DATA_OUT  0x10000000 /* SMBus Data Out */
#define E1000_MANC_SMB_CLK_OUT   0x20000000 /* SMBus Clock Out */

#define E1000_MANC_SMB_DATA_OUT_SHIFT  28 /* SMBus Data Out Shift */
#define E1000_MANC_SMB_CLK_OUT_SHIFT   29 /* SMBus Clock Out Shift */

/* Wake Up Packet Length */
#define E1000_WUPL_LENGTH_MASK 0x0FFF   /* Only the lower 12 bits are valid */

#define E1000_MDALIGN          4096

/* EEPROM Commands */
#define EEPROM_READ_OPCODE  0x6  /* EERPOM read opcode */
#define EEPROM_WRITE_OPCODE 0x5  /* EERPOM write opcode */
#define EEPROM_ERASE_OPCODE 0x7  /* EERPOM erase opcode */
#define EEPROM_EWEN_OPCODE  0x13 /* EERPOM erase/write enable */
#define EEPROM_EWDS_OPCODE  0x10 /* EERPOM erast/write disable */

/* EEPROM Word Offsets */
#define EEPROM_ID_LED_SETTINGS     0x0004
#define EEPROM_INIT_CONTROL1_REG   0x000A
#define EEPROM_INIT_CONTROL2_REG   0x000F
#define EEPROM_FLASH_VERSION       0x0032
#define EEPROM_CHECKSUM_REG        0x003F

/* Word definitions for ID LED Settings */
#define ID_LED_RESERVED_0000 0x0000
#define ID_LED_RESERVED_FFFF 0xFFFF
#define ID_LED_DEFAULT       ((ID_LED_OFF1_ON2 << 12) | \
		              (ID_LED_OFF1_OFF2 << 8) | \
		              (ID_LED_DEF1_DEF2 << 4) | \
			      (ID_LED_DEF1_DEF2))
#define ID_LED_DEF1_DEF2     0x1
#define ID_LED_DEF1_ON2      0x2
#define ID_LED_DEF1_OFF2     0x3
#define ID_LED_ON1_DEF2      0x4
#define ID_LED_ON1_ON2       0x5
#define ID_LED_ON1_OFF2      0x6
#define ID_LED_OFF1_DEF2     0x7
#define ID_LED_OFF1_ON2      0x8
#define ID_LED_OFF1_OFF2     0x9

/* Mask bits for fields in Word 0x0a of the EEPROM */
#define EEPROM_WORD0A_ILOS   0x0010
#define EEPROM_WORD0A_SWDPIO 0x01E0
#define EEPROM_WORD0A_LRST   0x0200
#define EEPROM_WORD0A_FD     0x0400
#define EEPROM_WORD0A_66MHZ  0x0800

/* Mask bits for fields in Word 0x0f of the EEPROM */
#define EEPROM_WORD0F_PAUSE_MASK 0x3000
#define EEPROM_WORD0F_PAUSE      0x1000
#define EEPROM_WORD0F_ASM_DIR    0x2000
#define EEPROM_WORD0F_ANE        0x0800
#define EEPROM_WORD0F_SWPDIO_EXT 0x00F0

/* For checksumming, the sum of all words in the EEPROM should equal 0xBABA. */
#define EEPROM_SUM 0xBABA

/* EEPROM Map defines (WORD OFFSETS)*/
#define EEPROM_NODE_ADDRESS_BYTE_0 0
#define EEPROM_PBA_BYTE_1          8

/* EEPROM Map Sizes (Byte Counts) */
#define PBA_SIZE 4

/* Collision related configuration parameters */
#define E1000_COLLISION_THRESHOLD       16
#define E1000_CT_SHIFT                  4
#define E1000_COLLISION_DISTANCE        64
#define E1000_FDX_COLLISION_DISTANCE    E1000_COLLISION_DISTANCE
#define E1000_HDX_COLLISION_DISTANCE    E1000_COLLISION_DISTANCE
#define E1000_GB_HDX_COLLISION_DISTANCE 512
#define E1000_COLD_SHIFT                12

/* The number of Transmit and Receive Descriptors must be a multiple of 8 */
#define REQ_TX_DESCRIPTOR_MULTIPLE  8
#define REQ_RX_DESCRIPTOR_MULTIPLE  8

/* Default values for the transmit IPG register */
#define DEFAULT_82542_TIPG_IPGT        10
#define DEFAULT_82543_TIPG_IPGT_FIBER  9
#define DEFAULT_82543_TIPG_IPGT_COPPER 8

#define E1000_TIPG_IPGT_MASK  0x000003FF
#define E1000_TIPG_IPGR1_MASK 0x000FFC00
#define E1000_TIPG_IPGR2_MASK 0x3FF00000

#define DEFAULT_82542_TIPG_IPGR1 2
#define DEFAULT_82543_TIPG_IPGR1 8
#define E1000_TIPG_IPGR1_SHIFT  10

#define DEFAULT_82542_TIPG_IPGR2 10
#define DEFAULT_82543_TIPG_IPGR2 6
#define E1000_TIPG_IPGR2_SHIFT  20

#define E1000_TXDMAC_DPP 0x00000001

/* Adaptive IFS defines */
#define TX_THRESHOLD_START     8
#define TX_THRESHOLD_INCREMENT 10
#define TX_THRESHOLD_DECREMENT 1
#define TX_THRESHOLD_STOP      190
#define TX_THRESHOLD_DISABLE   0
#define TX_THRESHOLD_TIMER_MS  10000
#define MIN_NUM_XMITS          1000
#define IFS_MAX                80
#define IFS_STEP               10
#define IFS_MIN                40
#define IFS_RATIO              4

/* PBA constants */
#define E1000_PBA_16K 0x0010    /* 16KB, default TX allocation */
#define E1000_PBA_24K 0x0018
#define E1000_PBA_40K 0x0028
#define E1000_PBA_48K 0x0030    /* 48KB, default RX allocation */

/* Flow Control Constants */
#define FLOW_CONTROL_ADDRESS_LOW  0x00C28001
#define FLOW_CONTROL_ADDRESS_HIGH 0x00000100
#define FLOW_CONTROL_TYPE         0x8808

/* The historical defaults for the flow control values are given below. */
#define FC_DEFAULT_HI_THRESH        (0x8000)    /* 32KB */
#define FC_DEFAULT_LO_THRESH        (0x4000)    /* 16KB */
#define FC_DEFAULT_TX_TIMER         (0x100)     /* ~130 us */


/* The number of bits that we need to shift right to move the "pause"
 * bits from the EEPROM (bits 13:12) to the "pause" (bits 8:7) field
 * in the TXCW register 
 */
#define PAUSE_SHIFT 5

/* The number of bits that we need to shift left to move the "SWDPIO"
 * bits from the EEPROM (bits 8:5) to the "SWDPIO" (bits 25:22) field
 * in the CTRL register 
 */
#define SWDPIO_SHIFT 17

/* The number of bits that we need to shift left to move the "SWDPIO_EXT"
 * bits from the EEPROM word F (bits 7:4) to the bits 11:8 of The
 * Extended CTRL register.
 * in the CTRL register 
 */
#define SWDPIO__EXT_SHIFT 4

/* The number of bits that we need to shift left to move the "ILOS"
 * bit from the EEPROM (bit 4) to the "ILOS" (bit 7) field
 * in the CTRL register 
 */
#define ILOS_SHIFT  3


#define RECEIVE_BUFFER_ALIGN_SIZE  (256)

/* The number of milliseconds we wait for auto-negotiation to complete */
#define LINK_UP_TIMEOUT             500

#define E1000_TX_BUFFER_SIZE ((uint32_t)1514)

/* The carrier extension symbol, as received by the NIC. */
#define CARRIER_EXTENSION   0x0F

/* TBI_ACCEPT macro definition:
 *
 * This macro requires:
 *      adapter = a pointer to struct em_hw 
 *      status = the 8 bit status field of the RX descriptor with EOP set
 *      error = the 8 bit error field of the RX descriptor with EOP set
 *      length = the sum of all the length fields of the RX descriptors that
 *               make up the current frame
 *      last_byte = the last byte of the frame DMAed by the hardware
 *      max_frame_length = the maximum frame length we want to accept.
 *      min_frame_length = the minimum frame length we want to accept.
 *
 * This macro is a conditional that should be used in the interrupt 
 * handler's Rx processing routine when RxErrors have been detected.
 *
 * Typical use:
 *  ...
 *  if (TBI_ACCEPT) {
 *      accept_frame = TRUE;
 *      em_tbi_adjust_stats(adapter, MacAddress);
 *      frame_length--;
 *  } else {
 *      accept_frame = FALSE;
 *  }
 *  ...
 */

#define TBI_ACCEPT(adapter, status, errors, length, last_byte) \
    ((adapter)->tbi_compatibility_on && \
     (((errors) & E1000_RXD_ERR_FRAME_ERR_MASK) == E1000_RXD_ERR_CE) && \
     ((last_byte) == CARRIER_EXTENSION) && \
     (((status) & E1000_RXD_STAT_VP) ? \
          (((length) > ((adapter)->min_frame_size - VLAN_TAG_SIZE)) && \
           ((length) <= ((adapter)->max_frame_size + 1))) : \
          (((length) > (adapter)->min_frame_size) && \
           ((length) <= ((adapter)->max_frame_size + VLAN_TAG_SIZE + 1)))))


/* Structures, enums, and macros for the PHY */

/* Bit definitions for the Management Data IO (MDIO) and Management Data
 * Clock (MDC) pins in the Device Control Register.
 */
#define E1000_CTRL_PHY_RESET_DIR  E1000_CTRL_SWDPIO0
#define E1000_CTRL_PHY_RESET      E1000_CTRL_SWDPIN0
#define E1000_CTRL_MDIO_DIR       E1000_CTRL_SWDPIO2
#define E1000_CTRL_MDIO           E1000_CTRL_SWDPIN2
#define E1000_CTRL_MDC_DIR        E1000_CTRL_SWDPIO3
#define E1000_CTRL_MDC            E1000_CTRL_SWDPIN3
#define E1000_CTRL_PHY_RESET_DIR4 E1000_CTRL_EXT_SDP4_DIR
#define E1000_CTRL_PHY_RESET4     E1000_CTRL_EXT_SDP4_DATA

/* PHY 1000 MII Register/Bit Definitions */
/* PHY Registers defined by IEEE */
#define PHY_CTRL         0x00 /* Control Register */
#define PHY_STATUS       0x01 /* Status Regiser */
#define PHY_ID1          0x02 /* Phy Id Reg (word 1) */
#define PHY_ID2          0x03 /* Phy Id Reg (word 2) */
#define PHY_AUTONEG_ADV  0x04 /* Autoneg Advertisement */
#define PHY_LP_ABILITY   0x05 /* Link Partner Ability (Base Page) */
#define PHY_AUTONEG_EXP  0x06 /* Autoneg Expansion Reg */
#define PHY_NEXT_PAGE_TX 0x07 /* Next Page TX */
#define PHY_LP_NEXT_PAGE 0x08 /* Link Partner Next Page */
#define PHY_1000T_CTRL   0x09 /* 1000Base-T Control Reg */
#define PHY_1000T_STATUS 0x0A /* 1000Base-T Status Reg */
#define PHY_EXT_STATUS   0x0F /* Extended Status Reg */

/* M88E1000 Specific Registers */
#define M88E1000_PHY_SPEC_CTRL     0x10  /* PHY Specific Control Register */
#define M88E1000_PHY_SPEC_STATUS   0x11  /* PHY Specific Status Register */
#define M88E1000_INT_ENABLE        0x12  /* Interrupt Enable Register */
#define M88E1000_INT_STATUS        0x13  /* Interrupt Status Register */
#define M88E1000_EXT_PHY_SPEC_CTRL 0x14  /* Extended PHY Specific Control */
#define M88E1000_RX_ERR_CNTR       0x15  /* Receive Error Counter */

#define MAX_PHY_REG_ADDRESS 0x1F        /* 5 bit address bus (0-0x1F) */

/* PHY Control Register */
#define MII_CR_SPEED_SELECT_MSB 0x0040  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_COLL_TEST_ENABLE 0x0080  /* Collision test enable */
#define MII_CR_FULL_DUPLEX      0x0100  /* FDX =1, half duplex =0 */
#define MII_CR_RESTART_AUTO_NEG 0x0200  /* Restart auto negotiation */
#define MII_CR_ISOLATE          0x0400  /* Isolate PHY from MII */
#define MII_CR_POWER_DOWN       0x0800  /* Power down */
#define MII_CR_AUTO_NEG_EN      0x1000  /* Auto Neg Enable */
#define MII_CR_SPEED_SELECT_LSB 0x2000  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_LOOPBACK         0x4000  /* 0 = normal, 1 = loopback */
#define MII_CR_RESET            0x8000  /* 0 = normal, 1 = PHY reset */

/* PHY Status Register */
#define MII_SR_EXTENDED_CAPS     0x0001 /* Extended register capabilities */
#define MII_SR_JABBER_DETECT     0x0002 /* Jabber Detected */
#define MII_SR_LINK_STATUS       0x0004 /* Link Status 1 = link */
#define MII_SR_AUTONEG_CAPS      0x0008 /* Auto Neg Capable */
#define MII_SR_REMOTE_FAULT      0x0010 /* Remote Fault Detect */
#define MII_SR_AUTONEG_COMPLETE  0x0020 /* Auto Neg Complete */
#define MII_SR_PREAMBLE_SUPPRESS 0x0040 /* Preamble may be suppressed */
#define MII_SR_EXTENDED_STATUS   0x0100 /* Ext. status info in Reg 0x0F */
#define MII_SR_100T2_HD_CAPS     0x0200 /* 100T2 Half Duplex Capable */
#define MII_SR_100T2_FD_CAPS     0x0400 /* 100T2 Full Duplex Capable */
#define MII_SR_10T_HD_CAPS       0x0800 /* 10T   Half Duplex Capable */
#define MII_SR_10T_FD_CAPS       0x1000 /* 10T   Full Duplex Capable */
#define MII_SR_100X_HD_CAPS      0x2000 /* 100X  Half Duplex Capable */
#define MII_SR_100X_FD_CAPS      0x4000 /* 100X  Full Duplex Capable */
#define MII_SR_100T4_CAPS        0x8000 /* 100T4 Capable */

/* Autoneg Advertisement Register */
#define NWAY_AR_SELECTOR_FIELD 0x0001   /* indicates IEEE 802.3 CSMA/CD */
#define NWAY_AR_10T_HD_CAPS    0x0020   /* 10T   Half Duplex Capable */
#define NWAY_AR_10T_FD_CAPS    0x0040   /* 10T   Full Duplex Capable */
#define NWAY_AR_100TX_HD_CAPS  0x0080   /* 100TX Half Duplex Capable */
#define NWAY_AR_100TX_FD_CAPS  0x0100   /* 100TX Full Duplex Capable */
#define NWAY_AR_100T4_CAPS     0x0200   /* 100T4 Capable */
#define NWAY_AR_PAUSE          0x0400   /* Pause operation desired */
#define NWAY_AR_ASM_DIR        0x0800   /* Asymmetric Pause Direction bit */
#define NWAY_AR_REMOTE_FAULT   0x2000   /* Remote Fault detected */
#define NWAY_AR_NEXT_PAGE      0x8000   /* Next Page ability supported */

/* Link Partner Ability Register (Base Page) */
#define NWAY_LPAR_SELECTOR_FIELD 0x0000 /* LP protocol selector field */
#define NWAY_LPAR_10T_HD_CAPS    0x0020 /* LP is 10T   Half Duplex Capable */
#define NWAY_LPAR_10T_FD_CAPS    0x0040 /* LP is 10T   Full Duplex Capable */
#define NWAY_LPAR_100TX_HD_CAPS  0x0080 /* LP is 100TX Half Duplex Capable */
#define NWAY_LPAR_100TX_FD_CAPS  0x0100 /* LP is 100TX Full Duplex Capable */
#define NWAY_LPAR_100T4_CAPS     0x0200 /* LP is 100T4 Capable */
#define NWAY_LPAR_PAUSE          0x0400 /* LP Pause operation desired */
#define NWAY_LPAR_ASM_DIR        0x0800 /* LP Asymmetric Pause Direction bit */
#define NWAY_LPAR_REMOTE_FAULT   0x2000 /* LP has detected Remote Fault */
#define NWAY_LPAR_ACKNOWLEDGE    0x4000 /* LP has rx'd link code word */
#define NWAY_LPAR_NEXT_PAGE      0x8000 /* Next Page ability supported */

/* Autoneg Expansion Register */
#define NWAY_ER_LP_NWAY_CAPS      0x0001 /* LP has Auto Neg Capability */
#define NWAY_ER_PAGE_RXD          0x0002 /* LP is 10T   Half Duplex Capable */
#define NWAY_ER_NEXT_PAGE_CAPS    0x0004 /* LP is 10T   Full Duplex Capable */
#define NWAY_ER_LP_NEXT_PAGE_CAPS 0x0008 /* LP is 100TX Half Duplex Capable */
#define NWAY_ER_PAR_DETECT_FAULT  0x0100 /* LP is 100TX Full Duplex Capable */

/* Next Page TX Register */
#define NPTX_MSG_CODE_FIELD 0x0001 /* NP msg code or unformatted data */
#define NPTX_TOGGLE         0x0800 /* Toggles between exchanges
                                    * of different NP
                                    */
#define NPTX_ACKNOWLDGE2    0x1000 /* 1 = will comply with msg
                                    * 0 = cannot comply with msg
                                    */
#define NPTX_MSG_PAGE       0x2000 /* formatted(1)/unformatted(0) pg */
#define NPTX_NEXT_PAGE      0x8000 /* 1 = addition NP will follow 
                                    * 0 = sending last NP
                                    */

/* Link Partner Next Page Register */
#define LP_RNPR_MSG_CODE_FIELD 0x0001 /* NP msg code or unformatted data */
#define LP_RNPR_TOGGLE         0x0800 /* Toggles between exchanges
                                       * of different NP
                                       */
#define LP_RNPR_ACKNOWLDGE2    0x1000 /* 1 = will comply with msg 
                                       * 0 = cannot comply with msg
                                       */
#define LP_RNPR_MSG_PAGE       0x2000  /* formatted(1)/unformatted(0) pg */
#define LP_RNPR_ACKNOWLDGE     0x4000  /* 1 = ACK / 0 = NO ACK */
#define LP_RNPR_NEXT_PAGE      0x8000  /* 1 = addition NP will follow
                                        * 0 = sending last NP 
                                        */

/* 1000BASE-T Control Register */
#define CR_1000T_ASYM_PAUSE      0x0080 /* Advertise asymmetric pause bit */
#define CR_1000T_HD_CAPS         0x0100 /* Advertise 1000T HD capability */
#define CR_1000T_FD_CAPS         0x0200 /* Advertise 1000T FD capability  */
#define CR_1000T_REPEATER_DTE    0x0400 /* 1=Repeater/switch device port */
                                        /* 0=DTE device */
#define CR_1000T_MS_VALUE        0x0800 /* 1=Configure PHY as Master */
                                        /* 0=Configure PHY as Slave */
#define CR_1000T_MS_ENABLE       0x1000 /* 1=Master/Slave manual config value */
                                        /* 0=Automatic Master/Slave config */
#define CR_1000T_TEST_MODE_NORMAL 0x0000 /* Normal Operation */
#define CR_1000T_TEST_MODE_1     0x2000 /* Transmit Waveform test */
#define CR_1000T_TEST_MODE_2     0x4000 /* Master Transmit Jitter test */
#define CR_1000T_TEST_MODE_3     0x6000 /* Slave Transmit Jitter test */
#define CR_1000T_TEST_MODE_4     0x8000 /* Transmitter Distortion test */

/* 1000BASE-T Status Register */
#define SR_1000T_IDLE_ERROR_CNT   0x00FF /* Num idle errors since last read */
#define SR_1000T_ASYM_PAUSE_DIR   0x0100 /* LP asymmetric pause direction bit */
#define SR_1000T_LP_HD_CAPS       0x0400 /* LP is 1000T HD capable */
#define SR_1000T_LP_FD_CAPS       0x0800 /* LP is 1000T FD capable */
#define SR_1000T_REMOTE_RX_STATUS 0x1000 /* Remote receiver OK */
#define SR_1000T_LOCAL_RX_STATUS  0x2000 /* Local receiver OK */
#define SR_1000T_MS_CONFIG_RES    0x4000 /* 1=Local TX is Master, 0=Slave */
#define SR_1000T_MS_CONFIG_FAULT  0x8000 /* Master/Slave config fault */
#define SR_1000T_REMOTE_RX_STATUS_SHIFT 12
#define SR_1000T_LOCAL_RX_STATUS_SHIFT  13

/* Extended Status Register */
#define IEEE_ESR_1000T_HD_CAPS 0x1000 /* 1000T HD capable */
#define IEEE_ESR_1000T_FD_CAPS 0x2000 /* 1000T FD capable */
#define IEEE_ESR_1000X_HD_CAPS 0x4000 /* 1000X HD capable */
#define IEEE_ESR_1000X_FD_CAPS 0x8000 /* 1000X FD capable */

#define PHY_TX_POLARITY_MASK   0x0100 /* register 10h bit 8 (polarity bit) */
#define PHY_TX_NORMAL_POLARITY 0      /* register 10h bit 8 (normal polarity) */

#define AUTO_POLARITY_DISABLE  0x0010 /* register 11h bit 4 */
                                      /* (0=enable, 1=disable) */

/* M88E1000 PHY Specific Control Register */
#define M88E1000_PSCR_JABBER_DISABLE    0x0001 /* 1=Jabber Function disabled */
#define M88E1000_PSCR_POLARITY_REVERSAL 0x0002 /* 1=Polarity Reversal enabled */
#define M88E1000_PSCR_SQE_TEST          0x0004 /* 1=SQE Test enabled */
#define M88E1000_PSCR_CLK125_DISABLE    0x0010 /* 1=CLK125 low, 
                                                * 0=CLK125 toggling
                                                */
#define M88E1000_PSCR_MDI_MANUAL_MODE  0x0000  /* MDI Crossover Mode bits 6:5 */
                                               /* Manual MDI configuration */
#define M88E1000_PSCR_MDIX_MANUAL_MODE 0x0020  /* Manual MDIX configuration */
#define M88E1000_PSCR_AUTO_X_1000T     0x0040  /* 1000BASE-T: Auto crossover,
                                                *  100BASE-TX/10BASE-T: 
                                                *  MDI Mode
                                                */
#define M88E1000_PSCR_AUTO_X_MODE      0x0060  /* Auto crossover enabled 
                                                * all speeds. 
                                                */
#define M88E1000_PSCR_10BT_EXT_DIST_ENABLE 0x0080 
                                        /* 1=Enable Extended 10BASE-T distance
                                         * (Lower 10BASE-T RX Threshold)
                                         * 0=Normal 10BASE-T RX Threshold */
#define M88E1000_PSCR_MII_5BIT_ENABLE      0x0100
                                        /* 1=5-Bit interface in 100BASE-TX
                                         * 0=MII interface in 100BASE-TX */
#define M88E1000_PSCR_SCRAMBLER_DISABLE    0x0200 /* 1=Scrambler disable */
#define M88E1000_PSCR_FORCE_LINK_GOOD      0x0400 /* 1=Force link good */
#define M88E1000_PSCR_ASSERT_CRS_ON_TX     0x0800 /* 1=Assert CRS on Transmit */

#define M88E1000_PSCR_POLARITY_REVERSAL_SHIFT    1
#define M88E1000_PSCR_AUTO_X_MODE_SHIFT          5
#define M88E1000_PSCR_10BT_EXT_DIST_ENABLE_SHIFT 7

/* M88E1000 PHY Specific Status Register */
#define M88E1000_PSSR_JABBER             0x0001 /* 1=Jabber */
#define M88E1000_PSSR_REV_POLARITY       0x0002 /* 1=Polarity reversed */
#define M88E1000_PSSR_MDIX               0x0040 /* 1=MDIX; 0=MDI */
#define M88E1000_PSSR_CABLE_LENGTH       0x0380 /* 0=<50M;1=50-80M;2=80-110M;
                                            * 3=110-140M;4=>140M */
#define M88E1000_PSSR_LINK               0x0400 /* 1=Link up, 0=Link down */
#define M88E1000_PSSR_SPD_DPLX_RESOLVED  0x0800 /* 1=Speed & Duplex resolved */
#define M88E1000_PSSR_PAGE_RCVD          0x1000 /* 1=Page received */
#define M88E1000_PSSR_DPLX               0x2000 /* 1=Duplex 0=Half Duplex */
#define M88E1000_PSSR_SPEED              0xC000 /* Speed, bits 14:15 */
#define M88E1000_PSSR_10MBS              0x0000 /* 00=10Mbs */
#define M88E1000_PSSR_100MBS             0x4000 /* 01=100Mbs */
#define M88E1000_PSSR_1000MBS            0x8000 /* 10=1000Mbs */

#define M88E1000_PSSR_REV_POLARITY_SHIFT 1
#define M88E1000_PSSR_MDIX_SHIFT         6
#define M88E1000_PSSR_CABLE_LENGTH_SHIFT 7

/* M88E1000 Extended PHY Specific Control Register */
#define M88E1000_EPSCR_FIBER_LOOPBACK 0x4000 /* 1=Fiber loopback */
#define M88E1000_EPSCR_DOWN_NO_IDLE   0x8000 /* 1=Lost lock detect enabled.
                                              * Will assert lost lock and bring
                                              * link down if idle not seen
                                              * within 1ms in 1000BASE-T 
                                              */
/* Number of times we will attempt to autonegotiate before downshifting if we
 * are the master */
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_MASK 0x0C00
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_1X   0x0000    
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_2X   0x0400
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_3X   0x0800
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_4X   0x0C00
/* Number of times we will attempt to autonegotiate before downshifting if we
 * are the slave */
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_MASK  0x0300
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_DIS   0x0000
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_1X    0x0100
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_2X    0x0200
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_3X    0x0300
#define M88E1000_EPSCR_TX_CLK_2_5     0x0060 /* 2.5 MHz TX_CLK */
#define M88E1000_EPSCR_TX_CLK_25      0x0070 /* 25  MHz TX_CLK */
#define M88E1000_EPSCR_TX_CLK_0       0x0000 /* NO  TX_CLK */

/* Bit definitions for valid PHY IDs. */
#define M88E1000_E_PHY_ID  0x01410C50
#define M88E1000_I_PHY_ID  0x01410C30
#define M88E1011_I_PHY_ID  0x01410C20
#define M88E1000_12_PHY_ID M88E1000_E_PHY_ID
#define M88E1000_14_PHY_ID M88E1000_E_PHY_ID

/* Miscellaneous PHY bit definitions. */
#define PHY_PREAMBLE        0xFFFFFFFF
#define PHY_SOF             0x01
#define PHY_OP_READ         0x02
#define PHY_OP_WRITE        0x01
#define PHY_TURNAROUND      0x02
#define PHY_PREAMBLE_SIZE   32
#define MII_CR_SPEED_1000   0x0040
#define MII_CR_SPEED_100    0x2000
#define MII_CR_SPEED_10     0x0000
#define E1000_PHY_ADDRESS   0x01
#define PHY_AUTO_NEG_TIME   45  /* 4.5 Seconds */
#define PHY_FORCE_TIME      20  /* 2.0 Seconds */
#define PHY_REVISION_MASK   0xFFFFFFF0
#define DEVICE_SPEED_MASK   0x00000300  /* Device Ctrl Reg Speed Mask */
#define REG4_SPEED_MASK     0x01E0
#define REG9_SPEED_MASK     0x0300
#define ADVERTISE_10_HALF   0x0001
#define ADVERTISE_10_FULL   0x0002
#define ADVERTISE_100_HALF  0x0004
#define ADVERTISE_100_FULL  0x0008
#define ADVERTISE_1000_HALF 0x0010
#define ADVERTISE_1000_FULL 0x0020
#define AUTONEG_ADVERTISE_SPEED_DEFAULT 0x002F  /* Everything but 1000-Half */

/*$FreeBSD$*/
/* if_em.h */

/* Tunables */
#define MAX_TXD                         256
#define MAX_RXD                         256
#define TX_CLEANUP_THRESHOLD            MAX_TXD / 8
#define TIDV                            128     
#define RIDV                            28      
#define DO_AUTO_NEG                     1       
#define WAIT_FOR_AUTO_NEG_DEFAULT       1       
#define AUTONEG_ADV_DEFAULT             (ADVERTISE_10_HALF | ADVERTISE_10_FULL | \
                                         ADVERTISE_100_HALF | ADVERTISE_100_FULL | \
                                         ADVERTISE_1000_FULL)
#define EM_ENABLE_RXCSUM_OFFLOAD        1
#define EM_REPORT_TX_EARLY              2
#define EM_CHECKSUM_FEATURES            (CSUM_TCP | CSUM_UDP)
#define EM_MAX_INTR                     3
#define EM_TX_TIMEOUT                   5    /* set to 5 seconds */


#define EM_MMBA                         0x0010 /* Mem base address */
#define EM_ROUNDUP(size, unit) (((size) + (unit) - 1) & ~((unit) - 1))
#define EM_JUMBO_PBA                    0x00000028
#define EM_DEFAULT_PBA                  0x00000030

#define IOCTL_CMD_TYPE                  int
#define MAX_NUM_MULTICAST_ADDRESSES     128
#define PCI_ANY_ID                      (~0U)
#define ETHER_ALIGN                     2
#define QTAG_TYPE                       0x8100

#define INIT_DEBUGOUT(S)            DEBUGOUT(S)
#define INIT_DEBUGOUT1(S, A)        DEBUGOUT1(S, A)
#define INIT_DEBUGOUT2(S, A, B)     DEBUGOUT2(S, A, B)
#define IOCTL_DEBUGOUT(S)           DEBUGOUT(S)
#define IOCTL_DEBUGOUT1(S, A)       DEBUGOUT1(S, A)
#define IOCTL_DEBUGOUT2(S, A, B)    DEBUGOUT2(S, A, B)
#define HW_DEBUGOUT(S)              DEBUGOUT(S)
#define HW_DEBUGOUT1(S, A)          DEBUGOUT1(S, A)
#define HW_DEBUGOUT2(S, A, B)       DEBUGOUT2(S, A, B)


/* Supported RX Buffer Sizes */
#define EM_RXBUFFER_2048        2048
#define EM_RXBUFFER_4096        4096
#define EM_RXBUFFER_8192        8192
#define EM_RXBUFFER_16384      16384

#ifdef __alpha__
	#undef vtophys
	#define vtophys(va)     alpha_XXX_dmamap((vm_offset_t)(va))
#endif /* __alpha__ */

/* ******************************************************************************
 * vendor_info_array
 *
 * This array contains the list of Subvendor/Subdevice IDs on which the driver
 * should load.
 *
 * ******************************************************************************/
typedef struct _em_vendor_info_t
{
	unsigned int vendor_id;
	unsigned int device_id;
	unsigned int subvendor_id;
	unsigned int subdevice_id;
	unsigned int index;
} em_vendor_info_t;


/*#include <sys/queue.h>*/

typedef enum _XSUM_CONTEXT_T {
	OFFLOAD_NONE,
	OFFLOAD_TCP_IP,
	OFFLOAD_UDP_IP
} XSUM_CONTEXT_T;

/* Our adapter structure */
struct adapter {
	struct em_hw    hw;

	/* FreeBSD operating-system-specific structures */
	struct resource *res_memory;
	struct resource *res_interrupt;
	void            *int_handler_tag;
	u_int8_t        unit;

	/* Info about the board itself */
	u_int32_t       part_num;
	u_int8_t        link_active;
	u_int16_t       link_speed;
	u_int16_t       link_duplex;
	u_int32_t       tx_int_delay;
	u_int32_t       rx_int_delay;

	u_int8_t        rx_checksum;
	XSUM_CONTEXT_T  active_checksum_context;

	char *dma_mem;		/* pointer to big chunk of dma-alloc memory */
	char *map_mem;		/* dma-mem mapped into device-space */
	u_int32_t mem_len;	/* length of big chunk of memory */

	char *tx_bufs;		/* ptr to contiguous transmit buffers */
	char *rx_bufs;		/* ptr to contiguous receive buffers */

	/* Transmit definitions */
	struct em_tx_desc *first_tx_desc;
	struct em_tx_desc *last_tx_desc;
	struct em_tx_desc *next_avail_tx_desc;
	struct em_tx_desc *oldest_used_tx_desc;
	struct em_tx_desc *tx_desc_base;
	u_int32_t tx_desc_base_pci;
	volatile u_int16_t num_tx_desc_avail;
	u_int16_t       num_tx_desc;
	u_int32_t       txd_cmd;

	/* Receive definitions */
	struct em_rx_desc *first_rx_desc;
	struct em_rx_desc *last_rx_desc;
	struct em_rx_desc *next_rx_desc_to_check;
	struct em_rx_desc *rx_desc_base;
	u_int32_t rx_desc_base_pci;
	u_int16_t       num_rx_desc;
	u_int32_t       rx_buffer_len;

	/* Jumbo frame */
	struct mbuf     *fmp;
	struct mbuf     *lmp;


	/* Misc stats maintained by the driver */
	unsigned long   dropped_pkts;
	unsigned long   mbuf_alloc_failed;
	unsigned long   mbuf_cluster_failed;
	unsigned long   xmit_pullup;
	unsigned long   no_tx_desc_avail;
	unsigned long   no_tx_buffer_avail1;
	unsigned long   no_tx_buffer_avail2;
#ifdef DBG_STATS
	unsigned long   no_pkts_avail;
	unsigned long   clean_tx_interrupts;

#endif

	struct em_hw_stats stats;
};

struct em_hw    g_hw;

XSUM_CONTEXT_T  g_active_checksum_context;
struct em_hw_stats g_stats;
struct adapter g_adapter;
int debug_regs = 1;


#if 0
char em_driver_version[] = "1.3.8";
#endif


/*$FreeBSD$*/
/* if_em_hw.c
 * Shared functions for accessing and configuring the MAC
 */

void
em_write_reg(struct em_hw *hw, int reg, uint32_t val)
{
	if (debug_regs)
	    DEBUGOUT2("em_write_reg: reg 0x%X val 0x%X ", reg, val);

	hw->regs[reg >> 2] = val;
}


static int32_t em_setup_fiber_link(struct em_hw *hw);
static int32_t em_setup_copper_link(struct em_hw *hw);
static int32_t em_phy_force_speed_duplex(struct em_hw *hw);
static int32_t em_config_mac_to_phy(struct em_hw *hw);
static int32_t em_force_mac_fc(struct em_hw *hw);
static void em_raise_mdi_clk(struct em_hw *hw, uint32_t *ctrl);
static void em_lower_mdi_clk(struct em_hw *hw, uint32_t *ctrl);
static void em_shift_out_mdi_bits(struct em_hw *hw, uint32_t data, uint16_t count);
static uint16_t em_shift_in_mdi_bits(struct em_hw *hw);
static int32_t em_phy_reset_dsp(struct em_hw *hw);
static void em_raise_ee_clk(struct em_hw *hw, uint32_t *eecd);
static void em_lower_ee_clk(struct em_hw *hw, uint32_t *eecd);
static void em_shift_out_ee_bits(struct em_hw *hw, uint16_t data, uint16_t count);
static uint16_t em_shift_in_ee_bits(struct em_hw *hw);
static void em_setup_eeprom(struct em_hw *hw);
static void em_standby_eeprom(struct em_hw *hw);

/******************************************************************************
 * Raises the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code
 * eecd - EECD's current value
 *****************************************************************************/
static void
em_raise_ee_clk(struct em_hw *hw,
                   uint32_t *eecd)
{
    /* Raise the clock input to the EEPROM (by setting the SK bit), and then
     * wait 50 microseconds.
     */
    *eecd = *eecd | E1000_EECD_SK;
    E1000_WRITE_REG(hw, EECD, *eecd);
    usec_delay(50);
}

/******************************************************************************
 * Lowers the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code 
 * eecd - EECD's current value
 *****************************************************************************/
static void
em_lower_ee_clk(struct em_hw *hw,
                   uint32_t *eecd)
{
    /* Lower the clock input to the EEPROM (by clearing the SK bit), and then 
     * wait 50 microseconds. 
     */
    *eecd = *eecd & ~E1000_EECD_SK;
    E1000_WRITE_REG(hw, EECD, *eecd);
    usec_delay(50);
}

/******************************************************************************
 * Shift data bits out to the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * data - data to send to the EEPROM
 * count - number of bits to shift out
 *****************************************************************************/
static void
em_shift_out_ee_bits(struct em_hw *hw,
                        uint16_t data,
                        uint16_t count)
{
    uint32_t eecd;
    uint32_t mask;

    /* We need to shift "count" bits out to the EEPROM. So, value in the
     * "data" parameter will be shifted out to the EEPROM one bit at a time.
     * In order to do this, "data" must be broken down into bits. 
     */
    mask = 0x01 << (count - 1);
    eecd = E1000_READ_REG(hw, EECD);
    eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
    do {
        /* A "1" is shifted out to the EEPROM by setting bit "DI" to a "1",
         * and then raising and then lowering the clock (the SK bit controls
         * the clock input to the EEPROM).  A "0" is shifted out to the EEPROM
         * by setting "DI" to "0" and then raising and then lowering the clock.
         */
        eecd &= ~E1000_EECD_DI;

        if(data & mask)
            eecd |= E1000_EECD_DI;


        E1000_WRITE_REG(hw, EECD, eecd);

        usec_delay(50);

        em_raise_ee_clk(hw, &eecd);
        em_lower_ee_clk(hw, &eecd);

        mask = mask >> 1;

    } while(mask);

    /* We leave the "DI" bit set to "0" when we leave this routine. */
    eecd &= ~E1000_EECD_DI;
    E1000_WRITE_REG(hw, EECD, eecd);
}

/******************************************************************************
 * Shift data bits in from the EEPROM
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static uint16_t
em_shift_in_ee_bits(struct em_hw *hw)
{
    uint32_t eecd;
    uint32_t i;
    uint16_t data;

    /* In order to read a register from the EEPROM, we need to shift 16 bits 
     * in from the EEPROM. Bits are "shifted in" by raising the clock input to
     * the EEPROM (setting the SK bit), and then reading the value of the "DO"
     * bit.  During this "shifting in" process the "DI" bit should always be 
     * clear..
     */

    eecd = E1000_READ_REG(hw, EECD);

    eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
    data = 0;

    for(i = 0; i < 16; i++) {
        data = data << 1;
        em_raise_ee_clk(hw, &eecd);

        eecd = E1000_READ_REG(hw, EECD);

        eecd &= ~(E1000_EECD_DI);
        if(eecd & E1000_EECD_DO)
            data |= 1;

        em_lower_ee_clk(hw, &eecd);
    }

    return data;
}

/******************************************************************************
 * Prepares EEPROM for access
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Lowers EEPROM clock. Clears input pin. Sets the chip select pin. This 
 * function should be called before issuing a command to the EEPROM.
 *****************************************************************************/
static void
em_setup_eeprom(struct em_hw *hw)
{
    uint32_t eecd;

    eecd = E1000_READ_REG(hw, EECD);

    /* Clear SK and DI */
    eecd &= ~(E1000_EECD_SK | E1000_EECD_DI);
    E1000_WRITE_REG(hw, EECD, eecd);

    /* Set CS */
    eecd |= E1000_EECD_CS;
    E1000_WRITE_REG(hw, EECD, eecd);
}

/******************************************************************************
 * Returns EEPROM to a "standby" state
 * 
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void
em_standby_eeprom(struct em_hw *hw)
{
    uint32_t eecd;

    eecd = E1000_READ_REG(hw, EECD);

    /* Deselct EEPROM */
    eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
    E1000_WRITE_REG(hw, EECD, eecd);
    usec_delay(50);

    /* Clock high */
    eecd |= E1000_EECD_SK;
    E1000_WRITE_REG(hw, EECD, eecd);
    usec_delay(50);

    /* Select EEPROM */
    eecd |= E1000_EECD_CS;
    E1000_WRITE_REG(hw, EECD, eecd);
    usec_delay(50);

    /* Clock low */
    eecd &= ~E1000_EECD_SK;
    E1000_WRITE_REG(hw, EECD, eecd);
    usec_delay(50);
}


boolean_t
em_request_eeprom(struct em_hw *hw, boolean_t *large_eeprom)
{
    uint32_t eecd;
    uint32_t i = 0;

    *large_eeprom = FALSE;

    /* Request EEPROM Access */
    if(hw->mac_type > em_82544)
    {
        eecd = E1000_READ_REG(hw, EECD);

        if (eecd & E1000_EECD_SIZE)
		*large_eeprom = TRUE;

        eecd |= E1000_EECD_REQ;
        E1000_WRITE_REG(hw, EECD, eecd);
        eecd = E1000_READ_REG(hw, EECD);

        while (!(eecd & E1000_EECD_GNT) && i < 100)
	{
            i++;
            usec_delay(5);
            eecd = E1000_READ_REG(hw, EECD);
        }

        if (!(eecd & E1000_EECD_GNT))
	{
            eecd &= ~E1000_EECD_REQ;
            E1000_WRITE_REG(hw, EECD, eecd);
            return FALSE;
        }
    }

    return TRUE;
}

void
em_release_eeprom(struct em_hw *hw)
{
    uint32_t eecd;

    /* Stop requesting EEPROM access */
    if (hw->mac_type > em_82544) {
        eecd = E1000_READ_REG(hw, EECD);
        eecd &= ~E1000_EECD_REQ;
        E1000_WRITE_REG(hw, EECD, eecd);
    }
}

boolean_t
em_eeprom_wait_rdy(struct em_hw *hw)
{
	hw = hw;
	return TRUE;
}

boolean_t
em_eeprom_send_cmd(struct em_hw *hw,
	boolean_t req,
	int cmd,
	int cmdbits,
	int offset,
	u_int16_t *data,
	int databits,
	boolean_t datadirout,
	boolean_t waitrdy)
{
    boolean_t large_eeprom = FALSE;
    boolean_t ok = TRUE;

/*    DEBUGFUNC("em_eeprom_send_cmd");	*/

    if (req && !em_request_eeprom(hw, &large_eeprom))
	    return FALSE;

    /*  Prepare the EEPROM for writing */
    em_setup_eeprom(hw);

    /*  Send the command (opcode + addr)  */
    em_shift_out_ee_bits(hw, cmd, cmdbits);

    if(large_eeprom) {
        /* If we have a 256 word EEPROM, there are 8 address bits */
        em_shift_out_ee_bits(hw, offset, 11 - cmdbits);
    } else {
        /* If we have a 64 word EEPROM, there are 6 address bits */
        em_shift_out_ee_bits(hw, offset, 9 - cmdbits);
    }

    if (databits) {
	if (datadirout) {
	    /* Write the data */
	    em_shift_out_ee_bits(hw, *data, 16);
	    DEBUGOUT2("em_write_eeprom: offset=%d data=0x%X ", offset, *data);
	} else {
	    /* Read the data */
	    *data = em_shift_in_ee_bits(hw);
	    DEBUGOUT2("em_read_eeprom: offset=%d data=0x%X ", offset, *data);
	}
    }

    /* Wait for ready */
    if (waitrdy)
	ok = em_eeprom_wait_rdy(hw);
    else
	/* End this operation */
	em_standby_eeprom(hw);

    /* Relase the EEPROM */
    if (req)
	em_release_eeprom(hw);

    return ok;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM 
 *****************************************************************************/
int32_t
em_read_eeprom(struct em_hw *hw,
                  uint16_t offset,
                  uint16_t *data)
{
    boolean_t ok;

    debug_regs = 0;
    ok = em_eeprom_send_cmd(hw, TRUE, EEPROM_READ_OPCODE, 3, offset, data, 16,
	    FALSE, FALSE);
    debug_regs = 1;

    if (!ok)
	return -E1000_ERR_EEPROM;

    return 0;
}

/******************************************************************************
 * Writes a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to write
 * data - word to wirte to the EEPROM 
 *****************************************************************************/
int32_t
em_write_eeprom(struct em_hw *hw,
                  uint16_t offset,
                  uint16_t data)
{
    boolean_t large_eeprom = FALSE;
    boolean_t ok;

    debug_regs = 0;

/*    DEBUGFUNC("em_write_eeprom");	*/

    if (!em_request_eeprom(hw, &large_eeprom))
    {
	    debug_regs = 1;
	    return -E1000_ERR_EEPROM;
    }

    /* enable the eeprom */
    em_eeprom_send_cmd(hw, FALSE, EEPROM_EWEN_OPCODE, 5, 0, NULL, 0,
	    FALSE, FALSE);

    ok = em_eeprom_send_cmd(hw, FALSE, EEPROM_WRITE_OPCODE, 3, offset,
	    &data, 16, TRUE, TRUE);

    em_eeprom_send_cmd(hw, FALSE, EEPROM_EWDS_OPCODE, 5, 0, NULL, 0,
	    FALSE, FALSE);

    /* Relase the EEPROM */
    em_release_eeprom(hw);
    debug_regs = 1;

    if (!ok)
	return -E1000_ERR_EEPROM;

    return 0;
}

/******************************************************************************
 * Verifies that the EEPROM has a valid checksum
 * 
 * hw - Struct containing variables accessed by shared code
 *
 * Reads the first 64 16 bit words of the EEPROM and sums the values read.
 * If the the sum of the 64 16 bit words is 0xBABA, the EEPROM's checksum is
 * valid.
 *****************************************************************************/
int32_t
em_validate_eeprom_checksum(struct em_hw *hw)
{
    uint16_t checksum = 0;
    uint16_t i, eeprom_data;

    DEBUGFUNC("em_validate_eeprom_checksum");

    for(i = 0; i < (EEPROM_CHECKSUM_REG + 1); i++) {
        if(em_read_eeprom(hw, i, &eeprom_data) < 0) {
            DEBUGOUT("EEPROM Read Error");
            return -E1000_ERR_EEPROM;
        }
        checksum += eeprom_data;
    }

    if(checksum == (uint16_t) EEPROM_SUM) {
        return 0;
    } else {
        DEBUGOUT("EEPROM Checksum Invalid");    
        return -E1000_ERR_EEPROM;
    }
}

/******************************************************************************
 * Reads the adapter's part number from the EEPROM
 *
 * hw - Struct containing variables accessed by shared code
 * part_num - Adapter's part number
 *****************************************************************************/
int32_t
em_read_part_num(struct em_hw *hw,
                    uint32_t *part_num)
{
    uint16_t offset = EEPROM_PBA_BYTE_1;
    uint16_t eeprom_data;

    DEBUGFUNC("em_read_part_num");

    /* Get word 0 from EEPROM */
    if(em_read_eeprom(hw, offset, &eeprom_data) < 0) {
        DEBUGOUT("EEPROM Read Error");
        return -E1000_ERR_EEPROM;
    }
    /* Save word 0 in upper half of part_num */
    *part_num = (uint32_t) (eeprom_data << 16);

    /* Get word 1 from EEPROM */
    if(em_read_eeprom(hw, ++offset, &eeprom_data) < 0) {
        DEBUGOUT("EEPROM Read Error");
        return -E1000_ERR_EEPROM;
    }
    /* Save word 1 in lower half of part_num */
    *part_num |= eeprom_data;

    return 0;
}

/******************************************************************************
 * Reads the adapter's MAC address from the EEPROM and inverts the LSB for the
 * second function of dual function devices
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t
em_read_mac_addr(struct em_hw * hw)
{
    uint16_t offset;
    uint16_t eeprom_data, i;

    DEBUGFUNC("em_read_mac_addr");

    for(i = 0; i < NODE_ADDRESS_SIZE; i += 2) {
        offset = i >> 1;
        if(em_read_eeprom(hw, offset, &eeprom_data) < 0) {
            DEBUGOUT("EEPROM Read Error");
            return -E1000_ERR_EEPROM;
        }
        hw->perm_mac_addr[i] = (uint8_t) (eeprom_data & 0x00FF);
        hw->perm_mac_addr[i+1] = (uint8_t) (eeprom_data >> 8);
    }
    if((hw->mac_type == em_82546) &&
       (E1000_READ_REG(hw, STATUS) & E1000_STATUS_FUNC_1)) {
        if(hw->perm_mac_addr[5] & 0x01)
            hw->perm_mac_addr[5] &= ~(0x01);
        else
            hw->perm_mac_addr[5] |= 0x01;
    }
    for(i = 0; i < NODE_ADDRESS_SIZE; i++)
        hw->mac_addr[i] = hw->perm_mac_addr[i];
    return 0;
}

/******************************************************************************
 * Reset the transmit and receive units; mask and clear all interrupts.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
void
em_reset_hw(struct em_hw *hw)
{
    uint32_t ctrl;
    uint32_t ctrl_ext;
    uint32_t icr;
    uint32_t manc;
    uint16_t pci_cmd_word;

    DEBUGFUNC("em_reset_hw");
    
    /* For 82542 (rev 2.0), disable MWI before issuing a device reset */
    if(hw->mac_type == em_82542_rev2_0) {
        if(hw->pci_cmd_word & CMD_MEM_WRT_INVALIDATE) {
            DEBUGOUT("Disabling MWI on 82542 rev 2.0");
            pci_cmd_word = hw->pci_cmd_word & ~CMD_MEM_WRT_INVALIDATE;
            em_write_pci_cfg(hw, PCI_COMMAND_REGISTER, &pci_cmd_word);
        }
    }

    /* Clear interrupt mask to stop board from generating interrupts */
    DEBUGOUT("Masking off all interrupts");
    E1000_WRITE_REG(hw, IMC, 0xffffffff);

    /* Disable the Transmit and Receive units.  Then delay to allow
     * any pending transactions to complete before we hit the MAC with
     * the global reset.
     */
    E1000_WRITE_REG(hw, RCTL, 0);
    E1000_WRITE_REG(hw, TCTL, E1000_TCTL_PSP);

    /* The tbi_compatibility_on Flag must be cleared when Rctl is cleared. */
    hw->tbi_compatibility_on = FALSE;

    /* Delay to allow any outstanding PCI transactions to complete before
     * resetting the device
     */ 
    msec_delay(10);

    /* Issue a global reset to the MAC.  This will reset the chip's
     * transmit, receive, DMA, and link units.  It will not effect
     * the current PCI configuration.  The global reset bit is self-
     * clearing, and should clear within a microsecond.
     */
    DEBUGOUT("Issuing a global reset to MAC");
    ctrl = E1000_READ_REG(hw, CTRL);
    E1000_WRITE_REG(hw, CTRL, (ctrl | E1000_CTRL_RST));

    /* Force a reload from the EEPROM if necessary */
    if(hw->mac_type < em_82540) {
        /* Wait for reset to complete */
        usec_delay(10);
        ctrl_ext = E1000_READ_REG(hw, CTRL_EXT);
        ctrl_ext |= E1000_CTRL_EXT_EE_RST;
        E1000_WRITE_REG(hw, CTRL_EXT, ctrl_ext);
        /* Wait for EEPROM reload */
        msec_delay(2);
    } else {
        /* Wait for EEPROM reload (it happens automatically) */
        msec_delay(4);
        /* Dissable HW ARPs on ASF enabled adapters */
        manc = E1000_READ_REG(hw, MANC);
        manc &= ~(E1000_MANC_ARP_EN);
        E1000_WRITE_REG(hw, MANC, manc);
    }
    
    /* Clear interrupt mask to stop board from generating interrupts */
    DEBUGOUT("Masking off all interrupts");
    E1000_WRITE_REG(hw, IMC, 0xffffffff);

    /* Clear any pending interrupt events. */
    icr = E1000_READ_REG(hw, ICR);

    /* If MWI was previously enabled, reenable it. */
    if(hw->mac_type == em_82542_rev2_0) {
        if(hw->pci_cmd_word & CMD_MEM_WRT_INVALIDATE)
            em_write_pci_cfg(hw, PCI_COMMAND_REGISTER, &hw->pci_cmd_word);
    }
}

static int32_t
em_id_led_init(struct em_hw * hw)
{
    uint32_t ledctl;
    const uint32_t ledctl_mask = 0x000000FF;
    const uint32_t ledctl_on = E1000_LEDCTL_MODE_LED_ON;
    const uint32_t ledctl_off = E1000_LEDCTL_MODE_LED_OFF;
    uint16_t eeprom_data, i, temp;
    const uint16_t led_mask = 0x0F;
	
    DEBUGFUNC("em_id_led_init");
    
    if(hw->mac_type < em_82540) {
	/* Nothing to do */
	return 0;
    }
    
    ledctl = E1000_READ_REG(hw, LEDCTL);
    hw->ledctl_default = ledctl;
    hw->ledctl_mode1 = hw->ledctl_default;
    hw->ledctl_mode2 = hw->ledctl_default;
        
    if(em_read_eeprom(hw, EEPROM_ID_LED_SETTINGS, &eeprom_data) < 0) {
        DEBUGOUT("EEPROM Read Error");
        return -E1000_ERR_EEPROM;
    }
    if((eeprom_data== ID_LED_RESERVED_0000) || 
       (eeprom_data == ID_LED_RESERVED_FFFF)) eeprom_data = ID_LED_DEFAULT;
    for(i = 0; i < 4; i++) {
        temp = (eeprom_data >> (i << 2)) & led_mask;
        switch(temp) {
	case ID_LED_ON1_DEF2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_ON1_ON2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_ON1_OFF2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_OFF1_DEF2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_off << (i << 3);
		break;
	    }
	case ID_LED_OFF1_ON2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_off << (i << 3);
		break;
	    }
	case ID_LED_OFF1_OFF2:
	    {
		hw->ledctl_mode1 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode1 |= ledctl_off << (i << 3);
		break;
	    }
	default:
	    /* Do nothing */
	    break;
	}
	switch(temp) {
	case ID_LED_DEF1_ON2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_ON1_ON2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_OFF1_ON2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_on << (i << 3);
		break;
	    }
	case ID_LED_DEF1_OFF2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_off << (i << 3);
		break;
	    }
	case ID_LED_ON1_OFF2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_off << (i << 3);
		break;
	    }
	case ID_LED_OFF1_OFF2:
	    {
		hw->ledctl_mode2 &= ~(ledctl_mask << (i << 3));
		hw->ledctl_mode2 |= ledctl_off << (i << 3);
		break;
	    }
	default:
	    /* Do nothing */
	    break;
	}
    }
    return 0;
}

/******************************************************************************
 * Writes a value to the specified offset in the VLAN filter table.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - Offset in VLAN filer table to write
 * value - Value to write into VLAN filter table
 *****************************************************************************/
void
em_write_vfta(struct em_hw *hw,
                 uint32_t offset,
                 uint32_t value)
{
    uint32_t temp;

    if((hw->mac_type == em_82544) && ((offset & 0x1) == 1)) {
        temp = E1000_READ_REG_ARRAY(hw, VFTA, (offset - 1));
        E1000_WRITE_REG_ARRAY(hw, VFTA, offset, value);
        E1000_WRITE_REG_ARRAY(hw, VFTA, (offset - 1), temp);
    } else {
        E1000_WRITE_REG_ARRAY(hw, VFTA, offset, value);
    }
}

/******************************************************************************
 * Clears the VLAN filer table
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
void
em_clear_vfta(struct em_hw *hw)
{
    uint32_t offset;

    for(offset = 0; offset < E1000_VLAN_FILTER_TBL_SIZE; offset++)
        E1000_WRITE_REG_ARRAY(hw, VFTA, offset, 0);
}

/******************************************************************************
* Raises the Management Data Clock
*
* hw - Struct containing variables accessed by shared code
* ctrl - Device control register's current value
******************************************************************************/
static void
em_raise_mdi_clk(struct em_hw *hw,
                    uint32_t *ctrl)
{
    /* Raise the clock input to the Management Data Clock (by setting the MDC
     * bit), and then delay 2 microseconds.
     */
    E1000_WRITE_REG(hw, CTRL, (*ctrl | E1000_CTRL_MDC));
    usec_delay(2);
}

/******************************************************************************
* Lowers the Management Data Clock
*
* hw - Struct containing variables accessed by shared code
* ctrl - Device control register's current value
******************************************************************************/
static void
em_lower_mdi_clk(struct em_hw *hw,
                    uint32_t *ctrl)
{
    /* Lower the clock input to the Management Data Clock (by clearing the MDC
     * bit), and then delay 2 microseconds.
     */
    E1000_WRITE_REG(hw, CTRL, (*ctrl & ~E1000_CTRL_MDC));
    usec_delay(2);
}

/******************************************************************************
* Shifts data bits out to the PHY
*
* hw - Struct containing variables accessed by shared code
* data - Data to send out to the PHY
* count - Number of bits to shift out
*
* Bits are shifted out in MSB to LSB order.
******************************************************************************/
static void
em_shift_out_mdi_bits(struct em_hw *hw,
                         uint32_t data,
                         uint16_t count)
{
    uint32_t ctrl;
    uint32_t mask;

    /* We need to shift "count" number of bits out to the PHY. So, the value
     * in the "data" parameter will be shifted out to the PHY one bit at a 
     * time. In order to do this, "data" must be broken down into bits.
     */
    mask = 0x01;
    mask <<= (count - 1);

    ctrl = E1000_READ_REG(hw, CTRL);

    /* Set MDIO_DIR and MDC_DIR direction bits to be used as output pins. */
    ctrl |= (E1000_CTRL_MDIO_DIR | E1000_CTRL_MDC_DIR);

    while(mask) {
        /* A "1" is shifted out to the PHY by setting the MDIO bit to "1" and
         * then raising and lowering the Management Data Clock. A "0" is
         * shifted out to the PHY by setting the MDIO bit to "0" and then
         * raising and lowering the clock.
         */
        if(data & mask) ctrl |= E1000_CTRL_MDIO;
        else ctrl &= ~E1000_CTRL_MDIO;

        E1000_WRITE_REG(hw, CTRL, ctrl);

        usec_delay(2);

        em_raise_mdi_clk(hw, &ctrl);
        em_lower_mdi_clk(hw, &ctrl);

        mask = mask >> 1;
    }

    /* Clear the data bit just before leaving this routine. */
    ctrl &= ~E1000_CTRL_MDIO;
}

/******************************************************************************
* Shifts data bits in from the PHY
*
* hw - Struct containing variables accessed by shared code
*
* Bits are shifted in in MSB to LSB order. 
******************************************************************************/
static uint16_t
em_shift_in_mdi_bits(struct em_hw *hw)
{
    uint32_t ctrl;
    uint16_t data = 0;
    uint8_t i;

    /* In order to read a register from the PHY, we need to shift in a total
     * of 18 bits from the PHY. The first two bit (turnaround) times are used
     * to avoid contention on the MDIO pin when a read operation is performed.
     * These two bits are ignored by us and thrown away. Bits are "shifted in"
     * by raising the input to the Management Data Clock (setting the MDC bit),
     * and then reading the value of the MDIO bit.
     */ 
    ctrl = E1000_READ_REG(hw, CTRL);

    /* Clear MDIO_DIR (SWDPIO1) to indicate this bit is to be used as input. */
    ctrl &= ~E1000_CTRL_MDIO_DIR;
    ctrl &= ~E1000_CTRL_MDIO;

    E1000_WRITE_REG(hw, CTRL, ctrl);

    /* Raise and Lower the clock before reading in the data. This accounts for
     * the turnaround bits. The first clock occurred when we clocked out the
     * last bit of the Register Address.
     */
    em_raise_mdi_clk(hw, &ctrl);
    em_lower_mdi_clk(hw, &ctrl);

    for(data = 0, i = 0; i < 16; i++) {
        data = data << 1;
        em_raise_mdi_clk(hw, &ctrl);
        ctrl = E1000_READ_REG(hw, CTRL);
        /* Check to see if we shifted in a "1". */
        if(ctrl & E1000_CTRL_MDIO) data |= 1;
        em_lower_mdi_clk(hw, &ctrl);
    }

    em_raise_mdi_clk(hw, &ctrl);
    em_lower_mdi_clk(hw, &ctrl);

    /* Clear the MDIO bit just before leaving this routine. */
    ctrl &= ~E1000_CTRL_MDIO;

    return data;
}

/*****************************************************************************
* Reads the value from a PHY register
*
* hw - Struct containing variables accessed by shared code
* reg_addr - address of the PHY register to read
******************************************************************************/
int32_t
em_read_phy_reg(struct em_hw *hw,
                   uint32_t reg_addr,
                   uint16_t *phy_data)
{
    uint32_t i;
    uint32_t mdic = 0;
    const uint32_t phy_addr = 1;

/*    DEBUGFUNC("em_read_phy_reg");	*/

    if(reg_addr > MAX_PHY_REG_ADDRESS) {
        DEBUGOUT1("PHY Address %d is out of range ", reg_addr);
        return -E1000_ERR_PARAM;
    }

    if(hw->mac_type > em_82543) {
	int ready = 0;
        /* Set up Op-code, Phy Address, and register address in the MDI
         * Control register.  The MAC will take care of interfacing with the
         * PHY to retrieve the desired data.
         */
        mdic = ((reg_addr << E1000_MDIC_REG_SHIFT) |
                (phy_addr << E1000_MDIC_PHY_SHIFT) | 
                (E1000_MDIC_OP_READ));

        E1000_WRITE_REG(hw, MDIC, mdic);

        /* Poll the ready bit to see if the MDI read completed */
        for(i = 0; i < 64 && !ready; i++) {
            usec_delay(10);
            mdic = E1000_READ_REG(hw, MDIC);
            if(mdic & E1000_MDIC_READY) ready = 1;
        }
        if(!(mdic & E1000_MDIC_READY)) {
            DEBUGOUT("MDI Read did not complete");
            return -E1000_ERR_PHY;
        }
        if(mdic & E1000_MDIC_ERROR) {
            DEBUGOUT("MDI Error");
            return -E1000_ERR_PHY;
        }
        *phy_data = (uint16_t) mdic;
    } else {
        /* We must first send a preamble through the MDIO pin to signal the
         * beginning of an MII instruction.  This is done by sending 32
         * consecutive "1" bits.
         */
        em_shift_out_mdi_bits(hw, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

        /* Now combine the next few fields that are required for a read
         * operation.  We use this method instead of calling the
         * em_shift_out_mdi_bits routine five different times. The format of
         * a MII read instruction consists of a shift out of 14 bits and is
         * defined as follows:
         *    <Preamble><SOF><Op Code><Phy Addr><Reg Addr>
         * followed by a shift in of 18 bits.  This first two bits shifted in
         * are TurnAround bits used to avoid contention on the MDIO pin when a
         * READ operation is performed.  These two bits are thrown away
         * followed by a shift in of 16 bits which contains the desired data.
         */
        mdic = ((reg_addr) | (phy_addr << 5) | 
                (PHY_OP_READ << 10) | (PHY_SOF << 12));

        em_shift_out_mdi_bits(hw, mdic, 14);

        /* Now that we've shifted out the read command to the MII, we need to
         * "shift in" the 16-bit value (18 total bits) of the requested PHY
         * register address.
         */
        *phy_data = em_shift_in_mdi_bits(hw);
    }
    return 0;
}

/******************************************************************************
* Writes a value to a PHY register
*
* hw - Struct containing variables accessed by shared code
* reg_addr - address of the PHY register to write
* data - data to write to the PHY
******************************************************************************/
int32_t
em_write_phy_reg(struct em_hw *hw,
                    uint32_t reg_addr,
                    uint16_t phy_data)
{
    uint32_t i;
    uint32_t mdic = 0;
    const uint32_t phy_addr = 1;

    DEBUGFUNC("em_write_phy_reg");

    if(reg_addr > MAX_PHY_REG_ADDRESS) {
        DEBUGOUT1("PHY Address %d is out of range ", reg_addr);
        return -E1000_ERR_PARAM;
    }

    if(hw->mac_type > em_82543) {
	int ready = 0;
        /* Set up Op-code, Phy Address, register address, and data intended
         * for the PHY register in the MDI Control register.  The MAC will take
         * care of interfacing with the PHY to send the desired data.
         */
        mdic = (((uint32_t) phy_data) |
                (reg_addr << E1000_MDIC_REG_SHIFT) |
                (phy_addr << E1000_MDIC_PHY_SHIFT) | 
                (E1000_MDIC_OP_WRITE));

        E1000_WRITE_REG(hw, MDIC, mdic);

        /* Poll the ready bit to see if the MDI read completed */
        for(i = 0; i < 64 && !ready; i++) {
            usec_delay(10);
            mdic = E1000_READ_REG(hw, MDIC);
            if(mdic & E1000_MDIC_READY) ready = 1;
        }
        if(!(mdic & E1000_MDIC_READY)) {
            DEBUGOUT("MDI Write did not complete");
            return -E1000_ERR_PHY;
        }
    } else {
        /* We'll need to use the SW defined pins to shift the write command
         * out to the PHY. We first send a preamble to the PHY to signal the
         * beginning of the MII instruction.  This is done by sending 32 
         * consecutive "1" bits.
         */
        em_shift_out_mdi_bits(hw, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

        /* Now combine the remaining required fields that will indicate a 
         * write operation. We use this method instead of calling the
         * em_shift_out_mdi_bits routine for each field in the command. The
         * format of a MII write instruction is as follows:
         * <Preamble><SOF><Op Code><Phy Addr><Reg Addr><Turnaround><Data>.
         */
        mdic = ((PHY_TURNAROUND) | (reg_addr << 2) | (phy_addr << 7) |
                (PHY_OP_WRITE << 12) | (PHY_SOF << 14));
        mdic <<= 16;
        mdic |= (uint32_t) phy_data;

        em_shift_out_mdi_bits(hw, mdic, 32);
    }
    return 0;
}

/******************************************************************************
* Returns the PHY to the power-on reset state
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
void
em_phy_hw_reset(struct em_hw *hw)
{
    uint32_t ctrl;
    uint32_t ctrl_ext;

    DEBUGFUNC("em_phy_hw_reset");

    DEBUGOUT("Resetting Phy...");

    if(hw->mac_type > em_82543) {
        /* Read the device control register and assert the E1000_CTRL_PHY_RST
         * bit. Then, take it out of reset.
         */
        ctrl = E1000_READ_REG(hw, CTRL);
        E1000_WRITE_REG(hw, CTRL, ctrl | E1000_CTRL_PHY_RST);
        msec_delay(10);
        E1000_WRITE_REG(hw, CTRL, ctrl);
    } else {
        /* Read the Extended Device Control Register, assert the PHY_RESET_DIR
         * bit to put the PHY into reset. Then, take it out of reset.
         */
        ctrl_ext = E1000_READ_REG(hw, CTRL_EXT);
        ctrl_ext |= E1000_CTRL_EXT_SDP4_DIR;
        ctrl_ext &= ~E1000_CTRL_EXT_SDP4_DATA;
        E1000_WRITE_REG(hw, CTRL_EXT, ctrl_ext);
        msec_delay(10);
        ctrl_ext |= E1000_CTRL_EXT_SDP4_DATA;
        E1000_WRITE_REG(hw, CTRL_EXT, ctrl_ext);
    }
    usec_delay(150);
}

/******************************************************************************
* Resets the PHY
*
* hw - Struct containing variables accessed by shared code
*
* Sets bit 15 of the MII Control regiser
******************************************************************************/
int32_t
em_phy_reset(struct em_hw *hw)
{
    uint16_t phy_data;

    DEBUGFUNC("em_phy_reset");

    if(em_read_phy_reg(hw, PHY_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    phy_data |= MII_CR_RESET;
    if(em_write_phy_reg(hw, PHY_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }
    usec_delay(1);
    return 0;
}

/******************************************************************************
* Probes the expected PHY address for known PHY IDs
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
int32_t
em_detect_gig_phy(struct em_hw *hw)
{
    uint16_t phy_id_high, phy_id_low;
    boolean_t match = FALSE;

    DEBUGFUNC("em_detect_gig_phy");

    /* Read the PHY ID Registers to identify which PHY is onboard. */
    if(em_read_phy_reg(hw, PHY_ID1, &phy_id_high) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    hw->phy_id = (uint32_t) (phy_id_high << 16);
    usec_delay(2);
    if(em_read_phy_reg(hw, PHY_ID2, &phy_id_low) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    hw->phy_id |= (uint32_t) (phy_id_low & PHY_REVISION_MASK);
    
    switch(hw->mac_type) {
    case em_82543:
	{
	    if(hw->phy_id == M88E1000_E_PHY_ID) match = TRUE;
	    break;
	}
    case em_82544:
	{
	    if(hw->phy_id == M88E1000_I_PHY_ID) match = TRUE;
	    break;
	}
    case em_82540:
	{
	    if(hw->phy_id == M88E1011_I_PHY_ID) match = TRUE;
	    break;
	}
    case em_82545:
	{
	    if(hw->phy_id == M88E1011_I_PHY_ID) match = TRUE;
	    break;
	}
    case em_82546:
	{
	    if(hw->phy_id == M88E1011_I_PHY_ID) match = TRUE;
	    break;
	}
    default:
	{
	    DEBUGOUT1("Invalid MAC type %d ", hw->mac_type);
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }
    if(match) {
        DEBUGOUT1("PHY ID 0x%X detected ", hw->phy_id);
        return 0;
    }
    DEBUGOUT1("Invalid PHY ID 0x%X ", hw->phy_id);
    return -E1000_ERR_PHY;
}

/******************************************************************************
* Resets the PHY's DSP
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t
em_phy_reset_dsp(struct em_hw *hw)
{
    int32_t ret_val = -E1000_ERR_PHY;
    DEBUGFUNC("em_phy_reset_dsp");
    
    if(em_write_phy_reg(hw, 29, 0x001d) < 0)
	;
    else if(em_write_phy_reg(hw, 30, 0x00c1) < 0)
	;
    else if(em_write_phy_reg(hw, 30, 0x0000) < 0)
	;
    else
        ret_val = 0;

    if(ret_val < 0) DEBUGOUT("PHY Write Error");
    return ret_val;
}

/******************************************************************************
* Get PHY information from various PHY registers
*
* hw - Struct containing variables accessed by shared code
* phy_info - PHY information structure
******************************************************************************/
int32_t
em_phy_get_info(struct em_hw *hw,
                   struct em_phy_info *phy_info)
{
    int32_t ret_val = -E1000_ERR_PHY;
    uint16_t phy_data;

    DEBUGFUNC("em_phy_get_info");

    phy_info->cable_length = em_cable_length_undefined;
    phy_info->extended_10bt_distance = em_10bt_ext_dist_enable_undefined;
    phy_info->cable_polarity = em_rev_polarity_undefined;
    phy_info->polarity_correction = em_polarity_reversal_undefined;
    phy_info->mdix_mode = em_auto_x_mode_undefined;
    phy_info->local_rx = em_1000t_rx_status_undefined;
    phy_info->remote_rx = em_1000t_rx_status_undefined;

    if(hw->media_type != em_media_type_copper) {
        DEBUGOUT("PHY info is only valid for copper media");
        return -E1000_ERR_CONFIG;
    }

    if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0)
	;
    else if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0)
	;
    else
    {
        if((phy_data & MII_SR_LINK_STATUS) != MII_SR_LINK_STATUS) {
            DEBUGOUT("PHY info is only valid if link is up");
            return -E1000_ERR_CONFIG;
        }

        if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data) < 0)
            ;
	else
	{
	    phy_info->extended_10bt_distance =
		(phy_data & M88E1000_PSCR_10BT_EXT_DIST_ENABLE) >>
		M88E1000_PSCR_10BT_EXT_DIST_ENABLE_SHIFT;
	    phy_info->polarity_correction =
		(phy_data & M88E1000_PSCR_POLARITY_REVERSAL) >>
		M88E1000_PSCR_POLARITY_REVERSAL_SHIFT;

	    if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data) < 0)
		;
	    else
	    {
		phy_info->cable_polarity = (phy_data & M88E1000_PSSR_REV_POLARITY) >>
		    M88E1000_PSSR_REV_POLARITY_SHIFT;
		phy_info->mdix_mode = (phy_data & M88E1000_PSSR_MDIX) >>
		    M88E1000_PSSR_MDIX_SHIFT;
		if(phy_data & M88E1000_PSSR_1000MBS) {
		    /* Cable Length Estimation and Local/Remote Receiver Informatoion
		     * are only valid at 1000 Mbps
		     */
		    phy_info->cable_length = ((phy_data & M88E1000_PSSR_CABLE_LENGTH) >>
					      M88E1000_PSSR_CABLE_LENGTH_SHIFT);
		    if(em_read_phy_reg(hw, PHY_1000T_STATUS, &phy_data) < 0) 
			;
		    else
		    {
			phy_info->local_rx = (phy_data & SR_1000T_LOCAL_RX_STATUS) >>
			    SR_1000T_LOCAL_RX_STATUS_SHIFT;
			phy_info->remote_rx = (phy_data & SR_1000T_REMOTE_RX_STATUS) >>
			    SR_1000T_REMOTE_RX_STATUS_SHIFT;
			ret_val = 0;
		    }
		}
		else
		    ret_val = 0;
	    }
	}
    }

    if(ret_val < 0) DEBUGOUT("PHY Read Error");
    return ret_val;
}

int32_t
em_validate_mdi_setting(struct em_hw *hw)
{
    DEBUGFUNC("em_validate_mdi_settings");

    if(!hw->autoneg && (hw->mdix == 0 || hw->mdix == 3)) {
        DEBUGOUT("Invalid MDI setting detected");
        hw->mdix = 1;
        return -E1000_ERR_CONFIG;
    }
    return 0;
}

/******************************************************************************
* Configures PHY autoneg and flow control advertisement settings
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
int32_t
em_phy_setup_autoneg(struct em_hw *hw)
{
    uint16_t mii_autoneg_adv_reg;
    uint16_t mii_1000t_ctrl_reg;

    DEBUGFUNC("em_phy_setup_autoneg");

    /* Read the MII Auto-Neg Advertisement Register (Address 4). */
    if(em_read_phy_reg(hw, PHY_AUTONEG_ADV, &mii_autoneg_adv_reg) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }

    /* Read the MII 1000Base-T Control Register (Address 9). */
    if(em_read_phy_reg(hw, PHY_1000T_CTRL, &mii_1000t_ctrl_reg) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }

    /* Need to parse both autoneg_advertised and fc and set up
     * the appropriate PHY registers.  First we will parse for
     * autoneg_advertised software override.  Since we can advertise
     * a plethora of combinations, we need to check each bit
     * individually.
     */

    /* First we clear all the 10/100 mb speed bits in the Auto-Neg
     * Advertisement Register (Address 4) and the 1000 mb speed bits in
     * the  1000Base-T Control Register (Address 9).
     */
    mii_autoneg_adv_reg &= ~REG4_SPEED_MASK;
    mii_1000t_ctrl_reg &= ~REG9_SPEED_MASK;

    DEBUGOUT1("autoneg_advertised %x ", hw->autoneg_advertised);

    /* Do we want to advertise 10 Mb Half Duplex? */
    if(hw->autoneg_advertised & ADVERTISE_10_HALF) {
        DEBUGOUT("Advertise 10mb Half duplex");
        mii_autoneg_adv_reg |= NWAY_AR_10T_HD_CAPS;
    }

    /* Do we want to advertise 10 Mb Full Duplex? */
    if(hw->autoneg_advertised & ADVERTISE_10_FULL) {
        DEBUGOUT("Advertise 10mb Full duplex");
        mii_autoneg_adv_reg |= NWAY_AR_10T_FD_CAPS;
    }

    /* Do we want to advertise 100 Mb Half Duplex? */
    if(hw->autoneg_advertised & ADVERTISE_100_HALF) {
        DEBUGOUT("Advertise 100mb Half duplex");
        mii_autoneg_adv_reg |= NWAY_AR_100TX_HD_CAPS;
    }

    /* Do we want to advertise 100 Mb Full Duplex? */
    if(hw->autoneg_advertised & ADVERTISE_100_FULL) {
        DEBUGOUT("Advertise 100mb Full duplex");
        mii_autoneg_adv_reg |= NWAY_AR_100TX_FD_CAPS;
    }

    /* We do not allow the Phy to advertise 1000 Mb Half Duplex */
    if(hw->autoneg_advertised & ADVERTISE_1000_HALF) {
        DEBUGOUT("Advertise 1000mb Half duplex requested, request denied!");
    }

    /* Do we want to advertise 1000 Mb Full Duplex? */
    if(hw->autoneg_advertised & ADVERTISE_1000_FULL) {
        DEBUGOUT("Advertise 1000mb Full duplex");
        mii_1000t_ctrl_reg |= CR_1000T_FD_CAPS;
    }

    /* Check for a software override of the flow control settings, and
     * setup the PHY advertisement registers accordingly.  If
     * auto-negotiation is enabled, then software will have to set the
     * "PAUSE" bits to the correct value in the Auto-Negotiation
     * Advertisement Register (PHY_AUTONEG_ADV) and re-start auto-negotiation.
     *
     * The possible values of the "fc" parameter are:
     *      0:  Flow control is completely disabled
     *      1:  Rx flow control is enabled (we can receive pause frames
     *          but not send pause frames).
     *      2:  Tx flow control is enabled (we can send pause frames
     *          but we do not support receiving pause frames).
     *      3:  Both Rx and TX flow control (symmetric) are enabled.
     *  other:  No software override.  The flow control configuration
     *          in the EEPROM is used.
     */
    switch (hw->fc) {
    case em_fc_none: /* 0 */
        /* Flow control (RX & TX) is completely disabled by a
         * software over-ride.
         */
	{
	    mii_autoneg_adv_reg &= ~(NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
	    break;
	}
    case em_fc_rx_pause: /* 1 */
        /* RX Flow control is enabled, and TX Flow control is
         * disabled, by a software over-ride.
         */
        /* Since there really isn't a way to advertise that we are
         * capable of RX Pause ONLY, we will advertise that we
         * support both symmetric and asymmetric RX PAUSE.  Later
         * (in em_config_fc_after_link_up) we will disable the
         *hw's ability to send PAUSE frames.
         */
	{
	    mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
	    break;
	}
    case em_fc_tx_pause: /* 2 */
        /* TX Flow control is enabled, and RX Flow control is
         * disabled, by a software over-ride.
         */
	{
	    mii_autoneg_adv_reg |= NWAY_AR_ASM_DIR;
	    mii_autoneg_adv_reg &= ~NWAY_AR_PAUSE;
	    break;
	}
    case em_fc_full: /* 3 */
        /* Flow control (both RX and TX) is enabled by a software
         * over-ride.
         */
	{
	    mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
	    break;
	}
    default:
	{
	    DEBUGOUT("Flow control param set incorrectly");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }

    if(em_write_phy_reg(hw, PHY_AUTONEG_ADV, mii_autoneg_adv_reg) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }

    DEBUGOUT1("Auto-Neg Advertising %x ", mii_autoneg_adv_reg);

    if(em_write_phy_reg(hw, PHY_1000T_CTRL, mii_1000t_ctrl_reg) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }
    return 0;
}

/******************************************************************************
* Sets the collision distance in the Transmit Control register
*
* hw - Struct containing variables accessed by shared code
*
* Link should have been established previously. Reads the speed and duplex
* information from the Device Status register.
******************************************************************************/
void
em_config_collision_dist(struct em_hw *hw)
{
    uint32_t tctl;

    tctl = E1000_READ_REG(hw, TCTL);

    tctl &= ~E1000_TCTL_COLD;
    tctl |= E1000_COLLISION_DISTANCE << E1000_COLD_SHIFT;

    E1000_WRITE_REG(hw, TCTL, tctl);
}

/******************************************************************************
* Force PHY speed and duplex settings to hw->forced_speed_duplex
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t
em_phy_force_speed_duplex(struct em_hw *hw)
{
    uint32_t ctrl;
    int32_t ret_val;
    uint16_t mii_ctrl_reg;
    uint16_t mii_status_reg;
    uint16_t phy_data;
    uint16_t i;

    DEBUGFUNC("em_phy_force_speed_duplex");

    /* Turn off Flow control if we are forcing speed and duplex. */
    hw->fc = em_fc_none;

    DEBUGOUT1("hw->fc = %d ", hw->fc);

    /* Read the Device Control Register. */
    ctrl = E1000_READ_REG(hw, CTRL);

    /* Set the bits to Force Speed and Duplex in the Device Ctrl Reg. */
    ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
    ctrl &= ~(DEVICE_SPEED_MASK);

    /* Clear the Auto Speed Detect Enable bit. */
    ctrl &= ~E1000_CTRL_ASDE;

    /* Read the MII Control Register. */
    if(em_read_phy_reg(hw, PHY_CTRL, &mii_ctrl_reg) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }

    /* We need to disable autoneg in order to force link and duplex. */

    mii_ctrl_reg &= ~MII_CR_AUTO_NEG_EN;

    /* Are we forcing Full or Half Duplex? */
    if(hw->forced_speed_duplex == em_100_full ||
       hw->forced_speed_duplex == em_10_full) {
        /* We want to force full duplex so we SET the full duplex bits in the
         * Device and MII Control Registers.
         */
        ctrl |= E1000_CTRL_FD;
        mii_ctrl_reg |= MII_CR_FULL_DUPLEX;
        DEBUGOUT("Full Duplex");
    } else {
        /* We want to force half duplex so we CLEAR the full duplex bits in
         * the Device and MII Control Registers.
         */
        ctrl &= ~E1000_CTRL_FD;
        mii_ctrl_reg &= ~MII_CR_FULL_DUPLEX;
        DEBUGOUT("Half Duplex");
    }

    /* Are we forcing 100Mbps??? */
    if(hw->forced_speed_duplex == em_100_full ||
       hw->forced_speed_duplex == em_100_half) {
        /* Set the 100Mb bit and turn off the 1000Mb and 10Mb bits. */
        ctrl |= E1000_CTRL_SPD_100;
        mii_ctrl_reg |= MII_CR_SPEED_100;
        mii_ctrl_reg &= ~(MII_CR_SPEED_1000 | MII_CR_SPEED_10);
        DEBUGOUT("Forcing 100mb ");
    } else {
        /* Set the 10Mb bit and turn off the 1000Mb and 100Mb bits. */
        ctrl &= ~(E1000_CTRL_SPD_1000 | E1000_CTRL_SPD_100);
        mii_ctrl_reg |= MII_CR_SPEED_10;
        mii_ctrl_reg &= ~(MII_CR_SPEED_1000 | MII_CR_SPEED_100);
        DEBUGOUT("Forcing 10mb ");
    }

    em_config_collision_dist(hw);

    /* Write the configured values back to the Device Control Reg. */
    E1000_WRITE_REG(hw, CTRL, ctrl);

    /* Write the MII Control Register with the new PHY configuration. */
    if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }

    /* Clear Auto-Crossover to force MDI manually. M88E1000 requires MDI
     * forced whenever speed are duplex are forced.
     */
    phy_data &= ~M88E1000_PSCR_AUTO_X_MODE;
    if(em_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }
    DEBUGOUT1("M88E1000 PSCR: %x ", phy_data);
    
    /* Need to reset the PHY or these changes will be ignored */
    mii_ctrl_reg |= MII_CR_RESET;
    if(em_write_phy_reg(hw, PHY_CTRL, mii_ctrl_reg) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }
    usec_delay(1);

    /* The wait_autoneg_complete flag may be a little misleading here.
     * Since we are forcing speed and duplex, Auto-Neg is not enabled.
     * But we do want to delay for a period while forcing only so we
     * don't generate false No Link messages.  So we will wait here
     * only if the user has set wait_autoneg_complete to 1, which is
     * the default.
     */
    if(hw->wait_autoneg_complete) {
	int ready = 0;
        /* We will wait for autoneg to complete. */
        DEBUGOUT("Waiting for forced speed/duplex link.");
        mii_status_reg = 0;

        /* We will wait for autoneg to complete or 4.5 seconds to expire. */
        for(i = PHY_FORCE_TIME; i > 0 && !ready; i--) {
            /* Read the MII Status Register and wait for Auto-Neg Complete bit
             * to be set.
             */
            if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
                DEBUGOUT("PHY Read Error");
                return -E1000_ERR_PHY;
            }
            if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
                DEBUGOUT("PHY Read Error");
                return -E1000_ERR_PHY;
            }
            if(mii_status_reg & MII_SR_LINK_STATUS)
		ready = 1;
	    else
		msec_delay(100);
        }
        if(i == 0) { /* We didn't get link */
            /* Reset the DSP and wait again for link. */
            
            ret_val = em_phy_reset_dsp(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error Resetting PHY DSP");
                return ret_val;
            }
        }

	ready = 0;

        /* This loop will early-out if the link condition has been met.  */
        for(i = PHY_FORCE_TIME; i > 0 && !ready; i--) {
            if(mii_status_reg & MII_SR_LINK_STATUS)
		ready = 1;
	    else
	    {
		msec_delay(100);
		/* Read the MII Status Register and wait for Auto-Neg Complete bit
		 * to be set.
		 */
		if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
		    DEBUGOUT("PHY Read Error");
		    return -E1000_ERR_PHY;
		}
		if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
		    DEBUGOUT("PHY Read Error");
		    return -E1000_ERR_PHY;
		}
	    }
        }
    }
    
    /* Because we reset the PHY above, we need to re-force TX_CLK in the
     * Extended PHY Specific Control Register to 25MHz clock.  This value
     * defaults back to a 2.5MHz clock when the PHY is reset.
     */
    if(em_read_phy_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    phy_data |= M88E1000_EPSCR_TX_CLK_25;
    if(em_write_phy_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }

    /* In addition, because of the s/w reset above, we need to enable CRS on
     * TX.  This must be set for both full and half duplex operation.
     */
    if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    phy_data |= M88E1000_PSCR_ASSERT_CRS_ON_TX;
    if(em_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }
    return 0;
}

/******************************************************************************
* Sets MAC speed and duplex settings to reflect the those in the PHY
*
* hw - Struct containing variables accessed by shared code
* mii_reg - data to write to the MII control register
*
* The contents of the PHY register containing the needed information need to
* be passed in.
******************************************************************************/
static int32_t
em_config_mac_to_phy(struct em_hw *hw)
{
    uint32_t ctrl;
    uint16_t phy_data;

    DEBUGFUNC("em_config_mac_to_phy");

    /* Read the Device Control Register and set the bits to Force Speed
     * and Duplex.
     */
    ctrl = E1000_READ_REG(hw, CTRL);
    ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
    ctrl &= ~(E1000_CTRL_SPD_SEL | E1000_CTRL_ILOS);

    /* Set up duplex in the Device Control and Transmit Control
     * registers depending on negotiated values.
     */
    if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    if(phy_data & M88E1000_PSSR_DPLX) ctrl |= E1000_CTRL_FD;
    else ctrl &= ~E1000_CTRL_FD;

    em_config_collision_dist(hw);

    /* Set up speed in the Device Control register depending on
     * negotiated values.
     */
    if((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_1000MBS)
        ctrl |= E1000_CTRL_SPD_1000;
    else if((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_100MBS)
        ctrl |= E1000_CTRL_SPD_100;
    /* Write the configured values back to the Device Control Reg. */
    E1000_WRITE_REG(hw, CTRL, ctrl);
    return 0;
}

/******************************************************************************
 * Forces the MAC's flow control settings.
 * 
 * hw - Struct containing variables accessed by shared code
 *
 * Sets the TFCE and RFCE bits in the device control register to reflect
 * the adapter settings. TFCE and RFCE need to be explicitly set by
 * software when a Copper PHY is used because autonegotiation is managed
 * by the PHY rather than the MAC. Software must also configure these
 * bits when link is forced on a fiber connection.
 *****************************************************************************/
static int32_t
em_force_mac_fc(struct em_hw *hw)
{
    uint32_t ctrl;

    DEBUGFUNC("em_force_mac_fc");

    /* Get the current configuration of the Device Control Register */
    ctrl = E1000_READ_REG(hw, CTRL);

    /* Because we didn't get link via the internal auto-negotiation
     * mechanism (we either forced link or we got link via PHY
     * auto-neg), we have to manually enable/disable transmit an
     * receive flow control.
     *
     * The "Case" statement below enables/disable flow control
     * according to the "hw->fc" parameter.
     *
     * The possible values of the "fc" parameter are:
     *      0:  Flow control is completely disabled
     *      1:  Rx flow control is enabled (we can receive pause
     *          frames but not send pause frames).
     *      2:  Tx flow control is enabled (we can send pause frames
     *          frames but we do not receive pause frames).
     *      3:  Both Rx and TX flow control (symmetric) is enabled.
     *  other:  No other values should be possible at this point.
     */

    switch (hw->fc) {
    case em_fc_none:
	{
	    ctrl &= (~(E1000_CTRL_TFCE | E1000_CTRL_RFCE));
	    break;
	}
    case em_fc_rx_pause:
	{
	    ctrl &= (~E1000_CTRL_TFCE);
	    ctrl |= E1000_CTRL_RFCE;
	    break;
	}
    case em_fc_tx_pause:
	{
	    ctrl &= (~E1000_CTRL_RFCE);
	    ctrl |= E1000_CTRL_TFCE;
	    break;
	}
    case em_fc_full:
	{
	    ctrl |= (E1000_CTRL_TFCE | E1000_CTRL_RFCE);
	    break;
	}
    default:
	{
	    DEBUGOUT("Flow control param set incorrectly");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }

    /* Disable TX Flow Control for 82542 (rev 2.0) */
    if(hw->mac_type == em_82542_rev2_0)
        ctrl &= (~E1000_CTRL_TFCE);

    E1000_WRITE_REG(hw, CTRL, ctrl);
    return 0;
}

/******************************************************************************
 * Detects the current speed and duplex settings of the hardware.
 *
 * hw - Struct containing variables accessed by shared code
 * speed - Speed of the connection
 * duplex - Duplex setting of the connection
 *****************************************************************************/
void
em_get_speed_and_duplex(struct em_hw *hw,
                           uint16_t *speed,
                           uint16_t *duplex)
{
    uint32_t status;

    DEBUGFUNC("em_get_speed_and_duplex");

    if(hw->mac_type >= em_82543) {
        status = E1000_READ_REG(hw, STATUS);
        if(status & E1000_STATUS_SPEED_1000) {
            *speed = SPEED_1000;
            DEBUGOUT("1000 Mbs, ");
        } else if(status & E1000_STATUS_SPEED_100) {
            *speed = SPEED_100;
            DEBUGOUT("100 Mbs, ");
        } else {
            *speed = SPEED_10;
            DEBUGOUT("10 Mbs, ");
        }

        if(status & E1000_STATUS_FD) {
            *duplex = FULL_DUPLEX;
            DEBUGOUT("Full Duplex");
        } else {
            *duplex = HALF_DUPLEX;
            DEBUGOUT(" Half Duplex");
        }
    } else {
        DEBUGOUT("1000 Mbs, Full Duplex");
        *speed = SPEED_1000;
        *duplex = FULL_DUPLEX;
    }
}

/******************************************************************************
 * Configures flow control settings after link is established
 * 
 * hw - Struct containing variables accessed by shared code
 *
 * Should be called immediately after a valid link has been established.
 * Forces MAC flow control settings if link was forced. When in MII/GMII mode
 * and autonegotiation is enabled, the MAC flow control settings will be set
 * based on the flow control negotiated by the PHY. In TBI mode, the TFCE
 * and RFCE bits will be automaticaly set to the negotiated flow control mode.
 *****************************************************************************/
int32_t
em_config_fc_after_link_up(struct em_hw *hw)
{
    int32_t ret_val;
    uint16_t mii_status_reg;
    uint16_t mii_nway_adv_reg;
    uint16_t mii_nway_lp_ability_reg;
    uint16_t speed;
    uint16_t duplex;

    DEBUGFUNC("em_config_fc_after_link_up");

    /* Check for the case where we have fiber media and auto-neg failed
     * so we had to force link.  In this case, we need to force the
     * configuration of the MAC to match the "fc" parameter.
     */
    if(((hw->media_type == em_media_type_fiber) && (hw->autoneg_failed)) ||
       ((hw->media_type == em_media_type_copper) && (!hw->autoneg))) {
        ret_val = em_force_mac_fc(hw);
        if(ret_val < 0) {
            DEBUGOUT("Error forcing flow control settings");
            return ret_val;
        }
    }

    /* Check for the case where we have copper media and auto-neg is
     * enabled.  In this case, we need to check and see if Auto-Neg
     * has completed, and if so, how the PHY and link partner has
     * flow control configured.
     */
    if((hw->media_type == em_media_type_copper) && hw->autoneg) {
        /* Read the MII Status Register and check to see if AutoNeg
         * has completed.  We read this twice because this reg has
         * some "sticky" (latched) bits.
         */
        if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
            DEBUGOUT("PHY Read Error ");
            return -E1000_ERR_PHY;
        }
        if(em_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
            DEBUGOUT("PHY Read Error ");
            return -E1000_ERR_PHY;
        }

        if(mii_status_reg & MII_SR_AUTONEG_COMPLETE) {
            /* The AutoNeg process has completed, so we now need to
             * read both the Auto Negotiation Advertisement Register
             * (Address 4) and the Auto_Negotiation Base Page Ability
             * Register (Address 5) to determine how flow control was
             * negotiated.
             */
            if(em_read_phy_reg(hw, PHY_AUTONEG_ADV, &mii_nway_adv_reg) < 0) {
                DEBUGOUT("PHY Read Error");
                return -E1000_ERR_PHY;
            }
            if(em_read_phy_reg(hw, PHY_LP_ABILITY, &mii_nway_lp_ability_reg) < 0) {
                DEBUGOUT("PHY Read Error");
                return -E1000_ERR_PHY;
            }

            /* Two bits in the Auto Negotiation Advertisement Register
             * (Address 4) and two bits in the Auto Negotiation Base
             * Page Ability Register (Address 5) determine flow control
             * for both the PHY and the link partner.  The following
             * table, taken out of the IEEE 802.3ab/D6.0 dated March 25,
             * 1999, describes these PAUSE resolution bits and how flow
             * control is determined based upon these settings.
             * NOTE:  DC = Don't Care
             *
             *   LOCAL DEVICE  |   LINK PARTNER
             * PAUSE | ASM_DIR | PAUSE | ASM_DIR | NIC Resolution
             *-------|---------|-------|---------|--------------------
             *   0   |    0    |  DC   |   DC    | em_fc_none
             *   0   |    1    |   0   |   DC    | em_fc_none
             *   0   |    1    |   1   |    0    | em_fc_none
             *   0   |    1    |   1   |    1    | em_fc_tx_pause
             *   1   |    0    |   0   |   DC    | em_fc_none
             *   1   |   DC    |   1   |   DC    | em_fc_full
             *   1   |    1    |   0   |    0    | em_fc_none
             *   1   |    1    |   0   |    1    | em_fc_rx_pause
             *
             */
            /* Are both PAUSE bits set to 1?  If so, this implies
             * Symmetric Flow Control is enabled at both ends.  The
             * ASM_DIR bits are irrelevant per the spec.
             *
             * For Symmetric Flow Control:
             *
             *   LOCAL DEVICE  |   LINK PARTNER
             * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
             *-------|---------|-------|---------|--------------------
             *   1   |   DC    |   1   |   DC    | em_fc_full
             *
             */
            if((mii_nway_adv_reg & NWAY_AR_PAUSE) &&
               (mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE)) {
                /* Now we need to check if the user selected RX ONLY
                 * of pause frames.  In this case, we had to advertise
                 * FULL flow control because we could not advertise RX
                 * ONLY. Hence, we must now check to see if we need to
                 * turn OFF  the TRANSMISSION of PAUSE frames.
                 */
                if(hw->original_fc == em_fc_full) {
                    hw->fc = em_fc_full;
                    DEBUGOUT("Flow Control = FULL.");
                } else {
                    hw->fc = em_fc_rx_pause;
                    DEBUGOUT("Flow Control = RX PAUSE frames only.");
                }
            }
            /* For receiving PAUSE frames ONLY.
             *
             *   LOCAL DEVICE  |   LINK PARTNER
             * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
             *-------|---------|-------|---------|--------------------
             *   0   |    1    |   1   |    1    | em_fc_tx_pause
             *
             */
            else if(!(mii_nway_adv_reg & NWAY_AR_PAUSE) &&
                    (mii_nway_adv_reg & NWAY_AR_ASM_DIR) &&
                    (mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE) &&
                    (mii_nway_lp_ability_reg & NWAY_LPAR_ASM_DIR)) {
                hw->fc = em_fc_tx_pause;
                DEBUGOUT("Flow Control = TX PAUSE frames only.");
            }
            /* For transmitting PAUSE frames ONLY.
             *
             *   LOCAL DEVICE  |   LINK PARTNER
             * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
             *-------|---------|-------|---------|--------------------
             *   1   |    1    |   0   |    1    | em_fc_rx_pause
             *
             */
            else if((mii_nway_adv_reg & NWAY_AR_PAUSE) &&
                    (mii_nway_adv_reg & NWAY_AR_ASM_DIR) &&
                    !(mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE) &&
                    (mii_nway_lp_ability_reg & NWAY_LPAR_ASM_DIR)) {
                hw->fc = em_fc_rx_pause;
                DEBUGOUT("Flow Control = RX PAUSE frames only.");
            }
            /* Per the IEEE spec, at this point flow control should be
             * disabled.  However, we want to consider that we could
             * be connected to a legacy switch that doesn't advertise
             * desired flow control, but can be forced on the link
             * partner.  So if we advertised no flow control, that is
             * what we will resolve to.  If we advertised some kind of
             * receive capability (Rx Pause Only or Full Flow Control)
             * and the link partner advertised none, we will configure
             * ourselves to enable Rx Flow Control only.  We can do
             * this safely for two reasons:  If the link partner really
             * didn't want flow control enabled, and we enable Rx, no
             * harm done since we won't be receiving any PAUSE frames
             * anyway.  If the intent on the link partner was to have
             * flow control enabled, then by us enabling RX only, we
             * can at least receive pause frames and process them.
             * This is a good idea because in most cases, since we are
             * predominantly a server NIC, more times than not we will
             * be asked to delay transmission of packets than asking
             * our link partner to pause transmission of frames.
             */
            else if(hw->original_fc == em_fc_none ||
                    hw->original_fc == em_fc_tx_pause) {
                hw->fc = em_fc_none;
                DEBUGOUT("Flow Control = NONE.");
            } else {
                hw->fc = em_fc_rx_pause;
                DEBUGOUT("Flow Control = RX PAUSE frames only.");
            }

            /* Now we need to do one last check...  If we auto-
             * negotiated to HALF DUPLEX, flow control should not be
             * enabled per IEEE 802.3 spec.
             */
            em_get_speed_and_duplex(hw, &speed, &duplex);

            if(duplex == HALF_DUPLEX)
                hw->fc = em_fc_none;

            /* Now we call a subroutine to actually force the MAC
             * controller to use the correct flow control settings.
             */
            ret_val = em_force_mac_fc(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error forcing flow control settings");
                return ret_val;
             }
        } else {
            DEBUGOUT("Copper PHY and Auto Neg has not completed.");
        }
    }
    return 0;
}

/******************************************************************************
 * Checks to see if the link status of the hardware has changed.
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Called by any function that needs to check the link status of the adapter.
 *****************************************************************************/
int32_t
em_check_for_link(struct em_hw *hw)
{
    uint32_t rxcw;
    uint32_t ctrl;
    uint32_t status;
    uint32_t rctl;
    uint32_t signal;
    int32_t ret_val;
    uint16_t phy_data;
    uint16_t lp_capability;

    DEBUGFUNC("em_check_for_link");
    
    /* On adapters with a MAC newer that 82544, SW Defineable pin 1 will be 
     * set when the optics detect a signal. On older adapters, it will be 
     * cleared when there is a signal
     */
    if(hw->mac_type > em_82544) signal = E1000_CTRL_SWDPIN1;
    else signal = 0;

    ctrl = E1000_READ_REG(hw, CTRL);
    status = E1000_READ_REG(hw, STATUS);
    rxcw = E1000_READ_REG(hw, RXCW);

    /* If we have a copper PHY then we only want to go out to the PHY
     * registers to see if Auto-Neg has completed and/or if our link
     * status has changed.  The get_link_status flag will be set if we
     * receive a Link Status Change interrupt or we have Rx Sequence
     * Errors.
     */
    if((hw->media_type == em_media_type_copper) && hw->get_link_status) {
        /* First we want to see if the MII Status Register reports
         * link.  If so, then we want to get the current speed/duplex
         * of the PHY.
         * Read the register twice since the link bit is sticky.
         */
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }

        if(phy_data & MII_SR_LINK_STATUS) {
            hw->get_link_status = FALSE;
        } else {
            /* No link detected */
            return 0;
        }

        /* If we are forcing speed/duplex, then we simply return since
         * we have already determined whether we have link or not.
         */
        if(!hw->autoneg) return -E1000_ERR_CONFIG;

        /* We have a M88E1000 PHY and Auto-Neg is enabled.  If we
         * have Si on board that is 82544 or newer, Auto
         * Speed Detection takes care of MAC speed/duplex
         * configuration.  So we only need to configure Collision
         * Distance in the MAC.  Otherwise, we need to force
         * speed/duplex on the MAC to the current PHY speed/duplex
         * settings.
         */
        if(hw->mac_type >= em_82544)
            em_config_collision_dist(hw);
        else {
            ret_val = em_config_mac_to_phy(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error configuring MAC to PHY settings");
                return ret_val;
            }
        }

        /* Configure Flow Control now that Auto-Neg has completed. First, we 
         * need to restore the desired flow control settings because we may
         * have had to re-autoneg with a different link partner.
         */
        ret_val = em_config_fc_after_link_up(hw);
        if(ret_val < 0) {
            DEBUGOUT("Error configuring flow control");
            return ret_val;
        }

        /* At this point we know that we are on copper and we have
         * auto-negotiated link.  These are conditions for checking the link
         * parter capability register.  We use the link partner capability to
         * determine if TBI Compatibility needs to be turned on or off.  If
         * the link partner advertises any speed in addition to Gigabit, then
         * we assume that they are GMII-based, and TBI compatibility is not
         * needed. If no other speeds are advertised, we assume the link
         * partner is TBI-based, and we turn on TBI Compatibility.
         */
        if(hw->tbi_compatibility_en) {
            if(em_read_phy_reg(hw, PHY_LP_ABILITY, &lp_capability) < 0) {
                DEBUGOUT("PHY Read Error");
                return -E1000_ERR_PHY;
            }
            if(lp_capability & (NWAY_LPAR_10T_HD_CAPS |
                                NWAY_LPAR_10T_FD_CAPS |
                                NWAY_LPAR_100TX_HD_CAPS |
                                NWAY_LPAR_100TX_FD_CAPS |
                                NWAY_LPAR_100T4_CAPS)) {
                /* If our link partner advertises anything in addition to 
                 * gigabit, we do not need to enable TBI compatibility.
                 */
                if(hw->tbi_compatibility_on) {
                    /* If we previously were in the mode, turn it off. */
                    rctl = E1000_READ_REG(hw, RCTL);
                    rctl &= ~E1000_RCTL_SBP;
                    E1000_WRITE_REG(hw, RCTL, rctl);
                    hw->tbi_compatibility_on = FALSE;
                }
            } else {
                /* If TBI compatibility is was previously off, turn it on. For
                 * compatibility with a TBI link partner, we will store bad
                 * packets. Some frames have an additional byte on the end and
                 * will look like CRC errors to to the hardware.
                 */
                if(!hw->tbi_compatibility_on) {
                    hw->tbi_compatibility_on = TRUE;
                    rctl = E1000_READ_REG(hw, RCTL);
                    rctl |= E1000_RCTL_SBP;
                    E1000_WRITE_REG(hw, RCTL, rctl);
                }
            }
        }
    }
    /* If we don't have link (auto-negotiation failed or link partner cannot
     * auto-negotiate), the cable is plugged in (we have signal), and our
     * link partner is not trying to auto-negotiate with us (we are receiving
     * idles or data), we need to force link up. We also need to give
     * auto-negotiation time to complete, in case the cable was just plugged
     * in. The autoneg_failed flag does this.
     */
    else if((hw->media_type == em_media_type_fiber) &&
            (!(status & E1000_STATUS_LU)) &&
            ((ctrl & E1000_CTRL_SWDPIN1) == signal) &&
            (!(rxcw & E1000_RXCW_C))) {
        if(hw->autoneg_failed == 0) {
            hw->autoneg_failed = 1;
            return 0;
        }
        DEBUGOUT("NOT RXing /C/, disable AutoNeg and force link.");

        /* Disable auto-negotiation in the TXCW register */
        E1000_WRITE_REG(hw, TXCW, (hw->txcw & ~E1000_TXCW_ANE));

        /* Force link-up and also force full-duplex. */
        ctrl = E1000_READ_REG(hw, CTRL);
        ctrl |= (E1000_CTRL_SLU | E1000_CTRL_FD);
        E1000_WRITE_REG(hw, CTRL, ctrl);

        /* Configure Flow Control after forcing link up. */
        ret_val = em_config_fc_after_link_up(hw);
        if(ret_val < 0) {
            DEBUGOUT("Error configuring flow control");
            return ret_val;
        }
    }
    /* If we are forcing link and we are receiving /C/ ordered sets, re-enable
     * auto-negotiation in the TXCW register and disable forced link in the
     * Device Control register in an attempt to auto-negotiate with our link
     * partner.
     */
    else if((hw->media_type == em_media_type_fiber) &&
              (ctrl & E1000_CTRL_SLU) &&
              (rxcw & E1000_RXCW_C)) {
        DEBUGOUT("RXing /C/, enable AutoNeg and stop forcing link.");
        E1000_WRITE_REG(hw, TXCW, hw->txcw);
        E1000_WRITE_REG(hw, CTRL, (ctrl & ~E1000_CTRL_SLU));
    }

    DEBUGOUT("em_check_for_link return");
    return 0;
}

/******************************************************************************
* Blocks until autoneg completes or times out (~4.5 seconds)
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
int32_t
em_wait_autoneg(struct em_hw *hw)
{
    uint16_t i;
    uint16_t phy_data;

    DEBUGFUNC("em_wait_autoneg");
    DEBUGOUT("Waiting for Auto-Neg to complete.");

    /* We will wait for autoneg to complete or 4.5 seconds to expire. */
    for(i = PHY_AUTO_NEG_TIME; i > 0; i--) {
        /* Read the MII Status Register and wait for Auto-Neg
         * Complete bit to be set.
         */
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        if(phy_data & MII_SR_AUTONEG_COMPLETE) {
            return 0;
        }
        msec_delay(100);
    }
    return 0;
}

/******************************************************************************
 * Sets up link for a fiber based adapter
 *
 * hw - Struct containing variables accessed by shared code
 * ctrl - Current value of the device control register
 *
 * Manipulates Physical Coding Sublayer functions in order to configure
 * link. Assumes the hardware has been previously reset and the transmitter
 * and receiver are not enabled.
 *****************************************************************************/
static int32_t 
em_setup_fiber_link(struct em_hw *hw)
{
    uint32_t ctrl;
    uint32_t status;
    uint32_t txcw = 0;
    uint32_t i;
    uint32_t signal;
    int32_t ret_val;

    DEBUGFUNC("em_setup_fiber_link");

    /* On adapters with a MAC newer that 82544, SW Defineable pin 1 will be 
     * set when the optics detect a signal. On older adapters, it will be 
     * cleared when there is a signal
     */
    ctrl = E1000_READ_REG(hw, CTRL);
    if(hw->mac_type > em_82544) signal = E1000_CTRL_SWDPIN1;
    else signal = 0;
   
    /* Take the link out of reset */
    ctrl &= ~(E1000_CTRL_LRST);
    
    em_config_collision_dist(hw);

    /* Check for a software override of the flow control settings, and setup
     * the device accordingly.  If auto-negotiation is enabled, then software
     * will have to set the "PAUSE" bits to the correct value in the Tranmsit
     * Config Word Register (TXCW) and re-start auto-negotiation.  However, if
     * auto-negotiation is disabled, then software will have to manually 
     * configure the two flow control enable bits in the CTRL register.
     *
     * The possible values of the "fc" parameter are:
     *      0:  Flow control is completely disabled
     *      1:  Rx flow control is enabled (we can receive pause frames, but 
     *          not send pause frames).
     *      2:  Tx flow control is enabled (we can send pause frames but we do
     *          not support receiving pause frames).
     *      3:  Both Rx and TX flow control (symmetric) are enabled.
     */
    switch (hw->fc) {
    case em_fc_none:
        /* Flow control is completely disabled by a software over-ride. */
	{
	    txcw = (E1000_TXCW_ANE | E1000_TXCW_FD);
	    break;
	}
    case em_fc_rx_pause:
        /* RX Flow control is enabled and TX Flow control is disabled by a 
         * software over-ride. Since there really isn't a way to advertise 
         * that we are capable of RX Pause ONLY, we will advertise that we
         * support both symmetric and asymmetric RX PAUSE. Later, we will
         *  disable the adapter's ability to send PAUSE frames.
         */
	{
	    txcw = (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK);
	    break;
	}
    case em_fc_tx_pause:
        /* TX Flow control is enabled, and RX Flow control is disabled, by a 
         * software over-ride.
         */
	{
	    txcw = (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_ASM_DIR);
	    break;
	}
    case em_fc_full:
        /* Flow control (both RX and TX) is enabled by a software over-ride. */
	{
	    txcw = (E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK);
	    break;
	}
    default:
	{
	    DEBUGOUT("Flow control param set incorrectly");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }

    /* Since auto-negotiation is enabled, take the link out of reset (the link
     * will be in reset, because we previously reset the chip). This will
     * restart auto-negotiation.  If auto-neogtiation is successful then the
     * link-up status bit will be set and the flow control enable bits (RFCE
     * and TFCE) will be set according to their negotiated value.
     */
    DEBUGOUT("Auto-negotiation enabled");

    E1000_WRITE_REG(hw, TXCW, txcw);
    E1000_WRITE_REG(hw, CTRL, ctrl);

    hw->txcw = txcw;
    msec_delay(1);

    /* If we have a signal (the cable is plugged in) then poll for a "Link-Up"
     * indication in the Device Status Register.  Time-out if a link isn't 
     * seen in 500 milliseconds seconds (Auto-negotiation should complete in 
     * less than 500 milliseconds even if the other end is doing it in SW).
     */
    if((E1000_READ_REG(hw, CTRL) & E1000_CTRL_SWDPIN1) == signal) {
	int ready = 0;
        DEBUGOUT("Looking for Link");
        for(i = 0; i < (LINK_UP_TIMEOUT / 10) && !ready; i++) {
            msec_delay(10);
            status = E1000_READ_REG(hw, STATUS);
            if(status & E1000_STATUS_LU) ready = 1;
        }
        if(i == (LINK_UP_TIMEOUT / 10)) {
            /* AutoNeg failed to achieve a link, so we'll call 
             * em_check_for_link. This routine will force the link up if we
             * detect a signal. This will allow us to communicate with
             * non-autonegotiating link partners.
             */
            DEBUGOUT("Never got a valid link from auto-neg!!!");
            hw->autoneg_failed = 1;
            ret_val = em_check_for_link(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error while checking for link");
                return ret_val;
            }
            hw->autoneg_failed = 0;
        } else {
            hw->autoneg_failed = 0;
            DEBUGOUT("Valid Link Found");
        }
    } else {
        DEBUGOUT("No Signal Detected");
    }
    return 0;
}

/******************************************************************************
* Detects which PHY is present and the speed and duplex
*
* hw - Struct containing variables accessed by shared code
* ctrl - current value of the device control register
******************************************************************************/
static int32_t 
em_setup_copper_link(struct em_hw *hw)
{
    uint32_t ctrl;
    int32_t ret_val;
    uint16_t i;
    uint16_t phy_data;

    DEBUGFUNC("em_setup_copper_link");

    ctrl = E1000_READ_REG(hw, CTRL);
    /* With 82543, we need to force speed and duplex on the MAC equal to what
     * the PHY speed and duplex configuration is. In addition, we need to
     * perform a hardware reset on the PHY to take it out of reset.
     */
    if(hw->mac_type > em_82543) {
        ctrl |= E1000_CTRL_SLU;
        ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
        E1000_WRITE_REG(hw, CTRL, ctrl);
    } else {
        ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX | E1000_CTRL_SLU);
        E1000_WRITE_REG(hw, CTRL, ctrl);
        em_phy_hw_reset(hw);
    }

    /* Make sure we have a valid PHY */
    ret_val = em_detect_gig_phy(hw);
    if(ret_val < 0) {
        DEBUGOUT("Error, did not detect valid phy.");
        return ret_val;
    }
    DEBUGOUT1("Phy ID = %x ", hw->phy_id);

    /* Enable CRS on TX. This must be set for half-duplex operation. */
    if(em_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    phy_data |= M88E1000_PSCR_ASSERT_CRS_ON_TX;

    /* Options:
     *   MDI/MDI-X = 0 (default)
     *   0 - Auto for all speeds
     *   1 - MDI mode
     *   2 - MDI-X mode
     *   3 - Auto for 1000Base-T only (MDI-X for 10/100Base-T modes)
     */
    phy_data &= ~M88E1000_PSCR_AUTO_X_MODE;

    switch (hw->mdix) {
    case 1:
	{
	    phy_data |= M88E1000_PSCR_MDI_MANUAL_MODE;
	    break;
	}
    case 2:
	{
	    phy_data |= M88E1000_PSCR_MDIX_MANUAL_MODE;
	    break;
	}
    case 3:
	{
	    phy_data |= M88E1000_PSCR_AUTO_X_1000T;
	    break;
	}
    case 0:
	{
	    phy_data |= M88E1000_PSCR_AUTO_X_MODE;
	    break;
	}
    default:
	{
	    phy_data |= M88E1000_PSCR_AUTO_X_MODE;
	    break;
	}
    }

    /* Options:
     *   disable_polarity_correction = 0 (default)
     *       Automatic Correction for Reversed Cable Polarity
     *   0 - Disabled
     *   1 - Enabled
     */
    phy_data &= ~M88E1000_PSCR_POLARITY_REVERSAL;
    if(hw->disable_polarity_correction == 1)
        phy_data |= M88E1000_PSCR_POLARITY_REVERSAL;
    if(em_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }

    /* Force TX_CLK in the Extended PHY Specific Control Register
     * to 25MHz clock.
     */
    if(em_read_phy_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, &phy_data) < 0) {
        DEBUGOUT("PHY Read Error");
        return -E1000_ERR_PHY;
    }
    phy_data |= M88E1000_EPSCR_TX_CLK_25;
    /* Configure Master and Slave downshift values */
    phy_data &= ~(M88E1000_EPSCR_MASTER_DOWNSHIFT_MASK |
                  M88E1000_EPSCR_SLAVE_DOWNSHIFT_MASK);
    phy_data |= (M88E1000_EPSCR_MASTER_DOWNSHIFT_1X |
                 M88E1000_EPSCR_SLAVE_DOWNSHIFT_1X);
    if(em_write_phy_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, phy_data) < 0) {
        DEBUGOUT("PHY Write Error");
        return -E1000_ERR_PHY;
    }

    /* SW Reset the PHY so all changes take effect */
    ret_val = em_phy_reset(hw);
    if(ret_val < 0) {
        DEBUGOUT("Error Resetting the PHY");
        return ret_val;
    }
    
    /* Options:
     *   autoneg = 1 (default)
     *      PHY will advertise value(s) parsed from
     *      autoneg_advertised and fc
     *   autoneg = 0
     *      PHY will be set to 10H, 10F, 100H, or 100F
     *      depending on value parsed from forced_speed_duplex.
     */

    /* Is autoneg enabled?  This is enabled by default or by software override.
     * If so, call em_phy_setup_autoneg routine to parse the
     * autoneg_advertised and fc options. If autoneg is NOT enabled, then the
     * user should have provided a speed/duplex override.  If so, then call
     * em_phy_force_speed_duplex to parse and set this up.
     */
    if(hw->autoneg) {
        /* Perform some bounds checking on the hw->autoneg_advertised
         * parameter.  If this variable is zero, then set it to the default.
         */
        hw->autoneg_advertised &= AUTONEG_ADVERTISE_SPEED_DEFAULT;

        /* If autoneg_advertised is zero, we assume it was not defaulted
         * by the calling code so we set to advertise full capability.
         */
        if(hw->autoneg_advertised == 0)
            hw->autoneg_advertised = AUTONEG_ADVERTISE_SPEED_DEFAULT;

        DEBUGOUT("Reconfiguring auto-neg advertisement params");
        ret_val = em_phy_setup_autoneg(hw);
        if(ret_val < 0) {
            DEBUGOUT("Error Setting up Auto-Negotiation");
            return ret_val;
        }
        DEBUGOUT("Restarting Auto-Neg");

        /* Restart auto-negotiation by setting the Auto Neg Enable bit and
         * the Auto Neg Restart bit in the PHY control register.
         */
        if(em_read_phy_reg(hw, PHY_CTRL, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        phy_data |= (MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG);
        if(em_write_phy_reg(hw, PHY_CTRL, phy_data) < 0) {
            DEBUGOUT("PHY Write Error");
            return -E1000_ERR_PHY;
        }

        /* Does the user want to wait for Auto-Neg to complete here, or
         * check at a later time (for example, callback routine).
         */
        if(hw->wait_autoneg_complete) {
            ret_val = em_wait_autoneg(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error while waiting for autoneg to complete");
                return ret_val;
            }
        }
    } else {
        DEBUGOUT("Forcing speed and duplex");
        ret_val = em_phy_force_speed_duplex(hw);
        if(ret_val < 0) {
            DEBUGOUT("Error Forcing Speed and Duplex");
            return ret_val;
        }
    }

    /* Check link status. Wait up to 100 microseconds for link to become
     * valid.
     */
    for(i = 0; i < 10; i++) {
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        if(em_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
            DEBUGOUT("PHY Read Error");
            return -E1000_ERR_PHY;
        }
        if(phy_data & MII_SR_LINK_STATUS) {
            /* We have link, so we need to finish the config process:
             *   1) Set up the MAC to the current PHY speed/duplex
             *      if we are on 82543.  If we
             *      are on newer silicon, we only need to configure
             *      collision distance in the Transmit Control Register.
             *   2) Set up flow control on the MAC to that established with
             *      the link partner.
             */
            if(hw->mac_type >= em_82544) {
                em_config_collision_dist(hw);
            } else {
                ret_val = em_config_mac_to_phy(hw);
                if(ret_val < 0) {
                    DEBUGOUT("Error configuring MAC to PHY settings");
                    return ret_val;
                  }
            }
            ret_val = em_config_fc_after_link_up(hw);
            if(ret_val < 0) {
                DEBUGOUT("Error Configuring Flow Control");
                return ret_val;
            }
            DEBUGOUT("Valid link established!!!");
            return 0;
        }
        usec_delay(10);
    }

    DEBUGOUT("Unable to establish link!!!");
    return 0;
}

/******************************************************************************
 * Configures flow control and link settings.
 * 
 * hw - Struct containing variables accessed by shared code
 * 
 * Determines which flow control settings to use. Calls the apropriate media-
 * specific link configuration function. Configures the flow control settings.
 * Assuming the adapter has a valid link partner, a valid link should be
 * established. Assumes the hardware has previously been reset and the 
 * transmitter and receiver are not enabled.
 *****************************************************************************/
int32_t
em_setup_link(struct em_hw *hw)
{
    uint32_t ctrl_ext;
    int32_t ret_val;
    uint16_t eeprom_data;

    DEBUGFUNC("em_setup_link");

    /* Read and store word 0x0F of the EEPROM. This word contains bits
     * that determine the hardware's default PAUSE (flow control) mode,
     * a bit that determines whether the HW defaults to enabling or
     * disabling auto-negotiation, and the direction of the
     * SW defined pins. If there is no SW over-ride of the flow
     * control setting, then the variable hw->fc will
     * be initialized based on a value in the EEPROM.
     */
    if(em_read_eeprom(hw, EEPROM_INIT_CONTROL2_REG, &eeprom_data) < 0) {
        DEBUGOUT("EEPROM Read Error");
        return -E1000_ERR_EEPROM;
    }

    if(hw->fc == em_fc_default) {
        if((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) == 0)
            hw->fc = em_fc_none;
        else if((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) == 
                EEPROM_WORD0F_ASM_DIR)
            hw->fc = em_fc_tx_pause;
        else
            hw->fc = em_fc_full;
    }

    /* We want to save off the original Flow Control configuration just
     * in case we get disconnected and then reconnected into a different
     * hub or switch with different Flow Control capabilities.
     */
    if(hw->mac_type == em_82542_rev2_0)
        hw->fc &= (~em_fc_tx_pause);

    if((hw->mac_type < em_82543) && (hw->report_tx_early == 1))
        hw->fc &= (~em_fc_rx_pause);

    hw->original_fc = hw->fc;

    DEBUGOUT1("After fix-ups FlowControl is now = %x ", hw->fc);

    /* Take the 4 bits from EEPROM word 0x0F that determine the initial
     * polarity value for the SW controlled pins, and setup the
     * Extended Device Control reg with that info.
     * This is needed because one of the SW controlled pins is used for
     * signal detection.  So this should be done before em_setup_pcs_link()
     * or em_phy_setup() is called.
     */
    if(hw->mac_type == em_82543) {
        ctrl_ext = ((eeprom_data & EEPROM_WORD0F_SWPDIO_EXT) << 
                    SWDPIO__EXT_SHIFT);
        E1000_WRITE_REG(hw, CTRL_EXT, ctrl_ext);
    }

    /* Call the necessary subroutine to configure the link. */
    ret_val = (hw->media_type == em_media_type_fiber) ?
              em_setup_fiber_link(hw) :
              em_setup_copper_link(hw);

    /* Initialize the flow control address, type, and PAUSE timer
     * registers to their default values.  This is done even if flow
     * control is disabled, because it does not hurt anything to
     * initialize these registers.
     */
    DEBUGOUT("Initializing the Flow Control address, type and timer regs");

    E1000_WRITE_REG(hw, FCAL, FLOW_CONTROL_ADDRESS_LOW);
    E1000_WRITE_REG(hw, FCAH, FLOW_CONTROL_ADDRESS_HIGH);
    E1000_WRITE_REG(hw, FCT, FLOW_CONTROL_TYPE);
    E1000_WRITE_REG(hw, FCTTV, hw->fc_pause_time);

    /* Set the flow control receive threshold registers.  Normally,
     * these registers will be set to a default threshold that may be
     * adjusted later by the driver's runtime code.  However, if the
     * ability to transmit pause frames in not enabled, then these
     * registers will be set to 0. 
     */
    if(!(hw->fc & em_fc_tx_pause)) {
        E1000_WRITE_REG(hw, FCRTL, 0);
        E1000_WRITE_REG(hw, FCRTH, 0);
    } else {
        /* We need to set up the Receive Threshold high and low water marks
         * as well as (optionally) enabling the transmission of XON frames.
         */
        if(hw->fc_send_xon) {
            E1000_WRITE_REG(hw, FCRTL, (hw->fc_low_water | E1000_FCRTL_XONE));
            E1000_WRITE_REG(hw, FCRTH, hw->fc_high_water);
        } else {
            E1000_WRITE_REG(hw, FCRTL, hw->fc_low_water);
            E1000_WRITE_REG(hw, FCRTH, hw->fc_high_water);
        }
    }
    return ret_val;
}

/******************************************************************************
 * Initializes receive address filters.
 *
 * hw - Struct containing variables accessed by shared code 
 *
 * Places the MAC address in receive address register 0 and clears the rest
 * of the receive addresss registers. Clears the multicast table. Assumes
 * the receiver is in reset when the routine is called.
 *****************************************************************************/
void
em_init_rx_addrs(struct em_hw *hw)
{
    uint32_t i;
    uint32_t addr_low;
    uint32_t addr_high;

    DEBUGFUNC("em_init_rx_addrs");

    /* Setup the receive address. */
    DEBUGOUT("Programming MAC Address into RAR[0]");
    addr_low = (hw->mac_addr[0] |
                (hw->mac_addr[1] << 8) |
                (hw->mac_addr[2] << 16) | (hw->mac_addr[3] << 24));

    addr_high = (hw->mac_addr[4] |
                 (hw->mac_addr[5] << 8) | E1000_RAH_AV);

    DEBUGOUT2("MAC ", addr_high, addr_low);
    DEBUGMAC("em_init_rx_addrs", hw->mac_addr);

    E1000_WRITE_REG_ARRAY(hw, RA, 0, addr_low);
    E1000_WRITE_REG_ARRAY(hw, RA, 1, addr_high);

    /* Zero out the other 15 receive addresses. */
    DEBUGOUT("Clearing RAR[1-15]");
    for(i = 1; i < E1000_RAR_ENTRIES; i++) {
        E1000_WRITE_REG_ARRAY(hw, RA, (i << 1), 0);
        E1000_WRITE_REG_ARRAY(hw, RA, ((i << 1) + 1), 0);
    }
}

/******************************************************************************
 * Puts an ethernet address into a receive address register.
 *
 * hw - Struct containing variables accessed by shared code
 * addr - Address to put into receive address register
 * index - Receive address register to write
 *****************************************************************************/
void
em_rar_set(struct em_hw *hw,
              uint8_t *addr,
              uint32_t index)
{
    uint32_t rar_low, rar_high;

    /* HW expects these in little endian so we reverse the byte order
     * from network order (big endian) to little endian              
     */
    rar_low = ((uint32_t) addr[0] |
               ((uint32_t) addr[1] << 8) |
               ((uint32_t) addr[2] << 16) | ((uint32_t) addr[3] << 24));

    rar_high = ((uint32_t) addr[4] | ((uint32_t) addr[5] << 8) | E1000_RAH_AV);

    E1000_WRITE_REG_ARRAY(hw, RA, (index << 1), rar_low);
    E1000_WRITE_REG_ARRAY(hw, RA, ((index << 1) + 1), rar_high);
}

/******************************************************************************
 * Hashes an address to determine its location in the multicast table
 *
 * hw - Struct containing variables accessed by shared code
 * mc_addr - the multicast address to hash 
 *****************************************************************************/
uint32_t
em_hash_mc_addr(struct em_hw *hw,
                   uint8_t *mc_addr)
{
    uint32_t hash_value = 0;

    /* The portion of the address that is used for the hash table is
     * determined by the mc_filter_type setting.  
     */
    switch (hw->mc_filter_type) {
    /* [0] [1] [2] [3] [4] [5]
     * 01  AA  00  12  34  56
     * LSB                 MSB
     */
    case 0:
        /* [47:36] i.e. 0x563 for above example address */
	{
	    hash_value = ((mc_addr[4] >> 4) | (((uint16_t) mc_addr[5]) << 4));
	    break;
	}
    case 1:
        /* [46:35] i.e. 0xAC6 for above example address */
	{
	    hash_value = ((mc_addr[4] >> 3) | (((uint16_t) mc_addr[5]) << 5));
	    break;
	}
    case 2:
        /* [45:34] i.e. 0x5D8 for above example address */
	{
	    hash_value = ((mc_addr[4] >> 2) | (((uint16_t) mc_addr[5]) << 6));
	    break;
	}
    case 3:
        /* [43:32] i.e. 0x634 for above example address */
	{
	    hash_value = ((mc_addr[4]) | (((uint16_t) mc_addr[5]) << 8));
	    break;
	}
    }

    hash_value &= 0xFFF;
    return hash_value;
}

/******************************************************************************
 * Sets the bit in the multicast table corresponding to the hash value.
 *
 * hw - Struct containing variables accessed by shared code
 * hash_value - Multicast address hash value
 *****************************************************************************/
void
em_mta_set(struct em_hw *hw,
              uint32_t hash_value)
{
    uint32_t hash_bit, hash_reg;
    uint32_t mta;
    uint32_t temp;

    /* The MTA is a register array of 128 32-bit registers.  
     * It is treated like an array of 4096 bits.  We want to set 
     * bit BitArray[hash_value]. So we figure out what register
     * the bit is in, read it, OR in the new bit, then write
     * back the new value.  The register is determined by the 
     * upper 7 bits of the hash value and the bit within that 
     * register are determined by the lower 5 bits of the value.
     */
    hash_reg = (hash_value >> 5) & 0x7F;
    hash_bit = hash_value & 0x1F;

    mta = E1000_READ_REG_ARRAY(hw, MTA, hash_reg);

    mta |= (1 << hash_bit);

    /* If we are on an 82544 and we are trying to write an odd offset
     * in the MTA, save off the previous entry before writing and
     * restore the old value after writing.
     */
    if((hw->mac_type == em_82544) && ((hash_reg & 0x1) == 1)) {
        temp = E1000_READ_REG_ARRAY(hw, MTA, (hash_reg - 1));
        E1000_WRITE_REG_ARRAY(hw, MTA, hash_reg, mta);
        E1000_WRITE_REG_ARRAY(hw, MTA, (hash_reg - 1), temp);
    } else {
        E1000_WRITE_REG_ARRAY(hw, MTA, hash_reg, mta);
    }
}

/******************************************************************************
 * Updates the MAC's list of multicast addresses.
 *
 * hw - Struct containing variables accessed by shared code
 * mc_addr_list - the list of new multicast addresses
 * mc_addr_count - number of addresses
 * pad - number of bytes between addresses in the list
 *
 * The given list replaces any existing list. Clears the last 15 receive
 * address registers and the multicast table. Uses receive address registers
 * for the first 15 multicast addresses, and hashes the rest into the 
 * multicast table.
 *****************************************************************************/
void
em_mc_addr_list_update(struct em_hw *hw,
                          uint8_t *mc_addr_list,
                          uint32_t mc_addr_count,
                          uint32_t pad)
{
    uint32_t hash_value;
    uint32_t i;
    uint32_t rar_used_count = 1; /* RAR[0] is used for our MAC address */

    DEBUGFUNC("em_mc_addr_list_update");

    /* Set the new number of MC addresses that we are being requested to use. */
    hw->num_mc_addrs = mc_addr_count;

    /* Clear RAR[1-15] */
    DEBUGOUT(" Clearing RAR[1-15]");
    for(i = rar_used_count; i < E1000_RAR_ENTRIES; i++) {
        E1000_WRITE_REG_ARRAY(hw, RA, (i << 1), 0);
        E1000_WRITE_REG_ARRAY(hw, RA, ((i << 1) + 1), 0);
    }

    /* Clear the MTA */
    DEBUGOUT(" Clearing MTA");
    for(i = 0; i < E1000_NUM_MTA_REGISTERS; i++) {
        E1000_WRITE_REG_ARRAY(hw, MTA, i, 0);
    }

    /* Add the new addresses */
    for(i = 0; i < mc_addr_count; i++) {
        DEBUGOUT(" Adding the multicast addresses:");
        DEBUGOUT7(" MC Addr #%d =%.2X %.2X %.2X %.2X %.2X %.2X ", i,
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad)],
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad) + 1],
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad) + 2],
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad) + 3],
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad) + 4],
                  mc_addr_list[i * (ETH_LENGTH_OF_ADDRESS + pad) + 5]);

        hash_value = em_hash_mc_addr(hw,
                                        mc_addr_list +
                                        (i * (ETH_LENGTH_OF_ADDRESS + pad)));

        DEBUGOUT1(" Hash value = 0x%03X ", hash_value);

        /* Place this multicast address in the RAR if there is room, *
         * else put it in the MTA            
         */
        if(rar_used_count < E1000_RAR_ENTRIES) {
            em_rar_set(hw,
                          mc_addr_list + (i * (ETH_LENGTH_OF_ADDRESS + pad)),
                          rar_used_count);
            rar_used_count++;
        } else {
            em_mta_set(hw, hash_value);
        }
    }
    DEBUGOUT("MC Update Complete");
}

/******************************************************************************
 * Prepares SW controlable LED for use and saves the current state of the LED.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t
em_setup_led(struct em_hw *hw)
{
    uint32_t ledctl;

    DEBUGFUNC("em_setup_led");
    ledctl = 0;		// this silences a compiler warning - unused var

    switch(hw->device_id) {

#if CHECK_DEV_ID(E1000_DEV_ID_82542)
    case E1000_DEV_ID_82542:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_FIBER)
    case E1000_DEV_ID_82543GC_FIBER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_COPPER)
    case E1000_DEV_ID_82543GC_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_COPPER)
    case E1000_DEV_ID_82544EI_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_FIBER)
    case E1000_DEV_ID_82544EI_FIBER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_COPPER)
    case E1000_DEV_ID_82544GC_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_LOM)
    case E1000_DEV_ID_82544GC_LOM:
        /* No setup necessary */
        break;
#endif
 
#if CHECK_DEV_ID(n1000_DEV_ID_82545EM_FIBER)
    case E1000_DEV_ID_82545EM_FIBER:
	{
	    ledctl = E1000_READ_REG(hw, LEDCTL);
	    /* Save current LEDCTL settings */
	    hw->ledctl_default = ledctl;
	    /* Turn off LED0 */
	    ledctl &= ~(E1000_LEDCTL_LED0_IVRT |
			E1000_LEDCTL_LED0_BLINK | 
			E1000_LEDCTL_LED0_MODE_MASK);
	    ledctl |= (E1000_LEDCTL_MODE_LED_OFF << E1000_LEDCTL_LED0_MODE_SHIFT);
	    E1000_WRITE_REG(hw, LEDCTL, ledctl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_FIBER)
    case E1000_DEV_ID_82546EB_FIBER:
	{
	    ledctl = E1000_READ_REG(hw, LEDCTL);
	    /* Save current LEDCTL settings */
	    hw->ledctl_default = ledctl;
	    /* Turn off LED0 */
	    ledctl &= ~(E1000_LEDCTL_LED0_IVRT |
			E1000_LEDCTL_LED0_BLINK | 
			E1000_LEDCTL_LED0_MODE_MASK);
	    ledctl |= (E1000_LEDCTL_MODE_LED_OFF << E1000_LEDCTL_LED0_MODE_SHIFT);
	    E1000_WRITE_REG(hw, LEDCTL, ledctl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM)
    case E1000_DEV_ID_82540EM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM_LOM)
    case E1000_DEV_ID_82540EM_LOM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_COPPER)
    case E1000_DEV_ID_82545EM_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_COPPER)
    case E1000_DEV_ID_82546EB_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

    default:
	{
	    DEBUGOUT("Invalid device ID");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }
    return 0;
}

/******************************************************************************
 * Restores the saved state of the SW controlable LED.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t
em_cleanup_led(struct em_hw *hw)
{
    DEBUGFUNC("em_cleanup_led");

    switch(hw->device_id) {

#if CHECK_DEV_ID(E1000_DEV_ID_82542)
    case E1000_DEV_ID_82542:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_FIBER)
    case E1000_DEV_ID_82543GC_FIBER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_COPPER)
    case E1000_DEV_ID_82543GC_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_COPPER)
    case E1000_DEV_ID_82544EI_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_FIBER)
    case E1000_DEV_ID_82544EI_FIBER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_COPPER)
    case E1000_DEV_ID_82544GC_COPPER:
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_LOM)
    case E1000_DEV_ID_82544GC_LOM:
        /* No cleanup necessary */
        break;
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM)
    case E1000_DEV_ID_82540EM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM_LOM)
    case E1000_DEV_ID_82540EM_LOM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_COPPER)
    case E1000_DEV_ID_82545EM_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_FIBER)
    case E1000_DEV_ID_82545EM_FIBER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_COPPER)
    case E1000_DEV_ID_82546EB_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_FIBER)
    case E1000_DEV_ID_82546EB_FIBER:
        /* Restore LEDCTL settings */
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_default);
	    break;
	}
#endif

    default:
	{
	    DEBUGOUT("Invalid device ID");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }
    return 0;
}
    
/******************************************************************************
 * Turns on the software controllable LED
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t
em_led_on(struct em_hw *hw)
{
    uint32_t ctrl;

    DEBUGFUNC("em_led_on");

    switch(hw->device_id) {

#if CHECK_DEV_ID(E1000_DEV_ID_82542)
    case E1000_DEV_ID_82542:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn on the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_FIBER)
    case E1000_DEV_ID_82543GC_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn on the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_COPPER)
    case E1000_DEV_ID_82543GC_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn on the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_FIBER)
    case E1000_DEV_ID_82544EI_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn on the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_COPPER)
    case E1000_DEV_ID_82544EI_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn on the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_COPPER)
    case E1000_DEV_ID_82544GC_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn on the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_LOM)
    case E1000_DEV_ID_82544GC_LOM:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn on the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_FIBER)
    case E1000_DEV_ID_82545EM_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn on the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_FIBER)
    case E1000_DEV_ID_82546EB_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn on the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM)
    case E1000_DEV_ID_82540EM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode2);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM_LOM)
    case E1000_DEV_ID_82540EM_LOM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode2);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_COPPER)
    case E1000_DEV_ID_82545EM_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode2);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_COPPER)
    case E1000_DEV_ID_82546EB_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode2);
	    break;
	}
#endif

    default:
	{
	    DEBUGOUT("Invalid device ID");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }
    return 0;
}

/******************************************************************************
 * Turns off the software controllable LED
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t
em_led_off(struct em_hw *hw)
{
    uint32_t ctrl;

    DEBUGFUNC("em_led_off");

    switch(hw->device_id) {

#if CHECK_DEV_ID(E1000_DEV_ID_82542)
    case E1000_DEV_ID_82542:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn off the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_FIBER)
    case E1000_DEV_ID_82543GC_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn off the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82543GC_COPPER)
    case E1000_DEV_ID_82543GC_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn off the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_FIBER)
    case E1000_DEV_ID_82544EI_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Clear SW Defineable Pin 0 to turn off the LED */
	    ctrl &= ~E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544EI_COPPER)
    case E1000_DEV_ID_82544EI_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn off the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_COPPER)
    case E1000_DEV_ID_82544GC_COPPER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn off the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82544GC_LOM)
    case E1000_DEV_ID_82544GC_LOM:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn off the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_FIBER)
    case E1000_DEV_ID_82545EM_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn off the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_FIBER)
    case E1000_DEV_ID_82546EB_FIBER:
	{
	    ctrl = E1000_READ_REG(hw, CTRL);
	    /* Set SW Defineable Pin 0 to turn off the LED */
	    ctrl |= E1000_CTRL_SWDPIN0;
	    ctrl |= E1000_CTRL_SWDPIO0;
	    E1000_WRITE_REG(hw, CTRL, ctrl);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM)
    case E1000_DEV_ID_82540EM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82540EM_LOM)
    case E1000_DEV_ID_82540EM_LOM:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82545EM_COPPER)
    case E1000_DEV_ID_82545EM_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

#if CHECK_DEV_ID(E1000_DEV_ID_82546EB_COPPER)
    case E1000_DEV_ID_82546EB_COPPER:
	{
	    E1000_WRITE_REG(hw, LEDCTL, hw->ledctl_mode1);
	    break;
	}
#endif

    default:
	{
	    DEBUGOUT("Invalid device ID");
	    return -E1000_ERR_CONFIG;
	    break;
	}
    }
    return 0;
}

/******************************************************************************
 * Clears all hardware statistics counters. 
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
void
em_clear_hw_cntrs(struct em_hw *hw)
{
    volatile uint32_t temp;

    temp = E1000_READ_REG(hw, CRCERRS);
    temp = E1000_READ_REG(hw, SYMERRS);
    temp = E1000_READ_REG(hw, MPC);
    temp = E1000_READ_REG(hw, SCC);
    temp = E1000_READ_REG(hw, ECOL);
    temp = E1000_READ_REG(hw, MCC);
    temp = E1000_READ_REG(hw, LATECOL);
    temp = E1000_READ_REG(hw, COLC);
    temp = E1000_READ_REG(hw, DC);
    temp = E1000_READ_REG(hw, SEC);
    temp = E1000_READ_REG(hw, RLEC);
    temp = E1000_READ_REG(hw, XONRXC);
    temp = E1000_READ_REG(hw, XONTXC);
    temp = E1000_READ_REG(hw, XOFFRXC);
    temp = E1000_READ_REG(hw, XOFFTXC);
    temp = E1000_READ_REG(hw, FCRUC);
    temp = E1000_READ_REG(hw, PRC64);
    temp = E1000_READ_REG(hw, PRC127);
    temp = E1000_READ_REG(hw, PRC255);
    temp = E1000_READ_REG(hw, PRC511);
    temp = E1000_READ_REG(hw, PRC1023);
    temp = E1000_READ_REG(hw, PRC1522);
    temp = E1000_READ_REG(hw, GPRC);
    temp = E1000_READ_REG(hw, BPRC);
    temp = E1000_READ_REG(hw, MPRC);
    temp = E1000_READ_REG(hw, GPTC);
    temp = E1000_READ_REG(hw, GORCL);
    temp = E1000_READ_REG(hw, GORCH);
    temp = E1000_READ_REG(hw, GOTCL);
    temp = E1000_READ_REG(hw, GOTCH);
    temp = E1000_READ_REG(hw, RNBC);
    temp = E1000_READ_REG(hw, RUC);
    temp = E1000_READ_REG(hw, RFC);
    temp = E1000_READ_REG(hw, ROC);
    temp = E1000_READ_REG(hw, RJC);
    temp = E1000_READ_REG(hw, TORL);
    temp = E1000_READ_REG(hw, TORH);
    temp = E1000_READ_REG(hw, TOTL);
    temp = E1000_READ_REG(hw, TOTH);
    temp = E1000_READ_REG(hw, TPR);
    temp = E1000_READ_REG(hw, TPT);
    temp = E1000_READ_REG(hw, PTC64);
    temp = E1000_READ_REG(hw, PTC127);
    temp = E1000_READ_REG(hw, PTC255);
    temp = E1000_READ_REG(hw, PTC511);
    temp = E1000_READ_REG(hw, PTC1023);
    temp = E1000_READ_REG(hw, PTC1522);
    temp = E1000_READ_REG(hw, MPTC);
    temp = E1000_READ_REG(hw, BPTC);

    if(hw->mac_type < em_82543) return;

    temp = E1000_READ_REG(hw, ALGNERRC);
    temp = E1000_READ_REG(hw, RXERRC);
    temp = E1000_READ_REG(hw, TNCRS);
    temp = E1000_READ_REG(hw, CEXTERR);
    temp = E1000_READ_REG(hw, TSCTC);
    temp = E1000_READ_REG(hw, TSCTFC);

    if(hw->mac_type <= em_82544) return;

    temp = E1000_READ_REG(hw, MGTPRC);
    temp = E1000_READ_REG(hw, MGTPDC);
    temp = E1000_READ_REG(hw, MGTPTC);
}

/******************************************************************************
 * Resets Adaptive IFS to its default state.
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Call this after em_init_hw. You may override the IFS defaults by setting
 * hw->ifs_params_forced to TRUE. However, you must initialize hw->
 * current_ifs_val, ifs_min_val, ifs_max_val, ifs_step_size, and ifs_ratio
 * before calling this function.
 *****************************************************************************/
void
em_reset_adaptive(struct em_hw *hw)
{
    DEBUGFUNC("em_reset_adaptive");

    if(hw->adaptive_ifs) {
        if(!hw->ifs_params_forced) {
            hw->current_ifs_val = 0;
            hw->ifs_min_val = IFS_MIN;
            hw->ifs_max_val = IFS_MAX;
            hw->ifs_step_size = IFS_STEP;
            hw->ifs_ratio = IFS_RATIO;
        }
        hw->in_ifs_mode = FALSE;
        E1000_WRITE_REG(hw, AIT, 0);
    } else {
        DEBUGOUT("Not in Adaptive IFS mode!");
    }
}

/******************************************************************************
 * Called during the callback/watchdog routine to update IFS value based on
 * the ratio of transmits to collisions.
 *
 * hw - Struct containing variables accessed by shared code
 * tx_packets - Number of transmits since last callback
 * total_collisions - Number of collisions since last callback
 *****************************************************************************/
void
em_update_adaptive(struct em_hw *hw)
{
    DEBUGFUNC("em_update_adaptive");

    if(hw->adaptive_ifs) {
        if((hw->collision_delta * hw->ifs_ratio) > 
           hw->tx_packet_delta) {
            if(hw->tx_packet_delta > MIN_NUM_XMITS) {
                hw->in_ifs_mode = TRUE;
                if(hw->current_ifs_val < hw->ifs_max_val) {
                    if(hw->current_ifs_val == 0)
                        hw->current_ifs_val = hw->ifs_min_val;
                    else
                        hw->current_ifs_val += hw->ifs_step_size;
                    E1000_WRITE_REG(hw, AIT, hw->current_ifs_val);
                }
            }
        } else {
            if((hw->in_ifs_mode == TRUE) && 
               (hw->tx_packet_delta <= MIN_NUM_XMITS)) {
                hw->current_ifs_val = 0;
                hw->in_ifs_mode = FALSE;
                E1000_WRITE_REG(hw, AIT, 0);
            }
        }
    } else {
        DEBUGOUT("Not in Adaptive IFS mode!");
    }
}

/******************************************************************************
 * Adjusts the statistic counters when a frame is accepted by TBI_ACCEPT
 * 
 * hw - Struct containing variables accessed by shared code
 * frame_len - The length of the frame in question
 * mac_addr - The Ethernet destination address of the frame in question
 *****************************************************************************/
void
em_tbi_adjust_stats(struct em_hw *hw,
                       struct em_hw_stats *stats,
                       uint32_t frame_len,
                       uint8_t *mac_addr)
{
    uint32_t carry_bit;

    /* First adjust the frame length. */
    frame_len--;
    /* We need to adjust the statistics counters, since the hardware
     * counters overcount this packet as a CRC error and undercount
     * the packet as a good packet
     */
    /* This packet should not be counted as a CRC error.    */
    stats->crcerrs--;
    /* This packet does count as a Good Packet Received.    */
    stats->gprc++;

    /* Adjust the Good Octets received counters             */
    carry_bit = 0x80000000 & stats->gorcl;
    stats->gorcl += frame_len;
    /* If the high bit of Gorcl (the low 32 bits of the Good Octets
     * Received Count) was one before the addition, 
     * AND it is zero after, then we lost the carry out, 
     * need to add one to Gorch (Good Octets Received Count High).
     * This could be simplified if all environments supported 
     * 64-bit integers.
     */
    if(carry_bit && ((stats->gorcl & 0x80000000) == 0))
        stats->gorch++;
    /* Is this a broadcast or multicast?  Check broadcast first,
     * since the test for a multicast frame will test positive on 
     * a broadcast frame.
     */
    if((mac_addr[0] == (uint8_t) 0xff) && (mac_addr[1] == (uint8_t) 0xff))
        /* Broadcast packet */
        stats->bprc++;
    else if(*mac_addr & 0x01)
        /* Multicast packet */
        stats->mprc++;

    if(frame_len == hw->max_frame_size) {
        /* In this case, the hardware has overcounted the number of
         * oversize frames.
         */
        if(stats->roc > 0)
            stats->roc--;
    }

    /* Adjust the bin counters when the extra byte put the frame in the
     * wrong bin. Remember that the frame_len was adjusted above.
     */
    if(frame_len == 64) {
        stats->prc64++;
        stats->prc127--;
    } else if(frame_len == 127) {
        stats->prc127++;
        stats->prc255--;
    } else if(frame_len == 255) {
        stats->prc255++;
        stats->prc511--;
    } else if(frame_len == 511) {
        stats->prc511++;
        stats->prc1023--;
    } else if(frame_len == 1023) {
        stats->prc1023++;
        stats->prc1522--;
    } else if(frame_len == 1522) {
        stats->prc1522++;
    }
}

/******************************************************************************
 * Gets the current PCI bus type, speed, and width of the hardware
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
void
em_get_bus_info(struct em_hw *hw)
{
    uint32_t status;

    if(hw->mac_type < em_82543) {
        hw->bus_type = em_bus_type_unknown;
        hw->bus_speed = em_bus_speed_unknown;
        hw->bus_width = em_bus_width_unknown;
        return;
    }

    status = E1000_READ_REG(hw, STATUS);
    hw->bus_type = (status & E1000_STATUS_PCIX_MODE) ?
                   em_bus_type_pcix : em_bus_type_pci;
    if(hw->bus_type == em_bus_type_pci) {
        hw->bus_speed = (status & E1000_STATUS_PCI66) ?
                        em_bus_speed_66 : em_bus_speed_33;
    } else {
        switch (status & E1000_STATUS_PCIX_SPEED) {
        case E1000_STATUS_PCIX_SPEED_66:
	    {
		hw->bus_speed = em_bus_speed_66;
		break;
	    }
        case E1000_STATUS_PCIX_SPEED_100:
	    {
		hw->bus_speed = em_bus_speed_100;
		break;
	    }
        case E1000_STATUS_PCIX_SPEED_133:
	    {
		hw->bus_speed = em_bus_speed_133;
		break;
	    }
        default:
	    {
		hw->bus_speed = em_bus_speed_reserved;
		break;
	    }
        }
    }
    hw->bus_width = (status & E1000_STATUS_BUS64) ?
                    em_bus_width_64 : em_bus_width_32;
}

/******************************************************************************
 * Performs basic configuration of the adapter.
 *
 * hw - Struct containing variables accessed by shared code
 * 
 * Assumes that the controller has previously been reset and is in a 
 * post-reset uninitialized state. Initializes the receive address registers,
 * multicast table, and VLAN filter table. Calls routines to setup link
 * configuration and flow control settings. Clears all on-chip counters. Leaves
 * the transmit and receive units disabled and uninitialized.
 *****************************************************************************/
int32_t
em_init_hw(struct em_hw *hw)
{
    uint32_t ctrl, status;
    uint32_t i;
    int32_t ret_val;
    uint16_t pci_cmd_word;

    DEBUGFUNC("em_init_hw");

    /* Initialize Identification LED */
    ret_val = em_id_led_init(hw);
    if(ret_val < 0) {
	DEBUGOUT("Error Initializing Identification LED");
	return ret_val;
    }
    
    /* Set the Media Type and exit with error if it is not valid. */
    if(hw->mac_type != em_82543) {
        /* tbi_compatibility is only valid on 82543 */
        hw->tbi_compatibility_en = FALSE;
    }

    if(hw->mac_type >= em_82543) {
        status = E1000_READ_REG(hw, STATUS);
        if(status & E1000_STATUS_TBIMODE) {
            hw->media_type = em_media_type_fiber;
            /* tbi_compatibility not valid on fiber */
            hw->tbi_compatibility_en = FALSE;
        } else {
            hw->media_type = em_media_type_copper;
        }
    } else {
        /* This is an 82542 (fiber only) */
        hw->media_type = em_media_type_fiber;
    }

    /* Disabling VLAN filtering. */
    DEBUGOUT("Initializing the IEEE VLAN");
    E1000_WRITE_REG(hw, VET, 0);

    em_clear_vfta(hw);

    /* For 82542 (rev 2.0), disable MWI and put the receiver into reset */
    if(hw->mac_type == em_82542_rev2_0) {
        if(hw->pci_cmd_word & CMD_MEM_WRT_INVALIDATE) {
            DEBUGOUT("Disabling MWI on 82542 rev 2.0");
            pci_cmd_word = hw->pci_cmd_word & ~CMD_MEM_WRT_INVALIDATE;
            em_write_pci_cfg(hw, PCI_COMMAND_REGISTER, &pci_cmd_word);
        }
        E1000_WRITE_REG(hw, RCTL, E1000_RCTL_RST);
        msec_delay(5);
    }

    /* Setup the receive address. This involves initializing all of the Receive
     * Address Registers (RARs 0 - 15).
     */
    em_init_rx_addrs(hw);

    /* For 82542 (rev 2.0), take the receiver out of reset and enable MWI */
    if(hw->mac_type == em_82542_rev2_0) {
        E1000_WRITE_REG(hw, RCTL, 0);
        msec_delay(1);
        if(hw->pci_cmd_word & CMD_MEM_WRT_INVALIDATE)
            em_write_pci_cfg(hw, PCI_COMMAND_REGISTER, &hw->pci_cmd_word);
    }

    /* Zero out the Multicast HASH table */
    DEBUGOUT("Zeroing the MTA");
    for(i = 0; i < E1000_MC_TBL_SIZE; i++)
        E1000_WRITE_REG_ARRAY(hw, MTA, i, 0);

    /* Set the PCI priority bit correctly in the CTRL register.  This
     * determines if the adapter gives priority to receives, or if it
     * gives equal priority to transmits and receives.
     */
    if(hw->dma_fairness) {
        ctrl = E1000_READ_REG(hw, CTRL);
        E1000_WRITE_REG(hw, CTRL, ctrl | E1000_CTRL_PRIOR);
    }

    /* Call a subroutine to configure the link and setup flow control. */
    ret_val = em_setup_link(hw);

    /* Clear all of the statistics registers (clear on read).  It is
     * important that we do this after we have tried to establish link
     * because the symbol error count will increment wildly if there
     * is no link.
     */
    em_clear_hw_cntrs(hw);

    return ret_val;
}

/*$FreeBSD$*/
/* if_em.c */

/*********************************************************************
 *  Set this to one to display debug statistics                                                   
 *********************************************************************/
int             em_display_debug_stats = 0;

/*********************************************************************
 *  Linked list of board private structures for all NICs found
 *********************************************************************/

struct adapter *em_adapter_list = NULL;


/*********************************************************************
 *  Driver version
 *********************************************************************/


/*********************************************************************
 *  Table of branding strings for all supported NICs.
 *********************************************************************/

static char *em_strings[] = {
	"Intel(R) PRO/1000 Network Connection",
	NULL,
};

#define __P(x) x

struct ifnet
{
	int pad;
};

typedef unsigned int caddr_t;
typedef int device_t;

/*********************************************************************
 *  Function prototypes            
 *********************************************************************/
static int em_probe     __P((struct adapter *));
static int em_open        __P((struct adapter *));
static void em_close        __P((struct adapter *));
static int em_shutdown        __P((struct adapter *));
static void em_intr __P((void *));
static void em_start __P((struct ifnet *));
static int em_ioctl __P((struct ifnet *, IOCTL_CMD_TYPE, caddr_t));
static void em_watchdog __P((struct ifnet *));
static void em_init __P((void *));
static void em_stop __P((struct adapter *adapter));
static void em_media_status __P((struct ifnet *, struct ifmediareq *));
static int em_media_change __P((struct ifnet *));
static void em_identify_hardware __P((struct adapter *));
static void em_local_timer __P((void *));
static int em_hardware_init __P((struct adapter *));
static int em_setup_transmit_structures __P((struct adapter *));
static void em_initialize_transmit_unit __P((struct adapter *));
static int em_setup_receive_structures __P((struct adapter *));
static void em_initialize_receive_unit __P((struct adapter *));
static void em_enable_intr __P((struct adapter *));
static void em_disable_intr __P((struct adapter *));
static void em_update_stats_counters __P((struct adapter *));
static void em_clean_transmit_interrupts __P((struct adapter *));
static int em_allocate_receive_structures __P((struct adapter *));
static int em_process_receive_packet __P((struct adapter *, char *buf, int len));
static int em_process_transmit_packet __P((struct adapter *));
static void em_receive_checksum __P((struct adapter *, 
				     struct em_rx_desc * rx_desc,
				     struct mbuf *));
static void em_set_promisc __P((struct adapter *, int));
static void em_disable_promisc __P((struct adapter *));
static void em_set_multi __P((struct adapter *));
static void em_print_hw_stats __P((struct adapter *));
static void em_print_link_status __P((struct adapter *));
static void em_enable_vlans __P((struct adapter *adapter));


unsigned int
pci_read_config(struct adapter */*adapter*/, int reg, int width)
{
	unsigned int val = 0;
	unsigned int addr = addr = my_space() & 0xFFFFFF00;
	DEBUGOUT2("read_config reg=%X width=%d ", reg, width);

	switch (width)
	{
	case 1:
		{
			val = config_getb(reg | addr);
			break;
		}
	case 2:
		{
			val = config_getw(reg | addr);
			break;
		}
	case 4:
		{
			val = config_getl(reg | addr);
			break;
		}
	}

	DEBUGOUT2("read_config addr=%X val=%X ", addr, val);
	return val;
}

void
pci_write_config(struct adapter */*adapter*/, int reg, unsigned int data, int width)
{
	unsigned int addr = my_space();

	switch (width)
	{
	case 1:
		{
			config_setb(data, reg | addr);
			break;
		}
	case 2:
		{
			config_setw(data, reg | addr);
			break;
		}
	case 4:
		{
			config_setl(data, reg | addr);
			break;
		}
	}
}

#define PCIR_COMMAND	0x04
#define PCIM_CMD_BUSMASTEREN	0x0004
#define PCIM_CMD_MEMEN	0x0002
#define PCIR_REVID	0x08
#define PCIR_SUBVEND_0	0x2C
#define PCIR_SUBDEV_0	0x2E
#define PCIR_VEND	0x00
#define PCIR_DEV	0x02

int
pci_get_vendor(struct adapter *adapter)
{
	return pci_read_config(adapter, PCIR_VEND, 2);
}

int
pci_get_device(struct adapter *adapter)
{
	return pci_read_config(adapter, PCIR_DEV, 2);
}

int 
pci_get_subvendor(struct adapter *adapter)
{
	return pci_read_config(adapter, PCIR_SUBVEND_0, 2);
}

int
pci_get_subdevice(struct adapter *adapter)
{
	return pci_read_config(adapter, PCIR_SUBDEV_0, 2);
}

/*********************************************************************
 *  Device identification routine
 *
 *  em_probe determines if the driver should be loaded on
 *  adapter based on PCI vendor/device id of the adapter.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
em_probe(struct adapter *adapter)
{
    u_int16_t pci_vendor_id = 0;
    u_int16_t pci_device_id = 0;
    u_int16_t pci_subvendor_id = 0;
    u_int16_t pci_subdevice_id = 0;

    INIT_DEBUGOUT("em_probe: begin");

    pci_vendor_id = pci_get_vendor(adapter);
    pci_device_id = pci_get_device(adapter);
    pci_subvendor_id = pci_get_subvendor(adapter);
    pci_subdevice_id = pci_get_subdevice(adapter);

#if EM_DEVICE_ID != E1000_DEV_ID_ANY && \
	defined(SUBSYSTEM_VENDOR_ID) && defined(SUBSYSTEM_ID)

    /* These IDs are defined on the command-line to build a ROM for
     * a specific target board and turn off some unnecessary code.
     */
    if (pci_vendor_id != ID2HEX(VENDOR_ID) ||
	    pci_device_id != ID2HEX(DEVICE_ID) ||
	    pci_subvendor_id != ID2HEX(SUBSYSTEM_VENDOR_ID) ||
	    pci_subdevice_id != ID2HEX(SUBSYSTEM_ID))
	return -1;
#else
    if (pci_vendor_id != ID2HEX(VENDOR_ID))
	return -1;
#endif

    return 0;
}

#define NUM_TX_BUFS		8
#define NUM_RX_BUFS		8
#define BUF_SIZE		2048

static boolean_t
em_sw_init(struct adapter *adapter)
{
	int i;
	char *dma, *map;
	struct em_tx_desc *txdesc;
	struct em_rx_desc *rxdesc;

	DEBUGFUNC("e1000_sw_init");

	/* allocate all DMA memory in one chunk, then parcel it out */
	adapter->mem_len = sizeof (struct em_tx_desc) * NUM_TX_BUFS +
			sizeof (struct em_rx_desc) * NUM_RX_BUFS +
			BUF_SIZE * NUM_TX_BUFS + BUF_SIZE * NUM_RX_BUFS +
			3 * 4096;
	dma = dma_alloc(adapter->mem_len);

	if (dma == NULL)
	{
		typecr("\$em_sw_init dma-alloc failed");
		return FALSE;
	}

	map = dma_map_in(dma, adapter->mem_len, FALSE);

	if (map == NULL)
	{
		dma_free(dma, adapter->mem_len);
		typecr("\$em_sw_init: map-in of dma-mem failed");
		return FALSE;
	}

	adapter->dma_mem = dma;
	adapter->map_mem = map;
	memset(dma, 0, adapter->mem_len);
	DEBUGOUT2("dma 0x%X map 0x%X ", dma, map);
	DEBUGOUT2("dma end 0x%X map end 0x%X ", dma + adapter->mem_len - 1,
			map + adapter->mem_len - 1);

	/* align the dma address to a multiple for 4K */
	i = (0x1000 - ((u_int32_t)map & 0xFFF)) & 0xFFF;
	map = (char *)((u_int32_t)map + i);
	dma = (char *)((u_int32_t)dma + i);

	/* Allocate memory for the transmit buffer descriptors */
	adapter->num_tx_desc = NUM_TX_BUFS;
	adapter->tx_desc_base_pci = (u_int32_t)map;
	adapter->tx_desc_base = (struct em_tx_desc *)dma;
	adapter->first_tx_desc = adapter->tx_desc_base;
	adapter->last_tx_desc =
			adapter->first_tx_desc + (adapter->num_tx_desc - 1);

	dma += sizeof (struct em_tx_desc) * NUM_TX_BUFS;
	map += sizeof (struct em_tx_desc) * NUM_TX_BUFS;

#ifdef DEBUG
	if (em_debug_level >= 2)
	{
		type("\$tbd_data ");
/*		udot((unsigned int)adapter->e1000_tbd_data);	*/
		cr();
	}
#endif // DEBUG

	/* align the dma address to a multiple for 4K */
	i = (0x1000 - ((u_int32_t)map & 0xFFF)) & 0xFFF;
	map = (char *)((u_int32_t)map + i);
	dma = (char *)((u_int32_t)dma + i);

	/* Allocate memory for the receive buffer descriptors */
	adapter->num_rx_desc = NUM_RX_BUFS;
	adapter->rx_desc_base_pci = (u_int32_t)map;
	adapter->rx_desc_base = (struct em_rx_desc *)dma;
	adapter->first_rx_desc = adapter->rx_desc_base;
	adapter->last_rx_desc =
			adapter->first_rx_desc + (adapter->num_rx_desc - 1);

	dma += sizeof (struct em_rx_desc) * NUM_RX_BUFS;
	map += sizeof (struct em_rx_desc) * NUM_RX_BUFS;

#ifdef DEBUG
	if (em_debug_level >= 2)
	{
		type("\$rbd_data ");
/*		udot((unsigned int)adapter->e1000_rbd_data);	*/
		cr();
	}
#endif // DEBUG

	/* align the dma address to a multiple for 4K */
	i = (0x1000 - ((u_int32_t)map & 0xFFF)) & 0xFFF;
	map = (char *)((u_int32_t)map + i);
	dma = (char *)((u_int32_t)dma + i);

	/* Allocate a buffer for each transmit descriptor. */
	adapter->tx_bufs = dma;
	DEBUGOUT2("TX dma= map= ", dma, map);
	txdesc = adapter->first_tx_desc;

	for (i = 0; i < adapter->num_tx_desc; i++)
	{
		txdesc->buffer_addr.lo = (unsigned int)map;
		dma += BUF_SIZE;
		map += BUF_SIZE;
		txdesc++;
	}

	/* Allocate a buffer for each receive descriptor. */
	adapter->rx_bufs = dma;
	DEBUGOUT2("RX dma= map= ", dma, map);
	rxdesc = adapter->first_rx_desc;

	for (i = 0; i < adapter->num_rx_desc; i++)
	{
		rxdesc->buffer_addr.lo = (unsigned int)map;
		dma += BUF_SIZE;
		map += BUF_SIZE;
		rxdesc++;
		DEBUGOUT2("rxbuf i map ", i, map);
	}

	dma_sync(adapter->dma_mem, adapter->map_mem, adapter->mem_len);
	return TRUE;
}

/*********************************************************************
 *
 *  Determine hardware revision.
 *
 **********************************************************************/
static void
em_identify_hardware(struct adapter * adapter)
{
	/* Make sure our PCI config space has the necessary stuff set */
	adapter->hw.pci_cmd_word = pci_read_config(adapter, PCIR_COMMAND, 2);
	DEBUGOUT1("em_identify_hardware: cmd_word = 0x%X ", adapter->hw.pci_cmd_word);
	if (!((adapter->hw.pci_cmd_word & PCIM_CMD_BUSMASTEREN) &&
	      (adapter->hw.pci_cmd_word & PCIM_CMD_MEMEN))) {
		type("\$em: Memory Access and/or Bus Master bits were not set!");
		cr();
		adapter->hw.pci_cmd_word |= 
			(PCIM_CMD_BUSMASTEREN | PCIM_CMD_MEMEN);
		pci_write_config(adapter, PCIR_COMMAND, adapter->hw.pci_cmd_word, 2);
	}

	/* Save off the information about this board */
	adapter->hw.vendor_id = pci_get_vendor(adapter);
	adapter->hw.device_id = pci_get_device(adapter);
	adapter->hw.revision_id = pci_read_config(adapter, PCIR_REVID, 1);
	adapter->hw.subsystem_vendor_id = pci_read_config(adapter, PCIR_SUBVEND_0, 2);
	adapter->hw.subsystem_id = pci_read_config(adapter, PCIR_SUBDEV_0, 2);


	/* Set MacType, etc. based on this PCI info */
	switch (adapter->hw.device_id) {

    #if CHECK_DEV_ID(E1000_DEV_ID_82542)
	case E1000_DEV_ID_82542:
		{
			adapter->hw.mac_type = (adapter->hw.revision_id == 3) ?
						   em_82542_rev2_1 : em_82542_rev2_0;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82543GC_FIBER)
	case E1000_DEV_ID_82543GC_FIBER:
		{
			adapter->hw.mac_type = em_82543;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82543GC_COPPER)
	case E1000_DEV_ID_82543GC_COPPER:
		{
			adapter->hw.mac_type = em_82543;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82544EI_FIBER)
	case E1000_DEV_ID_82544EI_FIBER:
		{
			adapter->hw.mac_type = em_82544;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82544EI_COPPER)
	case E1000_DEV_ID_82544EI_COPPER:
		{
			adapter->hw.mac_type = em_82544;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82544GC_COPPER)
	case E1000_DEV_ID_82544GC_COPPER:
		{
			adapter->hw.mac_type = em_82544;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82544GC_LOM)
	case E1000_DEV_ID_82544GC_LOM:
		{
			adapter->hw.mac_type = em_82544;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82540EM)
	case E1000_DEV_ID_82540EM:
		{
			adapter->hw.mac_type = em_82540;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82540EM_LOM)
	case E1000_DEV_ID_82540EM_LOM:
		{
			adapter->hw.mac_type = em_82540;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82545EM_FIBER)
	case E1000_DEV_ID_82545EM_FIBER:
		{
			adapter->hw.mac_type = em_82545;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82545EM_COPPER)
	case E1000_DEV_ID_82545EM_COPPER:
		{
			adapter->hw.mac_type = em_82545;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82546EB_FIBER)
	case E1000_DEV_ID_82546EB_FIBER:
		{
			adapter->hw.mac_type = em_82546;
			break;
		}
    #endif

    #if CHECK_DEV_ID(E1000_DEV_ID_82546EB_COPPER)
	case E1000_DEV_ID_82546EB_COPPER:
		{
			adapter->hw.mac_type = em_82546;
			break;
		}
    #endif

	default:
		{
			INIT_DEBUGOUT1("Unknown device id 0x%x ", adapter->hw.device_id);
			break;
		}
	}
}

static void
em_set_promisc(struct adapter * adapter, int flag)
{
	u_int32_t       reg_rctl;

	reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);

	if (flag)
		reg_rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
	else
		reg_rctl &= ~(E1000_RCTL_MPE | E1000_RCTL_UPE);

	E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
}

/*********************************************************************
 *
 *  Initialize the hardware to a configuration as specified by the
 *  adapter structure. The controller is reset, the EEPROM is
 *  verified, the MAC address is set, then the shared initialization
 *  routines are called.
 *
 **********************************************************************/
static int
em_hardware_init(struct adapter * adapter)
{
	Fstr str;
	extern Fstr encode_bytes(Fstr buf);

	/* Issue a global reset */
	em_reset_hw(&adapter->hw);

	/* Make sure we have a good EEPROM before we read from it */
	if (em_validate_eeprom_checksum(&adapter->hw) < 0) {
		type("\$em: The EEPROM Checksum Is Not Valid");
		cr();
		return -1;
	}
	/* Copy the permanent MAC address and part number out of the EEPROM */
	if (em_read_mac_addr(&adapter->hw) < 0) {
		type("\$em: EEPROM read error while reading mac address");
		cr();
		return -1;
	}

	DEBUGOUT("Spot #0");
	if (em_read_part_num(&adapter->hw, &(adapter->part_num)) < 0) {
		type("\$em: EEPROM read error while reading part number");
		cr();
		return -1;
	}

	DEBUGOUT("Spot #1");

	/* save away our MAC address for OBPTFTP */
	str.str = (char*)&adapter->hw.perm_mac_addr[0];
	str.len = MAC_ADDR;
	property(encode_bytes(str), "\$local-mac-address");
	DEBUGOUT2("Setting local-mac-address ", (int)str.str, str.len);
	DEBUGOUT("Spot #2");
	DEBUGMAC("LOCAL", (uint8_t*)str.str);

	if (em_init_hw(&adapter->hw) < 0) {
		type("\$em: Hardware Initialization Failed");
		cr();
		return -1;
	}
	DEBUGOUT("Spot #3");

	/* get our MAC address, which may not be our local-mac-address */
	str = mac_address();
	DEBUGOUT("Spot #4");

	DEBUGOUT2("Our MAC addr ", (int)str.str, str.len);
	DEBUGMAC("OUR", (uint8_t*)str.str);

	/* use this returned value for our MAC addr */
	if (str.len == MAC_ADDR)
	{
		memcpy((char*)adapter->hw.mac_addr, str.str, str.len);
		property(encode_bytes(str), "\$mac-address");
	}

	DEBUGOUT("Spot #5");
	/* and set it */
	em_init_rx_addrs(&adapter->hw);
	DEBUGOUT("Spot #6");

	{
	    u_int8_t  mta[4 * ETH_LENGTH_OF_ADDRESS];
	    mta[0] = 0x33;
	    mta[1] = 0x33;
	    mta[2] = 0x56;
	    mta[3] = 0x66;
	    mta[4] = 0x31;
	    mta[5] = 0xf1;
	    mta[6+0] = 0x33;
	    mta[6+1] = 0x33;
	    mta[6+2] = 0x0;
	    mta[6+3] = 0x0;
	    mta[6+4] = 0x0;
	    mta[6+5] = 0x1;
	    mta[12+0] = 0x33;
	    mta[12+1] = 0x33;
	    mta[12+2] = 0xff;
	    mta[12+3] = 0x3;
	    mta[12+4] = 0x57;
	    mta[12+5] = 0xc;
	    mta[18+0] = 0x1;
	    mta[18+1] = 0x0;
	    mta[18+2] = 0x5e;
	    mta[18+3] = 0x0;
	    mta[18+4] = 0x0;
	    mta[18+5] = 0x1;
	    em_mc_addr_list_update(&adapter->hw, mta, 4, 0);
	}

	/* setup everything else that needs setting */
	em_set_promisc(adapter, adapter->hw.promiscuous);
	DEBUGOUT("Spot #7");

	em_check_for_link(&adapter->hw);

	if (E1000_READ_REG(&adapter->hw, STATUS) & E1000_STATUS_LU)
		adapter->link_active = 1;
	else
		adapter->link_active = 0;

	if (adapter->link_active) {
		em_get_speed_and_duplex(&adapter->hw, 
					&adapter->link_speed, 
					&adapter->link_duplex);
	} else {
		adapter->link_speed = 0;
		adapter->link_duplex = 0;
	}

	return(0);
}

/**********************************************************************
 *
 *  Update the board statistics counters. 
 *
 **********************************************************************/
static void
em_update_stats_counters(struct adapter *adapter)
{
	adapter->stats.crcerrs += E1000_READ_REG(&adapter->hw, CRCERRS);
	adapter->stats.symerrs += E1000_READ_REG(&adapter->hw, SYMERRS);
	adapter->stats.mpc += E1000_READ_REG(&adapter->hw, MPC);
	adapter->stats.scc += E1000_READ_REG(&adapter->hw, SCC);
	adapter->stats.ecol += E1000_READ_REG(&adapter->hw, ECOL);
	adapter->stats.mcc += E1000_READ_REG(&adapter->hw, MCC);
	adapter->stats.latecol += E1000_READ_REG(&adapter->hw, LATECOL);
	adapter->stats.colc += E1000_READ_REG(&adapter->hw, COLC);
	adapter->stats.dc += E1000_READ_REG(&adapter->hw, DC);
	adapter->stats.sec += E1000_READ_REG(&adapter->hw, SEC);
	adapter->stats.rlec += E1000_READ_REG(&adapter->hw, RLEC);
	adapter->stats.xonrxc += E1000_READ_REG(&adapter->hw, XONRXC);
	adapter->stats.xontxc += E1000_READ_REG(&adapter->hw, XONTXC);
	adapter->stats.xoffrxc += E1000_READ_REG(&adapter->hw, XOFFRXC);
	adapter->stats.xofftxc += E1000_READ_REG(&adapter->hw, XOFFTXC);
	adapter->stats.fcruc += E1000_READ_REG(&adapter->hw, FCRUC);
	adapter->stats.prc64 += E1000_READ_REG(&adapter->hw, PRC64);
	adapter->stats.prc127 += E1000_READ_REG(&adapter->hw, PRC127);
	adapter->stats.prc255 += E1000_READ_REG(&adapter->hw, PRC255);
	adapter->stats.prc511 += E1000_READ_REG(&adapter->hw, PRC511);
	adapter->stats.prc1023 += E1000_READ_REG(&adapter->hw, PRC1023);
	adapter->stats.prc1522 += E1000_READ_REG(&adapter->hw, PRC1522);
	adapter->stats.gprc += E1000_READ_REG(&adapter->hw, GPRC);
	adapter->stats.bprc += E1000_READ_REG(&adapter->hw, BPRC);
	adapter->stats.mprc += E1000_READ_REG(&adapter->hw, MPRC);
	adapter->stats.gptc += E1000_READ_REG(&adapter->hw, GPTC);

	/* For the 64-bit byte counters the low dword must be read first. */
	/* Both registers clear on the read of the high dword */

	adapter->stats.gorcl += E1000_READ_REG(&adapter->hw, GORCL); 
	adapter->stats.gorch += E1000_READ_REG(&adapter->hw, GORCH);
	adapter->stats.gotcl += E1000_READ_REG(&adapter->hw, GOTCL);
	adapter->stats.gotch += E1000_READ_REG(&adapter->hw, GOTCH);

	adapter->stats.rnbc += E1000_READ_REG(&adapter->hw, RNBC);
	adapter->stats.ruc += E1000_READ_REG(&adapter->hw, RUC);
	adapter->stats.rfc += E1000_READ_REG(&adapter->hw, RFC);
	adapter->stats.roc += E1000_READ_REG(&adapter->hw, ROC);
	adapter->stats.rjc += E1000_READ_REG(&adapter->hw, RJC);

	adapter->stats.torl += E1000_READ_REG(&adapter->hw, TORL);
	adapter->stats.torh += E1000_READ_REG(&adapter->hw, TORH);
	adapter->stats.totl += E1000_READ_REG(&adapter->hw, TOTL);
	adapter->stats.toth += E1000_READ_REG(&adapter->hw, TOTH);

	adapter->stats.tpr += E1000_READ_REG(&adapter->hw, TPR);
	adapter->stats.tpt += E1000_READ_REG(&adapter->hw, TPT);
	adapter->stats.ptc64 += E1000_READ_REG(&adapter->hw, PTC64);
	adapter->stats.ptc127 += E1000_READ_REG(&adapter->hw, PTC127);
	adapter->stats.ptc255 += E1000_READ_REG(&adapter->hw, PTC255);
	adapter->stats.ptc511 += E1000_READ_REG(&adapter->hw, PTC511);
	adapter->stats.ptc1023 += E1000_READ_REG(&adapter->hw, PTC1023);
	adapter->stats.ptc1522 += E1000_READ_REG(&adapter->hw, PTC1522);
	adapter->stats.mptc += E1000_READ_REG(&adapter->hw, MPTC);
	adapter->stats.bptc += E1000_READ_REG(&adapter->hw, BPTC);

	if (adapter->hw.mac_type >= em_82543) {
		adapter->stats.algnerrc += 
			E1000_READ_REG(&adapter->hw, ALGNERRC);
		adapter->stats.rxerrc += 
			E1000_READ_REG(&adapter->hw, RXERRC);
		adapter->stats.tncrs += 
			E1000_READ_REG(&adapter->hw, TNCRS);
		adapter->stats.cexterr += 
			E1000_READ_REG(&adapter->hw, CEXTERR);
		adapter->stats.tsctc += 
			E1000_READ_REG(&adapter->hw, TSCTC);
		adapter->stats.tsctfc += 
			E1000_READ_REG(&adapter->hw, TSCTFC);
	}
}

/*********************************************************************
 *
 *  Allocate and initialize transmit structures. 
 *
 **********************************************************************/
static int
em_setup_transmit_structures(struct adapter * adapter)
{
	adapter->first_tx_desc = adapter->tx_desc_base;
	adapter->last_tx_desc =
	adapter->first_tx_desc + (adapter->num_tx_desc - 1);

	/* Setup TX descriptor pointers */
	adapter->next_avail_tx_desc = adapter->first_tx_desc;
	adapter->oldest_used_tx_desc = adapter->first_tx_desc;

	/* Set number of descriptors available */
	adapter->num_tx_desc_avail = adapter->num_tx_desc;

	/* Set checksum context */
	adapter->active_checksum_context = OFFLOAD_NONE;

	return 0;
}

typedef unsigned long u_long;

/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
static void
em_initialize_transmit_unit(struct adapter * adapter)
{
	u_int32_t       reg_tctl;
	u_int32_t       reg_tipg = 0;

	/* Setup the Base and Length of the Tx Descriptor Ring */
	E1000_WRITE_REG(&adapter->hw, TDBAL, adapter->tx_desc_base_pci);
	E1000_WRITE_REG(&adapter->hw, TDBAH, 0);
	E1000_WRITE_REG(&adapter->hw, TDLEN, 
			adapter->num_tx_desc *
			sizeof(struct em_tx_desc));

	/* Setup the HW Tx Head and Tail descriptor pointers */
	E1000_WRITE_REG(&adapter->hw, TDH, 0);
	E1000_WRITE_REG(&adapter->hw, TDT, 0);


	HW_DEBUGOUT2("Base = %x, Length = %x ", 
		     E1000_READ_REG(&adapter->hw, TDBAL),
		     E1000_READ_REG(&adapter->hw, TDLEN));


	/* Set the default values for the Tx Inter Packet Gap timer */
	switch (adapter->hw.mac_type) {
	case em_82543:
		{
			if (adapter->hw.media_type == em_media_type_fiber)
				reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
			else
				reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82544:
		{
			if (adapter->hw.media_type == em_media_type_fiber)
				reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
			else
				reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82540:
		{
			if (adapter->hw.media_type == em_media_type_fiber)
				reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
			else
				reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82545:
		{
			if (adapter->hw.media_type == em_media_type_fiber)
				reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
			else
				reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82546:
		{
			if (adapter->hw.media_type == em_media_type_fiber)
				reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
			else
				reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82542_rev2_0:
		{
			reg_tipg = DEFAULT_82542_TIPG_IPGT;
			reg_tipg |= DEFAULT_82542_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82542_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	case em_82542_rev2_1:
		{
			reg_tipg = DEFAULT_82542_TIPG_IPGT;
			reg_tipg |= DEFAULT_82542_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
			reg_tipg |= DEFAULT_82542_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
			break;
		}
	default:
		{
			type("\$em: Invalid mac type detected");
			cr();
			break;
		}
	}
	E1000_WRITE_REG(&adapter->hw, TIPG, reg_tipg);
	E1000_WRITE_REG(&adapter->hw, TIDV, adapter->tx_int_delay);

	/* Program the Transmit Control Register */
	reg_tctl = E1000_TCTL_PSP | E1000_TCTL_EN |
		   (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT);
	if (adapter->link_duplex == 1) {
		reg_tctl |= E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
	} else {
		reg_tctl |= E1000_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
	}
	E1000_WRITE_REG(&adapter->hw, TCTL, reg_tctl);

	/* Setup Transmit Descriptor Settings for this adapter */   
	adapter->txd_cmd = E1000_TXD_CMD_IFCS;

	if (adapter->tx_int_delay > 0)
		adapter->txd_cmd |= E1000_TXD_CMD_IDE;

	if (adapter->hw.report_tx_early == 1)
		adapter->txd_cmd |= E1000_TXD_CMD_RS;
	else
		adapter->txd_cmd |= E1000_TXD_CMD_RPS;
}

/*********************************************************************
 *
 *  Allocate and initialize receive structures.
 *  
 **********************************************************************/
static int
em_setup_receive_structures(struct adapter * adapter)
{
	adapter->first_rx_desc =
	(struct em_rx_desc *) adapter->rx_desc_base;
	adapter->last_rx_desc =
	adapter->first_rx_desc + (adapter->num_rx_desc - 1);

	/* Setup our descriptor pointers */
	adapter->next_rx_desc_to_check = adapter->first_rx_desc;

	return(0);
}

/*********************************************************************
 *
 *  Enable receive unit.
 *  
 **********************************************************************/
static void
em_initialize_receive_unit(struct adapter * adapter)
{
	u_int32_t       reg_rctl;

	/* Make sure receives are disabled while setting up the descriptor ring */
	E1000_WRITE_REG(&adapter->hw, RCTL, 0);

	/* Set the Receive Delay Timer Register */
	E1000_WRITE_REG(&adapter->hw, RDTR, 
			adapter->rx_int_delay | E1000_RDT_FPDB);

	/* Setup the Base and Length of the Rx Descriptor Ring */
	E1000_WRITE_REG(&adapter->hw, RDBAL, adapter->rx_desc_base_pci);
	E1000_WRITE_REG(&adapter->hw, RDBAH, 0);
	E1000_WRITE_REG(&adapter->hw, RDLEN, adapter->num_rx_desc *
			sizeof(struct em_rx_desc));

	/* Setup the HW Rx Head and Tail Descriptor Pointers */
	E1000_WRITE_REG(&adapter->hw, RDH, 0);
	E1000_WRITE_REG(&adapter->hw, RDT,
			(((u_long) adapter->last_rx_desc -
			  (u_long) adapter->first_rx_desc) >> 4));

	/* Setup the Receive Control Register */
	reg_rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_LBM_NO |
		   E1000_RCTL_RDMTS_HALF |
		   (adapter->hw.mc_filter_type << E1000_RCTL_MO_SHIFT);

	if (adapter->hw.tbi_compatibility_on == TRUE)
		reg_rctl |= E1000_RCTL_SBP;


	switch (adapter->rx_buffer_len) {
	case EM_RXBUFFER_2048:
		{
			reg_rctl |= E1000_RCTL_SZ_2048 | E1000_RCTL_LPE;
			break;
		}
	case EM_RXBUFFER_4096:
		{
			reg_rctl |= E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
			break;            
		}
	case EM_RXBUFFER_8192:
		{
			reg_rctl |= E1000_RCTL_SZ_8192 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
			break;
		}
	case EM_RXBUFFER_16384:
		{
			reg_rctl |= E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
			break;
		}
	default:
		{
			reg_rctl |= E1000_RCTL_SZ_2048;
			break;
		}
	}

	if (adapter->hw.mac_type >= em_82543) {
		u_int32_t       reg_rxcsum;
		reg_rxcsum = E1000_READ_REG(&adapter->hw, RXCSUM);
		reg_rxcsum &= ~(E1000_RXCSUM_IPOFL | E1000_RXCSUM_TUOFL | E1000_RXCSUM_IPV6OFL);
		E1000_WRITE_REG(&adapter->hw, RXCSUM, reg_rxcsum);
	}

	/* Enable Receives */
	E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
}

static void em_enable_vlans(struct adapter *adapter)
{
	uint32_t ctrl;

	E1000_WRITE_REG(&adapter->hw, VET, QTAG_TYPE);

	ctrl = E1000_READ_REG(&adapter->hw, CTRL);
	ctrl |= E1000_CTRL_VME; 
	E1000_WRITE_REG(&adapter->hw, CTRL, ctrl);
}


/*********************************************************************
 *  Device initialization routine
 *
 *  The attach entry point is called when the driver is being loaded.
 *  This routine identifies the type of hardware, allocates all resources 
 *  and initializes the hardware.     
 *  
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
em_open(struct adapter *adapter)
{
	INIT_DEBUGOUT("em_open: begin");

#ifdef DEBUG
	type("\$em_open: 2: adapter->hw.regs ");
	udot((int)adapter->hw.regs);
	cr();
#endif

	/* Determine hardware revision */
	em_identify_hardware(adapter);

	adapter->hw.wait_autoneg_complete = WAIT_FOR_AUTO_NEG_DEFAULT;
	/*adapter->hw.tbi_compatibility_en = TRUE;*/
	adapter->rx_buffer_len = EM_RXBUFFER_2048;
	adapter->rx_checksum = EM_ENABLE_RXCSUM_OFFLOAD;

	adapter->hw.fc_high_water = FC_DEFAULT_HI_THRESH;
	adapter->hw.fc_low_water  = FC_DEFAULT_LO_THRESH;
	adapter->hw.fc_pause_time = FC_DEFAULT_TX_TIMER;
	adapter->hw.fc_send_xon   = TRUE;
	adapter->hw.fc = em_fc_full;

	/* Set the max frame size assuming standard ethernet sized frames */   
	adapter->hw.max_frame_size = 
		ETHERMTU + ETHER_HDR_LEN + ETHER_CRC_LEN;

	adapter->hw.min_frame_size = MINIMUM_ETHERNET_PACKET_SIZE;

	/* This controls when hardware reports transmit completion status. */
	if ((EM_REPORT_TX_EARLY == 0) || (EM_REPORT_TX_EARLY == 1)) {
		adapter->hw.report_tx_early = EM_REPORT_TX_EARLY;
	} else {
		if (adapter->hw.mac_type < em_82543) {
			adapter->hw.report_tx_early = 0;
		} else {
			adapter->hw.report_tx_early = 1;
		}
	}

	if (!em_sw_init(adapter))
	{
		type("\$em: Allocation of PCI resources failed");
		cr();
		return -1;
	}

#ifdef DEBUG
	type("\$em_open: 2: adapter->hw.regs ");
	udot((int)adapter->hw.regs);
	cr();
#endif

	/* Initialize the hardware */
	if (em_hardware_init(adapter)) {
		type("\$em: Unable to initialize the hardware");
		cr();
		return -1;
	}

	em_enable_vlans(adapter);

	/* initialize tx/rx setup */
	em_setup_transmit_structures(adapter);
	em_initialize_transmit_unit(adapter);
	em_setup_receive_structures(adapter);
	em_initialize_receive_unit(adapter);

	/* Initialize statistics */
	em_clear_hw_cntrs(&adapter->hw);
	em_update_stats_counters(adapter);
	adapter->hw.get_link_status = 1;
	em_check_for_link(&adapter->hw);

	typecr("\$Copyright 2002-2003 (c) CodeGen, Inc.  All rights reserved.");
	typecr("\$Copyright 2001-2002 (c) Intel Corporation.  All rights reserved.");
	typecr("\$Intel Gigabit Ethernet Fcode Driver");
	typecr("\$" MANUFACTURER " " BOARD_TYPE " version -- " __DATE__);

	/* Print the link status */
	if (adapter->link_active == 1) {
		em_get_speed_and_duplex(&adapter->hw, &adapter->link_speed, 
					&adapter->link_duplex);
		type("\$Speed:");
		asm(" base @ d# 10 base ! ");
		udot((int)adapter->link_speed);
		asm(" base !");
		type("\$Mbps  Duplex:");
	        type(adapter->link_duplex == FULL_DUPLEX ? "\$Full" : "\$Half");
		cr();
	} else
	{
		typecr("\$em:  Speed:N/A  Duplex:N/A");
		msec_delay(10000);
	}

	INIT_DEBUGOUT("em_open: end");
	return 0;
}

static void
em_enable_intr(struct adapter * adapter)
{
	E1000_WRITE_REG(&adapter->hw, IMS, (IMS_ENABLE_MASK));
}

static void
em_disable_intr(struct adapter *adapter)
{
	E1000_WRITE_REG(&adapter->hw, IMC, 
			(0xffffffff & ~E1000_IMC_RXSEQ));
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC and deallocates TX/RX buffers. 
 *
 **********************************************************************/

static void
em_stop(struct adapter *adapter)
{
	INIT_DEBUGOUT("em_stop: begin");
	em_disable_intr(adapter);
	em_reset_hw(&adapter->hw);
}

/*********************************************************************
 *  Device removal routine
 *
 *  The detach entry point is called when the driver is being removed.
 *  This routine stops the adapter and deallocates all the resources
 *  that were allocated for driver operation.
 *  
 *  return 0 on success, positive on failure
 *********************************************************************/

static void
em_close(struct adapter *adapter)
{
	INIT_DEBUGOUT("em_close: begin");

	em_stop(adapter);
	em_phy_hw_reset(&adapter->hw);
}

static int
em_shutdown(struct adapter *adapter)
{
	em_stop(adapter);
	return 0;
}


/*********************************************************************
 *
 *  This routine executes in interrupt context. It replenishes
 *  the mbufs in the descriptor and sends data which has been
 *  dma'ed into host memory to upper layer.
 *
 *********************************************************************/
static int
em_process_receive_packet(struct adapter *adapter, char *buf, int blen)
{
	u_int16_t           len = -2;
	u_int8_t            accept_frame = 0;
	ushort_t icr;

	/* Pointer to the receive descriptor being examined. */
	struct em_rx_desc   *current_desc;

        DEBUGFUNC("em_process_receive_packet");

	icr = E1000_READ_REG(&adapter->hw, ICR);
	dma_sync(adapter->dma_mem, adapter->map_mem, adapter->mem_len);

	current_desc = adapter->next_rx_desc_to_check;

	if (!((current_desc->status) & E1000_RXD_STAT_DD)) {
#ifdef DEBUG
		DEBUGOUT("Proc_rx_ints: Nothing to do!");

		current_desc = adapter->first_rx_desc;

		/* Dump descriptors */
		while (current_desc <= adapter->last_rx_desc)
		{
			volatile u_int32_t *p;
/*			DEBUGOUT2("RxDesc ", (int)(current_rx_desc - adapter->first_rx_desc),
				current_desc->status); */
		    if (em_debug_level >= 2)
		    {
			type("\$RxDesc ");
			udot((int)(current_desc - adapter->first_rx_desc));
			p = (volatile u_int32_t *)current_desc;
			udot(current_desc->status);
			udot(current_desc->buffer_addr.hi);
			udot(current_desc->buffer_addr.lo);
			udot(current_desc->length);
			udot(current_desc->csum);
			udot(current_desc->status);
			udot(current_desc->errors);
			udot(current_desc->special);
			udot(p[0]);
			udot(p[1]);
			udot(p[2]);
			udot(p[3]);
			cr();
		    }

		    current_desc++;
		}
#endif // DEBUG

		/* nothing received to buffer yet */
		return -2;
	}

	/* Make sure this is also the last descriptor in the packet. */
	if (current_desc->status & E1000_RXD_STAT_EOP) 
	{                /* packet has EOP set - begin */
		/* 
		 * Store the packet length. Take the CRC lenght out of the length 
		 *  calculation.   
		 */
		len = current_desc->length - ETHER_CRC_LEN;

		if (current_desc->errors & E1000_RXD_ERR_FRAME_ERR_MASK) {
			type("\$Proc_rx_ints: errors ");
			udot((int)current_desc->errors);
			cr();
			return -2;
		}

		/*
		 * Only the packets that are valid ethernet packets are processed.
		 * Normally, hardware will discard bad packets . 
		 */

		if ((len <= adapter->hw.max_frame_size) &&
				(len >= adapter->hw.min_frame_size) &&
				((current_desc->errors == 0) || 
					(current_desc->errors == E1000_RXD_ERR_CE)))
		{
			accept_frame = TRUE;

			if (len > blen)
				len = blen;

			/* copy the data into the return buffer */
			memcpy(buf, adapter->rx_bufs + BUF_SIZE *
					(current_desc - adapter->first_rx_desc), len);

#ifdef DEBUG
			if (em_debug_level >= 3)
				DEBUGOUT2("rx buf map-buf ", 
					adapter->rx_bufs + BUF_SIZE *
						(current_desc - adapter->first_rx_desc), 
					current_desc->buffer_addr.lo);
#endif // DEBUG
		}
		else
		{
			type("\$em: Bad packet length: len ");
			udot(len);
			type("\$ errors ");
			udot(current_desc->errors);
			cr();
		}                    /* packet len is good/bad if */
	}                        /* packet has EOP set - end */

	/* Zero out the receive descriptors fields */
	current_desc->status = 0;
	current_desc->length = 0;
	current_desc->csum = 0;
	current_desc->errors = 0;
	current_desc->special = 0;

	/* Advance our pointers to the next descriptor (checking for wrap). */
	if (current_desc == adapter->last_rx_desc)
		adapter->next_rx_desc_to_check = adapter->first_rx_desc;
	else
		adapter->next_rx_desc_to_check++;

#ifdef DEBUG
	if (em_debug_level >= 2)
	{
		type("\$Next RX Desc to check is ");
		udot((unsigned int)current_desc);
		cr();
	}
#endif // DEBUG

	dma_sync(adapter->dma_mem, adapter->map_mem, adapter->mem_len);

	/* Advance the E1000's Receive Queue #0  "Tail Pointer". */
	E1000_WRITE_REG(&adapter->hw, RDT, current_desc - adapter->first_rx_desc);

#ifdef DEBUG
	if (em_debug_level >= 3)
	{
		type("\$Rx Tail pointer is now ");
		udot(current_desc - adapter->first_rx_desc);
		typecr("\$ em_process_receive_packet: done");
	}
#endif // DEBUG

	return len;
}


/**********************************************************************
 *
 *  This routine is called only when em_display_debug_stats is enabled.
 *  This routine provides a way to take a look at important statistics
 *  maintained by the driver and hardware.
 *
 **********************************************************************/
static void
em_print_hw_stats(struct adapter *adapter)
{
#ifdef DBG_STATS
	type("\$em: Packets not Avail = \n");
	udot(adapter->no_pkts_avail);
	cr();
	type("\$em: CleanTxInterrupts = \n");
	udot(adapter->clean_tx_interrupts);
	cr();
#endif

	type("\$em: Tx Descriptors not Avail = ");
	udot((int)adapter->no_tx_desc_avail);
	cr();
	type("\$em: Tx Buffer not avail1 = ");
	udot((int)adapter->no_tx_buffer_avail1);
	cr();
	type("\$em: Tx Buffer not avail2 = ");
	udot((int)adapter->no_tx_buffer_avail2);
	cr();
	type("\$em: Std Mbuf Failed = ");
	udot((int)adapter->mbuf_alloc_failed);
	cr();
	type("\$em: Std Cluster Failed = ");
	udot((int)adapter->mbuf_cluster_failed);
	cr();

	type("\$em: Symbol errors = ");
	udot((int)adapter->stats.symerrs);
	type("\$em: Sequence errors = ");
	udot((int)adapter->stats.sec);
	type("\$em: Defer count = ");
	udot((int)adapter->stats.dc);

	type("\$em: Missed Packets = ");
	udot((int)adapter->stats.mpc);
	cr();
	type("\$em: Receive No Buffers = ");
	udot((int)adapter->stats.rnbc);
	cr();
	type("\$em: Receive length errors = ");
	udot((int)adapter->stats.rlec);
	cr();
	type("\$em: Receive errors = ");
	udot((int)adapter->stats.rxerrc);
	cr();
	type("\$em: Crc errors = ");
	udot((int)adapter->stats.crcerrs);
	cr();
	type("\$em: Alignment errors = ");
	udot((int)adapter->stats.algnerrc);
	cr();
	type("\$em: Carrier extension errors = ");
	udot((int)adapter->stats.cexterr);
	cr();
	type("\$em: Driver dropped packets = ");
	udot(adapter->dropped_pkts);
	cr();

	type("\$em: XON Rcvd = ");
	udot((int)adapter->stats.xonrxc);
	cr();
	type("\$em: XON Xmtd = ");
	udot((int)adapter->stats.xontxc);
	cr();
	type("\$em: XOFF Rcvd = ");
	udot((int)adapter->stats.xoffrxc);
	cr();
	type("\$em: XOFF Xmtd = ");
	udot((int)adapter->stats.xofftxc);
	cr();

	type("\$em: Good Packets Rcvd = ");
	udot((int)adapter->stats.gprc);
	cr();
	type("\$em: Good Packets Xmtd = ");
	udot((int)adapter->stats.gptc);
	cr();
}


/**********************************************************************
 *
 *  Examine each tx_buffer in the used queue. If the hardware is done
 *  processing the packet then free associated resources. The
 *  tx_buffer is put back on the free queue. 
 *
 **********************************************************************/
static void
em_clean_transmit_interrupts(struct adapter * adapter)
{
	adapter = adapter;
#ifdef DBG_STATS
	adapter->clean_tx_interrupts++;
#endif
}

static Bool
em_process_transmit_packet(struct adapter *adapter)
{
    	struct em_tx_desc *current_tx_desc;
	Bool done = FALSE;
/*	int timeout = 10 * 1000;	/ * 10 seconds */
	int timeout = 10 *   10;	/* 10 seconds */
	ushort_t icr;

	/* loop waiting for packet(s) to be transmitted
	 */
	while (!done && timeout > 0)
	{
		/* Get hold of the next descriptor that the E1000 will report status 
		 * back within.
		 */
		icr = E1000_READ_REG(&adapter->hw, ICR);

		dma_sync(adapter->dma_mem, adapter->map_mem, adapter->mem_len);
		current_tx_desc = adapter->oldest_used_tx_desc;
		DEBUGOUT1("em_process_transmit_packet: current_tx_desc ", 
			(int)(current_tx_desc - adapter->first_tx_desc));

		/* Check for wrap case */
		if (current_tx_desc > adapter->last_tx_desc)
		    current_tx_desc -= adapter->num_tx_desc;

		DEBUGOUT1("em_process_transmit_packet: status ",
				current_tx_desc->upper.fields.status);
		if (current_tx_desc->upper.fields.status & E1000_TXD_STAT_DD)
		{
			DEBUGOUT2("em_process_transmit_packet: upper lower ",
					current_tx_desc->upper.data,
					current_tx_desc->lower.data);
			current_tx_desc->lower.data = 0;
			current_tx_desc->upper.data = 0;

			/* Free descriptors used by this packet. */
			if (current_tx_desc == adapter->last_tx_desc)
				adapter->oldest_used_tx_desc = adapter->first_tx_desc;
			else
				adapter->oldest_used_tx_desc++;
			DEBUGOUT1("em_process_transmit_packet: oldest_used_tx_desc ", 
				(int)(adapter->oldest_used_tx_desc - adapter->first_tx_desc));

			/* Make available the descriptor that was previously used */
			adapter->num_tx_desc_avail = adapter->num_tx_desc_avail + 1;
			done = TRUE;	/* and we can return */
#if 1
			DEBUGOUT("Packet sent");
#endif
		}
		else
		{
			ms(10);		/* else wait for a bit */
			timeout -= 10;
		}
	}

	if (!done)
	{
		current_tx_desc = adapter->first_tx_desc;
		/* Check for wrap case */
		while (current_tx_desc <= adapter->last_tx_desc)
		{
		    if (em_debug_level >= 2)
		    {
/*			DEBUGOUT2("TxDesc ", (int)(current_tx_desc - adapter->first_tx_desc),
				current_tx_desc->upper.fields.status); */
			type("\$TxDesc ");
			udot((int)(current_tx_desc - adapter->first_tx_desc));
			udot(current_tx_desc->upper.fields.status);
			udot(current_tx_desc->buffer_addr.hi);
			udot(current_tx_desc->buffer_addr.lo);
			udot(current_tx_desc->lower.data);
			udot(current_tx_desc->upper.data);
			cr();
		    }

		    current_tx_desc++;
		}
	}

	return done;
}

static Bool
em_send_buffer(struct adapter *adapter, char *buf, int len)
{
    	struct em_tx_desc *current_tx_desc;
	char *txbuf;

#ifdef DEBUG
	if (em_debug_level >= 3)
		DEBUGFUNC("em_send_buffer");
#endif // DEBUG

	DEBUGOUT1("em_send_buffer: num_tx_desc_aval ", adapter->num_tx_desc_avail);
	DEBUGOUT1("em_send_buffer: num_tx_desc ", adapter->num_tx_desc);

	if (adapter->num_tx_desc_avail <= 1)
		return FALSE;

	current_tx_desc = adapter->next_avail_tx_desc;
	DEBUGOUT1("em_send_buffer: TDBAL ", E1000_READ_REG(&adapter->hw, TDBAL));
	DEBUGOUT1("em_send_buffer: TDBAH ", E1000_READ_REG(&adapter->hw, TDBAH));
	DEBUGOUT1("em_send_buffer: TDLEN ", E1000_READ_REG(&adapter->hw, TDLEN));
	DEBUGOUT1("em_send_buffer: TDH ", E1000_READ_REG(&adapter->hw, TDH));
	DEBUGOUT1("em_send_buffer: TDT ", E1000_READ_REG(&adapter->hw, TDT));
	DEBUGOUT1("em_send_buffer: TIPG ", E1000_READ_REG(&adapter->hw, TIPG));
	DEBUGOUT1("em_send_buffer: TIDV ", E1000_READ_REG(&adapter->hw, TIDV));
	DEBUGOUT1("em_send_buffer: TCTL ", E1000_READ_REG(&adapter->hw, TCTL));
	DEBUGOUT1("em_send_buffer: dma_mem ", adapter->dma_mem);
	DEBUGOUT1("em_send_buffer: map_mem ", adapter->map_mem);
	DEBUGOUT1("em_send_buffer: mem_len ", adapter->mem_len);
	DEBUGOUT1("em_send_buffer: first_tx ", adapter->first_tx_desc);
	DEBUGOUT1("em_send_buffer: last_tx ", adapter->last_tx_desc);
	DEBUGOUT1("em_send_buffer: next_avail_tx ", adapter->next_avail_tx_desc);
	DEBUGOUT1("em_send_buffer: num_tx ", adapter->num_tx_desc);
	DEBUGOUT1("em_send_buffer: num_avail_tx ", adapter->num_tx_desc_avail);
	DEBUGOUT1("em_send_buffer: tx_int_delay ", adapter->tx_int_delay);
	DEBUGOUT1("em_send_buffer: current_tx_desc ", 
		(int)(adapter->next_avail_tx_desc - adapter->first_tx_desc));
	txbuf = adapter->tx_bufs + BUF_SIZE *
			(current_tx_desc - adapter->first_tx_desc);
	DEBUGOUT1("em_send_buffer: txbuf ", (int)txbuf);
	memcpy(txbuf, buf, len);
	current_tx_desc->lower.data = len;

	/* zero out the status field in the descriptor. */
	current_tx_desc->upper.data = 0;

	/* Check the wrap-around case */
	if (current_tx_desc == adapter->last_tx_desc)
		adapter->next_avail_tx_desc = adapter->first_tx_desc;
	else
		adapter->next_avail_tx_desc++;

	DEBUGOUT1("em_send_buffer: next_avail_tx_desc ", 
		(int)(adapter->next_avail_tx_desc - adapter->first_tx_desc));

	adapter->num_tx_desc_avail = adapter->num_tx_desc_avail - 1;

	/*
	 * If there is a valid value for the transmit interrupt delay, set up the
	 * delay in the descriptor field
	 */
	if (adapter->tx_int_delay) {
		current_tx_desc->lower.data |=
				(E1000_TXD_CMD_EOP | E1000_TXD_CMD_IDE | E1000_TXD_CMD_IFCS);
	} else {
		current_tx_desc->lower.data |=
				(E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS);
	}

	/* Set the RS or the RPS bit by looking at the ReportTxEarly setting */
	current_tx_desc->lower.data |= E1000_TXD_CMD_RS;
	//current_tx_descriptor->lower.data |= E1000_TXD_CMD_RPS;

	DEBUGOUT1("em_process_transmit_packet: status before ",
			current_tx_desc->upper.fields.status);
	dma_sync(adapter->dma_mem, adapter->map_mem, adapter->mem_len);

	/* Advance the Transmit Descriptor Tail (Tdt), this tells the 
	 * E1000 that this frame is available to transmit. 
	 */
	E1000_WRITE_REG(&adapter->hw, TDT,
			adapter->next_avail_tx_desc - adapter->first_tx_desc);

	DEBUGOUT1("em_send_buffer: TDT ", (int)(adapter->next_avail_tx_desc - adapter->first_tx_desc));
	DEBUGOUT1("em_send_buffer: tx_int_delay ",  adapter->tx_int_delay);
	DEBUGOUT1("PCI config CSR ", config_getl((my_space() & 0xFFFFFF00) | PCI_CONFIG_COMMAND));
	DEBUGOUT1("em_process_transmit_packet: status after ",
			current_tx_desc->upper.fields.status);
	/* and wait until the packet is actually sent */
	return em_process_transmit_packet(adapter);
}

/* END HACKED INTEL CODE */

/* package variables (globals) */

static struct adapter g_adapter;

static volatile unsigned char *g_mmio = NULL;
static int g_mmiosize = 0;
static Instance g_obptftp = NULL;

/* some handy routines */

static Bool
fstreq(Fstr s1, Fstr s2)
{
	Bool ret = FALSE;

	if (s1.len == s2.len)
		ret = (comp(s1.str, s2.str, s1.len) == 0);

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
	Bool promiscuous;
	int speed;
	Bool fullduplex;
	struct adapter *adapter;

#ifdef DEBUG
	type("\$Start of open"); cr();
	asm(" .s");
#endif
	DEBUGFUNC("open");


	addr = my_space() & 0xFFFFFF00;

	/* init globals */
	promiscuous = FALSE;
	fullduplex = TRUE;
	speed = 0;	/* auto-negotiate */
	adapter = &g_adapter;
	memset((char *)&g_adapter, 0, sizeof g_adapter);

	/* parse our assigned-addresses property to find our register memory-map */
	pe = get_my_property("\$assigned-addresses");

	if (pe.err)
	{
		type("\$cannot find assigned-addresses property");
		cr();
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

#ifdef DEBUG
	type("\$em.open: prop.len=");
	udot((int)pi.prop.len);
	cr();

	type("\$em.open: orig phys=");
	udot((int)physhi);
	type("\$.");
	udot((int)physmid);
	type("\$.");
	udot((int)physlo);
	type("\$ size=");
	udot((int)sizehi);
	type("\$.");
	udot((int)sizelo);
	cr();
#endif

	physhi &= 0x7FFFFFFF;
	physmid = 0;
	physlo = 0;

#ifdef DEBUG
	type("\$em.open: prop.len=");
	udot((int)pi.prop.len);
	cr();

	type("\$em.open: phys=");
	udot((int)physhi);
	type("\$.");
	udot((int)physmid);
	type("\$.");
	udot((int)physlo);
	type("\$ size=");
	udot((int)sizehi);
	type("\$.");
	udot((int)sizelo);
	cr();
#endif

	/* assume first memory-space map is for our registers */
	g_mmiosize = sizelo;
	g_mmio = (unsigned char*)map_in(physlo, physmid, physhi, g_mmiosize);

#ifdef DEBUG
	type("\$em.open: mmio/size=");
	udot((int)g_mmio);
	type("\$/");
	udot(g_mmiosize);
	cr();
#endif

	if (g_mmio == NULL)
	{
		type("\$em.open: could not map in registers!");
		cr();
		return FALSE;
	}

	/* parse our args and set various options */
	args = my_args();
	DEBUGOUT2("args ", (int)args.str, args.len);
	args2 = left_parse_string(args, ',');
	DEBUGOUT2("args2 left ", (int)args2.left.str, args2.left.len);
	DEBUGOUT2("args2 right ", (int)args2.right.str, args2.right.len);

	while (args2.left.len > 0)
	{
		Bool nextarg = TRUE;
		Fstr arg = args2.left;

		DEBUGOUT2("arg ", (int)arg.str, arg.len);
		type("\$arg = ");
		type(arg);
		cr();
		//type("\$fstreq(arg, 'speed=10') = ");
		//udot(fstreq(arg, "\$speed=10"));
		//cr();

		if (fstreq(arg, "\$promiscuous"))
			promiscuous = TRUE;
		else if (fstreq(arg, "\$speed=auto"))
			speed = 0;
		else if (fstreq(arg, "\$speed=10"))
			speed = 10;
		else if (fstreq(arg, "\$speed=100"))
			speed = 100;
		else if (fstreq(arg, "\$speed=1000"))
			speed = 1000;
		else if (fstreq(arg, "\$duplex=half"))
			fullduplex = FALSE;
		else if (fstreq(arg, "\$duplex=full"))
			fullduplex = TRUE;
		else
			nextarg = FALSE;

		DEBUGOUT1("nextarg ", nextarg);
		args = args2.right;
		DEBUGOUT2("args ", (int)args.str, args.len);

		if (nextarg)
		{
			args2 = left_parse_string(args, ',');
			DEBUGOUT2("args2 left ", (int)args2.left.str, args2.left.len);
			DEBUGOUT2("args2 right ", (int)args2.right.str, args2.right.len);
		}
		else
			args2.left.len = 0;
	}

	/* create an instance of the obp-tftp package, passing it the
	   any remaining unparsed args */
	DEBUGOUT1("promiscuous ", promiscuous);

	/* setup pointer to registers and other fields */
	g_adapter.hw.regs = (uint32_t *)g_mmio;

#ifdef DEBUG
	type("\$g_adapter.hw.regs ");
	udot((int)g_adapter.hw.regs);
	cr();
#endif
	g_adapter.hw.promiscuous = promiscuous;

	/* setup auto-negotiation or forced speed/duplex */
	if (speed == 0 || speed == 1000)
	{
		g_adapter.hw.autoneg = DO_AUTO_NEG;

		if (speed == 1000)
			g_adapter.hw.autoneg_advertised = ADVERTISE_1000_FULL;
		else
			g_adapter.hw.autoneg_advertised = AUTONEG_ADV_DEFAULT;
	}
	else
	{
		/* generates the proper enum SPEED_DUPLEX_TYPE */
		g_adapter.hw.autoneg = FALSE;
		g_adapter.hw.autoneg_advertised = 0;

		if (speed == 100)
			if (fullduplex == TRUE)
				g_adapter.hw.forced_speed_duplex = em_100_full;
			else
				g_adapter.hw.forced_speed_duplex = em_100_half;
		else
			if (fullduplex == TRUE)
				g_adapter.hw.forced_speed_duplex = em_10_full;
			else
				g_adapter.hw.forced_speed_duplex = em_10_half;
	}

	DEBUGOUT2("speed ", speed, g_adapter.hw.autoneg);
	DEBUGOUT2("    advert forced ", g_adapter.hw.autoneg_advertised,
			g_adapter.hw.forced_speed_duplex);

	/* Dump BARs */
	cfg = config_getl(addr | PCI_CONFIG_BAR0);
	DEBUGOUT1("PCI config BAR0 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_BAR1);
	DEBUGOUT1("PCI config BAR1 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_BAR2);
	DEBUGOUT1("PCI config BAR2 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_BAR3);
	DEBUGOUT1("PCI config BAR3 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_BAR4);
	DEBUGOUT1("PCI config BAR4 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_BAR5);
	DEBUGOUT1("PCI config BAR5 ", cfg);
	cfg = config_getl(addr | PCI_CONFIG_ROMADDR);
	DEBUGOUT1("PCI config ROMADDR ", cfg);

	/* enable I/O and memory */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	DEBUGOUT1("PCI config CSR ", cfg);
	config_setl((cfg & 0xFFFF) | 
			PCI_COMMAND_MEMENABLE | PCI_COMMAND_MASTERENABLE,
			addr | PCI_CONFIG_COMMAND);
	DEBUGOUT1("PCI config CSR new ", config_getl(addr | PCI_CONFIG_COMMAND));
	DEBUGOUT1("PCI pci@4 CSR ", config_getl(0x012000 | PCI_CONFIG_COMMAND));
	DEBUGOUT1("PCI pci@1,1 CSR ", config_getl(0x0900 | PCI_CONFIG_COMMAND));

	if (force_open)
	    return TRUE;


#ifdef DEBUG
	type("\$Before em_open in open"); cr();
	asm(" .s");
#endif

	/* initialize the adapter */
	if (em_open(adapter) != 0)
	{
		type("\$em.open: could not open adapter!");
		cr();
		map_out((char*)g_mmio, g_mmiosize);
		g_mmio = NULL;
		return FALSE;
	}

	g_obptftp = dopen_package(args, "\$obp-tftp");

	if (g_obptftp == NULL)
	{
		type("\$em.open: could not create instance of obp-tftp!");
		cr();
		em_close(adapter);
		map_out((char*)g_mmio, g_mmiosize);
		g_mmio = NULL;
		return FALSE;
	}

	/* let the user know the MAC addr to help configure BOOTP */
	if (diag)
	{
		type("\$Ethernet MAC addr: ");

		for (i = 0; i < 6; i++)
			udot(g_adapter.hw.mac_addr[i]);

		cr();
	}

#ifdef DEBUG
	type("\$End of open"); cr();
	asm(" .s");
#endif

	return TRUE;
}

void
close(void)			/* close (--) */
{
	int cfg;
	int addr;

	if (g_obptftp)
		close_package(g_obptftp);

	addr = my_space() & 0xFFFFFF00;

	em_close(&g_adapter);

	/* turn I/O and memory access off */
	cfg = config_getl(addr | PCI_CONFIG_COMMAND);
	config_setl(cfg & 0xFFF8, addr | PCI_CONFIG_COMMAND);
	
	if (g_mmio)
		map_out((char*)g_mmio, g_mmiosize);
	
	g_obptftp = NULL;
	g_mmio = NULL;
}

int
read(char *buf, int len)		/* read (addr len -- actual) */
{
	return em_process_receive_packet(&g_adapter, buf, len);
}

int
write(char *buf, int len)		/* write (addr len -- actual) */
{
	return (em_send_buffer(&g_adapter, buf, len)) ? len : 0;
}

inline Instance get_myself() { asm(" my-self"); }
inline void set_myself(Instance) { asm(" to my-self"); }

int
ee_read(int offset)
{
	Instance oldinst;
	Instance inst;
	uint16_t data;
	int ok;

	oldinst = get_myself();
	force_open = 1;
	inst = open_dev("\$netnv");
	force_open = 0;

	if (inst == NULL)
	{
		type("\$Unable to open netnv");
		cr();
		return -1;
	}

	set_myself(inst);
	ok = (em_read_eeprom(&g_adapter.hw, offset, &data) == 0);
	set_myself(oldinst);
	close_dev(inst);

	if (!ok)
	{
		type("\$Unable to read eeprom");
		cr();
		return -1;
	}

	return data;
}

void
ee_write(int data, int offset)
{
	Instance oldinst;
	Instance inst;
	int ok;

	oldinst = get_myself();
	force_open = 1;
	inst = open_dev("\$netnv");
	force_open = 0;

	if (inst == NULL)
	{
		type("\$Unable to open netnv");
		cr();
		return;
	}

	set_myself(inst);
	ok = (em_write_eeprom(&g_adapter.hw, offset, data) == 0);
	set_myself(oldinst);
	close_dev(inst);

	if (!ok)
	{
		type("\$Unable to write eeprom");
		cr();
	}
}

static void
ee_fill_array(u_int16_t *data)
{
#include "eeprom.h"
}

void
ee_prog()
{
	Instance oldinst;
	Instance inst;
	int ok = TRUE;
	u_int16_t data[64];
	int i;

	oldinst = get_myself();
	force_open = 1;
	inst = open_dev("\$netnv");
	force_open = 0;

	if (inst == NULL)
	{
		type("\$Unable to open netnv");
		cr();
		return;
	}

	set_myself(inst);
	ee_fill_array(data);

	for (i = 0; i < 64; i++)
		ok = (em_write_eeprom(&g_adapter.hw, i, data[i]) == 0)&& ok;

	set_myself(oldinst);
	close_dev(inst);

	if (!ok)
	{
		type("\$Unable to program eeprom");
		cr();
	}
}

void
ee_dump()
{
	Instance oldinst;
	Instance inst;
	int ok = TRUE;
	u_int16_t data;
	int i;

	oldinst = get_myself();
	force_open = 1;
	inst = open_dev("\$netnv");
	force_open = 0;

	if (inst == NULL)
	{
		type("\$Unable to open netnv");
		cr();
		return;
	}

	set_myself(inst);

	for (i = 0; i < 64; i++)
	{
		ok = (em_read_eeprom(&g_adapter.hw, i, &data) == 0)&& ok;
		udot(i);
		udot(data);
		cr();
	}

	set_myself(oldinst);
	close_dev(inst);

	if (!ok)
	{
		type("\$Unable to program eeprom");
		cr();
	}
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
		type("\$em selftest...");

	err = R_OK;		/* TODO */

	if (err)
	{
		if (diag)
		{
			type("\$FAILED!");
			cr();
		}
	}
	else
	{
		if (diag)
		{
			type("\$ok");
			cr();
		}
	}

	return err;
}

static int
map_flash(unsigned char volatile **fbase, int *fsize)
{
	int physhi, physmid, physlo;
	int sizehi, sizelo;
	Prop_err pe;
	Prop_int pi;
	Fstr args;
	Two_fstrs args2;
	Bool diag = diagnostic_mode();
	int addr;
	unsigned char volatile *fb;
	int fs;

	diag = diag;
	addr = my_space() & 0xFFFFFF00;

	/* parse our assigned-addresses property to find our register memory-map */
	pe = get_my_property("\$assigned-addresses");

	if (pe.err)
	{
		type("\$cannot find assigned-addresses property");
		cr();
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

	} while (pi.prop.len > 0 && (PCI_PHYSHI_SPACE(physhi) != PCI_SPACE_MEM || PCI_PHYSHI_REG(physhi) != 0x30));

#ifdef DEBUG
	type("\$em.flash_update: prop.len=");
	udot((int)pi.prop.len);
	cr();

	type("\$em.flash_update: orig phys=");
	udot((int)physhi);
	type("\$.");
	udot((int)physmid);
	type("\$.");
	udot((int)physlo);
	type("\$ size=");
	udot((int)sizehi);
	type("\$.");
	udot((int)sizelo);
	cr();
#endif

	physhi &= 0x7FFFFFFF;
	physmid = 0;
	physlo = 0;

#ifdef DEBUG
	type("\$em.flash_update: prop.len=");
	udot((int)pi.prop.len);
	cr();

	type("\$em.flash_update: phys=");
	udot((int)physhi);
	type("\$.");
	udot((int)physmid);
	type("\$.");
	udot((int)physlo);
	type("\$ size=");
	udot((int)sizehi);
	type("\$.");
	udot((int)sizelo);
	cr();
#endif

	/* assume first memory-space map is for our registers */
	fs = sizelo;
	fb = (unsigned char*)map_in(physlo, physmid, physhi, fs);

#ifdef DEBUG
	type("\$em.flash_update: fbase/fsize=");
	udot((int)fb);
	type("\$/");
	udot(fs);
	cr();
#endif

	if (fb == NULL)
	{
		type("\$em.flash_update: could not map in registers!");
		cr();
		return FALSE;
	}

	DEBUGOUT1("PCI config CSR ", config_getl(addr | PCI_CONFIG_COMMAND));

	*fbase = fb;
	*fsize = fs;

	return TRUE;
}

int
flash_update(char *buf, int len)
{
	Bool diag = diagnostic_mode();
	unsigned char volatile *fbase;
	int fsize;
	struct adapter *adapter;
	int addr;
	int cfg;
	int dev;
	int mfg;

	DEBUGOUT("flash-update");

	addr = my_space() & 0xFFFFFF00;

	if (!map_flash(&fbase, &fsize))
	{
		DEBUGOUT("could not map flash memory space");
		return FALSE;
	}

	DEBUGOUT2("mapped flash memory space to ", fbase, fsize);

	buf = buf;
	len = len;
	diag = diag;

	adapter = &g_adapter;
	E1000_WRITE_REG(&adapter->hw, EECD, E1000_EECD_FWE_EN);
	cfg = config_getl(addr | PCI_CONFIG_ROMADDR);
	DEBUGOUT1("PCI ROMADDR ", cfg);
	config_setl(cfg | 1, addr | PCI_CONFIG_ROMADDR);
	DEBUGOUT1("PCI ROMADDR ", config_getl(addr | PCI_CONFIG_ROMADDR));

	fbase[0x555] = 0xF0;
	fbase[0x555] = 0xAA;
	fbase[0x2AA] = 0x55;
	fbase[0x555] = 0x90;
	mfg = fbase[0];
	dev = fbase[1];
	fbase[0x555] = 0xF0;

	config_setl(cfg, addr | PCI_CONFIG_ROMADDR);
	E1000_WRITE_REG(&adapter->hw, EECD, E1000_EECD_FWE_DIS);
	map_out((char*)fbase, fsize);

	DEBUGOUT2("Flash mfg, dev ", mfg, dev);
	return FALSE;
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
	physhi |= my_space();

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
make_compatible_property()
{
    Fstr s1, s2;

#if defined(MANUFACTURER) && defined(BOARD_TYPE)
    s1 = encode_string("\$" MANUFACTURER "," BOARD_TYPE);
#endif

#if defined(DEVICE_ID) && defined(SUBSYSTEM_VENDOR_ID) && \
	defined(SUBSYSTEM_ID) && defined(REVISION)
    s2 = encode_string("\$pci" ID2STR(VENDOR_ID) "," ID2STR(DEVICE_ID) "."
	    ID2STR(SUBSYSTEM_VENDOR_ID) "." ID2STR(SUBSYSTEM_ID) "."
	    ID2STR(REVISION));
    s1 = encodeplus(s1, s2);
#endif

#if defined(DEVICE_ID) && \
	defined(SUBSYSTEM_VENDOR_ID) && defined(SUBSYSTEM_ID)
    s2 = encode_string("\$pci" ID2STR(VENDOR_ID) "," ID2STR(DEVICE_ID) "."
	    ID2STR(SUBSYSTEM_VENDOR_ID) "." ID2STR(SUBSYSTEM_ID));
    s1 = encodeplus(s1, s2);
#endif

#if defined(SUBSYSTEM_VENDOR_ID) && defined(SUBSYSTEM_ID)
    s2 = encode_string("\$pci" ID2STR(SUBSYSTEM_VENDOR_ID) "."
	    ID2STR(SUBSYSTEM_ID));
    s1 = encodeplus(s1, s2);
#endif

#if defined(DEVICE_ID) && defined(REVISION)
    s2 = encode_string("\$pci" ID2STR(VENDOR_ID) "," ID2STR(DEVICE_ID) "."
	    ID2STR(REVISION));
    s1 = encodeplus(s1, s2);
#endif

#if defined(DEVICE_ID)
    s2 = encode_string("\$pci" ID2STR(VENDOR_ID) "," ID2STR(DEVICE_ID));
    s1 = encodeplus(s1, s2);
#endif

    /* hard-wired for ethernet device class */
    s2 = encode_string("\$pciclass,020000");
    s1 = encodeplus(s1, s2);
    s2 = encode_string("\$pciclass,0200");
    s1 = encodeplus(s1, s2);

    property(s1, "\$compatible");
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
#ifdef BLADE100NETBOOT
	make_blade100_props();
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
	property(encode_int(ETH_LENGTH_OF_ADDRESS * 8), "\$address-bits");
	property(encode_int(E1000_TX_BUFFER_SIZE), "\$maximum-frame-size");

	make_compatible_property();

#ifdef STANDALONE
	// needs to be here so it is the last thing executed
	finish_device();
	close_dev(my_self());
#endif /* STANDALONE */
}
