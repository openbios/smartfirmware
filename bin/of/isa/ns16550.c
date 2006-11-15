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

/* (c) Copyright 1996,2000 by CodeGen, Inc.  All Rights Reserved. */

/* builtin ISA NS16550 serial device driver */

/*#define DEBUG	*/

#include "defs.h"
#include "isa.h"

#define SIO_THR		0x0		/* transmit holding register - write-only */
#define SIO_RBR		0x0		/* receiver buffer register - read-only */
#define SIO_DLL		0x0		/* divisor latch LSB - r/w - DLAB == 1 */
#define SIO_DLM		0x1		/* divisor latch MSB - r/w 0 DLAB == 1 */
#define SIO_IER		0x1		/* interrupt enable register - w/o */
#define SIO_IIR		0x2		/* interrupt identification reg - r/o */
#define SIO_FCR		0x2		/* FIFO control register - w/o */
#define SIO_LCR		0x3		/* line control register - r/w */
#define SIO_MCR		0x4		/* modem control register - r/w */
#define SIO_LSR		0x5		/* line status register - r/w */
#define SIO_MSR		0x6		/* modem status register - r/w */
#define SIO_SCR		0x7		/* scratch pad register - r/w */

#define get_lcr(dev)	((uPtr)((dev)->self))
#define set_lcr(dev, val)	((dev)->self = (struct isa_self *)(uPtr)(val))

#define NS16550_WRITE(dev, reg, val) \
	((dev)->physhi ? isa_io_write : isa_mem_write)((dev)->physlo + (reg), \
		(val), 1)
#define NS16550_READ(dev, reg, valp) \
	((dev)->physhi ? isa_io_read : isa_mem_read)((dev)->physlo + (reg), \
		(valp), 1)

C(f_ns16550_set_mode)			/* set-mode (str len --) */
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
	Cell error;
	uByte newlcr;
	Int divisor;
	Isa_device *dev;

	IFCKSP(e, 2, 0);
	POP(e, len);
	POPT(e, str, Byte*);

	if (inst == NULL)
		return E_BAD_INSTANCE;

	if (inst->package->self == NULL)
		return E_BAD_PACKAGE;

	dev = (Isa_device*)inst->package->self;

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

	DPRINTF(("baud %d, rate %d\n", baud, dev->clkfreq));
	divisor = ((dev->clkfreq / baud) + 7) / 16;
	newlcr = (bits - 5) | ((stop != 1) << 2) | ((parity != 0) << 3) |
			((parity == 2) << 4);
	DPRINTF(("divisor %d, newlcr %#X\n", divisor, newlcr));

	DPRINTF(("ns16550_set_mode: %#X: wait for FIFO to drain\n", dev->physlo));

	/* wait for output FIFO to drain */
	while (1)
	{
		uByte b;
		NS16550_READ(dev, SIO_LSR, &b);

#if 0
		DPRINTF(("ns16550_set_mode: %#X: status %#X\n", dev->physlo, b));
#endif

		if (b & 0x40)		/* FIFO completely empy */
			break;
	}

	DPRINTF(("ns16550_set_mode: %#X: writing divisor, %#X (%d)\n",
		dev->physlo, divisor, divisor));

	NS16550_WRITE(dev, SIO_LCR, get_lcr(dev) | 0x80);	/* DLAB on */
	NS16550_WRITE(dev, SIO_DLL, divisor & 0xFF);		/* low byte */
	NS16550_WRITE(dev, SIO_DLM, (divisor >> 8) & 0xFF);	/* high byte */
	NS16550_WRITE(dev, SIO_LCR, get_lcr(dev));		/* DLAB off */
	NS16550_WRITE(dev, SIO_LCR, newlcr);			/* update format */

	set_lcr(dev, newlcr);
	DPRINTF(("ns16550_set_mode: %#X: writing new lcr, %#X\n", dev->physlo,
			newlcr));

	/* setup the 16550 */
	return NO_ERROR;
}

C(f_ns16550_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Isa_device *dev;
	Retcode ret;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL || inst->package->self == NULL)
		return E_BAD_PACKAGE;

	dev = (Isa_device*)inst->package->self;

	if (!dev->clkfreq)
		dev->clkfreq = 1843200;

	if (inst->args && inst->args[0])
	{
		/* set mode by init string */
		PUSHP(e, inst->args + 1);
		PUSH(e, inst->args[0]);
		DPRINTF(("args %#X, %d,\"%s\"\n", inst->args, inst->args[0],
				inst->args + 1));
	}
	else
	{
		PUSHP(e, "");
		PUSH(e, 0);
	}

	ret = f_ns16550_set_mode(e);

	if (ret != NO_ERROR)
	{
		PUSH(e, FFALSE);
		return NO_ERROR;
	}

	/* generic tty values should be adequate to page output */
	e->lines = 24;
	e->cols = 80;

	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_ns16550_close)			/* close (--) */
{
	return NO_ERROR;
}

C(f_ns16550_selftest)		/* selftest (-- 0|error-code) */
{
	PUSH(e, 0);
	return NO_ERROR;
}

C(f_ns16550_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Int actual = 0;
	Byte *addr, i;
	Isa_device *dev;

	if (inst == NULL)
		return E_BAD_INSTANCE;

	if (inst->package->self == NULL)
		return E_BAD_PACKAGE;

	dev = (Isa_device*)inst->package->self;
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	while (len)
	{
		NS16550_READ(dev, SIO_LSR, &i);

		if (i & 0x9E) 		/* error occurred? */
		{
			actual = -1;
			break;
		}

		if (!(i & 0x01))	/* char ready in RBR? */
			break;

		NS16550_READ(dev, SIO_RBR, addr);
		addr++;
		actual++;
		len--;
	}

	PUSH(e, actual == 0 ? -2 : actual);
	return NO_ERROR;
}

C(f_ns16550_write)			/* write (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len;
	Byte *addr, b;
	Isa_device *dev;

	if (inst == NULL)
		return E_BAD_INSTANCE;

	if (inst->package->self == NULL)
		return E_BAD_PACKAGE;

	dev = (Isa_device*)inst->package->self;
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	PUSH(e, len);

	while (len)
	{
		NS16550_READ(dev, SIO_LSR, &b);

		if (b & 0x20)	/* THR ready for a char? */
		{
			NS16550_WRITE(dev, SIO_THR, *addr++);
			len--;
		}
	}

	return NO_ERROR;
}

C(f_ns16550_install_abort)		/* install-abort (--) */
{
	return NO_ERROR;
}

C(f_ns16550_remove_abort)		/* remove-abort (--) */
{
	return NO_ERROR;
}

const Initentry ns16550_methods[] =
{
	{ "open",          f_ns16550_open,          INVALID_FCODE },
	{ "close",         f_ns16550_close,         INVALID_FCODE },
	{ "selftest",      f_ns16550_selftest,      INVALID_FCODE },
	{ "read",          f_ns16550_read,          INVALID_FCODE },
	{ "write",         f_ns16550_write,         INVALID_FCODE },
	{ "set-mode",      f_ns16550_set_mode,      INVALID_FCODE },
	{ "install-abort", f_ns16550_install_abort, INVALID_FCODE },
	{ "remove-abort",  f_ns16550_remove_abort,  INVALID_FCODE },
	{ NULL,            NULL },
};

ISA(ns16550_probe)
{
	Byte b = 0;

	NS16550_WRITE(dev, SIO_LCR, 0x03);	/* DLAB off, 8-1-N */
	NS16550_WRITE(dev, SIO_MCR, 0x01);	/* drop all - raise DTR */
	NS16550_WRITE(dev, SIO_IER, 0x00);	/* disable all interrupts */
	NS16550_WRITE(dev, SIO_FCR, 0x06);	/* turn on & clear FIFOs */

	NS16550_READ(dev, SIO_IIR, &b);

	if (b != 0x01)				/* better read this or else not serial */
		return E_NO_DEVICE;

	NS16550_WRITE(dev, SIO_FCR, 0x07);	/* turn on & clear FIFOs */
	return NO_ERROR;
}

ISA(ns16550_install)
{
	Retcode ret = new_isa_device(e, dev);
	Byte *prop;
	Int plen = 0;

	if (ret == NO_ERROR)
	{
		prop = prop_alloc(e, sizeof (Int));

		if (prop == NULL)
			return E_OUT_OF_MEMORY;

		plen = 0;
		prop_encode_int(prop + plen, &plen, dev->clkfreq);
		ret = add_property(dev->pkg->props, "clock-frequency", CSTR, prop, plen);
	}

	return ret;
}

Isa_device ns16550_com1 =
{
	"serial",
	"serial",
	ISA_IO_ADDRESS,
	0x3F8,
	8,
	{ 4, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0x0,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ns16550_probe,
	ns16550_install,
	ns16550_methods
};

Isa_device ns16550_com2 =
{
	"serial",
	"serial",
	ISA_IO_ADDRESS,
	0x2F8,
	8,
	{ 3, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0x0,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ns16550_probe,
	ns16550_install,
	ns16550_methods
};

Isa_device ns16550_com3 =
{
	"serial",
	"serial",
	ISA_IO_ADDRESS,
	0x3E8,
	8,
	{ 4, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0x0,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ns16550_probe,
	ns16550_install,
	ns16550_methods
};

Isa_device ns16550_com4 =
{
	"serial",
	"serial",
	ISA_IO_ADDRESS,
	0x2E8,
	8,
	{ 3, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0x0,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	ns16550_probe,
	ns16550_install,
	ns16550_methods
};
