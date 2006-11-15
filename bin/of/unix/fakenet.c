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

/* (c) Copyright 1996-2002 by CodeGen, Inc.  All Rights Reserved. */

/* Driver for faking a network controller in standalone environment.
   This code is specific to FreeBSD and uses the "tap" psuedo-device
   interface.
 */

/*#define DEBUG*/

#include <stdio.h>
#include "defs.h"

#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

#define FAKE_NET_DEV		"/dev/tap%d"
#define FAKE_NET_NAME		"tap%d"

#define PACKET_MTU		1600

#define MAC_ADDR_SIZE		6

extern Bool parse_mac_addr(Environ *e, Byte **str, uByte mac_addr[], const char *errstr);
extern Bool parse_ip(Environ *e, Byte **str, uByte ip[], const char *errstr);

/* instance-specific vars */
struct self
{
	int fd;
	Instance *obptftp;
	uByte netname[20];
	uByte ee_mac_addr[MAC_ADDR_SIZE];
	uByte mac_addr[MAC_ADDR_SIZE];
	uByte rip_addr[4];
	uByte buf[PACKET_MTU];
	Bool promiscuous;
	Bool duplex;
	Int speed;
};
typedef struct self Self;

static int
tap_open(char *netname)
{
	int unit;
	int fd;
	int flag;
	char devname[20];

	for (unit = 0; ; unit++)
	{
		sprintf(devname, FAKE_NET_DEV, unit);

		fd = open(devname, O_RDWR);

		if (fd >= 0)
			break;

		if (errno == ENOENT)
			return -1;
	}

	sprintf(netname, FAKE_NET_NAME, unit);

	flag = 1;
	ioctl(fd, FIONBIO, &flag);
	return fd;
}

static int
read_net_info(Environ *e, Package *pkg, Self *s)
{
	FILE *fp;
	Byte buf[80];
	Byte *bp;
	Byte *mac;
	Byte *ip;

	fp = fopen("fakenet.info", "r");

	if (fp == NULL)
		return 0;

	/* read a line out of the file, it should be of the form */
	/* xx:xx:xx:xx:xx:xx xxx.xxx.xxx.xxx */

	bp = fgets(buf, sizeof buf, fp);
	fclose(fp);

	if (bp == NULL)
		return 0;

	while (*bp == ' ' || *bp == '\t')
		bp++;

	mac = bp;

	for (; *bp; bp++)
		if (*bp == ' ' || *bp == '\t')
		{
			*bp++ = '\0';
			break;
		}

	while (*bp == ' ' || *bp == '\t')
		bp++;

	ip = bp;

	for (; *bp; bp++)
		if (*bp == ' ' || *bp == '\t' || *bp == '\r' || *bp == '\n')
		{
			*bp = '\0';
			break;
		}

	/* parse mac address */
	if (!parse_mac_addr(e, &mac, s->ee_mac_addr, "fakenet"))
		return 0;

	if (!parse_ip(e, &ip, s->rip_addr, "fakenet"))
		return 0;
	  
	snprintf(buf, sizeof(buf), "ifconfig %s inet %d.%d.%d.%d", s->netname,
		s->rip_addr[0], s->rip_addr[1], s->rip_addr[2], s->rip_addr[3]);
	system(buf);
	snprintf(buf, sizeof(buf), "ifconfig %s", s->netname);
	system(buf);

	memcpy(s->mac_addr, s->ee_mac_addr, MAC_ADDR_SIZE);

	return set_property(pkg->props, "local-mac-address", CSTR,
			(Byte *)s->mac_addr, MAC_ADDR_SIZE) == NO_ERROR;
}

C(f_fake_net_open)			/* open (-- okay?) */
{
	Bool diag = diagnostic_mode(e);
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Package *pkg;
	Self *s;
	Bool promiscuous = FALSE;
	Bool duplex = FALSE;
	Int speed = 100;
	Retcode ret;
	Int maclen;
	uByte *macaddr;

	IFCKSP(e, 0, 4);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	pkg = inst->package;

	if (!pkg)
		return E_NULL_PACKAGE;

	DPRINTF(("fake_net_open: pkg=%p parent=%p\n", pkg, pkg->parent));

	/* allocate and clear a self object */
	s = (Self*)malloc(sizeof *s);

	if (s == NULL)
		return E_OUT_OF_MEMORY;

	memset(s, 0, sizeof *s);
	inst->self = s;

	/* parse arguments - they are all optional - following args
	   are handed off to the obptftp package */
	if (inst->args && *inst->args)
	{
		Byte *str;
		Int len;
		Cell val, error;
		DPRINTF(("fake_net_open: args='%s'\n", inst->args));
		
		for (str = inst->args + 1; *str; str++)
		{
			while (isspace(*str))
				str++;
			
			if (*str == '\0')
				break;

			DPRINTF(("ethexpro: args=\"%s\"\n", str));

			#define IS_END(c)	((c) == ',' || (c) == '\0')
			#define IS_MORE(c)	(!IS_END(c))
		
			if (compare_strs(str, 11, "promiscuous", 11) && IS_END(str[11]))
			{
				str += 11;
				promiscuous = TRUE;
			}
			else if (compare_strs(str, 6, "speed=", 6) && IS_MORE(str[6]))
			{
				str += 6;
				len = strlen(str);
				parse_number(10, &str, &len, &val, &error, FALSE);
				
				if (!error || (str[0] == ',' && val > 0))
				{
					switch (val)
					{
					case 10:
					case 100:
						speed = val;
						break;
						
					case 4:
					case 16:
					case 155:
					case 622:
					case 1000:
					default:
						cprintf(e, "ethexpro: unsupported \"speed=%d\"\n", val);
						break;
					}
				}
			}
			else if (compare_strs(str, 7, "duplex=", 7) && IS_MORE(str[7]))
			{
				str += 7;

				if (compare_strs(str, 4, "half", 4) && IS_END(str[4]))
				{
					duplex = FALSE;
					str += 4;
				}
				else if (compare_strs(str, 4, "full", 4) && IS_END(str[4]))
				{
					duplex = TRUE;
					str += 4;
				}
				else
					cprintf(e, "ethexpro: bad arg format for \"duplex=\"");
			}
			else
			{
				/* pass all remaining args to obptftp */
				break;
			}
			
			if (*str != ',')
				break;
		}
		
		/* remainder goes to obptftp */
		PUSH(e, (uPtr)str);		/* arg-str */
		PUSH(e, strlen(str));	/* arg-len */
	}
	else
	{
		PUSH(e, "");	/* arg-str */
		PUSH(e, 0);		/* arg-len */
	}

	s->promiscuous = promiscuous;
	s->duplex = duplex;
	s->speed = speed;

	/* now open our fake net device reading and writing */
	s->fd = tap_open(s->netname);

	if (s->fd < 0)
		return E_NO_FILE;

	if (diag)
		cprintf(e, "Remote interface %s\n", s->netname);

	/* get networking info */
	read_net_info(e, pkg, s);

	/* create an instance of /packages/obptftp to support load etc. */
	PUSHP(e, "");	/* arg-str */
	PUSH(e, 0);		/* arg-len */
	PUSHP(e, "obp-tftp");			/* name-str */
	PUSH(e, strlen("obp-tftp"));	/* name-len */
	ret = execute_word(e, "$open-package");
	DPRINTF(("fake_net_open: $open-package obptftp ret=%d:%s\n",
			ret, err2str(ret)));

	if (ret != NO_ERROR)
		return ret;

	POPT(e, s->obptftp, Instance*);
	DPRINTF(("fake_net_open: obptftp=%p\n", s->obptftp));

	if (s->obptftp == NULL)
		return E_NULL_INSTANCE;

	ret = execute_word(e, "mac-address");

	if (ret != NO_ERROR)
		return ret;
	
	POP(e, maclen);
	POPT(e, macaddr, uByte*);

	if (maclen != MAC_ADDR_SIZE)
		return E_ABORT;

	/* use this returned value for our MAC addr */
	memcpy(s->mac_addr, macaddr, maclen);

	/* let the user know the MAC addr to help configure BOOTP */
	if (diag)
		cprintf(e, "Ethernet MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
				macaddr[0], macaddr[1], macaddr[2],
				macaddr[3], macaddr[4], macaddr[5]);

	PUSH(e, FTRUE);
	return ret;
}

C(f_fake_net_close)			/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;

	DPRINTF(("fake_net_close\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || !s->obptftp || s->fd < 0)
		return E_BAD_INSTANCE;

	/* close up our helper packages */
	IFCKSP(e, 0, 1);
	PUSHP(e, s->obptftp);
	(void)execute_word(e, "close-package");

	/* close our fake device */
	close(s->fd);

	/* free our self block */
	free(s);

	return NO_ERROR;
}

C(f_fake_net_read)		/* read (addr len -- #read) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Byte *addr;
	uInt count;
	uInt actual = 0;

	DPRINTF(("fake_net_read: inst=%p s=%p\n", inst, s));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || s->fd < 0)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, count);
	POPT(e, addr, Byte*);

	actual = read(s->fd, addr, count);

	if (actual == -1 && errno == EWOULDBLOCK)
		actual = 0;

	DPRINTF(("fake_net_read: count=%d addr=%p actual=%d\n",
			count, addr, actual));

	if (actual > 0)
		dump_packet(e, addr, count);

	PUSH(e, actual > 0 ? actual : (actual == 0 ? -2 : -1));
	return actual >= 0 ? NO_ERROR : E_FILE_IO;
}

C(f_fake_net_write)		/* write ( addr len -- #written ) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Self *s;
	Byte *addr;
	uInt actual, count;

	DPRINTF(("fake_net_write: inst=%p s=%p\n", inst, s));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s || s->fd < 0)
		return E_BAD_INSTANCE;

	IFCKSP(e, 2, 1);
	POP(e, count);
	POPT(e, addr, Byte*);

	dump_packet(e, addr, count);
	actual = write(s->fd, addr, count);

	PUSH(e, actual == count ? actual : -1);

	DPRINTF(("fake_net_write: count=%u addr=%p actual=%u\n",
			count, addr, actual));
	return actual == count ? NO_ERROR : E_FILE_IO;
}

static Retcode
do_obptftp_method(Environ *e, Byte *method)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Instance *subpkg;
	Self *s;

	if (!inst)
		return E_NULL_INSTANCE;

	s = inst->self;

	if (!s)
		return E_BAD_INSTANCE;

	subpkg = s->obptftp;

	if (!subpkg)
		return E_NULL_INSTANCE;

	return execute_method_name(e, subpkg, method, CSTR);
}

C(f_fake_net_load)				/* load (addr -- size) */
{
	return do_obptftp_method(e, "load");
}

C(f_fake_net_ping)				/* ping (ip-addr msecs -- reply-msecs) */
{
	return do_obptftp_method(e, "ping");
}

C(f_fake_net_selftest)			/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);
	Retcode ret = NO_ERROR;

	DPRINTF(("fake_net selftest: enter\n"));

	if (diag)
		cprintf(e, "fake_net selftest...\n");

	PUSH(e, ret);			/* successful */
	return NO_ERROR;
}


static const Initentry fake_net_methods[] =
{
	{ "open",           f_fake_net_open,          INVALID_FCODE },
	{ "close",          f_fake_net_close,         INVALID_FCODE },

	{ "read",           f_fake_net_read,          INVALID_FCODE },
	{ "write",          f_fake_net_write,         INVALID_FCODE },

	{ "load",           f_fake_net_load,          INVALID_FCODE },
	{ "ping",           f_fake_net_ping,          INVALID_FCODE },
	{ "selftest",       f_fake_net_selftest,      INVALID_FCODE },

	{ NULL,             NULL },
};


CC(install_fake_net_driver)
{
	Package *pkg = new_pkg_name(e->root, "net");
	Retcode ret;

	DPRINTF(("install_fake_net_driver: pkg=%p\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	ret = init_entries(e, pkg->dict, fake_net_methods);

	if (ret != NO_ERROR)
		return ret;

	prop_set_str(pkg->props, "device_type", CSTR, "network", CSTR);

	DPRINTF(("install_fake_net_driver: return %s (%d)\n", err2str(ret), ret));
	return ret;
}
