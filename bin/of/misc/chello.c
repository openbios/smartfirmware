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

/* (c) Copyright 1999-2000 by CodeGen, Inc.  All Rights Reserved. */

/* Simple hello program to demo SmartFirmware's client-interface.
 */

#include "sfclient.h"


/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }


/* debug printf - could use varargs and vsprintf if so desired
 */
void
dprintf(const char *str)
{
	sf_puts(str);
}

void
printhexdigs(int hex)
{
	char buf[4];
	int dig = (hex >> 4) & 0xF;

	if (dig >= 0 && dig <= 9)
		buf[0] = '0' + dig;
	else
		buf[0] = 'A' + dig - 10;

	dig = hex & 0xF;

	if (dig >= 0 && dig <= 9)
		buf[1] = '0' + dig;
	else
		buf[1] = 'A' + dig - 10;

	buf[2] = '\0';
	dprintf(buf);
}

void
printmacaddr(char *pbuf)
{
	int i;

	dprintf("  [");

	for (i = 0; i < 6; i++)
	{
	    if (i)
		dprintf(":");

	    printhexdigs(pbuf[i]);
	}

	dprintf("]");
}

extern int sf_strlen(const char *str);

int
sf_strncmp(const char *s1, const char *s2, int len)
{
	if (s1 == NULL)
		s1 = "";

	if (s2 == NULL)
		s2 = "";

	for (; *s1 && *s2 && len--; s1++, s2++)
		if (*s1 != *s2)
			break;

	return (!len && !*s1 && !*s2) ? 0 : *s2 - *s1;
}

void
walk_tree(Package *pkg, const char *type)
{
	char pbuf[256];
	int plen;
	Package *p;
	int netdevs = 0;

	if (sf_strncmp(type, "network", 8) == 0)
		netdevs = 1;

	for (; pkg != NULL; pkg = sf_peer(pkg))
	{
		plen = sf_getprop(pkg, "device_type", pbuf, sizeof pbuf);

		if (plen > 0 && sf_strlen(type) < plen &&
				sf_strncmp(pbuf, type, plen) == 0)
		{
			sf_package_to_path(pkg, pbuf, sizeof pbuf);
			dprintf("\t");
			dprintf(pbuf);

			if (netdevs)
			{
				/* get the "local-mac-address" property */
				plen = sf_getprop(pkg, "local-mac-address", pbuf, sizeof pbuf);

				if (plen < 0)
					plen = sf_getprop(pkg, "mac-address", pbuf, sizeof pbuf);

				if (plen == 6)
					printmacaddr(pbuf);
			}

			dprintf("\r\n");
		}

		p = sf_child(pkg);

		if (p != NULL)
			walk_tree(p, type);
	}
}

int
main(int argc, const char *argv[], const char *envv[])
{
	/* addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register
	 */
	sf_init((void*)argv[argc + 1]);

	dprintf("Hello, client-interface world!\r\n\r\n");

	dprintf("ethernet devices:\r\n");
	walk_tree(sf_getroot(), "network");

	return R_OK;
}
