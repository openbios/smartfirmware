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

/* (c) Copyright 2001 by CodeGen, Inc.  All Rights Reserved. */

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
printhexnum(unsigned int num)
{
	char buf[12];
	int i;

	buf[11] = '\0';
	i = 10;

	while (num || i >= 10)
	{
		int dig = num & 0xF;
		num >>= 4;

		if (dig >= 0 && dig <= 9)
			buf[i] = '0' + dig;
		else
			buf[i] = 'A' + dig - 10;

		i--;
	}

	buf[i] = 'x';
	i--;
	buf[i] = '0';
	dprintf(&buf[i]);
}

#define SYSCON1	0xF7200100
#define PORTA	0xF7200000

#define IN(addr) (*(volatile unsigned int *)(addr))
#define OUT(addr, val) (*(volatile unsigned int *)(addr) = (val))

unsigned int
getkeystate()
{
	unsigned int syscon1 = IN(SYSCON1);
	unsigned int porta;
	OUT(SYSCON1, (syscon1 & ~0xF) | 0xD);
	porta = IN(PORTA);
	OUT(SYSCON1, syscon1);
	return porta & 0x3F;
}

int
main(int argc, const char *argv[], const char *envv[])
{
	unsigned int keystate;
	unsigned int newstate;

	/* addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register
	 */

	dprintf("Hello, client-interface world!\r\n\r\n");

	keystate = getkeystate();
	dprintf("Initial key state ");
	printhexnum(keystate);
	dprintf("\r\n");

	while (1)
	{
		newstate = getkeystate();

		if (keystate ^ newstate)
		{
			*((uInt *)0x43215600) = 0;
			break;
			dprintf("Key state changed, new state ");
			printhexnum(newstate);
			dprintf(", changed bits ");
			printhexnum(keystate ^ newstate);
			dprintf("\r\n");
		}

		keystate = newstate;
	}

	return R_OK;
}
