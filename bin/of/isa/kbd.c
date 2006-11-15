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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

/* Builtin ISA keyboard interface - much cribbed-n-hacked from BSD pccons.c.
   Original copyright follows.
 */

/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Copyright (c) 1994 Charles Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "defs.h"
#include "isa.h"

/* traditional ISA ports to access keyboard */
#define KBD_CMD		0x64
#define KBD_DATA	0x60

#define CMD_OBUF_FULL	0x01
#define CMD_IBUF_FULL	0x02

#define KBD_READ_CMD	0x20
#define KBD_LOAD_CMD	0x60

#define KBC_DISABLE_AUX	0xA7
#define KBC_ENABLE_AUX	0xA8
#define KBC_SELF_TEST	0xAA
#define KBC_TEST_CLKDAT	0xAB
#define KBC_DISABLE_KBD	0xAD
#define KBC_ENABLE_KBD	0xAE
#define KBC_A20_GATE	0xD1
#define KBC_SYS_RESET	0xFE

#define KB_RESET		0xFF
#define KB_RESEND		0xFE
#define	KB_ACK			0xFA
#define KB_ENABLE		0xF4
#define KB_SET_TABLE	0xF0
#define	KB_ECHO			0xEE
#define KB_SET_LEDS		0xED
#define KB_EXTENDED		0xE0

#define	KB_SCROLL	0x0001	/* stop output */
#define	KB_NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	KB_CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	KB_SHIFT	0x0008	/* keyboard shift */
#define	KB_CTL		0x0010	/* control shift  -- allows ctl function */
#define	KB_ASCII	0x0020	/* ascii code for this key */
#define	KB_ALTGR	0x0040	/* alternate graphics shift */
#define	KB_ALT		0x0080	/* alternate shift -- alternate chars */
#define	KB_FUNC		0x0100	/* function key */
#define	KB_KP		0x0200	/* Keypad keys */
#define	KB_NONE		0x0400	/* no function */

#define	KB_CODE_SIZE	4	/* Use a max of 4 for now... */
#define KB_NUM_KEYS	128	/* Number of scan codes */

struct isa_self
{
	uInt lock;
	uInt shift;
	Bool extended;
};
typedef struct isa_self Isa_self;

typedef struct
{
	uShort type;
	Byte unshift;
	Byte shift;
	Byte ctl;
} keymap;

static keymap scan_codes[] =
{
/*	  type       unshift   shift     ctl            scancode */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /*  0 unused */
	{ KB_ASCII,  '\033',   '\033',   '\033' },   /*  1 ESCape */
	{ KB_ASCII,  '1',      '!',      '!'    },   /*  2 1 */
	{ KB_ASCII,  '2',      '@',      '\000' },   /*  3 2 */
	{ KB_ASCII,  '3',      '#',      '#'    },   /*  4 3 */
	{ KB_ASCII,  '4',      '$',      '$'    },   /*  5 4 */
	{ KB_ASCII,  '5',      '%',      '%'    },   /*  6 5 */
	{ KB_ASCII,  '6',      '^',      '\036' },   /*  7 6 */
	{ KB_ASCII,  '7',      '&',      '&'    },   /*  8 7 */
	{ KB_ASCII,  '8',      '*',      '\010' },   /*  9 8 */
	{ KB_ASCII,  '9',      '(',      '('    },   /* 10 9 */
	{ KB_ASCII,  '0',      ')',      ')'    },   /* 11 0 */
	{ KB_ASCII,  '-',      '_',      '\037' },   /* 12 - */
	{ KB_ASCII,  '=',      '+',      '+'    },   /* 13 = */
	{ KB_ASCII,  '\177',   '\177',   '\010' },   /* 14 backspace */
	{ KB_ASCII,  '\t',     '\t',     '\t'   },   /* 15 tab */
	{ KB_ASCII,  'q',      'Q',      '\021' },   /* 16 q */
	{ KB_ASCII,  'w',      'W',      '\027' },   /* 17 w */
	{ KB_ASCII,  'e',      'E',      '\005' },   /* 18 e */
	{ KB_ASCII,  'r',      'R',      '\022' },   /* 19 r */
	{ KB_ASCII,  't',      'T',      '\024' },   /* 20 t */
	{ KB_ASCII,  'y',      'Y',      '\031' },   /* 21 y */
	{ KB_ASCII,  'u',      'U',      '\025' },   /* 22 u */
	{ KB_ASCII,  'i',      'I',      '\011' },   /* 23 i */
	{ KB_ASCII,  'o',      'O',      '\017' },   /* 24 o */
	{ KB_ASCII,  'p',      'P',      '\020' },   /* 25 p */
	{ KB_ASCII,  '[',      '{',      '\033' },   /* 26 [ */
	{ KB_ASCII,  ']',      '}',      '\035' },   /* 27 ] */
	{ KB_ASCII,  '\r',     '\r',     '\n'   },   /* 28 return */
	{ KB_CTL,    '\0',     '\0',     '\0'   },   /* 29 control */
	{ KB_ASCII,  'a',      'A',      '\001' },   /* 30 a */
	{ KB_ASCII,  's',      'S',      '\023' },   /* 31 s */
	{ KB_ASCII,  'd',      'D',      '\004' },   /* 32 d */
	{ KB_ASCII,  'f',      'F',      '\006' },   /* 33 f */
	{ KB_ASCII,  'g',      'G',      '\007' },   /* 34 g */
	{ KB_ASCII,  'h',      'H',      '\010' },   /* 35 h */
	{ KB_ASCII,  'j',      'J',      '\n'   },   /* 36 j */
	{ KB_ASCII,  'k',      'K',      '\013' },   /* 37 k */
	{ KB_ASCII,  'l',      'L',      '\014' },   /* 38 l */
	{ KB_ASCII,  ';',      ':',      ';'    },   /* 39 ; */
	{ KB_ASCII,  '\'',     '"',      '\''   },   /* 40 ' */
	{ KB_ASCII,  '`',      '~',      '`'    },   /* 41 ` */
	{ KB_SHIFT,  '\001',   '\0',     '\0'   },   /* 42 shift */
	{ KB_ASCII,  '\\',     '|',      '\034' },   /* 43 \ */
	{ KB_ASCII,  'z',      'Z',      '\032' },   /* 44 z */
	{ KB_ASCII,  'x',      'X',      '\030' },   /* 45 x */
	{ KB_ASCII,  'c',      'C',      '\003' },   /* 46 c */
	{ KB_ASCII,  'v',      'V',      '\026' },   /* 47 v */
	{ KB_ASCII,  'b',      'B',      '\002' },   /* 48 b */
	{ KB_ASCII,  'n',      'N',      '\016' },   /* 49 n */
	{ KB_ASCII,  'm',      'M',      '\r'   },   /* 50 m */
	{ KB_ASCII,  ',',      '<',      '<'    },   /* 51 , */
	{ KB_ASCII,  '.',      '>',      '>'    },   /* 52 . */
	{ KB_ASCII,  '/',      '?',      '\037' },   /* 53 / */
	{ KB_SHIFT,  '\002',   '\0',     '\0'   },   /* 54 shift */
	{ KB_KP,     '*',      '*',      '*'    },   /* 55 kp * */
	{ KB_ALT,    '\0',     '\0',     '\0'   },   /* 56 alt */
	{ KB_ASCII,  ' ',      ' ',      '\000' },   /* 57 space */
	{ KB_CAPS,   '\0',     '\0',     '\0'   },   /* 58 caps */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 59 f1 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 60 f2 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 61 f3 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 62 f4 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 63 f5 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 64 f6 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 65 f7 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 66 f8 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 67 f9 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 68 f10 */
	{ KB_NUM,    '\0',     '\0',     '\0'   },   /* 69 num lock */
	{ KB_SCROLL, '\0',     '\0',     '\0'   },   /* 70 scroll lock */
	{ KB_KP,     '7',      '\0',     '7'    },   /* 71 kp 7 */
	{ KB_KP,     '8',      '\020',   '8'    },   /* 72 kp 8 */
	{ KB_KP,     '9',      '\0',     '9'    },   /* 73 kp 9 */
	{ KB_KP,     '-',      '-',      '-'    },   /* 74 kp - */
	{ KB_KP,     '4',      '\002',   '4'    },   /* 75 kp 4 */
	{ KB_KP,     '5',      '\0',     '5'    },   /* 76 kp 5 */
	{ KB_KP,     '6',      '\006',   '6'    },   /* 77 kp 6 */
	{ KB_KP,     '+',      '+',      '+'    },   /* 78 kp + */
	{ KB_KP,     '1',      '\0',     '1'    },   /* 79 kp 1 */
	{ KB_KP,     '2',      '\016',   '2'    },   /* 80 kp 2 */
	{ KB_KP,     '3',      '\0',     '3'    },   /* 81 kp 3 */
	{ KB_KP,     '0',      '\0',     '0'    },   /* 82 kp 0 */
	{ KB_KP,     ',',      '\177',   ','    },   /* 83 kp , */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 84 0 */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 85 0 */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 86 0 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 87 f11 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 88 f12 */
};

#define NUM_SCAN_CODES	(sizeof scan_codes / sizeof scan_codes[0])


/* I/O routines to talk to ISA ports */
static uByte
kbd_in(int port)
{
	uByte val = 0;

	isa_io_read(port, &val, 1);
	u_sleep(10);
	DPRINTF(("kbd_in: port=%#x val=%#x\n", port, (uInt)val));
	return val;
}

static void
kbd_out(int port, uByte val)
{
	DPRINTF(("kbd_out: port=%#x val=%#x\n", port, (uInt)val));
	isa_io_write(port, val, 1);
	u_sleep(10);
}

/* loops to wait for I/O to finish */
static void
kbd_wait_kbd()
{
	Int i;
	
	DPRINTF(("kbd_wait_kbd\n"));

	for (i = 0; i < 100000; i++)
		if (!(kbd_in(KBD_CMD) & CMD_IBUF_FULL))
			break;
}

static void
kbd_wait_sys()
{
	Int i;
	
	DPRINTF(("kbd_wait_sys\n"));

	for (i = 0; i < 100000; i++)
		if (kbd_in(KBD_CMD) & CMD_OBUF_FULL)
			break;
}

static void
kbd_flush()
{
	uByte c;
	
	DPRINTF(("kbd_flush\n"));

	while ((c = kbd_in(KBD_CMD)) & (CMD_OBUF_FULL | CMD_IBUF_FULL))
		if (c & CMD_OBUF_FULL)
			c = kbd_in(KBD_DATA);
}

/* talk to 8042 controlling keyboard */
/***
static uByte
kbc_get_cmd()
{
	DPRINTF(("kbd_get_cmd\n"));
	kbd_wait_kbd();
	kbd_out(KBD_CMD, KBD_READ_CMD);
	kbd_wait_sys();
	return kbd_in(KBD_DATA);
}
***/

static void
kbc_put_cmd(uByte cmd)
{
	DPRINTF(("kbd_put_cmd: cmd=%#x\n", cmd));
	kbd_wait_kbd();
	kbd_out(KBD_CMD, KBD_LOAD_CMD);
	kbd_wait_kbd();
	kbd_out(KBD_DATA, cmd);
}

/* talk to keyboard itself */
static void
kbd_cmd(uByte cmd)
{
	int i, c;

	DPRINTF(("kbd_cmd: cmd=%#x\n", cmd));

again:
	kbd_wait_kbd();
	kbd_out(KBD_DATA, cmd);
	
	for (i = 0; i < 100000; i++)
	{
		if (kbd_in(KBD_CMD) & CMD_OBUF_FULL)
		{
			c = kbd_in(KBD_DATA);
			DPRINTF(("kbd_cmd: c=%#x\n", c));
		
			if (c == KB_ACK || c == KB_ECHO)
				return;
			
			if (c == KB_RESEND)
				goto again;
		}
	}
}

static void
update_leds(uInt lock)
{
	DPRINTF(("update_leds: locl=%#x\n", lock));
	kbd_out(KBD_CMD, KBC_DISABLE_KBD);
	kbd_cmd(KB_SET_LEDS);
	kbd_cmd(lock);
	kbd_cmd(KB_ENABLE);
	kbd_out(KBD_CMD, KBC_ENABLE_KBD);
}


C(f_kbd_open)			/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Isa_device *dev;
	Isa_self *s;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)inst->package->self;
	
	if (dev == NULL)
		return E_BAD_INSTANCE;
	
	s = dev->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;
		
	DPRINTF(("kbd_open\n"));
#if 0
	kbd_out(KBD_CMD, KBC_A20_GATE);
	kbd_wait_kbd();
	kbd_out(KBD_DATA, 0x03);	/* enable A20 */
	kbd_wait_kbd();
#endif

	kbd_out(KBD_CMD, KBC_DISABLE_KBD);
	kbc_put_cmd(0x44);	/* no interrupts, reset, translate scan codes */
	kbd_flush();
	kbd_cmd(KB_RESET);
	kbd_wait_sys();
	kbd_flush();
	kbd_cmd(KB_ENABLE);
	kbd_cmd(KB_SET_TABLE);
	kbd_cmd(2);			/* AT codes */
	kbd_out(KBD_CMD, KBC_ENABLE_KBD);

	/* clear the state of keys */	
	memset(s, 0, sizeof (*s));
	PUSH(e, FTRUE);
	return NO_ERROR;
}

C(f_kbd_close)			/* close (--) */
{
	return NO_ERROR;
}

C(f_kbd_read)			/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int len, c;
	Int actual = 0;
	Byte *addr, ch;
	Isa_device *dev;
	Isa_self *s;

	DPRINTF(("enter kbd_read\n"));

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)inst->package->self;
	
	if (dev == NULL)
		return E_BAD_INSTANCE;

	s = dev->self;
	
	if (s == NULL)
		return E_BAD_INSTANCE;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	DPRINTF(("kbd_read: &e=%p e=%p addr=%p len=%d\n", &e, e, addr, len));

	/* got something? */
	while (actual < len && (kbd_in(KBD_CMD) & CMD_OBUF_FULL))
	{
		c = kbd_in(KBD_DATA);
		DPRINTF(("kbd_read: c=%#x shift=%#x s->lock=%#x\n",
				c, s->shift, s->lock));

		if (c == KB_EXTENDED)
		{
			s->extended = TRUE;
			continue;
		}
		
		if (c & 0x80)
		{
			switch (scan_codes[c & 0x7F].type)
			{
			case KB_NUM:
				s->shift &= ~KB_NUM;
				break;
				
			case KB_CAPS:
				s->shift &= ~KB_CAPS;
				break;
				
			case KB_SCROLL:
				s->shift &= ~KB_SCROLL;
				break;
				
			case KB_SHIFT:
				s->shift &= ~KB_SHIFT;
				break;
				
			case KB_ALT:
				if (s->extended)
					s->shift &= ~KB_ALTGR;
				else
					s->shift &= ~KB_ALT;
					
				break;
				
			case KB_CTL:
				s->shift &= ~KB_CTL;
				break;
			}
		}
		else
		{
			if (c >= NUM_SCAN_CODES)
			{
				cprintf(e, "keyboard: keycode %d\n", c);
				continue;
			}
			
			switch (scan_codes[c].type)
			{
			/*
			 * locking keys
			 */
			case KB_NUM:
				if (s->shift & KB_NUM)
					break;
					
				s->shift |= KB_NUM;
				s->lock ^= KB_NUM;
				update_leds(s->lock);
				break;
				
			case KB_CAPS:
				if (s->shift & KB_CAPS)
					break;
					
				s->shift |= KB_CAPS;
				s->lock ^= KB_CAPS;
				update_leds(s->lock);
				break;
				
			case KB_SCROLL:
				if (s->shift & KB_SCROLL)
					break;
					
				s->shift |= KB_SCROLL;
				s->lock ^= KB_SCROLL;
				update_leds(s->lock);
				break;
			/*
			 * non-locking keys
			 */
			case KB_SHIFT:
				s->shift |= KB_SHIFT;
				break;
				
			case KB_ALT:
				if (s->extended)
					s->shift |= KB_ALTGR;
				else
					s->shift |= KB_ALT;
					
				break;
				
			case KB_CTL:
				s->shift |= KB_CTL;
				break;
				
			case KB_ASCII:
				/* control has highest priority */
				if (s->shift & KB_CTL)
					ch = scan_codes[c].ctl;
				else if (s->shift & KB_SHIFT)
					ch = scan_codes[c].shift;
				else
					ch = scan_codes[c].unshift;
				
				if ((s->lock & KB_CAPS) && ch >= 'a' &&
						ch <= 'z')
					ch -= ('a' - 'A');
				
				ch |= (s->shift & KB_ALT);
				s->extended = 0;
				addr[actual++] = ch;
				break;
			
			case KB_KP:
				if ((s->shift & (KB_CTL | KB_SHIFT)) ||
						!(s->lock & KB_NUM) || s->extended)
					ch = scan_codes[c].shift;
				else
					ch = scan_codes[c].unshift;

				ch |= (s->shift & KB_ALT);
				s->extended = 0;
				addr[actual++] = ch;
				break;
								
			case KB_FUNC:
			case KB_NONE:
			default:
				cprintf(e, "keyboard: keycode %d\n", c);
				break;
			}
		}
	}

	addr[actual] = '\0';
	PUSH(e, actual == 0 ? -2 : actual);

#ifdef DEBUG
	if (actual)
		dprintf(
"kbd_read: addr=%p \"%s\" actual=%d lock=%#x shift=%#x extended=%d\n",
				addr, addr ? addr : "", actual, s->lock, s->shift, s->extended);
#endif

	return NO_ERROR;
}

C(f_kbd_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "keyboard selftest\n");

	PUSH(e, 0);
	return NO_ERROR;
}

static const Initentry kbd_methods[] =
{
	{ "open",          f_kbd_open,          INVALID_FCODE },
	{ "close",         f_kbd_close,         INVALID_FCODE },
	{ "selftest",      f_kbd_selftest,      INVALID_FCODE },
	{ "read",          f_kbd_read,          INVALID_FCODE },
	{ NULL,            NULL },
};


ISA(kbd_probe)
{
	/* TODO */
	return NO_ERROR;
}

ISA(kbd_install)
{
	dev->self = (Isa_self*)malloc(sizeof (Isa_self));
	
	if (dev->self == NULL)
		return E_OUT_OF_MEMORY;
	
	DPRINTF(("kbd_install: dev=%p self=%p\n", dev, dev->self));
	memset(dev->self, 0, sizeof (Isa_self));
	return new_isa_device(e, dev);
}

Isa_device isa_keyboard =
{
	"keyboard",
	"keyboard",
	ISA_IO_ADDRESS,
	0x60,
	5,
	{ 1, 0 },	/* IRQ */
	{ -1, },	/* DMA */
	0x0,		/* BIOS */
	0, NULL, 	/* no extra reg props */
	0,			/* clock freq, N/A here */
	kbd_probe,
	kbd_install,
	kbd_methods
};
