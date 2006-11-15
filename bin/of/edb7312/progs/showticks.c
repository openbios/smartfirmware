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

/* Simple program to catch clock ticks from OpenFirmware */

#include "sfclient.h"

extern void strcpy(char *, char *);
extern void strcat(char *, char *);
extern int strcmp(char *, char *);

/* dummies to avoid normal gcc/g++ runtime goo */
void __gccmain() { }
void __main() { }

/* debug printf - could use varargs and vsprintf if so desired
 */
void
dprintf(const char *str)
{
#ifdef DEBUG
	sf_puts(str);
#endif
}

volatile int g_ticks = 0;
volatile uByte g_altstack[1024];
volatile uInt g_altstate[17];
volatile uInt g_mainstate[17];
volatile int g_inmain;
int loops = 0;

struct linkage
{
	uInt pc;
	uInt lr;
	int count;
};

typedef struct linkage Linkage;

Linkage linkages[1000];
int numlinkages = 0;

void
add_linkage(uInt pc, uInt lr)
{
	 int i;

	 for (i = 0; i < numlinkages; i++)
		if (linkages[i].pc == pc && linkages[i].lr == lr)
		{
			linkages[i].count++;
			return;
		}

	if (i >= 1000)
		return;

	linkages[i].pc = pc;
	linkages[i].lr = lr;
	linkages[i].count = 1;
	numlinkages = i + 1;
}

void
tick_handler(uInt *state)
{
	int i;

	g_ticks++;
	add_linkage(state[16], state[15]);

	if (g_inmain)
	{
		for (i = 0; i < 17; i++)
		{
			g_mainstate[i] = state[i];
			state[i] = g_altstate[i];
		}

		g_inmain = 0;
	}
	else
	{
		for (i = 0; i < 17; i++)
		{
			g_altstate[i] = state[i];
			state[i] = g_mainstate[i];
		}

		g_inmain = 1;
	}
}

int
callback_func(Cell *array)
{
	char *op = (char *)array[0];
	uInt nargs = array[1];
	uInt nrets = array[2];
	uInt *state = (uInt *)array[3];

	if (strcmp(op, "tick") == 0)
	{
		tick_handler(state);
		array[3 + nargs] = R_OK;
		return R_OK;
	}

	array[3 + nargs] = R_ERR;
	return R_ERR;
}

void
show_ticks()
{
	int ticks1 = g_ticks;
	int ticks2 = g_ticks;
	int i = 0;

	while (ticks2 < ticks1 + 1000 && i < 1000)
	{
		ticks2 = g_ticks;
		i++;
		sf_printf("%d clock ticks\r", ticks2);
	}

	sf_puts("\n");
}

void
show_linkages()
{
	int i;

	for (i = 0; i < numlinkages; i++)
		sf_printf("%08X %08X %d\n", linkages[i].pc, linkages[i].lr,
				linkages[i].count);
}

void
looper()
{
	while (1)
	    loops++;
}

int
main(int argc, const char *argv[], const char *envv[])
{
	Callback *oldfunc;

	/* addr of C client-interface is passed in argc after terminating NULL
	   - it is also passed as a hex string in envv as "client_interface"
	   - the addr of the asm interface is in a machine-dependent register
	 */
	sf_puts("In showticks\r\n");
	g_altstate[0] = 0x10;			/* user mode */
	g_altstate[14] = (uInt)&g_altstack[512];
	g_altstate[16] = (uInt)looper;
	g_inmain = 1;
	oldfunc = sf_set_callback(callback_func);
	sf_puts("Counting ticks\r\n");

	show_ticks();

	sf_set_callback(oldfunc);
	show_linkages();
	sf_printf("looper cycled %d times\n", loops);
	sf_puts("Done!\r\n");
	return R_OK;
}
