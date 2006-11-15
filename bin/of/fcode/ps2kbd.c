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

/* (c) Copyright 1999 by CodeGen, Inc.  All Rights Reserved. */

/* generic on-board Fcode driver for PS/2-style keyboards
   - include into master file before compiling with cc-fcode, wrapped
   by pragma new_device and finish_device.
   - the macros KBD_DATA and KBD_CMD should be defined beforehand to the
   actual port numbers, and the routines kbd_in() and kbd_out() must also
   be defined to access those port numbers.
 */

#include "fcode.h"

//#define DEBUG

#ifndef KBD_CMD
	/* traditional ISA ports to access keyboard */
#	define KBD_DATA		0x60
#	define KBD_CMD		0x64

/* sample I/O routines to talk to ISA ports */
static unsigned char
kbd_in(int port)
{
	return isa_io_read(port, 1);
}

static void
kbd_out(int port, unsigned char val)
{
	isa_io_write(port, val, 1);
}
#endif	/* KBD_CMD */

#define KBD_TIMEOUT		10000

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
#define KB_TEST_FAIL	0xFD
#define KB_RESET_FAIL	0xFC
#define	KB_ACK			0xFA
#define KB_ENABLE		0xF4
#define KB_SET_TABLE	0xF0
#define	KB_ECHO			0xEE
#define KB_SET_LEDS		0xED
#define KB_EXTENDED		0xE0
#define KB_TEST_OK		0x55

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

static unsigned int g_lock;
static unsigned int g_shift;
static Bool g_extended;

typedef struct keymap
{
	unsigned short type;
	unsigned char unshift;
	unsigned char shift;
	unsigned char ctl;
} Keymap;


/* this used to be a big static array, but it blows the Forth stack
   so it is now broken up into several pieces and accessed by a function
   - each sub-table must be 10 elements each
 */

#define SCAN_CHUNK (sizeof scan_codes_0 / sizeof scan_codes_0[0])

static Keymap scan_codes_0[] =
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
};

static Keymap scan_codes_1[] =
{
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
};

static Keymap scan_codes_2[] =
{
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
};

static Keymap scan_codes_3[] =
{
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
};

static Keymap scan_codes_4[] =
{
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
};

static Keymap scan_codes_5[] =
{
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
};

static Keymap scan_codes_6[] =
{
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
};

static Keymap scan_codes_7[] =
{
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
};

static Keymap scan_codes_8[] =
{
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

static Keymap *scan_tab[] =
{
	scan_codes_0,
	scan_codes_1,
	scan_codes_2,
	scan_codes_3,
	scan_codes_4,
	scan_codes_5,
	scan_codes_6,
	scan_codes_7,
	scan_codes_8,
};

Keymap
scan_codes(int k)
{
	if (k < 0 || k > 88)	/* sanity check */
		k = 0;

#ifdef DEBUG
	{
		Keymap *t;
		t = scan_tab[k / SCAN_CHUNK];
		type("\$scan_codes: k=");
		udot(k);
		type("\$tab=");
		udot((int)t);
		return t[k % SCAN_CHUNK];
	}
#else
	return scan_tab[k / SCAN_CHUNK][k % SCAN_CHUNK];
#endif
};


/* loops to wait for I/O to finish - returns TRUE if OK, FALSE on error */
static Bool
kbd_wait_kbd()
{
	int i = 0;
	
	while (i < KBD_TIMEOUT && (kbd_in(KBD_CMD) & CMD_IBUF_FULL))
	{
		ms(10);
		i++;
	}
	
	return (i < KBD_TIMEOUT) ? TRUE : FALSE;
}

static Bool
kbd_wait_sys()
{
	int i = 0;
	
	while (i < KBD_TIMEOUT && !(kbd_in(KBD_CMD) & CMD_OBUF_FULL))
	{
		ms(10);
		i++;
	}

	return (i < KBD_TIMEOUT) ? TRUE : FALSE;
}

static Bool
kbd_flush()
{
	unsigned char c;
	int i = 0;
	
	while (i < KBD_TIMEOUT &&
			(c = kbd_in(KBD_CMD)) & (CMD_OBUF_FULL | CMD_IBUF_FULL))
	{
		if (c & CMD_OBUF_FULL)
		{
			ms(10);
			c = kbd_in(KBD_DATA);
		}

		ms(10);
		i++;
	}

	return (i < KBD_TIMEOUT) ? TRUE : FALSE;
}

/* talk to 8042 controlling keyboard */
/***
static unsigned char
kbc_get_cmd()
{
	kbd_wait_kbd();
	kbd_out(KBD_CMD, KBD_READ_CMD);
	kbd_wait_sys();
	return kbd_in(KBD_DATA);
}
***/

static Bool
kbc_put_cmd(unsigned char cmd)
{
	Bool r;

	r = kbd_wait_kbd();

	if (r)
	{
		kbd_out(KBD_CMD, KBD_LOAD_CMD);

		if (r)
		{
			r = kbd_wait_kbd();
			kbd_out(KBD_DATA, cmd);
		}
	}

	return r;
}

/* talk to keyboard itself - return TRUE if successful, FALSE on any error */
static Bool
kbd_cmd(unsigned char cmd)
{
	int i, c;
	Bool r;
	Bool cont = TRUE;

	r = kbd_wait_kbd();

	if (r)
	{
		kbd_out(KBD_DATA, cmd);
	
		for (i = 0; cont && i < KBD_TIMEOUT; i++)
		{
			ms(10);
			c = kbd_in(KBD_CMD);

			if (c & CMD_OBUF_FULL)
			{
				ms(10);
				c = kbd_in(KBD_DATA);
			
				if (c == KB_ACK || c == KB_ECHO)
				{
					cont = FALSE;		// break
				}
				else if (c == KB_RESEND)
				{
					r = kbd_wait_kbd();
					ms(10);

					if (r)
						kbd_out(KBD_DATA, cmd);
					else
						cont = FALSE;
				}
			}
		}
	}

	return (r && i < KBD_TIMEOUT) ? TRUE : FALSE;
}

static void
update_leds(unsigned int lock)
{
	kbd_out(KBD_CMD, KBC_DISABLE_KBD);
	(void)kbd_cmd(KB_SET_LEDS);
	(void)kbd_cmd(lock);
	(void)kbd_cmd(KB_ENABLE);
	kbd_out(KBD_CMD, KBC_ENABLE_KBD);
}


/* keyboard methods */

Bool
open(void)			/* open (-- okay?) */
{
	Bool r;

#if 0
	kbd_out(KBD_CMD, KBC_A20_GATE);
	kbd_wait_kbd();
	kbd_out(KBD_DATA, 0x02);	/* enable A20 */
	kbd_wait_kbd();
#endif

	kbd_out(KBD_CMD, KBC_DISABLE_KBD);
	r = kbc_put_cmd(0x44);	/* no interrupts, reset, translate scan codes */

	if (r)
		r = kbd_flush();

	if (r)
		r = kbd_cmd(KB_RESET);

	if (r)
		r = kbd_wait_sys();

	if (r)
		r = kbd_flush();

	if (r)
		r = kbd_cmd(KB_ENABLE);

	if (r)
		r = kbd_cmd(KB_SET_TABLE);

	if (r)
		r = kbd_cmd(2);			/* AT codes */

	kbd_out(KBD_CMD, KBC_ENABLE_KBD);

	g_lock = 0;
	g_shift = 0;
	g_extended = FALSE;
	return r;
}

void
close(void)			/* close (--) */
{
}

int
read(char *addr, int len)	/* read (addr len -- actual) */
{
	int c;
	int actual = 0;
	char ch;
	Bool loop = TRUE;
	Keymap k;

	/* got something? */
	while (loop && actual < len && (kbd_in(KBD_CMD) & CMD_OBUF_FULL))
	{
		c = kbd_in(KBD_DATA);

	#ifdef DEBUG
		type("\$kbd_read: char=");
		udot(c);
		cr();
	#endif

		if (c == KB_EXTENDED)
		{
			g_extended = TRUE;
		}
		else
		{
			k = scan_codes(c & 0x7F);

		#ifdef DEBUG
			type("\$kbd_read: k.unshift=");
			udot(k.unshift);
			cr();
		#endif

			if (c & 0x80)
			{
				switch (k.type)
				{
				case KB_NUM:
					g_shift &= ~KB_NUM;
					break;
					
				case KB_CAPS:
					g_shift &= ~KB_CAPS;
					break;
					
				case KB_SCROLL:
					g_shift &= ~KB_SCROLL;
					break;
					
				case KB_SHIFT:
					g_shift &= ~KB_SHIFT;
					break;
					
				case KB_ALT:
					if (g_extended)
						g_shift &= ~KB_ALTGR;
					else
						g_shift &= ~KB_ALT;
						
					break;
					
				case KB_CTL:
					g_shift &= ~KB_CTL;
					break;
				}
			}
			else
			{
				switch (k.type)
				{
				/*
				 * locking keys
				 */
				case KB_NUM:
					if (g_shift & KB_NUM)
						loop = FALSE;
					else
					{
						g_shift |= KB_NUM;
						g_lock ^= KB_NUM;
						update_leds(g_lock);
					}

					break;
					
				case KB_CAPS:
					if (g_shift & KB_CAPS)
						loop = FALSE;
					else
					{
						g_shift |= KB_CAPS;
						g_lock ^= KB_CAPS;
						update_leds(g_lock);
					}

					break;
					
				case KB_SCROLL:
					if (g_shift & KB_SCROLL)
						loop = FALSE;
					else
					{
						g_shift |= KB_SCROLL;
						g_lock ^= KB_SCROLL;
						update_leds(g_lock);
					}

					break;
				/*
				 * non-locking keys
				 */
				case KB_SHIFT:
					g_shift |= KB_SHIFT;
					break;
					
				case KB_ALT:
					if (g_extended)
						g_shift |= KB_ALTGR;
					else
						g_shift |= KB_ALT;
						
					break;
					
				case KB_CTL:
					g_shift |= KB_CTL;
					break;
					
				case KB_ASCII:
					/* control has highest priority */
					if (g_shift & KB_CTL)
						ch = k.ctl;
					else if (g_shift & KB_SHIFT)
						ch = k.shift;
					else
						ch = k.unshift;
					
					if ((g_lock & KB_CAPS) && ch >= 'a' &&
							ch <= 'z')
						ch -= ('a' - 'A');
					
					ch |= (g_shift & KB_ALT);
					g_extended = 0;
					addr[actual++] = ch;
					break;
				
				case KB_KP:
					if ((g_shift & (KB_CTL | KB_SHIFT)) ||
							!(g_lock & KB_NUM) || g_extended)
						ch = k.shift;
					else
						ch = k.unshift;

					ch |= (g_shift & KB_ALT);
					g_extended = 0;
					addr[actual++] = ch;
					break;
									
				case KB_FUNC:
				case KB_NONE:
				default:
					break;
				}
			}
		}
	}

	addr[actual] = '\0';

#ifdef DEBUG
	if (actual)
	{
		type("\$kbd_read: addr=");
		udot((int)addr);
		type("\$lock=");
		udot(g_lock);
		type("\$shift=");
		udot(g_shift);
		type("\$extended=");
		udot(g_extended);
		cr();
	}
#endif

	return actual == 0 ? -2 : actual;
}

int
selftest(void)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode();

	if (diag)
		type("\$keyboard selftest\r\n");

	return R_OK;
}

void
install_abort(void)
{
}

void
remove_abort(void)
{
}


#pragma expand_inline
void
ps2_keyboard_main(void)
{
	int i;
	int c = 0xFF;

	/* probe for a keyboard */
	kbd_out(KBD_CMD, KBC_SELF_TEST);

	for (i = 0; c == 0xFF && i < KBD_TIMEOUT; i++)
	{
		ms(10);
		c = kbd_in(KBD_DATA);
	}

#ifdef DEBUG
	type("\$keyboard-install: i=");
	udot(i);
	type("\$c=");
	udot(c);
	cr();
#endif

	if (c == KB_TEST_OK)
	{
		/* create this device's name and type */
		device_name("\$keyboard");
		device_type("\$keyboard");
	}
}
