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

/* Builtin RS-232 interface */

/*#define DEBUG	*/
#include "defs.h"
#include "ep7312.h"

/*									
												1			2
	control reg with enable					sysctl1		sysctl2		100
	enable bit in reg							8			8
	status reg with busy					sysflg1		sysflg2		140
	busy bit									11			11
	rx fifo empty								22			22
	tx fifo full								23			23
	interrupt status reg					intsr1		intsr2		240
	interrupt mask reg						intmr1		intmr2		280
	tx fifo half empty							12			12
	rx fifo half full							13			13
	modem status changed						14
*/

struct pself
{
	uInt phys;
	uInt virt;
};

typedef struct pself PSelf;

#define UART1_PHYS			0x80000000
#define UART2_PHYS			0x80001000
#define UART1_VIRT			(EDB7312_VA_EP7312 + 0x0000)
#define UART2_VIRT			(EDB7312_VA_EP7312 + 0x1000)

#define UART_CTL			0x100
#define UART_CTL_ENB		0x00000100
#define UART_STAT			0x140
#define UART_STAT_BUSY		0x00000800
#define UART_STAT_RXEMPTY	0x00400000
#define UART_STAT_TXFULL	0x00800000
#define UART_INTSTAT		0x240
#define UART_INTMASK		0x280
#define UART_INT_TXEMPTY	0x00001000
#define UART_INT_RXFULL		0x00002000
#define UART_INT_MDMCHG		0x00004000
#define UART_DATA			0x480
#define UART_DATA_MASK		0x000000FF
#define UART_DATA_FRMERR	0x00000100
#define UART_DATA_PARERR	0x00000200
#define UART_DATA_OVERR		0x00000400
#define UART_LINECTL		0x4C0
#define UART_LINECTL_BRD	0x00000FFF
#define UART_LINECTL_BREAK	0x00001000
#define UART_LINECTL_PRTEN	0x00002000
#define UART_LINECTL_EVEN	0x00004000
#define UART_LINECTL_XSTOP	0x00008000
#define UART_LINECTL_FIFOEN	0x00010000
#define UART_LINECTL_WRDLEN	0x00060000

static Int
uart_baudrate_divisor(int baud)
{
	Int rates[] = { 115200, 76800, 57600, 38400, 28800, 19200, 14400, 9600,
			4800, 2400, 1200, 110 };
	Int divisors[] = { 1, 2, 3, 5, 7, 11, 15, 23, 47, 95, 191, 2094 };
	int i;

	for (i = 0; i < sizeof rates / sizeof rates[0]; i++)
		if (rates[i] == baud)
			break;

	if (i >= sizeof rates / sizeof rates[0])
		return -1;

	return divisors[i];
}

static Int
uart_datawidth(int bits)
{
	switch (bits)
	{
	case 5:
		return 0x00000000;
	case 6:
		return 0x00020000;
	case 7:
		return 0x00040000;
	case 8:
		return 0x00060000;
	}

	return -1;
}

static uInt
uart_in(uInt addr)
{
	return *(uInt volatile *)addr;
}

static void
uart_out(uInt addr, uInt data)
{
	*(uInt volatile *)addr = data;
}

C(f_uart_set_mode)			/* set-mode (str len --) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Byte *str;
	Int len;
	Int baud = 9600;
	Int bits = 8;
	Int stop = 1;		/* 0 1.5bits, 1 1bit, 2 2bits */
	Int parity = 0;		/* 0 none, 1 odd, 2 even */
	Int handshake = 0;	/* 0 none, 1 RTS/CTS, 2 XON/XOFF */
	Cell val;
	Bool error;
	uInt linectl;
	uInt ctl;
	Int divisor;
	struct pself *pself;
	uInt base;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);

	if (inst == NULL || inst->package == NULL)
		return E_BAD_INSTANCE;

	if (inst->package->self == NULL)
		return E_BAD_PACKAGE;

	pself = inst->package->self;
	base = pself->virt;

	if (len)
	{
		DPRINTF(("baud = %d\n", baud));

		/* this leaves str at the end of the number with an error */
		/* if a comma is seen */
		parse_number(10, &str, &len, &val, &error, FALSE);

		if (!error || (*str == ',' && val > 0))
			baud = val;

		DPRINTF(("baud = %d, error %d, *str %#X, val %d\n", baud, error, *str, val));
		
		if (*str == ',')
		{
			/* get data bits */
			str++;

			if (*str == '5' || *str == '6' || *str == '7' || *str == '8')
				bits = *str++ - '0';

			if (*str == ',')
			{
				/* get stop bits */
				str++;

				if (*str == '1' || *str == '.' || *str == '2')
				{
					if (*str == '1')
						stop = 1;
					else if (*str == '.')
						stop = 0;
					else 
						stop = 2;

					str++;
				}
					
				if (*str == ',')
				{
					/* get the parity as a character */
					str++;

					if (*str == 'O' || *str == 'o' || *str == 'E' || *str == 'e' ||
							*str == 'M' || *str == 'm' || *str == 'S' || 
							*str == 's' || *str == 'N' || *str == 'n')
					{
						if (*str == 'O' || *str == 'o' || *str == 'M' ||
								*str == 'm')
							parity = 1;
						else if (*str == 'E' || *str == 'e' || *str == 'S' ||
								*str == 's')
							parity = 2;
						else
							parity = 0;

						str++;
					}

					if (*str == ',')
					{
						/* get the handshake as a character */
						str++;

						if (*str == '-' || *str == 'H' || *str == 'h' ||
								*str == 'S' || *str == 's')
						{
							if (*str == '-')
								handshake = 0;
							else if (*str == 'H' || *str == 'h')
								handshake = 1;
							else
								handshake = 2;

							str++;
						}
					}
				}
			}
		}
	}

	DPRINTF(("baud %d\n", baud));
	divisor = uart_baudrate_divisor(baud);

	if (divisor < 0)
	{
		cprintf(e, "uart: baud rate %d not support, defaulting to 9600\n", baud);
		divisor = uart_baudrate_divisor(9600);
	}

	linectl = divisor;

	if (parity)
	{
		linectl |= UART_LINECTL_PRTEN;

		if (parity == 2)
			linectl |= UART_LINECTL_EVEN;
	}

	if (stop != 1)
			linectl |= UART_LINECTL_XSTOP;

	linectl |= uart_datawidth(bits);
	linectl |= UART_LINECTL_FIFOEN;
	DPRINTF(("divisor %d, bits %d, parity %d, stop %d, linectl %#x\n", divisor,
			bits, parity, stop, linectl));

	DPRINTF(("uart_set_mode: %#X: wait for FIFO to drain\n", base));

	ctl = uart_in(base + UART_CTL);

	if (ctl & UART_CTL_ENB)
	{
		/* wait for output FIFO to drain */
		while (uart_in(base + UART_STAT) & UART_STAT_BUSY)
			;
	}

	uart_out(base + UART_LINECTL, linectl);
	ctl = uart_in(base + UART_INTMASK);
	uart_out(base + UART_INTMASK, ctl & ~(UART_INT_RXFULL|UART_INT_TXEMPTY));
	ctl = uart_in(base + UART_CTL);
	uart_out(base + UART_CTL, ctl | UART_CTL_ENB);

	/* setup the UART */
	DPRINTF(("uart_set_mode: done.\n"));
	return NO_ERROR;
}

C(f_uart_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	PSelf *s;
	uInt ctl;
	uInt base;
	Retcode ret;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->package->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	if (inst->args && inst->args[0])
	{
		/* set mode by init string */
		PUSHP(e, inst->args + 1);
		PUSH(e, inst->args[0]);
		DPRINTF(("args %#X, %d,\"%s\"\n", inst->args, inst->args[0],
				inst->args + 1));
		ret = f_uart_set_mode(e);

		if (ret != NO_ERROR)
		{
			PUSH(e, FFALSE);
			return NO_ERROR;
		}
	}
	else
	{
		base = s->virt;
		DPRINTF(("uart_open: base=%#x\n", base));

		/* 9600 baud, 8bits, no parity, fifo enabled */
		ctl = uart_in(base + UART_CTL);

		if ((ctl & UART_CTL_ENB) == 0)
		{
			uart_out(base + UART_LINECTL,
					UART_LINECTL_WRDLEN|UART_LINECTL_FIFOEN|0x0017);
			ctl = uart_in(base + UART_INTMASK);
			uart_out(base + UART_INTMASK,
					ctl & ~(UART_INT_RXFULL|UART_INT_TXEMPTY));
			ctl = uart_in(base + UART_CTL);
			uart_out(base + UART_CTL, ctl | UART_CTL_ENB);
		}
	}

	PUSH(e, FTRUE);
	e->cols = 80;
	e->lines = 24;
	e->linesperpage = 0;		/* paging off by default */
	return NO_ERROR;
}

C(f_uart_close)			/* close (--) */
{
	return NO_ERROR;
}

C(f_uart_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Int actual = 0;
	Byte *addr;
	PSelf *s;
	uInt base;

	DPRINTF(("enter uart_read\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->package->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	base = s->virt;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("uart_read: &e=%p e=%p addr=%p len=%d\n", &e, e, addr, len));

	while (actual < len && (uart_in(base + UART_STAT) & UART_STAT_RXEMPTY) == 0)
		addr[actual++] = uart_in(base + UART_DATA) & UART_DATA_MASK;

	PUSH(e, actual == 0 ? -2 : actual);

	if (actual)
	{
		DPRINTF(("uart_read: addr=%p \"%s\" actual=%d\n", addr,
				addr ? addr : "", actual));
	}

	return NO_ERROR;
}

C(f_uart_write)			/* write (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Int actual = 0;
	Byte *addr;
	PSelf *s;
	uInt base;
#ifdef DEBUG
	Byte buf[256];
	int i;
#endif

	DPRINTF(("enter uart_write\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	s = inst->package->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;

	base = s->virt;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
#ifdef DEBUG
	DPRINTF(("uart_write: &e=%p e=%p addr=%p len=%d\n", &e, e, addr, len));

	for (i = 0; i < 255 && i < len; i++)
		if (addr[i] < ' ' || addr[i] > '~')
			buf[i] = '?';
		else
			buf[i] = addr[i];

	buf[i] = '\0';
	DPRINTF(("uart_write: buf='%s'\n", buf));
#endif

	for (actual = 0; actual < len; actual++)
	{
		while (uart_in(base + UART_STAT) & UART_STAT_TXFULL)
			;

		uart_out(base + UART_DATA, addr[actual] & UART_DATA_MASK);
	}

	while (uart_in(base + UART_STAT) & UART_STAT_BUSY)
		;

	PUSH(e, actual);
	DPRINTF(("uart_write: actual=%d\n", actual));
	return NO_ERROR;
}

C(f_uart_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "uart selftest\n");

	PUSH(e, 0);
	return NO_ERROR;
}

C(f_uart_install_abort)		/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_uart_remove_abort)		/* remove-abort (--) */
{
	return NO_ERROR;
}

static const Initentry uart_methods[] =
{
	{ "open",          f_uart_open,          INVALID_FCODE },
	{ "close",         f_uart_close,         INVALID_FCODE },
	{ "selftest",      f_uart_selftest,      INVALID_FCODE },
	{ "read",          f_uart_read,          INVALID_FCODE },
	{ "write",         f_uart_write,         INVALID_FCODE },
	{ "install-abort", f_uart_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_uart_remove_abort,  INVALID_FCODE },
	{ "set-mode",      f_uart_set_mode,      INVALID_FCODE },
	{ NULL,            NULL },
};

Retcode
uart_makedev(Environ *e, uInt virt, uInt phys)
{
	Retcode ret;
	Package *pkg;
	Byte *prop;
	Int plen = 0;

	pkg = new_pkg_name(e->currpkg, "uart");

	if (pkg == NULL)
		return E_OUT_OF_MEMORY;

	pkg->self = (PSelf *)malloc(sizeof (PSelf));
	
	if (pkg->self == NULL)
		return E_OUT_OF_MEMORY;

	memset(pkg->self, 0, sizeof (PSelf));
	pkg->self->virt = virt;
	pkg->self->phys = phys;
	prop_set_str(pkg->props, "device_type", CSTR, "serial", CSTR);
	DPRINTF(("uart_install: pkg=%p self=%p\n", pkg, pkg->self));

	/* encode "reg" property for unit address */
	prop = prop_alloc(e, 2 * sizeof (Int));

	if (prop == NULL)
		return E_OUT_OF_MEMORY;

	prop_encode_int(prop + plen, &plen, phys + UART_DATA);
	prop_encode_int(prop + plen, &plen, 0x80);
	ret = add_property(pkg->props, "reg", CSTR, prop, plen);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, uart_methods);

	return ret;
}

CC(uart_install)
{
	Retcode ret;

	ret = uart_makedev(e, UART1_VIRT, UART1_PHYS);

	if (ret != NO_ERROR)
		return ret;

	return uart_makedev(e, UART2_VIRT, UART2_PHYS);
}
