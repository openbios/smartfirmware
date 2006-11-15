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

/* (c) Copyright 1996-1999 by CodeGen, Inc.  All Rights Reserved. */

/*	Implementation of the "obp-tftp" package for SmartFirwmare.
	Supports DHCP, BOOTP, TFTP, ARP, RARP, and PING.
	(Note that network byte-order is always big-endian.)
 */

#include "defs.h"

#undef DPRINTF
#define DPRINTF(args) if (g_e->debug) dprintf args
#define MORE_DEBUG

#ifndef __GNUC__
#	define __attribute__(arg)	/* @#$%ing Gnu compiler */
#endif

/* initial timeout in milliseconds - this is increased as needed */
#ifndef NET_TIMEOUT
#	define NET_TIMEOUT		500
#endif

#ifndef TFTP_TIMEOUT
#	define TFTP_TIMEOUT		111
#endif

/* special return types - for internal obptftp use only
 */
enum Pkt_type
{
	PKT_NONE = NO_ERROR,
	PKT_IP,
	PKT_ARP,
	PKT_ARPR,
	PKT_RARP,
	PKT_RARPR,
	PKT_ICMP,
	PKT_UDP,
	PKT_BOOTP,
	PKT_TFTP,
	PKT_PING
};

#define IP_ADDR_SIZE		4

#define RETRIES	15		/* number of retries */

/* MAC stuff */
#define MAC_ADDR_SIZE		6
#define MAC_BUFFER_SIZE		1514	/* 1600? */

/* MAC ethernet protocols */
#define MAC_IP1		0x08
#define MAC_IP2		0x00
#define MAC_ARP1	0x08
#define MAC_ARP2	0x06
#define MAC_RARP1	0x80
#define MAC_RARP2	0x35

struct mac_header
{
	uByte dest_addr[MAC_ADDR_SIZE];
	uByte src_addr[MAC_ADDR_SIZE];
	uByte pkt_type[2];		/* protocols as defined above */
	/* n data uBytes */
} __attribute__((packed));

#define MAC_HEADER_SIZE		14
#define MAC_HEADER_OFFSET	0


/* ARP/RARP stuff */
#define ARP_REQUEST 	1
#define ARP_REPLY		2
#define RARP_REQUEST 	3
#define RARP_REPLY		4

/* this struct is suitable only for Ethernet + IP packets */
struct arp_packet
{
	uByte hardware[2];		/* hardware address space - 0x0001 for Ethernet */
	uByte protocol[2];		/* protocol address space - 0x0800 for IP */
	uByte hardware_len;		/* byte length of each hardware address (6) */
	uByte protocol_len;		/* byte length of each protocol address (4) */
	uByte op[2];			/* opcode - op[0]=0, op[1]=ARP_REQUEST,ARP_REPLY */
	uByte src_mac[MAC_ADDR_SIZE];	/* hardware addr of sender */
	uByte src_ip[IP_ADDR_SIZE];	/* protocol addr of sender */
	uByte target_mac[MAC_ADDR_SIZE];	/* hw addr of target if known */
	uByte target_ip[IP_ADDR_SIZE];	/* protocol addr of target */
} __attribute__((packed));

#define ARP_PACKET_SIZE		28
#define ARP_PACKET_OFFSET	(MAC_HEADER_OFFSET + MAC_HEADER_SIZE)


/* IP stuff */
struct ip_header
{	
	uByte vers_hdr;			/* 0..3: vers, 4..7: len */
	uByte service_type;
	uByte len[2];
	uByte id[2];
	uByte flag_offset[2];	/* 0..3: flags 4..15 fragment offset */
	uByte time;
	uByte protocol;
	uByte checksum[2];
	uByte src_addr[IP_ADDR_SIZE];
	uByte dest_addr[IP_ADDR_SIZE];
#ifdef SWITCHED_OFF
	uByte options[4];		/* not necessary for our IP packets */
#endif
	/* n data uBytes */
} __attribute__((packed));

#define IP_HEADER_SIZE		20
#define IP_HEADER_OFFSET	(MAC_HEADER_OFFSET + MAC_HEADER_SIZE)


/* ICMP stuff */
#define ICMP_PROTOCOL			1

struct icmp_header
{
	uByte type;
	uByte code;
	uByte checksum[2];
} __attribute__((packed));

#define ICMP_HEADER_SIZE	4
#define ICMP_HEADER_OFFSET	(IP_HEADER_OFFSET + IP_HEADER_SIZE)
#define ICMP_PACKET_OFFSET	(ICMP_HEADER_OFFSET + ICMP_HEADER_SIZE)


/* ping stuff */
#define ICMP_ECHO			8	/* ICMP type */
#define ICMP_ECHO_REPLY		0

#define PING_ID				0xDEAF		/* for id field */

struct ping_packet
{
	uShort id;
	uShort seq;
	uByte data[16];		/* arbitrary amount */
} __attribute__((packed));

#define PING_PKT_SIZE	20


/* UDP stuff */
#define UDP_PROTOCOL	17

struct udp_header
{
	uByte src_port[2];
	uByte dest_port[2];
	uByte len[2];
	uByte checksum[2];
	/* n data uBytes */
} __attribute__((packed));

#define UDP_HEADER_SIZE		8
#define UDP_HEADER_OFFSET	(IP_HEADER_OFFSET + IP_HEADER_SIZE)

#define UDP_PACKET_OFFSET		(UDP_HEADER_OFFSET + UDP_HEADER_SIZE)


/* BOOTP stuff */
#define BOOTP_SERVER_PORT		67
#define BOOTP_CLIENT_PORT		68

#define BOOT_REQUEST	1
#define BOOT_REPLY		2

#define BOOT_FLAG_BROADCAST		0x80

#define FILE_NAME_SIZE		128

#define OPT_PAD				0
#define OPT_REQUEST_IP		50		/* len=4 IP-addr */
#define OPT_IP_LEASE_TIME	51		/* len=4 time */
#define OPT_OVERLOAD		52		/* len=1 type */
#define OPT_DHCP			53		/* len=1 type */
#define OPT_SERVER_IP		54		/* len=4 IP-addr */
#define OPT_PARAM_REQUEST	55		/* len=n>=1 DHCP-options */
#define OPT_MESSAGE			56		/* len=n>=1 string */
#define OPT_MAX_SIZE		57		/* len=2 length */
#define OPT_RENEWAL_TIME	58		/* len=4 time */
#define OPT_REBINDING_TIME	59		/* len=4 time */
#define OPT_VENDOR_ID		60		/* len=n>=1 unique-ID */
#define OPT_CLIENT_ID		61		/* len=n>=2 unique-ID */
#define OPT_TFTP_SERVER		66		/* len=n>=1 string */
#define OPT_BOOT_FILE		67		/* len=n>=1 string */
#define OPT_END				255

/* OPT_OVERLOAD types */
#define DHCP_OV_FILE		1
#define DHCP_OV_SNAME		2
#define DHCP_OV_BOTH		3

/* OPT_DHCP types */
#define DHCP_DISCOVER		1
#define DHCP_OFFER			2
#define DHCP_REQUEST		3
#define DHCP_DECLINE		4
#define DHCP_ACK			5
#define DHCP_NAK			6
#define DHCP_RELEASE		7
#define DHCP_INFORM			8

struct bootp_packet
{
	uByte op;				/* either BOOT_REQUEST or BOOT_REPLY */
	uByte htype;			/* hardware type; 1 = 10Mb Ethernet */
	uByte hlen;				/* hardware addr length; 6 = 10Mb Ethernet*/
	uByte hops;				/* client set to zero */
	uByte xid[4];			/* random transaction ID number */
	uByte secs[2];			/* seconds elapsed since attempting to boot */
	uByte flags[2];			/* option flags - BROADCAST */
	uByte ciaddr[IP_ADDR_SIZE];		/* client IP addr, if known */
	uByte yiaddr[IP_ADDR_SIZE];		/* "your" IP addr, returned by server */
	uByte siaddr[IP_ADDR_SIZE];		/* server's IP addr, returned by server */
	uByte giaddr[IP_ADDR_SIZE];		/* gateway's IP addr; cross-gateway boot */
	uByte chaddr[16];			/* client hardware address, ie MAC addr */
	uByte sname[64];			/* optional server host name */
	char file[FILE_NAME_SIZE];	/* null-term'd boot file, returned by server */
	uByte options[64];			/* optional data - may be up to max pkt size */
} __attribute__((packed));

#define BOOTP_PKT_SIZE		300


/* TFTP stuff */
#define TFTP_DEST_PORT		69
#define TFTP_RECV_PORT		0x8000

#define TFTP_RRQ		1
#define TFTP_WRQ		2
#define TFTP_DATA		3
#define TFTP_ACK		4
#define TFTP_ERROR		5

#define TFTP_XFER_SIZE	512

struct tftp_rwrq_packet
{
	uByte op[2];			/* TFTP_RRQ or TFTP_WRQ */
	char filename[1];		/* null-terminated filename */
	/* null-terminated "mode" string follows filename */
} __attribute__((packed));

#define TFTP_RWRQ_PKT_SIZE	3

struct tftp_packet
{
	uByte op[2];			/* TFTP_DATA, TFTP_ACK, TFTP_ERROR */
	uByte block[2];			/* for DATA or ACK packets */
	uByte data[TFTP_XFER_SIZE];
			/* n uBytes for DATA, nothing for ACK, string for ERROR */
} __attribute__((packed));

#define TFTP_PKT_SIZE		(TFTP_XFER_SIZE + 4)


/* instance-specific storage, stashed in e->currinst->self */
struct self
{
	uByte sendbuf[MAC_BUFFER_SIZE];			/* buffers for I/O */
	uByte _align1[8 + (8 - (MAC_BUFFER_SIZE & 0x7))];	/* unused */
	uByte recvbuf[MAC_BUFFER_SIZE];			/* (here for alignment) */
	uByte _align2[8 + (8 - (MAC_BUFFER_SIZE & 0x7))];	/* unused */

	uByte mac_addr[MAC_ADDR_SIZE];			/* our ether addr */
	uByte server_mac[MAC_ADDR_SIZE];		/* server's ether addr */
	uByte ip_addr[IP_ADDR_SIZE];			/* our IP addr */
	uByte server_ip[IP_ADDR_SIZE];			/* server's IP addr */
	uByte gateway_ip[IP_ADDR_SIZE];			/* gateway's IP addr, if any */
	uByte user_ip_addr[IP_ADDR_SIZE];		/* user-provided our IP */
	uByte user_server_ip[IP_ADDR_SIZE];		/* user-provided server's IP */
	uByte user_gateway_ip[IP_ADDR_SIZE];	/* user-provided gateway's IP */
	char file_name[FILE_NAME_SIZE];			/* boot file name */
	uInt xid;								/* unique transaction ID */
	uInt time;								/* boot time */
	int port;								/* port to use for TFTP */
	Bool bootp;								/* use BOOTP or not? */
	Bool dhcp;								/* use DHCP or not? */
	Bool rarp;								/* use RARP or not? */
	int bootp_retries;						/* number of BOOT retries */
	int tftp_retries;						/* number of TFTP retries */
	int ping_timeout;						/* timeout for ECHO */
	Bool diag;								/* diagnostics/verbose mode */
};
typedef struct self Self;


/*#ifdef MORE_DEBUG*/
extern void dump_packet(Environ *e, uByte *packet, uInt len);

void
dump_packet(Environ *e, uByte *packet, uInt len)
{
	struct mac_header *mac = (struct mac_header*)(packet + MAC_HEADER_OFFSET);
	struct ip_header *ip = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	struct udp_header *udp = (struct udp_header*)(packet + UDP_HEADER_OFFSET);
	struct icmp_header *icmp =
			(struct icmp_header*)(packet + ICMP_HEADER_OFFSET);

	if (!e->debug)
		return;
	
	dprintf("dump_packet mac=%p ip=%p udp=%p len=%u\n",
			mac, ip, udp, len);
	
	if (len < MAC_HEADER_SIZE)
		return;

	len -= MAC_HEADER_SIZE;
	dprintf("mac dest-addr: %#x.%x.%x.%x.%x.%x\n",
		mac->dest_addr[0], mac->dest_addr[1], mac->dest_addr[2],
		mac->dest_addr[3], mac->dest_addr[4], mac->dest_addr[5]);
	dprintf("mac scr-addr: %#x.%x.%x.%x.%x.%x\n",
		mac->src_addr[0], mac->src_addr[1], mac->src_addr[2],
		mac->src_addr[3], mac->src_addr[4], mac->src_addr[5]);
	dprintf("mac packet-type: %#x.%x\n",
		mac->pkt_type[0], mac->pkt_type[1]);
	
	if ((mac->pkt_type[0] == MAC_ARP1 && mac->pkt_type[1] == MAC_ARP2) ||
			(mac->pkt_type[0] == MAC_RARP1 && mac->pkt_type[1] == MAC_RARP2))
	{
		struct arp_packet *arp =
				(struct arp_packet*)(packet + ARP_PACKET_OFFSET);
	
		if (len < ARP_PACKET_SIZE)
			return;
		
		len -= ARP_PACKET_SIZE;
		dprintf("ARP hardware = %#x.%x\n",
				arp->hardware[0], arp->hardware[1]);
		dprintf("ARP protocol = %#x.%x\n",
				arp->protocol[0], arp->protocol[1]);
		dprintf("ARP hardware_len=%u protocol_len=%u\n",
				arp->hardware_len, arp->protocol_len);
		dprintf("ARP op = %#x.%x\n", arp->op[0], arp->op[1]);
		dprintf("ARP scr_mac: %#x.%x.%x.%x.%x.%x\n",
			arp->src_mac[0], arp->src_mac[1], arp->src_mac[2],
			arp->src_mac[3], arp->src_mac[4], arp->src_mac[5]);
		dprintf("ARP src_ip = %u.%u.%u.%u\n", arp->src_ip[0],
				arp->src_ip[1], arp->src_ip[2], arp->src_ip[3]);
		dprintf("ARP target_mac: %#x.%x.%x.%x.%x.%x\n",
			arp->target_mac[0], arp->target_mac[1], arp->target_mac[2],
			arp->target_mac[3], arp->target_mac[4], arp->target_mac[5]);
		dprintf("ARP target_ip = %u.%u.%u.%u\n", arp->target_ip[0],
				arp->target_ip[1], arp->target_ip[2], arp->target_ip[3]);
		return;
	}

	if (len < IP_HEADER_SIZE)
		return;

	len -= IP_HEADER_SIZE;
	dprintf("IP version/header = %#x\n", ip->vers_hdr);
	dprintf("IP service type = %u\n", ip->service_type);
	dprintf("IP len = %#x.%x\n", ip->len[0], ip->len[1]);
	dprintf("IP ID = %#x.%x\n", ip->id[0], ip->id[1]);
	dprintf("IP flag/offset = %#x.%x\n",
			ip->flag_offset[0], ip->flag_offset[1]);
	dprintf("IP time = %u\n", ip->time);
	dprintf("IP protocol = %u\n", ip->protocol);
	dprintf("IP checksum = %#x.%x\n", ip->checksum[0], ip->checksum[1]);
	dprintf("IP src-addr = %u.%u.%u.%u\n", ip->src_addr[0],
			ip->src_addr[1], ip->src_addr[2], ip->src_addr[3]);
	dprintf("IP dest-addr = %u.%u.%u.%u\n", ip->dest_addr[0],
			ip->dest_addr[1], ip->dest_addr[2], ip->dest_addr[3]);

	switch (ip->protocol)
	{
	case UDP_PROTOCOL:
		if (len < UDP_HEADER_SIZE)
			return;
	
		len -= UDP_HEADER_SIZE;
		dprintf("UDP src port = %u.%u\n", udp->src_port[0], udp->src_port[1]);
		dprintf("UDP dest port = %u.%u\n",
				udp->dest_port[0], udp->dest_port[1]);
		dprintf("UDP len = %#x.%x\n", udp->len[0], udp->len[1]);
		dprintf("UDP checksum = %#x.%x\n",
				udp->checksum[0], udp->checksum[1]);
		
		packet += UDP_HEADER_SIZE + IP_HEADER_SIZE + MAC_HEADER_SIZE;
		break;
	
	case ICMP_PROTOCOL:
		if (len < ICMP_HEADER_SIZE)
			return;
		
		len -= ICMP_HEADER_SIZE;
		dprintf("ICMP type = %u\n", icmp->type);
		dprintf("ICMP code = %u\n", icmp->code);
		dprintf("ICMP checksum = %#x.%x\n",
				icmp->checksum[0], icmp->checksum[1]);
		packet += ICMP_HEADER_SIZE + IP_HEADER_SIZE + MAC_HEADER_SIZE;
		break;
	}
	
#ifdef EVEN_MORE_DEBUG
	{
		int i, l;

		/* dump the rest of the packet in hex */
		l = 1;
		
		for (i = 0; i < len; i++)
		{
			dprintf("%#02x ", packet[i]);
			
			if (l++ >= 8)
			{
				dprintf("\n");
				l = 1;
			}
		}
	}

	if (l)
		dprintf("\n");
#endif
	
}
/*#endif*/


static Bool
is_mac_zero(uByte mac[])
{
	return mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0	&&
			mac[4] == 0 && mac[5] == 0;
}

static Bool
is_mac_broadcast(uByte mac[])
{
	return mac[0] == 0xFF && mac[1] == 0xFF &&
			mac[2] == 0xFF && mac[3] == 0xFF &&
			mac[4] == 0xFF && mac[5] == 0xFF;
}

static Bool
is_ip_zero(uByte ip[])
{
	return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

static Bool
is_ip_broadcast(uByte ip[])
{
	return ip[0] == 0xFF && ip[1] == 0xFF && ip[2] == 0xFF && ip[3] == 0xFF;
}


static Bool
get_net_params(Environ *e, Self *s)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	uInt val;
	uByte *addr;
	Int len;
	Instance *saveinst;
	
	DPRINTF(("obptftp: get_net_params\n"));
	ret = prop_get_int(inst->parent->package->props,
			"address-bits", CSTR, &val);

	if (ret == NO_ERROR && val != MAC_ADDR_SIZE * 8)	/* wrong size for us */
		return FALSE;
		
	ret = prop_get_int(inst->parent->package->props,
			"max-frame-size", CSTR, &val);

	if (ret == NO_ERROR && val < MAC_BUFFER_SIZE)		/* wrong size for us */
		return FALSE;

	/* call the mac-address word and let it select the address, but
	   make sure they check our parent for a local-mac-address property */
	saveinst = (Instance*)(uPtr)e->currinst;
	e->currinst = (uPtr)saveinst->parent;
	ret = execute_word(e, "mac-address");
	e->currinst = (uPtr)saveinst;

	if (ret != NO_ERROR)
		return FALSE;
	
	POP(e, len);
	POPT(e, addr, uByte*);

	if (len != MAC_ADDR_SIZE)		/* unexpected size */
		return FALSE;

	DPRINTF(("obptftp: mac_addr = %02X:%02X:%02X:%02X:%02X:%02X\n",
			addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]));
	
	/* now stash this address as the "mac-address" property */
	ret = set_property(inst->parent->package->props,
			"mac-address", CSTR, (Byte*)addr, MAC_ADDR_SIZE);

	if (ret != NO_ERROR)
		return FALSE;

	/* and save a C copy for ourselves */
	memcpy(s->mac_addr, addr, MAC_ADDR_SIZE);
	return TRUE;
}

static Retcode
send_mac_pkt(Environ *e, Self *s, uByte *packet, int len, enum Pkt_type pkt)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int actual = 0;
	struct mac_header *hdr = (struct mac_header*)(packet + MAC_HEADER_OFFSET);
	Retcode ret;
	static uByte pkt_map[][2] =
	{
		{ 0, 0 }, 
		{ MAC_IP1, MAC_IP2 },
		{ MAC_ARP1, MAC_ARP2 }, 
		{ MAC_ARP1, MAC_ARP2 }, 
		{ MAC_RARP1, MAC_RARP2 },
		{ MAC_RARP1, MAC_RARP2 },
		{ 0, 0 }, 
	};
	
	len += MAC_HEADER_SIZE;
	
	if (pkt == PKT_IP)
		memcpy(hdr->dest_addr, s->server_mac, MAC_ADDR_SIZE);
	else if (pkt == PKT_ARPR || pkt == PKT_RARPR)
		memcpy(hdr->dest_addr, hdr->src_addr, MAC_ADDR_SIZE);
	else
		memset(hdr->dest_addr, 0xFF, MAC_ADDR_SIZE);
	
	memcpy(hdr->src_addr, s->mac_addr, MAC_ADDR_SIZE);
	hdr->pkt_type[0] = pkt_map[pkt][0];
	hdr->pkt_type[1] = pkt_map[pkt][1];
#ifdef MORE_DEBUG
	if (e->debug) dprintf("send_mac_pkt: ");
	dump_packet(e, packet, len);
#endif

	if (len < 60)		/* min data length is 46, plus 14 for header */
		len = 60;

	/* send the packet by calling our parent's "write" method */
	do
	{
		PUSHP(e, packet);
		PUSH(e, len);
		DPRINTF(("send_mac_pkt: packet=%#x len=%d\n", packet, len));
	
		ret = execute_method_name(e, inst->parent, "write", CSTR);

		if (ret != NO_ERROR)
			return ret;
		
		POP(e, actual);
		DPRINTF(("send_mac_pkt: actual=%d\n", actual));
	
		if (actual < len)
		{
			if (s->diag)
				cprintf(e, "W");
			else
				spin_cursor(e);
		}
	} while (actual < len);

	return NO_ERROR;
}

static uInt
ip_checksum(uByte *packet)
{
	int i;
	uInt csum = 0;

	/* calculate the IP header checksum */
	for (i = IP_HEADER_OFFSET; i < IP_HEADER_OFFSET + IP_HEADER_SIZE; i += 2)
		csum += (packet[i] << BYTE_SIZE) + packet[i + 1];

	csum = (csum & DOUBLET_MASK) + ((csum >> DOUBLET_SIZE) & DOUBLET_MASK);

	/* add in last carry */
	if (csum > DOUBLET_MASK)
	    csum -= DOUBLET_MASK;

	csum &= DOUBLET_MASK;	/* paranoia */
	
	if (csum == DOUBLET_MASK)
		csum = 0;
	
	return ~csum;
}

static Retcode
send_ip_pkt(Environ *e, Self *s, uByte *packet, int len, int protocol)
{
	static int id = 0;
	struct ip_header *hdr = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	uInt csum;
	
	len += IP_HEADER_SIZE;
	hdr->vers_hdr = (4 << 4) + 5;	/* Version 4, size 5 (no options) */
	hdr->service_type = 0;			/* normal everything */
	hdr->len[0] = (len >> BYTE_SIZE) & BYTE_MASK;
	hdr->len[1] = len & BYTE_MASK;
	hdr->id[0] = (id >> BYTE_SIZE) & BYTE_MASK;	/* generate a new packet id */
	hdr->id[1] = id++ & BYTE_MASK;
	hdr->flag_offset[0] = 0x40;		/* not fragmented but allow it */
	hdr->flag_offset[1] = 0;
	hdr->time = 255;				/* max time to live */
	hdr->protocol = protocol;
	hdr->checksum[0] = 0;			/* calculated below */
	hdr->checksum[1] = 0;
	memcpy(hdr->src_addr, s->ip_addr, IP_ADDR_SIZE);
	memcpy(hdr->dest_addr, s->server_ip, IP_ADDR_SIZE);
	csum = ip_checksum(packet);
	hdr->checksum[0] = (csum >> BYTE_SIZE) & BYTE_MASK;
	hdr->checksum[1] = csum & BYTE_MASK;
	return send_mac_pkt(e, s, packet, len, PKT_IP);
}

static uInt
icmp_checksum(uByte *packet, int len)
{
	int i;
	uInt csum = 0;

	/* calculate the ICMP packet checksum */
	for (i = ICMP_HEADER_OFFSET; i < ICMP_HEADER_OFFSET + len; i += 2)
		csum += (packet[i] << BYTE_SIZE) + packet[i + 1];

	csum = (csum & DOUBLET_MASK) + ((csum >> DOUBLET_SIZE) & DOUBLET_MASK);

	/* add in last carry */
	if (csum > DOUBLET_MASK)
	    csum -= DOUBLET_MASK;

	csum &= DOUBLET_MASK;	/* paranoia */
	
	if (csum == DOUBLET_MASK)
		csum = 0;
	
	return ~csum;
}

static Retcode
send_icmp_pkt(Environ *e, Self *s, uByte *packet, int len, int type, int code)
{
	struct icmp_header *icmp =
			(struct icmp_header*)(packet + ICMP_HEADER_OFFSET);
	uInt csum;
	
	len += ICMP_HEADER_SIZE;
	icmp->type = type & BYTE_MASK;
	icmp->code = code & BYTE_MASK;
	icmp->checksum[0] = 0;
	icmp->checksum[1] = 0;
	csum = icmp_checksum(packet, len);
	icmp->checksum[0] = (csum >> BYTE_SIZE) & BYTE_MASK;
	icmp->checksum[1] = csum & BYTE_MASK;
	return send_ip_pkt(e, s, packet, len, ICMP_PROTOCOL);
}

static Retcode
send_ping_pkt(Environ *e, Self *s, uByte *packet, int seq)
{
	struct ping_packet *ping =
			(struct ping_packet*)(packet + ICMP_PACKET_OFFSET);
	
	memset(packet, 0, MAC_BUFFER_SIZE);
	ping->id = PING_ID;
	ping->seq = seq;
	strcpy((char*)ping->data, "Are you deaf?");
	return send_icmp_pkt(e, s, packet, PING_PKT_SIZE, ICMP_ECHO, 0);
}

static uInt
udp_checksum(uByte *packet, int len, uByte ip_addr[], uByte server_ip[])
{
	int i;
	uInt csum;
	
	/* calculate the checksum, starting with a "fake" pseudo-header */
	csum = ((ip_addr[0] + ip_addr[2] +
					server_ip[0] + server_ip[2] +
					((len >> BYTE_SIZE) & BYTE_MASK)) << BYTE_SIZE) +
			ip_addr[1] + ip_addr[3] + server_ip[1] + server_ip[3] +
			UDP_PROTOCOL + (len & BYTE_MASK);

	DPRINTF(("udp_checksum: fake header csum=%#x\n", csum));

	for (i = UDP_HEADER_OFFSET; i < UDP_HEADER_OFFSET + len - 1; i += 2)
		csum += (packet[i] << BYTE_SIZE) + packet[i + 1];

	DPRINTF(("udp_checksum: UDP header csum=%#x\n", csum));

	if (len & 1)		/* odd - need to csum one more byte */
	{
		csum += (packet[i] << BYTE_SIZE);
		DPRINTF(("udp_checksum: odd length csum=%#x\n", csum));
	}
	
	csum = (csum & DOUBLET_MASK) + ((csum >> DOUBLET_SIZE) & DOUBLET_MASK);
	DPRINTF(("udp_checksum: hi>>8 + lo csum=%#x\n", csum));

	/* add in last carry */
	if (csum > DOUBLET_MASK)
	    csum -= DOUBLET_MASK;

	csum &= DOUBLET_MASK;	/* paranoia */
	DPRINTF(("udp_checksum: masked csum=%#x\n", csum));
	
	if (csum == DOUBLET_MASK)
		csum = 0;
	
	DPRINTF(("udp_checksum: final csum=%#x ~csum=%#x\n", csum, ~csum));
	return ~csum;
}

static Retcode
send_udp_pkt(Environ *e, Self *s, uByte *packet, int src_port, int dest_port,
		int len)
{
	struct udp_header *hdr = (struct udp_header*)(packet + UDP_HEADER_OFFSET);
	uInt csum;
	
	len += UDP_HEADER_SIZE;
	hdr->src_port[0] = (src_port >> BYTE_SIZE) & BYTE_MASK;
	hdr->src_port[1] = src_port & BYTE_MASK;
	hdr->dest_port[0] = (dest_port >> BYTE_SIZE) & BYTE_MASK;
	hdr->dest_port[1] = dest_port & BYTE_MASK;
	hdr->len[0] = (len >> BYTE_SIZE) & BYTE_MASK;
	hdr->len[1] = len & BYTE_MASK;
	hdr->checksum[0] = 0;			/* calculated below */
	hdr->checksum[1] = 0;
	csum = udp_checksum(packet, len, s->ip_addr, s->server_ip);
	hdr->checksum[0] = (csum >> BYTE_SIZE) & BYTE_MASK;
	hdr->checksum[1] = csum & BYTE_MASK;
	return send_ip_pkt(e, s, packet, len, UDP_PROTOCOL);
}

static int dhcp_state = 0;

static Retcode
send_bootp_request(Environ *e, Self *s, uByte *packet)
{
	struct bootp_packet *hdr =
			(struct bootp_packet*)(packet + UDP_PACKET_OFFSET);

	dhcp_state = 0;
	memset(packet, 0, MAC_BUFFER_SIZE);
	hdr->op = BOOT_REQUEST;
	hdr->htype = 1;		/* 10Mb Ethernet */
	hdr->hlen = MAC_ADDR_SIZE;
	hdr->hops = 0;
	hdr->xid[0] = (s->xid >> (BYTE_SIZE * 3)) & BYTE_MASK;
	hdr->xid[1] = (s->xid >> (BYTE_SIZE * 2)) & BYTE_MASK;
	hdr->xid[2] = (s->xid >> BYTE_SIZE) & BYTE_MASK;
	hdr->xid[3] = s->xid & BYTE_MASK;
	hdr->secs[0] = (s->time >> BYTE_SIZE) & BYTE_MASK;
	hdr->secs[1] = s->time & BYTE_MASK;
	memcpy(hdr->ciaddr, s->ip_addr, IP_ADDR_SIZE);
	memcpy(hdr->chaddr, s->mac_addr, MAC_ADDR_SIZE);
	memcpy(hdr->giaddr, s->gateway_ip, IP_ADDR_SIZE);

	if (s->dhcp)
	{
		hdr->options[0] = 99;	/* magic cookie - 0x63.82.53.63 */
		hdr->options[1] = 130;
		hdr->options[2] = 83;
		hdr->options[3] = 99;
		hdr->options[4] =  OPT_DHCP;
		hdr->options[5] =  1;
		hdr->options[6] =  DHCP_DISCOVER;
		hdr->options[7] =  OPT_END;
	}

	return send_udp_pkt(e, s,  packet, BOOTP_CLIENT_PORT, BOOTP_SERVER_PORT,
			BOOTP_PKT_SIZE);
}

static Retcode
send_arp(Environ *e, Self *s, uByte *buf, uByte op)
{
	struct arp_packet *arp = (struct arp_packet*)(buf + ARP_PACKET_OFFSET);
	
	memset(buf, 0, MAC_BUFFER_SIZE);
	arp->hardware[0] = 0x00;
	arp->hardware[1] = 0x01;		/* 0x0001 for Ethernet */
	arp->protocol[0] = 0x08;
	arp->protocol[1] = 0x00;		/* 0x0800 for IP address space */
	arp->hardware_len = MAC_ADDR_SIZE;
	arp->protocol_len = IP_ADDR_SIZE;
	arp->op[0] = 0;
	arp->op[1] = op;	/* ARP_REQUEST or RARP_REQUEST */

	if (op == RARP_REQUEST)
	{
		memcpy(arp->src_mac, s->mac_addr, MAC_ADDR_SIZE);
		memcpy(arp->target_mac, s->mac_addr, MAC_ADDR_SIZE);
	}
	else
	{
		memcpy(arp->src_mac, s->mac_addr, MAC_ADDR_SIZE);
		memcpy(arp->src_ip, s->ip_addr, IP_ADDR_SIZE);
		memcpy(arp->target_mac, s->server_mac, MAC_ADDR_SIZE);

		/* if a gateway was specified, use its MAC address */
		if (!is_ip_zero(s->gateway_ip) && !is_ip_broadcast(s->gateway_ip))
			memcpy(arp->target_ip, s->gateway_ip, IP_ADDR_SIZE);
		else
			memcpy(arp->target_ip, s->server_ip, IP_ADDR_SIZE);
	}

	return send_mac_pkt(e, s, buf, ARP_PACKET_SIZE,
			op == ARP_REQUEST ? PKT_ARP : PKT_RARP);
}

/* returns PKT_ARP if we successfully got an ARP REPLY packet, PKT_RARP
   if we got a RARP reply, PKT_NONE for any other packet, and Retcode
   for any internal exception
 */
static Int
process_arp(Environ *e, Self *s, uByte *packet, int *len)
{
	struct arp_packet *arp = (struct arp_packet*)(packet + ARP_PACKET_OFFSET);

	DPRINTF(("process_arp: len=%d hardware=%#x.%x hw_len=%d\n", *len,
			arp->hardware[0], arp->hardware[1], arp->hardware_len));

	/* hardware address space - 0x0001 for Ethernet */
	if (arp->hardware[0] != 0x00 || arp->hardware[1] != 0x01 ||
			arp->hardware_len != MAC_ADDR_SIZE)
		return PKT_NONE;

	DPRINTF(("process_arp: protocol=%#x.%x protocol_len=%d\n", 
			arp->protocol[0], arp->protocol[1], arp->protocol_len));

	/* protocol address space - 0x0800 for IP */
	if (arp->protocol[0] != 0x08 || arp->protocol[1] != 0x00 ||
			arp->protocol_len != IP_ADDR_SIZE)
		return PKT_NONE;

	DPRINTF(("process_arp: target_IP=%#x.%x.%x.%x\n", 
			arp->target_ip[0], arp->target_ip[1],
			arp->target_ip[2], arp->target_ip[3]));

	if (arp->op[0] != 0x00)
		return PKT_NONE;

	if (arp->op[1] == ARP_REPLY || arp->op[1] == ARP_REQUEST)
	{
		/* are we the target IP? */
		if (is_ip_zero(s->ip_addr) ||
				memcmp(s->ip_addr, arp->target_ip, IP_ADDR_SIZE) != 0)
			return PKT_NONE;
	}
	else if (is_ip_zero(s->mac_addr) ||
				memcmp(s->mac_addr, arp->target_mac, IP_ADDR_SIZE) != 0)
			return PKT_NONE;

	DPRINTF(("process_arp: op=%#x.%x\n", arp->op[0], arp->op[1]));

	switch (arp->op[1])
	{
	case ARP_REPLY:
		/* see if this is a reply for our earlier query */
		if (!memcmp(arp->src_ip, s->server_ip, IP_ADDR_SIZE))
		{
			memcpy(s->server_mac, arp->src_mac, MAC_ADDR_SIZE);
			DPRINTF(("process_arp: got ARP_REPLY\n"));

			if (s->diag)
				cprintf(e, "ar");
			else
				spin_cursor(e);

			return PKT_ARP;
		}

		break;

	case ARP_REQUEST:	/* handle ARP_REQUEST packets here for ourself */
		/* set target to the src addrs, then fill in our addrs */
		memcpy(arp->target_mac, arp->src_mac, MAC_ADDR_SIZE);
		memcpy(arp->target_ip, arp->src_ip, IP_ADDR_SIZE);
		memcpy(arp->src_mac, s->mac_addr, MAC_ADDR_SIZE);
		memcpy(arp->src_ip, s->ip_addr, IP_ADDR_SIZE);
	
		/* finally send an ARP reply */
		arp->op[1] = ARP_REPLY;

		if (s->diag)
			cprintf(e, "aq");
		else
			spin_cursor(e);
	
		DPRINTF(("process_arp: sending ARP_REPLY\n"));
		return send_mac_pkt(e, s, packet, ARP_PACKET_SIZE, PKT_ARPR);

	case RARP_REPLY:	/* reply from a RARP_REQUEST   */
		if (s->rarp && !memcmp(arp->target_mac, s->mac_addr, MAC_ADDR_SIZE))
		{
			DPRINTF(("process_arp: got RARP_REQUEST\n"));

			if (is_ip_zero(s->ip_addr) || is_ip_broadcast(s->ip_addr))
				memcpy(s->ip_addr, arp->target_ip, IP_ADDR_SIZE);

			if (is_ip_zero(s->server_ip) || is_ip_broadcast(s->server_ip)
					|| !memcmp(s->server_ip, arp->src_ip, IP_ADDR_SIZE))
			{
				memcpy(s->server_ip, arp->src_ip, IP_ADDR_SIZE);
				memcpy(s->server_mac, arp->src_mac, MAC_ADDR_SIZE);
			}

			if (s->diag)
				cprintf(e, "rr");
			else
				spin_cursor(e);

			return PKT_RARP;
		}

		break;

	case RARP_REQUEST:	/* request to reverse ARP - for completeness */
		/* set target to the src addrs, then fill in our addrs */
		memcpy(arp->target_mac, arp->src_mac, MAC_ADDR_SIZE);
		memcpy(arp->target_ip, arp->src_ip, IP_ADDR_SIZE);
		memcpy(arp->src_mac, s->mac_addr, MAC_ADDR_SIZE);
		memcpy(arp->src_ip, s->ip_addr, IP_ADDR_SIZE);

		/* send a RARP reply */
		arp->op[1] = RARP_REPLY;

		if (s->diag)
			cprintf(e, "rq");
		else
			spin_cursor(e);
	
		DPRINTF(("process_arp: sending ARP_REPLY\n"));
		return send_mac_pkt(e, s, packet, ARP_PACKET_SIZE, PKT_RARPR);
	}
	
	return PKT_NONE;
}

/* return Retcode for any exception or PKT_* for specific type of packet
 */
static Int
get_mac_pkt(Environ *e, Self *s, uByte *packet, int *len)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	struct mac_header *hdr = (struct mac_header*)(packet + MAC_HEADER_OFFSET);
	Retcode ret;

	/* read a packet by calling the parent "read" method */
	PUSHP(e, packet);
	PUSH(e, MAC_BUFFER_SIZE);

	ret = execute_method_name(e, inst->parent, "read", CSTR);

	if (ret != NO_ERROR)
		return ret;
	
	POP(e, *len);
	
	if (*len == -1)		/* error occurred - bail out */
		return E_NET_ERROR;

	/* no input pending, or packet was not big enough, so try again */
	if (*len == -2 || *len < MAC_HEADER_SIZE)
		return PKT_NONE;

	if (*len > MAC_BUFFER_SIZE)
		cprintf(e, "obptftp: possible packet overrun -"
				" expected %d but got %d\n",
				MAC_BUFFER_SIZE, *len);
		
	/* ARP or RARP protocol */
	if ((hdr->pkt_type[0] == MAC_ARP1 && hdr->pkt_type[1] == MAC_ARP2) ||
			(hdr->pkt_type[0] == MAC_RARP1 && hdr->pkt_type[1] == MAC_RARP2))
		return process_arp(e, s, packet, len);

	/* not IP protocol */
	if (hdr->pkt_type[0] != MAC_IP1 || hdr->pkt_type[1] != MAC_IP2)
		return PKT_NONE;

	*len -= MAC_HEADER_SIZE;
	return PKT_IP;
}

/* return Retcode for any exception, PKT_IP for IP packet, or PKT_NONE
 */
static Int
get_ip_pkt(Environ *e, Self *s, uByte *packet, int *len)
{
	struct ip_header *hdr = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	int pktlen;
	uInt csum;
	Int pkt;
	
	pkt = get_mac_pkt(e, s, packet, len);

	if (pkt != PKT_IP)
		return pkt;

	pktlen = (hdr->len[0] << BYTE_SIZE) + hdr->len[1];
	DPRINTF(("get_ip_pkt: checking packet headers\n"));

	/* bad packet? - try another */
	if ((hdr->vers_hdr >> 4) != 4 ||
			*len < IP_HEADER_SIZE ||
			pktlen > *len ||
			((hdr->flag_offset[0] & 0x3F) != 0) ||
			hdr->flag_offset[1] != 0)
		return PKT_NONE;
	
	DPRINTF(("get_ip_pkt: dest IP-addr=%u.%u.%u.%u\n",
			hdr->dest_addr[0], hdr->dest_addr[1],
			hdr->dest_addr[2], hdr->dest_addr[3]));
	DPRINTF(("get_ip_pkt: our-IP-addr=%u.%u.%u.%u\n",
			s->ip_addr[0], s->ip_addr[1], s->ip_addr[2], s->ip_addr[3]));

	/* check for our IP addr, if we know who we are yet */
	if (!is_ip_zero(s->ip_addr) &&
			memcmp(s->ip_addr, hdr->dest_addr, IP_ADDR_SIZE) != 0)
		return PKT_NONE;
	
	/* verify checksum */
	csum = ip_checksum(packet);
	DPRINTF(("get_ip_pkt: checking csum=%#x\n", csum));
	
	if ((csum & DOUBLET_MASK) != DOUBLET_MASK)	/* no luck - try another */
		return PKT_NONE;
	
	/* got one! */
	*len = pktlen - IP_HEADER_SIZE;
	DPRINTF(("get_ip_pkt: got *len=%d\n", *len));
	return PKT_IP;
}

/* return Retcode for any exception, PKT_ICMP for ICMP packet, or PKT_NONE
 */
static Int
get_icmp_pkt(Environ *e, Self *s, uByte *packet, int *len)
{
	struct icmp_header *icmp =
			(struct icmp_header*)(packet + ICMP_HEADER_OFFSET);
	uInt csum;
	Int pkt;
		
	pkt = get_ip_pkt(e, s, packet, len);

	if (pkt != PKT_IP)
		return pkt;

	DPRINTF(("get_icmp_pkt: len = %d\n", *len));
	
	if (*len < ICMP_HEADER_SIZE)
		return PKT_IP;
	
	csum = icmp_checksum(packet, *len);
	
	if ((icmp->checksum[0] || icmp->checksum[1]) &&
			(csum & DOUBLET_MASK) != DOUBLET_MASK)
	{
		/* no luck - try another packet */
#ifdef MORE_DEBUG
		dump_packet(e, packet, *len + IP_HEADER_SIZE + MAC_HEADER_SIZE);
#endif
		return PKT_IP;
	}
	
	*len -= ICMP_HEADER_SIZE;
	DPRINTF(("get_icmp_pkt: got len=%d type=%d code=%d\n",
			*len, icmp->type, icmp->code));
	return PKT_ICMP;
}

/* return Retcode for any exception, PKT_PING for successful echo,
   E_NET_TIMEOUT for no echo received but no other error either
 */
static Int
get_ping_pkt(Environ *e, Self *s, uByte *packet, int *len,
		int seq, Int *timeout)
{
	struct ping_packet *ping =
			(struct ping_packet*)(packet + ICMP_PACKET_OFFSET);
	struct icmp_header *icmp =
			(struct icmp_header*)(packet + ICMP_HEADER_OFFSET);
	uLong stime = get_msecs(e);
	uLong etime;
	Int ret;

	while (*timeout > 0)
	{
		etime = get_msecs(e);
		*timeout -= etime - stime;
		stime = etime;
		
		ret = get_icmp_pkt(e, s, packet, len);

		if (ret < PKT_NONE)
			return ret;

		if (ret != PKT_ICMP)
			continue;

		DPRINTF(("get_ping_pkt: got packet\n"));
		
		if (*len < PING_PKT_SIZE)
			continue;

		DPRINTF(("get_ping_pkt: len = %d\n", *len));
		
		if (icmp->type != ICMP_ECHO_REPLY)
			continue;

		DPRINTF(("get_ping_pkt: got ECHO reply\n"));
		
		if (ping->id != PING_ID)
			continue;
		
		DPRINTF(("get_ping_pkt: got ID\n"));
		
		if (ping->seq != seq)
			continue;
		
		DPRINTF(("get_ping_pkt: got echo reply\n"));
		return PKT_PING;
	}
	
	return E_NET_TIMEOUT;
}

/* return Retcode for any exception, PKT_UDP for UDP packet, PKT_*
   for any other packet
 */
static Int
get_udp_pkt(Environ *e, Self *s, uByte *packet, int *len, int dest_port)
{
	struct udp_header *hdr = (struct udp_header*)(packet + UDP_HEADER_OFFSET);
	struct ip_header *ip = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	int pktlen;
	uInt csum;
	Int r;
		
	r = get_ip_pkt(e, s, packet, len);

	if (r != PKT_IP)
		return r;
	
	pktlen = (hdr->len[0] << BYTE_SIZE) + hdr->len[1];
	DPRINTF(("get_udp_pkt: pktlen = %d\n", pktlen));

	if (*len < UDP_HEADER_SIZE || pktlen > *len)
		return PKT_IP;
	
	csum = udp_checksum(packet, pktlen, ip->src_addr, ip->dest_addr);
	DPRINTF(("get_udp_pkt: csum=%#x\n", csum));
	
	/* make sure there is a checksum to check */
	if ((hdr->checksum[0] || hdr->checksum[1]) &&
			(csum & DOUBLET_MASK) != DOUBLET_MASK)
	{
		/* no luck - try another packet */
#ifdef MORE_DEBUG
		dump_packet(e, packet, *len + IP_HEADER_SIZE + MAC_HEADER_SIZE);
#endif
		return PKT_IP;
	}

	/* verify that we have the correct destination port number */
	if (dest_port != (hdr->dest_port[0] << BYTE_SIZE) + hdr->dest_port[1])
	{
		/* no luck - try another packet */
#ifdef MORE_DEBUG
		dump_packet(e, packet, *len + IP_HEADER_SIZE + MAC_HEADER_SIZE);
#endif
		return PKT_IP;
	}

	*len = pktlen - UDP_HEADER_SIZE;
	DPRINTF(("get_udp_pkt: got port=%d *len=%d\n", s->port, *len));
	return PKT_UDP;
}

static uByte *
search_dhcp(uByte *p, int option)
{
	while (*p != OPT_END)
	{
		if (*p == option)
			return p + 2;

		if (*p == OPT_PAD)
			p++;
		else
			p += p[1] + 2;	/* skip this option, length, and any data */
	}

	return NULL;
}

static uByte *
find_dhcp(Environ *e, Self *s, struct bootp_packet *hdr, int option)
{
	uByte *p;
	int ov;

	/* sanity check of magic numbers */
	if (hdr->options[0] != 99 ||
			hdr->options[1] != 130 ||
			hdr->options[2] != 83 ||
			hdr->options[3] != 99)
		return NULL;

	/* look in the options field first */
	p = search_dhcp(hdr->options + 4, option);

	if (p)
		return p;

	/* see if there are overlay options */
	p = search_dhcp(hdr->options + 4, OPT_OVERLOAD);

	if (p == NULL)
		return NULL;

	ov = *p;

	/* check file field for more options */
	if (ov == DHCP_OV_FILE || ov == DHCP_OV_BOTH)
	{
		p = search_dhcp((uByte*)hdr->file, option);

		if (p)
			return p;
	}

	/* then check sname field for more options */
	if (ov == DHCP_OV_SNAME || ov == DHCP_OV_BOTH)
	{
		p = search_dhcp(hdr->sname, option);

		if (p)
			return p;
	}

	return NULL;
}

static Retcode
send_dhcp_request(Environ *e, Self *s, uByte *packet, struct bootp_packet *hdr)
{
	struct ip_header *ip = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	uByte *p;

	/* get the server IP address */
	p = find_dhcp(e, s, hdr, OPT_SERVER_IP);
	memcpy(s->server_ip, p ? p : hdr->siaddr, IP_ADDR_SIZE);

	/* no server specified, so use sender address of packet */
	if (is_ip_zero(s->server_ip) || is_ip_broadcast(s->server_ip))
		memcpy(s->server_ip, ip->src_addr, IP_ADDR_SIZE);

	DPRINTF(("send_dhcp_request: target_IP=%#x.%x.%x.%x\n", 
			p[0], p[1], p[2], p[3]));

	hdr->op = BOOT_REQUEST;
	hdr->options[4] =  OPT_DHCP;
	hdr->options[5] =  1;
	hdr->options[6] =  DHCP_REQUEST;
	hdr->options[7] = OPT_REQUEST_IP;
	hdr->options[8] = IP_ADDR_SIZE;
	memcpy(hdr->options + 9, hdr->yiaddr, IP_ADDR_SIZE);
	hdr->options[9 + IP_ADDR_SIZE] = OPT_SERVER_IP;
	hdr->options[10 + IP_ADDR_SIZE] = IP_ADDR_SIZE;
	memcpy(hdr->options + 11 + IP_ADDR_SIZE, s->server_ip, IP_ADDR_SIZE);
	hdr->options[11 + IP_ADDR_SIZE * 2] = OPT_END;

	return send_udp_pkt(e, s,  packet, BOOTP_CLIENT_PORT, BOOTP_SERVER_PORT,
			BOOTP_PKT_SIZE);
}

/* return Retcode for any exception, PKT_BOOTP if we got a BOOTP reply,
   and any other PKT_* for other packet types
 */
static Int
get_bootp_reply(Environ *e, Self *s, uByte *packet)
{
	struct mac_header *mac = (struct mac_header*)(packet + MAC_HEADER_OFFSET);
	struct ip_header *ip = (struct ip_header*)(packet + IP_HEADER_OFFSET);
	struct bootp_packet *hdr =
			(struct bootp_packet*)(packet + UDP_PACKET_OFFSET);
	int len;
	Int r;

	r = get_udp_pkt(e, s, packet, &len, BOOTP_CLIENT_PORT);

	if (r != PKT_UDP)
		return r;

	DPRINTF(("get_bootp_reply: checking reply headers\n"));

	/* make sure this boot reply is intended for us */
	if (hdr->op != BOOT_REPLY ||
			len < BOOTP_PKT_SIZE ||
			hdr->xid[0] != ((s->xid >> (BYTE_SIZE * 3)) & BYTE_MASK) ||
			hdr->xid[1] != ((s->xid >> (BYTE_SIZE * 2)) & BYTE_MASK) ||
			hdr->xid[2] != ((s->xid >> BYTE_SIZE) & BYTE_MASK) ||
			hdr->xid[3] != (s->xid & BYTE_MASK) ||
			hdr->htype != 1 ||
			hdr->hlen != MAC_ADDR_SIZE ||
			memcmp(hdr->chaddr, s->mac_addr, MAC_ADDR_SIZE) != 0)
	{
	#ifdef MORE_DEBUG
		if (e->debug)
		{
			dprintf("get_bootp_reply: op=%d len=%d xid=%x.%x.%x.%x"
					" s->xid=%x htype=%d hlen=%d\n",
					hdr->op, len, hdr->xid[0], hdr->xid[1], hdr->xid[2],
					hdr->xid[3], s->xid, hdr->htype, hdr->hlen);
			dprintf("\thdr->chaddr=%02X:%02X:%02X:%02X:%02X:%02X"
					" s->mac_addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
					hdr->chaddr[0], hdr->chaddr[1], hdr->chaddr[2],
					hdr->chaddr[3], hdr->chaddr[4], hdr->chaddr[5],
					s->mac_addr[0], s->mac_addr[1], s->mac_addr[2],
					s->mac_addr[3], s->mac_addr[4], s->mac_addr[5]);
		}
	#endif
		return PKT_UDP;
	}
	
	DPRINTF(("get_bootp_reply: got one!\n"));

	/* check the magic number to see if we got a DHCP reply */
	if (hdr->options[0] == 99 &&
			hdr->options[1] == 130 &&
			hdr->options[2] == 83 &&
			hdr->options[3] == 99)
	{
		/* look for a DHCP OFFER packet */
		uByte *p = find_dhcp(e, s, hdr, OPT_DHCP);
		DPRINTF(("get_bootp_reply: got DHCP packet!\n"));
	#ifdef MORE_DEBUG
		if (e->debug) dprintf("get_bootp_reply: p=%x *p=%x state=%d\n", p,
				p ? *p : 0, dhcp_state);
		if (e->debug) display_memory(e, (Byte*)hdr->options, 128);
	#endif

		if (p && s->dhcp)
		{
			if (dhcp_state == 0)
			{
				if (*p != DHCP_OFFER)
					return PKT_UDP;

				/* take the 1st offer */
				if (s->diag)
					cprintf(e, "O");

				DPRINTF(("get_bootp_reply: got DHCP OFFER - send REQUEST\n"));
				r = send_dhcp_request(e, s, packet, hdr);
				dhcp_state = 1;
				return r;
			}
			else /* dhcp_state == 1 */
			{
				if (*p == DHCP_NAK)
				{
					/* try again - send */
					if (s->diag)
						cprintf(e, "N");

					DPRINTF(("get_bootp_reply: got DHCP NAK - retrying\n"));
					r = send_bootp_request(e, s, packet);
					dhcp_state = 0;
					return r;
				}

				if (*p != DHCP_ACK)
					return PKT_UDP;

				/* finally got a packet we can use */
				dhcp_state = 0;
				DPRINTF(("get_bootp_reply: got DHCP ACK!\n"));

				if (s->diag)
					cprintf(e, "K");
			}
		}
	}

	/* now we know these, so stash them for later */
	if (is_ip_zero(s->user_ip_addr))
		memcpy(s->ip_addr, hdr->yiaddr, IP_ADDR_SIZE);
	else
		memcpy(s->ip_addr, s->user_ip_addr, IP_ADDR_SIZE);

	if (is_ip_zero(s->user_server_ip))
		memcpy(s->server_ip, hdr->siaddr, IP_ADDR_SIZE);
	else
		memcpy(s->server_ip, s->user_server_ip, IP_ADDR_SIZE);

	if (is_ip_zero(s->user_gateway_ip))
		memcpy(s->gateway_ip, hdr->giaddr, IP_ADDR_SIZE);
	else
		memcpy(s->gateway_ip, s->user_gateway_ip, IP_ADDR_SIZE);

	/* if we don't know our server's mac address, save it */
	if ((is_mac_zero(s->server_mac) || is_mac_broadcast(s->server_mac)) &&
			(!is_mac_zero(mac->src_addr) &&
					!is_mac_broadcast(mac->src_addr)) &&
			memcmp(ip->src_addr, s->server_ip, IP_ADDR_SIZE) == 0)
		memcpy(s->server_mac, mac->src_addr, MAC_ADDR_SIZE);

	/* use the returned filename only if there no args are specified */
	if (!s->file_name[0])
		strncpy(s->file_name, hdr->file, FILE_NAME_SIZE);

	DPRINTF(("IP-addr = %u.%u.%u.%u\n", s->ip_addr[0],
			s->ip_addr[1], s->ip_addr[2], s->ip_addr[3]));
	DPRINTF(("server-IP = %u.%u.%u.%u\n", s->server_ip[0],
			s->server_ip[1], s->server_ip[2], s->server_ip[3]));
	DPRINTF(("filename = %s\n", s->file_name));
	return PKT_BOOTP;
}

/* return Retcode for any exception, PKT_RARP if we got the RARP info,
   E_NET_TIMEOUT if we did not receive a RARP in time
 */
static Int
get_rarp_info(Environ *e, Self *s)
{
	uByte *buf = s->sendbuf;
	uByte *rbuf = s->recvbuf;
	int len = MAC_BUFFER_SIZE;
	Int r = E_NET_TIMEOUT;
	int i;
	Retcode ret;
	uLong stime, etime, secs;
	uLong timeout = NET_TIMEOUT;

	DPRINTF(("obptftp: get_rarp_info\n"));
	memset(buf, 0, MAC_BUFFER_SIZE);
	
	if (s->diag)
		cprintf(e, "RARP ");
	else
		spin_cursor(e);

	for (i = 0; i < RETRIES; i++)
	{
		ret = send_arp(e, s, buf, RARP_REQUEST);

		if (ret != NO_ERROR)
			return ret;

		stime = get_msecs(e);
		etime = stime;
		secs = stime / 1000;

		while (etime - stime < timeout)
		{
			r = get_mac_pkt(e, s, rbuf, &len);

			if (r < NO_ERROR || r == PKT_RARP)
			{
				if (s->diag)
					cprintf(e, "\n");

				return r;
			}

			etime = get_msecs(e);

			if (etime / 1000 > secs)
			{
				if (s->diag)
					cprintf(e, ".");
				else
					spin_cursor(e);

				secs = etime / 1000;
			}
		}

		if (s->diag)
			cprintf(e, " \nTimeout %ldms waiting for RARP packet.", timeout);
		else
			cprintf(e, "R");

		r = E_NET_TIMEOUT;
		timeout += rand() % (timeout / 2);
	}
	
	if (r == E_NET_TIMEOUT)
		cprintf(e, "RARP send failed.  Check Ethernet cable and tranceiver.\n");

	return r;
}

/* return Retcode for any exception, PKT_ARP if we received the correct ARP
   info, and E_NET_TIMEOUT if we did not receive an ARP packet in time
 */
static Int
get_arp_info(Environ *e, Self *s)
{
	uByte *buf = s->sendbuf;
	uByte *rbuf = s->recvbuf;
	int len = MAC_BUFFER_SIZE;
	Int r = E_NET_TIMEOUT;
	int i;
	Retcode ret;
	uLong stime, etime, secs;
	uLong timeout = NET_TIMEOUT;

	DPRINTF(("obptftp: get_arp_info\n"));
	memset(buf, 0, MAC_BUFFER_SIZE);
	
	if (s->diag)
		cprintf(e, "ARP ");
	else
		spin_cursor(e);

	for (i = 0; i < RETRIES; i++)
	{
		ret = send_arp(e, s, buf, ARP_REQUEST);

		if (ret != NO_ERROR)
			return ret;

#ifdef MORE_DEBUG
		dump_packet(e, buf, ARP_PACKET_SIZE + MAC_HEADER_SIZE);
#endif
		stime = get_msecs(e);
		etime = stime;
		secs = etime / 1000;

		while (etime - stime < timeout)
		{
			r = get_mac_pkt(e, s, rbuf, &len);

			if (r < NO_ERROR || r == PKT_ARP)
			{
				if (s->diag)
					cprintf(e, "\n");

				return r;
			}

			etime = get_msecs(e);

			if (etime / 1000 > secs)
			{
				if (s->diag)
					cprintf(e, ".");
				else
					spin_cursor(e);

				secs = etime / 1000;
			}
		}

		if (s->diag)
			cprintf(e, " \nTimeout %ldms waiting for ARP packet", timeout);
		else
			cprintf(e, "R");

		r = E_NET_TIMEOUT;
		timeout += rand() % (timeout / 2);
	}

	if (s->diag)
		cprintf(e, "\n");

	if (r == E_NET_TIMEOUT)
		cprintf(e, "ARP send failed.  Check Ethernet cable and tranceiver.\n");

	return r;
}

/* return Retcode for any exception, PKT_BOOTP if we got the correct bootp
   information, PKT_RARP if we got the correct RARP info, E_NET_TIMEOUT
   if we did not get anything in time, or PKT_NONE if we did not try anything
 */
static Int
get_bootp_info(Environ *e, Self *s)
{
	uByte *buf = s->sendbuf;
	uByte *rbuf = s->recvbuf;
	Int r = PKT_NONE;
	int i;
	Retcode ret;
	uLong stime = get_msecs(e);
	uLong etime = stime;
	uLong secs = etime / 1000;
	uLong timeout = NET_TIMEOUT;

	DPRINTF(("obptftp: get_bootp_info\n"));

	/* already got all BOOTP/RARP info from the command-line? */
	if (!is_ip_zero(s->user_ip_addr) && !is_ip_broadcast(s->user_ip_addr) &&
		!is_ip_zero(s->user_server_ip) && !is_ip_broadcast(s->user_server_ip))
	{
		s->rarp = FALSE;
		s->bootp = FALSE;
		memcpy(s->ip_addr, s->user_ip_addr, IP_ADDR_SIZE);
		memcpy(s->server_ip, s->user_server_ip, IP_ADDR_SIZE);
	}

	if (s->bootp)
	{
		/* send 1st BOOT_REQUEST packet - a broadcast */
		r = E_NET_TIMEOUT;
		
		/* s->ip_addr will already be all zeros or filled in */

		if (is_ip_zero(s->server_ip))		/* unknown */
		{
			memset(s->server_ip, 0xFF, IP_ADDR_SIZE);	/* use broadcast */
			memset(s->server_mac, 0xFF, MAC_ADDR_SIZE);	/* ditto */
		}

		DPRINTF(("get_boot_info: IP-addr = %u.%u.%u.%u", s->ip_addr[0],
				s->ip_addr[1], s->ip_addr[2], s->ip_addr[3]));
		DPRINTF((" server-IP = %u.%u.%u.%u\n", s->server_ip[0],
				s->server_ip[1], s->server_ip[2], s->server_ip[3]));

		s->xid = rand();
		s->time = 0;

		if (s->diag)
			cprintf(e, s->dhcp ? "DHCP " : "BOOTP ");
		else
			spin_cursor(e);
	    
		ret = send_bootp_request(e, s, buf);

		if (ret != NO_ERROR)
			return ret;

		for (i = 0; i < s->bootp_retries; )
		{
			r = get_bootp_reply(e, s, rbuf);

			if (r < NO_ERROR)
				break;

			if (r == PKT_BOOTP)
			{
				ret = set_property(e->chosen->props, "bootreply-packet", CSTR,
						(Byte*)rbuf, BOOTP_PKT_SIZE + UDP_HEADER_SIZE +
								IP_HEADER_SIZE + MAC_HEADER_SIZE);
				
				if (ret != NO_ERROR)
					return ret;

				break;
			}

			etime = get_msecs(e);
			s->time = (etime - stime) / 1000;

			if (etime / 1000 > secs)
			{
				if (s->diag)
					cprintf(e, ".");
				else
					spin_cursor(e);

				secs = etime / 1000;
			}

			if (etime - stime > timeout)
			{
				/* try again */
				if (s->diag)
					cprintf(e, " \nTimeout %ldms waiting for BOOTP/DHCP packet",
							timeout);
				else
					cprintf(e, "R");

				r = E_NET_TIMEOUT;
				ret = send_bootp_request(e, s, buf);

				if (ret != NO_ERROR)
					return ret;

				timeout += rand() % (timeout / 2);
				i++;
				stime = etime;
				secs = etime / 1000;
			}
		}

		if (s->diag)
			cprintf(e, "\n");
	}
	else if (s->rarp)	/* else use RARP to get server info? */
		r = get_rarp_info(e, s);

	/* we may need to send an ARP to get the server's MAC address */
	if (r >= NO_ERROR &&
			(is_mac_zero(s->server_mac) || is_mac_broadcast(s->server_mac)))
		r = get_arp_info(e, s);

	/* do not need to do this again if we remain open */
	if (r >= NO_ERROR)
	{
		s->bootp = s->rarp = FALSE;

		if (s->diag)
			cprintf(e, "Our IP addr: %u.%u.%u.%u"
					"    Server's IP addr: %u.%u.%u.%u\n",
					s->ip_addr[0], s->ip_addr[1],
					s->ip_addr[2], s->ip_addr[3],
					s->server_ip[0], s->server_ip[1],
					s->server_ip[2], s->server_ip[3]);
	}

	return r;
}

static int tftp_recv_port = TFTP_RECV_PORT;

static Retcode
tftp_send_rrq(Environ *e, Self *s, uByte *packet, int *len)
{
	struct tftp_rwrq_packet *hdr =
			(struct tftp_rwrq_packet*)(packet + UDP_PACKET_OFFSET);
	
	memset(packet, 0, MAC_BUFFER_SIZE);
	hdr->op[0] = 0;
	hdr->op[1] = TFTP_RRQ;

	if (strlen(s->file_name) >= FILE_NAME_SIZE)
		s->file_name[FILE_NAME_SIZE - 1] = '\0';

	strcpy(hdr->filename, s->file_name);
	strcpy(hdr->filename + strlen(s->file_name) + 1, "octet");
	*len = TFTP_RWRQ_PKT_SIZE + strlen(s->file_name) + 6;
	DPRINTF(("tftp_send_rrq: len=%d %s %s\n", *len, hdr->filename,
			hdr->filename + strlen(s->file_name) + 1));
	return send_udp_pkt(e, s, packet, tftp_recv_port, TFTP_DEST_PORT, *len);
}

static Retcode
tftp_send_ack(Environ *e, Self *s, int block, uByte *packet, int *len)
{
	struct tftp_packet *hdr =
			(struct tftp_packet*)(packet + UDP_PACKET_OFFSET);
	
	memset(packet, 0, MAC_BUFFER_SIZE);
	hdr->op[0] = 0;
	hdr->op[1] = TFTP_ACK;
	hdr->block[0] = (block >> BYTE_SIZE) & BYTE_MASK;
	hdr->block[1] = block & BYTE_MASK;
	*len = 4;	/* op + block only */
	DPRINTF(("tftp_send_ack: len=%d block=%d port=%d\n",
			*len, block, s->port));
	return send_udp_pkt(e, s, packet, tftp_recv_port, s->port, *len);
}

/* return Retcode for any exception, PKT_TFTP if we got a TFTP packet,
   and E_NET_TIMEOUT if we did not receive one in time
 */
static Int
tftp_get_reply(Environ *e, Self *s, uByte *packet, int *len, uLong timeout)
{
	struct tftp_packet *hdr =
			(struct tftp_packet*)(packet + UDP_PACKET_OFFSET);
	uLong etime = get_msecs(e);
	uLong stime = etime + timeout;
	Int r;

	while (etime < stime)
	{
		etime = get_msecs(e);
		r = get_udp_pkt(e, s, packet, len, tftp_recv_port);

		if (r < NO_ERROR)
			return r;

		if (r != PKT_UDP)
			continue;

		DPRINTF(("tftp_get_reply: len=%d op=%u.%u\n", *len,
				hdr->op[0], hdr->op[1]));

		/* make sure this boot reply is intended for us */
		if (hdr->op[0] != 0 ||
				hdr->op[1] < TFTP_RRQ ||
				hdr->op[1] > TFTP_ERROR)
			continue;

		*len -= 4;		/* sizeof op + block */
		return PKT_TFTP;
	}

	if (s->diag)
		cprintf(e, " \nTimeout %ldms waiting for TFTP packet", timeout);
	else
		cprintf(e, "R");

	return E_NET_TIMEOUT;
}

/* return Retcode for any exception, PKT_TFTP if we got a TFTP file,
   and anything else as appropriate
 */
static Int
get_tftp_file(Environ *e, Self *s, uByte *addr, uLong *len)
{
	uByte *sendbuf = s->sendbuf;
	uByte *recvbuf = s->recvbuf;
	struct tftp_packet *pkt =
			(struct tftp_packet*)(recvbuf + UDP_PACKET_OFFSET);
	struct udp_header *udp =
			(struct udp_header*)(recvbuf + UDP_HEADER_OFFSET);
	struct ip_header *ip =
			(struct ip_header*)(recvbuf + IP_HEADER_OFFSET);
	int currblock = 0;
	Byte *p;
	int sendlen, recvlen;
	int retry = 0;
	uLong timeout = TFTP_TIMEOUT;
	Int r = E_NET_ERROR;
	Retcode ret;
	
	DPRINTF(("obptftp: get_tftp_file\n"));
	*len = 0;

	/* default filename to hex of IP address */
	if (s->file_name[0] == '\0')
		bprintf(s->file_name, "%02X%02X%02X%02X", s->ip_addr[0], s->ip_addr[1],
				s->ip_addr[2], s->ip_addr[3]);
	
	/* ignore everything after a space in case the file arg has options */
	p = strchr(s->file_name, ' ');

	if (p)
		*p = '\0';

	/* change all vertical-bars to slashes - OBP-compatible and workaround
	   when passing in a filename with slashs in it - device expansion will
	   otherwise mess up parsing "/pci/ethernet:,/foo/bar" */
	for (p = s->file_name; (p = strchr(p, '|')) != NULL; )
		*p++ = '/';

	tftp_recv_port++;		/* use a unique port for each file */
	s->port = 0;			/* clear any server port number from */
					/* a previous TFTP */

	/* first request the filename we got from BOOTP */
	ret = tftp_send_rrq(e, s, sendbuf, &sendlen);

	if (ret != NO_ERROR)
		return ret;

	if (s->diag)
		cprintf(e, "TFTP %s ", s->file_name);
	else
		spin_cursor(e);
	
	while (retry < s->tftp_retries)
	{
		DPRINTF(("get_tftp_file: currblock=%d\n", currblock));

		r = tftp_get_reply(e, s, recvbuf, &recvlen, timeout);

		if (r < NO_ERROR && r != E_NET_TIMEOUT)
			return r;

		if (r != PKT_TFTP ||
				pkt->op[1] == TFTP_ERROR ||
				currblock + 1 != (pkt->block[0] << BYTE_SIZE) + pkt->block[1] ||
			    memcmp(s->server_ip, ip->src_addr, IP_ADDR_SIZE) != 0 ||
				(currblock > 0 &&
				(s->port != (udp->src_port[0] << BYTE_SIZE) + udp->src_port[1])))
		{
			if (r == PKT_TFTP && pkt->op[1] == TFTP_ERROR)
			{
				/* bail on any TFTP protocol error */
				cprintf(e, "TFTP error %d: %s\n",
						(pkt->block[0] << BYTE_SIZE) + pkt->block[1],
						pkt->data);
				r = E_NET_PROTOCOL_ERROR;
				break;
			}

			/* retransmit the last packet, whatever it was */
			timeout += rand() % (timeout / 2);
			retry++;

			DPRINTF(("get_tftp_file: retry=%d op[1]=%d block[0.1]=%#x.%x\n",
					retry, pkt->op[1], pkt->block[0], pkt->block[1]));

			ret = (currblock == 0) ?
					tftp_send_rrq(e, s, sendbuf, &sendlen) :
					tftp_send_ack(e, s, currblock, sendbuf, &sendlen);

			if (ret != NO_ERROR)
				return ret;

			if (!s->diag)
				spin_cursor(e);

			continue;
		}
		
		DPRINTF(("get_tftp_file: op[1]=%d\n", pkt->op[1]));

		if (pkt->op[1] != TFTP_DATA)	/* abort out on any error */
		{
			if (s->diag)
				cprintf(e, "E");
			else
				spin_cursor(e);

			r = E_NET_PROTOCOL_ERROR;
			break;
		}
		
		/* got the next block, send an ack */
		if (currblock == 0)
		{
			/* save port number for TFTP */
			s->port = (udp->src_port[0] << BYTE_SIZE) + udp->src_port[1];
		}

		ret = tftp_send_ack(e, s, ++currblock, sendbuf, &sendlen);

		if (ret != NO_ERROR)
		{
			if (s->diag)
				cprintf(e, "S");
			else
				spin_cursor(e);

			r = ret;
			break;
		}
		
		/* got a good data block - copy it in */
		memcpy(addr, pkt->data, recvlen);
		addr += recvlen;
		*len += recvlen;
	
		if ((currblock & 0x1F) == 0)
		{
		    if (s->diag)
			cprintf(e, "#");
		    else
			spin_cursor(e);
		}
		
		DPRINTF(("get_tftp_file: recvlen=%d\n", recvlen));

		if (recvlen < TFTP_XFER_SIZE)		/* eof? */
		{
			r = PKT_TFTP;
			break;
		}
		
		retry = 0;
		timeout = TFTP_TIMEOUT;
	}
	
	if (s->diag)
		cprintf(e, "\n");

	return r;
}

/* return Retcode for any exception, PKT_PING if we successfully pinged,
   and anything else as appropriate
 */
static Int
do_ping(Environ *e, Self *s, uByte ip[], Int msecs, Int *reply)
{
	uByte *buf = s->sendbuf;
	Int r = E_NET_ERROR;
	Retcode ret;
	int seq, len;

	DPRINTF(("do_ping: IP-addr = %u.%u.%u.%u", s->ip_addr[0],
			s->ip_addr[1], s->ip_addr[2], s->ip_addr[3]));
	DPRINTF((" server-IP = %u.%u.%u.%u\n", s->server_ip[0],
			s->server_ip[1], s->server_ip[2], s->server_ip[3]));

	/* use RARP or BOOTP to get our IP address if needed */
	if (is_ip_zero(s->ip_addr) || is_ip_broadcast(s->ip_addr))
	{
		if (s->rarp)
		{
			r = get_rarp_info(e, s);

			if (r != PKT_RARP)
				return r;
		}
		else if (s->bootp)
		{
			r = get_bootp_info(e, s);

			if (r < NO_ERROR)
				return r;
		}
		else if (!s->bootp && !s->rarp)
			return E_NET_ERROR;
	}

	if (s->diag)
		cprintf(e, "PINGing %u.%u.%u.%u from %u.%u.%u.%u ",
				ip[0], ip[1], ip[2], ip[3],
				s->ip_addr[0], s->ip_addr[1], s->ip_addr[2], s->ip_addr[3]);
	
	memcpy(s->server_ip, ip, IP_ADDR_SIZE);
	
	/* send an ARP to get the target's MAC address */
	r = get_arp_info(e, s);

	if (r != PKT_ARP)
		return r;

	memset(buf, 0, sizeof buf);
	*reply = msecs;
	seq = rand() & 0xFFFF;
	
	ret = send_ping_pkt(e, s, buf, seq);

	if (ret != NO_ERROR)
		return ret;

	r = get_ping_pkt(e, s, buf, &len, seq, reply);

	if (r == PKT_PING)
		*reply = msecs - *reply;

	return r;
}

C(f_obptftp_load)			/* load (addr -- size) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	uByte *addr;
	uLong len = 0;
	Int r = E_NET_ERROR;
	
	IFCKSP(e, 1, 1);
	POPT(e, addr, uByte*);
	
	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;
	
	if (get_net_params(e, inst->self))
	{
		r = get_bootp_info(e, inst->self);

		if (r >= NO_ERROR)
		{
			r = get_tftp_file(e, inst->self, addr, &len);

			if (r == PKT_TFTP)
			{
				PUSH(e, len);
				DPRINTF(("obptftp_load: len=%d ret-len=%d\n", len, TOP(e)));
				return NO_ERROR;
			}
		}
	}

	PUSH(e, 0);
	DPRINTF(("obptftp_load: len=%d ret-len=0, r=%d\n", len, r));
	return (Retcode)r;
}

C(f_obptftp_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Cell ipval;
	Int msecs;
	Int reply = -1;
	Int r;
	uByte ip[IP_ADDR_SIZE];

	IFCKSP(e, 2, 1);
	POP(e, msecs);
	POP(e, ipval);

	/* if no IP is provided on the stack, assume that the open args
	   provided one */
	if (ipval == 0)
		memcpy(ip, inst->self->server_ip, IP_ADDR_SIZE);
	else
	{
		ip[0] = (ipval >> (BYTE_SIZE * 3)) & BYTE_MASK;
		ip[1] = (ipval >> (BYTE_SIZE * 2)) & BYTE_MASK;
		ip[2] = (ipval >> BYTE_SIZE) & BYTE_MASK;
		ip[3] = ipval & BYTE_MASK;
	}

	if (inst == NULL || inst->self == NULL)
		return E_BAD_INSTANCE;
	
	if (get_net_params(e, inst->self))
	{
		r = do_ping(e, inst->self, ip, msecs, &reply);

		if (r != PKT_PING)
			return (Retcode)r;
	}
	
	PUSH(e, reply);	
	return NO_ERROR;
}

/* parse an IP addr from a string - we expect either the dotted-decimal
   representation x.y.z.q or a hex string with an optional "0x" prefix
 */
static Bool
parse_ip(Environ *e, Byte **str, uByte ip[], const char *errstr)
{
	Int i;
	Cell error, val;
	Int len = strlen(*str);
	Byte *ostr = *str;
	
	for (i = 0; i < 4; i++)
	{
		parse_number(10, str, &len, &val, &error, FALSE);
		DPRINTF(("parse_ip: str=%s len=%d val=%Cd error=%Cd\n",
				*str, len, val, error));
		
		if (!error || 
				(i < 3 && **str == '.') ||
				(i == 3 && (**str == ',' || **str == '\0')))
			ip[i] = (uByte)val;
		else
		{
			/* some sort of error - try parsing it again as a hex string */
			*str = ostr;
			len = strlen(*str);

			/* skip any optional "0x" prefix */
			if ((*str)[0] == '0' && ((*str)[1] == 'x' || (*str)[1] == 'X'))
			{
				*str += 2;
				len -= 2;
			}

			parse_number(16, str, &len, &val, &error, FALSE);

			if (!error || **str == ',' || **str == '\0')
			{
				ip[0] = (val >> (3 * BYTE_SIZE)) & BYTE_MASK;
				ip[1] = (val >> (2 * BYTE_SIZE)) & BYTE_MASK;
				ip[2] = (val >> BYTE_SIZE) & BYTE_MASK;
				ip[3] = val & BYTE_MASK;
				break;
			}

			cprintf(e, "%s bad IP format \"%s\"\n", errstr, *str);
			return FALSE;
		}
		
		if (**str == '.')		/* leave pointer at comma */
			(*str)++;
	}

	DPRINTF(("parse_ip: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]));
	return TRUE;
}

C(f_obptftp_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Retcode ret = NO_ERROR;
	char *str;
	
	if (inst == NULL || inst->self != NULL)
		return E_BAD_INSTANCE;
	
	/* allocate DMA-capable buffer */
#ifdef OBPTFTP_DMA_ALLOCATE
	IFCKSP(e, 0, 1);
	PUSH(e, sizeof *s);
	ret = execute_method_name(e, inst->parent->parent,
			"dma-alloc", CSTR);
	DPRINTF(("obptftp_open: dma-alloc ret=%d\n", ret));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s, Self*);
#else
	s = (Self*)malloc(sizeof *s);
#endif

	DPRINTF(("obptftp_open: self=%#x\n", s));
	
	if (s == NULL)
		return E_OUT_OF_MEMORY;

	inst->self = s;
	memset(s, 0, sizeof (Self));
	s->bootp_retries = 5;
	s->tftp_retries = RETRIES;
	s->ping_timeout = 3000;
	s->diag = diagnostic_mode(e);
	memset(s->server_mac, 0xFF, MAC_ADDR_SIZE);

#ifdef SUN_COMPATIBILITY
	s->rarp = TRUE;
#else
	s->bootp = TRUE;
	DPRINTF(("obptftp: loadargs=\"%s\"\n", e->loadargs));

	/* Sun OBP doesn't parse the rest of the arguments from the cmd line */
	if (e->loadargs)
		strncpy(s->file_name, e->loadargs, FILE_NAME_SIZE);
#endif

	/* get defaults from NVRAM vars is possible */
	if ((str = get_config(e, "boot-protocol", CSTR)) != NULL)
	{
		if (compare_strs(str, CSTR, "rarp", CSTR))
		{
			s->bootp = FALSE;
			s->rarp = TRUE;
		}
		else if (compare_strs(str, CSTR, "bootp", CSTR))
		{
			s->bootp = TRUE;
			s->rarp = FALSE;
		}
		else if (compare_strs(str, CSTR, "dhcp", CSTR))
		{
			s->dhcp = TRUE;
			s->bootp = TRUE;
			s->rarp = FALSE;
		}
		else
			cprintf(e, "obptftp: nvram: boot-protocol: ignoring protocol \"%s\"\n", str);
	}

	if ((str = get_config(e, "server-ip", CSTR)) != NULL)
	{
		if (!parse_ip(e, &str, s->user_server_ip, "obptftp: nvram: server-ip:"))
			memset(s->user_server_ip, 0, IP_ADDR_SIZE);
	}

	if ((str = get_config(e, "client-ip", CSTR)) != NULL)
	{
		if (!parse_ip(e, &str, s->user_ip_addr, "obptftp: nvram: client-ip:"))
			memset(s->user_server_ip, 0, IP_ADDR_SIZE);
	}

	/* parse any arguements that were passed to "open" */
	if (inst->args && *inst->args)
	{
		Byte *str = inst->args + 1;
		Int len;
		Cell val, error;
		Bool ping = FALSE;

		DPRINTF(("obptftp: start: args=\"%s\"\n", str));

		if (compare_strs(str, 5, "bootp", 5) &&
				(str[5] == ',' || str[5] == '\0'))
		{
			str += 5;
			s->bootp = TRUE;
#ifdef SUN_COMPATIBILITY
			/* this only affects the 1st outgoing request packet
			   - always send a DHCP request for Sun */
			s->dhcp = TRUE;
#endif
			s->rarp = FALSE;
			
			if (*str == ',')
				str++;
			else
				goto done;
		}
		else if (compare_strs(str, 4, "dhcp", 4) &&
				(str[4] == ',' || str[4] == '\0'))
		{
			str += 4;
			s->dhcp = TRUE;
			s->bootp = TRUE;
			s->rarp = FALSE;
			
			if (*str == ',')
				str++;
			else
				goto done;
		}
		else if (compare_strs(str, 4, "rarp", 4) &&
				(str[4] == ',' || str[4] == '\0'))
		{
			str += 4;
			s->bootp = FALSE;
			s->rarp = TRUE;
			
			if (*str == ',')
				str++;
			else
				goto done;
		}
		else if (compare_strs(str, 4, "ping", 4) &&
				(str[4] == ',' || str[4] == '\0'))
		{
			str += 4;			
			ping = TRUE;
		
			if (*str == ',')
				str++;
			else
				goto done;
		}
		
		DPRINTF(("obptftp: after keyword: args=\"%s\"\n", str));		

		/* allow an empty server_ip */
		if (*str == ',')
			str++;
		else
		{
			if (!parse_ip(e, &str, s->user_server_ip, "obptftp: server_ip:"))
			{
				ret = E_BAD_ARGUMENT;
				goto done;
			}
		
			if (*str == ',')
				str++;
			else
				goto done;
		}

		DPRINTF(("obptftp: after server_ip: args=\"%s\"\n", str));		

		if (!ping)
		{
			if (*str)
			{
				char *f = s->file_name;
				
				while (*str != ',' && *str != '\0')
					*f++ = *str++;
				
				*f = '\0';
			}
			
			if (*str == ',')
				str++;
			else
				goto done;
		}

		DPRINTF(("obptftp: after filename: args=\"%s\"\n", str));		

		if (*str != ',')
		{
			if (!parse_ip(e, &str, s->user_ip_addr, "obptftp: IP addr arg:"))
			{
				ret = E_BAD_ARGUMENT;
				goto done;
			}
		}
		
		if (*str == ',')
			str++;
		else
			goto done;

		DPRINTF(("obptftp: after ip_addr: args=\"%s\"\n", str));		

		if (!parse_ip(e, &str, s->user_gateway_ip, "obptftp: gateway IP arg:"))
		{
			ret = E_BAD_ARGUMENT;
			goto done;
		}
		
		if (*str == ',')
			str++;
		else
			goto done;

		DPRINTF(("obptftp: after gateway_ip: args=\"%s\"\n", str));		

		if (*str)
		{
			len = strlen(str);
			parse_number(10, &str, &len, &val, &error, FALSE);
			
			if (!error || *str == ',')
			{
				if (ping)
					s->ping_timeout = val;
				else
					s->bootp_retries = val;
			}
			else
			{
				ret = E_BAD_ARGUMENT;
				goto done;
			}
		}
		
		if (ping)			/* no more arguments for ping */
			goto done;
			
		if (*str == ',')
			str++;
		else
			goto done;
		
		DPRINTF(("obptftp: after bootp_retries: args=\"%s\"\n", str));

		if (*str)
		{
			len = strlen(str);
			parse_number(10, &str, &len, &val, &error, FALSE);
			
			if (!error || *str == ',')
				s->tftp_retries = val;
			else
			{
				ret = E_BAD_ARGUMENT;
				goto done;
			}
		}
	}

done:
	PUSH(e, ret == NO_ERROR ? FTRUE : FFALSE);
	return ret;
}

C(f_obptftp_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret = NO_ERROR;

	if (inst && inst->self)
	{
#ifdef OBPTFTP_DMA_ALLOCATE
		IFCKSP(e, 0, 2);
		PUSHP(e, inst->self);
		PUSH(e, sizeof *inst->self);
		ret = execute_method_name(e, inst->parent->parent,
				"dma-free", CSTR);
		DPRINTF(("obptftp_close: dma-free ret=%d\n", ret));
#else
		free(inst->self);
#endif

		inst->self = NULL;
	}
	
	return ret;
}


/* our-IP is not parsed after IP-addr as that should be an argument
   to the device - if IP-addr is not provided, assume it is passed
   in as argument siaddr to the device
 */
C(f_ping)	/* ping ("{device:args [IP-addr] [timeout]}<eol>" --) */
{
	Byte *dev = parse_word(e);
	Byte *args = parse_line(e);
	Cell timeout = 3000;
	Cell ip = 0;
	Instance *inst;
	Int time;
	Retcode ret;
	uByte ipaddr[IP_ADDR_SIZE];

	IFCKSP(e, 0, 2);

	if (!*dev)
		return E_NO_DEVICE;
	
	/* parse the target IP address */
	if (*args)
	{
		Byte *str = args + 1;
		Int len = *(uByte*)args;
		Cell error;

		if (parse_ip(e, &str, ipaddr, "obptftp: ping IP addr:"))
		{
			ip = (ipaddr[0] << (BYTE_SIZE * 3)) |
					(ipaddr[1] << (BYTE_SIZE * 2)) |
					(ipaddr[2] << BYTE_SIZE) |
					ipaddr[3];

			if (*str == ' ')		/* parse the timeout, if any */
			{
				str++;
				parse_number(10, &str, &len, &timeout, &error, FALSE);
			}
		}
		else	/* no IP - perhaps a timeout */
		{
			str = args + 1;
			parse_number(10, &str, &len, &timeout, &error, FALSE);
		}
	}

	/* try to open this device */
	PUSHP(e, dev + 1);
	PUSH(e, *(uByte*)dev);
	ret = execute_word(e, "open-dev");
	
	if (ret != NO_ERROR)
		return ret;
	
	POPT(e, inst, Instance*);
	
	if (inst == NULL)
	{
		Byte *str = dev + 1;
		Int len = *(uByte*)dev;
		Byte buf[256];

		if (len > 255)
			len = 255;

		strncpy(buf, str, len);
		cprintf(e, "Unable to open device: %s\n", buf);
		return E_ABORT;
	}

	cprintf(e, "ping ... ");
	PUSH(e, ip);
	PUSH(e, timeout);
	ret = execute_method_name(e, inst, "ping", CSTR);

	if (ret != NO_ERROR)
		return ret;

	POP(e, time);		/* length of code loaded into memory */

	if (time < 0)
		cprintf(e, "no reply in %d msecs\n", timeout);
	else
		cprintf(e, "reply in %d msecs\n", time);
		
	/* and close the device */
	PUSHP(e, inst);
	(void)execute_word(e, "close-dev");
	return ret;
}


static const Initentry obptftp_methods[] =
{
	{ "open",   f_obptftp_open,   INVALID_FCODE },
	{ "close",  f_obptftp_close,  INVALID_FCODE },
	{ "load",   f_obptftp_load,   INVALID_FCODE },
	{ "ping",   f_obptftp_ping,   INVALID_FCODE },
	{ NULL, NULL }
};

static const Initentry ping_word[] = 
{
	{ "ping",   f_ping,   INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{device:args [IP-addr] [timeout]}<eol>\" --)\n" \
					"\tping IP address using network device and timeout") },
	{ NULL, NULL }
};

CC(install_obptftp)
{
	Package *pkg = new_pkg_name(e->packages, "obp-tftp");
	Retcode ret;
	
	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	ret = init_entries(e, pkg->dict, obptftp_methods);
	
	if (ret == NO_ERROR)
		ret = init_entries(e, e->names, ping_word);
	
	return ret;
}
