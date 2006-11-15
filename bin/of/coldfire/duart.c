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

/* (c) Copyright 1997-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Coldfire builtin DUART serial device driver */

//#define DEBUG
#include "defs.h"

#define MBAR	M5206_REGISTERS
#define UMR		(MBAR + 0x140)	/* r/w */
#define USR		(MBAR + 0x144)	/* r */
#define UCSR	(MBAR + 0x144)	/* w */
#define UCR		(MBAR + 0x148)	/* w */
#define URB		(MBAR + 0x14C)	/* r */
#define UTB		(MBAR + 0x14C)	/* w */
#define UIPCR	(MBAR + 0x150)	/* r */
#define UACR	(MBAR + 0x150)	/* w */
#define UISR	(MBAR + 0x154)	/* r */
#define UIMR	(MBAR + 0x154)	/* w */
#define UBG1	(MBAR + 0x158)	/* r/w */
#define UBG2	(MBAR + 0x15C)	/* r/w */
#define UIVR	(MBAR + 0x170)	/* r/w */
#define UIP		(MBAR + 0x174) 	/* r */
#define UOP1	(MBAR + 0x178)	/* w */
#define UOP0	(MBAR + 0x17C)	/* w */

#define UART1	0x0		/* add to register to access UART 1 */
#define UART2	0x40	/* add to register to access UART 2 */

#define out(uart, reg, val)	(*(volatile uByte*)((uart) + (reg)) = (val))
#define in(uart, reg)		(*(volatile uByte*)((uart) + (reg)))


C(f_duart_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int uart;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_BAD_INSTANCE;

	/* initialize this serial port */
	uart = (Int)inst->package->self;
	out(uart, UCR, 0x10);	/* reset Mode register (UMR) */
	out(uart, UCR, 0x20);	/* reset receiver */
	out(uart, UCR, 0x30);	/* reset transmitter */
	out(uart, UCR, 0x40);	/* reset error status */
	out(uart, UCR, 0x50);	/* reset break */
	out(uart, UMR, 0x93);	/* UMR1: auto RxRTS, 8-bits, no parity */
	out(uart, UMR, 0x37);	/* UMR2: 1 stop bit, TxRTS, TxCTS */
	out(uart, UBG1, 0x00);	/* upper-8 bits of baud timer */
	out(uart, UBG2, 0x28);	/* lower-8 bits of baud timer */
	out(uart, UIMR, 0x00);	/* interrupt mask - all disabled */
	out(uart, UCSR, 0xDD);	/* use timer for baud rate */
	out(uart, UCR, 0x04);	/* enable transmitter */
	out(uart, UCR, 0x01);	/* enable receiver */
	out(uart, UOP1, 0x01);	/* raise RTS */

	/* generic tty values should be adequate to page output */
	e->lines = 24;
	e->cols = 80;

	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_duart_close)			/* close (--) */
{
	return NO_ERROR;
}

C(f_duart_selftest)		/* selftest (-- 0|error-code) */
{
	PUSH(e, 0);
	return NO_ERROR;
}

C(f_duart_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int uart;
	Int len;
	Int actual = 0;
	uByte *addr, i;
#ifdef DEBUG
	static uByte pi = 0;
#endif

	if (inst == NULL)
		return E_BAD_INSTANCE;

	uart = (Int)inst->package->self;
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, uByte*);

	while (len)
	{
		i = in(uart, USR);

#ifdef DEBUG
		if (pi != i)
		{
			DPRINTF(("duart_read: USR=%#x\n", i));
			pi = i;
		}
#endif

		if (i & 0xF0) 		/* error occurred? */
		{
			out(uart, UCR, 0x40);	/* reset error status */

			if (i & 0xE0)		/* not overrun error */
			{
				actual = -1;
				break;
			}
		}

		if (!(i & 0x01))	/* RXRDY? */
			break;

		*addr = in(uart, URB);
		DPRINTF(("duart_read: URB=%#x\n", *addr));
		addr++;
		actual++;
		len--;
	}

	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
}

C(f_duart_write)			/* write (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int uart;
	Int len;
	uByte *addr, b;
#ifdef DEBUG
	static uByte pb = 0;
#endif

	if (inst == NULL)
		return E_BAD_INSTANCE;

	uart = (Int)inst->package->self;
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, uByte*);
	PUSH(e, len);

	while (len)
	{
		b = in(uart, USR);

#ifdef DEBUG
		if (pb != b)
		{
			DPRINTF(("duart_write: USR=%#x\n", b));
			pb = b;
		}
#endif

		if (b & 0xF0) 		/* error occurred? */
			break;

		if (b & 0x04)		/* TXRDY? */
		{
			DPRINTF(("duart_write: UTB=%#x\n", *addr));
			out(uart, UTB, *addr);
			addr++;
			len--;
		}
	}

	return NO_ERROR;
}

C(f_duart_install_abort)		/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_duart_remove_abort)		/* remove-abort (--) */
{
	return NO_ERROR;
}

static const Initentry duart_methods[] =
{
	{ "open",          f_duart_open,          INVALID_FCODE },
	{ "close",         f_duart_close,         INVALID_FCODE },
	{ "selftest",      f_duart_selftest,      INVALID_FCODE },
	{ "read",          f_duart_read,          INVALID_FCODE },
	{ "write",         f_duart_write,         INVALID_FCODE },
	{ "install-abort", f_duart_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_duart_remove_abort,  INVALID_FCODE },
	{ NULL,            NULL },
};

Retcode
add_uart(Environ *e, Int uart)
{
	Package *pkg = new_pkg_name(e->root, "uart");
	Package *savepkg = e->currpkg;
	Int cells = get_address_cells(e->root);
	Int i;
	Retcode ret;

	DPRINTF(("duart_probe: pkg=%#x\n", pkg));

	if (pkg == NULL)
		return E_NULL_PACKAGE;

	ret = init_entries(e, pkg->dict, duart_methods);

	if (ret != NO_ERROR)
		return ret;

	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);

	/* create the "reg" property */
	PUSH(e, uart + 1);		/* phys.lo */

	for (i = 1; i < cells; i++)
		PUSH(e, 0);			/* ...phys.hi */

	PUSH(e, 0x40);			/* size */
	e->currpkg = pkg;
	ret = execute_word(e, "reg");
	e->currpkg = savepkg;

	/* save our uart offset in pkg->self for convenience */
	uart = uart ? UART2 : UART1;
	pkg->self = (struct pself*)(uPtr)uart;
	DPRINTF(("duart_probe: return %s (%d)\n", err2str(ret), ret));
	return ret;
}

CC(duart_probe)
{
	Retcode ret;

	ret = add_uart(e, 0);

	if (ret == NO_ERROR)
		ret = add_uart(e, 1);

	return ret;
}
