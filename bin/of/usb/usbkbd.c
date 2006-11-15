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

/* (c) Copyright 1998 by CodeGen, Inc.  All Rights Reserved. */

/* USB support for keyboards.
 */

#include "defs.h"
#include "usb.h"


/* following declarations copied from NetBSD usbkbd.c code
 */

/* LEDs */
#define NUM_LOCK	0x01
#define CAPS_LOCK	0x02
#define SCROLL_LOCK	0x04

#define NKEYCODE 6

struct ukbd_data {
	uByte	modifiers;
#define MOD_CONTROL_L	0x01
#define MOD_CONTROL_R	0x10
#define MOD_SHIFT_L		0x02
#define MOD_SHIFT_R		0x20
#define MOD_ALT_L		0x04
#define MOD_ALT_R		0x40
#define MOD_COMPOSE_L	0x08
#define MOD_COMPOSE_R	0x80
	uByte	reserved;
	uByte	keycode[NKEYCODE];
};
typedef struct ukbd_data ukbd_data_t;

/* end copied NetBSD code
 */

#define	KB_NUM		0x01	/* numeric shift  cursors vs. numeric */
#define	KB_SHIFT	0x02	/* keyboard shift */
#define	KB_CTL		0x04	/* control shift */
#define	KB_ASCII	0x08	/* ascii - not a modifier */
#define	KB_FUNC		0x10	/* function key */
#define	KB_KP		0x20	/* keypad key */
#define	KB_ERROR	0x40	/* error */
#define	KB_LOCK		0x80	/* locking modifier key - key is code */
#define	KB_NONE		0x00	/* no function */


/* to remember currently pressed-down keys */
struct self
{
	uByte lock;
	uByte shift;
	ukbd_data_t okbd;
};
typedef struct self Self;


typedef struct
{
	uByte type;
	Byte unshift;
	Byte shift;
	Byte ctl;
} keymap;

static keymap key_codes[] =
{
/*	  type       unshift   shift     ctl            keycode */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /*  0 (reserved - no event) */
	{ KB_ERROR,  '\0',     '\0',     '\0'   },   /*  1 error: roll-over */
	{ KB_ERROR,  '\0',     '\0',     '\0'   },   /*  2 error: POSTfail */
	{ KB_ERROR,  '\0',     '\0',     '\0'   },   /*  3 error: undefined  */

	{ KB_ASCII,  'a',      'A',      '\001' },   /*  4 a */
	{ KB_ASCII,  'b',      'B',      '\002' },   /*  5 b */
	{ KB_ASCII,  'c',      'C',      '\003' },   /*  6 c */
	{ KB_ASCII,  'd',      'D',      '\004' },   /*  7 d */
	{ KB_ASCII,  'e',      'E',      '\005' },   /*  8 e */
	{ KB_ASCII,  'f',      'F',      '\006' },   /*  9 f */
	{ KB_ASCII,  'g',      'G',      '\007' },   /* 10 g */
	{ KB_ASCII,  'h',      'H',      '\010' },   /* 11 h */
	{ KB_ASCII,  'i',      'I',      '\011' },   /* 12 i */
	{ KB_ASCII,  'j',      'J',      '\012' },   /* 13 j */
	{ KB_ASCII,  'k',      'K',      '\013' },   /* 14 k */
	{ KB_ASCII,  'l',      'L',      '\014' },   /* 15 l */
	{ KB_ASCII,  'm',      'M',      '\013' },   /* 16 m */
	{ KB_ASCII,  'n',      'N',      '\016' },   /* 17 n */
	{ KB_ASCII,  'o',      'O',      '\017' },   /* 18 o */
	{ KB_ASCII,  'p',      'P',      '\020' },   /* 19 p */
	{ KB_ASCII,  'q',      'Q',      '\021' },   /* 20 q */
	{ KB_ASCII,  'r',      'R',      '\022' },   /* 21 r */
	{ KB_ASCII,  's',      'S',      '\023' },   /* 22 s */
	{ KB_ASCII,  't',      'T',      '\024' },   /* 23 t */
	{ KB_ASCII,  'u',      'U',      '\025' },   /* 24 u */
	{ KB_ASCII,  'v',      'V',      '\026' },   /* 25 v */
	{ KB_ASCII,  'w',      'W',      '\027' },   /* 26 w */
	{ KB_ASCII,  'x',      'X',      '\030' },   /* 27 x */
	{ KB_ASCII,  'y',      'Y',      '\031' },   /* 28 y */
	{ KB_ASCII,  'z',      'Z',      '\032' },   /* 29 z */

	{ KB_ASCII,  '1',      '!',      '!'    },   /* 30 1 */
	{ KB_ASCII,  '2',      '@',      '@'    },   /* 31 2 */
	{ KB_ASCII,  '3',      '#',      '#'    },   /* 32 3 */
	{ KB_ASCII,  '4',      '$',      '$'    },   /* 33 4 */
	{ KB_ASCII,  '5',      '%',      '%'    },   /* 34 5 */
	{ KB_ASCII,  '6',      '^',      '\036' },   /* 35 6 */
	{ KB_ASCII,  '7',      '&',      '&'    },   /* 36 7 */
	{ KB_ASCII,  '8',      '*',      '\010' },   /* 37 8 */
	{ KB_ASCII,  '9',      '(',      '('    },   /* 38 9 */
	{ KB_ASCII,  '0',      ')',      ')'    },   /* 39 0 */

	{ KB_ASCII,  '\r',     '\r',     '\n'   },   /* 40 return */
	{ KB_ASCII,  '\033',   '\033',   '\033' },   /* 41 ESCape */
	{ KB_ASCII,  '\177',   '\177',   '\010' },   /* 42 backspace */
	{ KB_ASCII,  '\t',     '\t',     '\t'   },   /* 43 tab */
	{ KB_ASCII,  ' ',      ' ',      '\000' },   /* 44 space */
	{ KB_ASCII,  '-',      '_',      '\037' },   /* 45 - */
	{ KB_ASCII,  '=',      '+',      '+'    },   /* 46 = */
	{ KB_ASCII,  '[',      '{',      '\033' },   /* 47 [ */
	{ KB_ASCII,  ']',      '}',      '\035' },   /* 48 ] */
	{ KB_ASCII,  '\\',     '|',      '\034' },   /* 49 \ */
	{ KB_ASCII,  '\\',     '|',      '\034' },   /* 50 non-US */
	{ KB_ASCII,  ';',      ':',      ';'    },   /* 51 ; */
	{ KB_ASCII,  '\'',     '"',      '\''   },   /* 52 ' */
	{ KB_ASCII,  '`',      '~',      '`'    },   /* 53 ` */
	{ KB_ASCII,  ',',      '<',      '<'    },   /* 54 , */
	{ KB_ASCII,  '.',      '>',      '>'    },   /* 55 . */
	{ KB_ASCII,  '/',      '?',      '\037' },   /* 56 / */

	{ KB_LOCK,   CAPS_LOCK, '\0',    '\0'   },   /* 57 caps-lock */

	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 58 f1 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 59 f2 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 60 f3 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 61 f4 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 62 f5 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 63 f6 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 64 f7 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 65 f8 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 66 f9 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 67 f10 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 68 f11 */
	{ KB_FUNC,   '\0',     '\0',     '\0'   },   /* 60 f12 */

	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 70 print screen */

	{ KB_LOCK,   SCROLL_LOCK, '\0',  '\0'   },   /* 71 scroll lock */

	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 72 pause */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 73 insert */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 74 home */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 75 page-up */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 76 delete-forward */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 77 end */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 78 page-down */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 79 right-arrow */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 80 left-arrow */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 81 down-arrow */
	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 82 up-arrow */

	{ KB_LOCK,   NUM_LOCK, '\0',     '\0'  },   /* 83 num-lock */

	{ KB_KP,     '/',      '/',      '/'    },   /* 84 kp / */
	{ KB_KP,     '*',      '*',      '*'    },   /* 85 kp * */
	{ KB_KP,     '-',      '-',      '-'    },   /* 86 kp - */
	{ KB_KP,     '+',      '+',      '+'    },   /* 87 kp + */
	{ KB_KP,     '\r',     '\r',     '\n'   },   /* 88 enter */

	{ KB_KP,     '1',      '\0',     '1'    },   /* 89 kp 1 */
	{ KB_KP,     '2',      '\016',   '2'    },   /* 90 kp 2 */
	{ KB_KP,     '3',      '\0',     '3'    },   /* 91 kp 3 */
	{ KB_KP,     '4',      '\002',   '4'    },   /* 92 kp 4 */
	{ KB_KP,     '5',      '\0',     '5'    },   /* 93 kp 5 */
	{ KB_KP,     '6',      '\006',   '6'    },   /* 94 kp 6 */
	{ KB_KP,     '7',      '\0',     '7'    },   /* 95 kp 7 */
	{ KB_KP,     '8',      '\020',   '8'    },   /* 96 kp 8 */
	{ KB_KP,     '9',      '\0',     '9'    },   /* 97 kp 9 */
	{ KB_KP,     '0',      '\0',     '0'    },   /* 98 kp 0 */

	{ KB_KP,     '.',      '\177',   '.'    },   /* 83 kp . */

	{ KB_ASCII,  '\\',     '|',      '\034' },   /* 100 non-US */

	{ KB_NONE,   '\0',     '\0',     '\0'   },   /* 101 compose */
};

#define NUM_KEY_CODES	(sizeof key_codes / sizeof key_codes[0])


CC(f_usb_kbd_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
		cprintf(e, "USB keyboard selftest\n");

	/* TODO */

	PUSH(e, 0);
	return NO_ERROR;
}

Retcode
usb_set_leds(Environ *e, USB_self *usb, uByte lock)
{
	return usb_setup(e, usb, usb->addr, USB_CONTROL_ENDPOINT,
			UT_WRITE_CLASS_INTERFACE, UR_SET_REPORT,
			UHID_OUTPUT_REPORT, 0, usb->interface,
			1, &lock);
}

CC(f_usb_kbd_read)		/* read (addr len -- actual) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int actual = 0;
	Int len, k, c, i, j;
	Byte *addr;
	USB_self *usb;
	Self *s;
	Retcode ret = NO_ERROR;
	ukbd_data_t kbd;

	if (inst == NULL)
		return E_NULL_INSTANCE;
	
	s = inst->self;

	if (s == NULL)
		return E_BAD_INSTANCE;

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	usb = (USB_self*)inst->package->self;
	
	if (usb == NULL)
		return E_BAD_PACKAGE;
	
	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);

	while (actual < len)
	{
		ret = usb_setup(e, usb, usb->addr, USB_CONTROL_ENDPOINT,
				UT_READ_CLASS_INTERFACE, UR_GET_REPORT,
				UHID_INPUT_REPORT, 0, usb->interface,
				sizeof kbd, (Byte*)&kbd);

		if (memcmp(&s->okbd, &kbd, sizeof kbd) == 0)	/* no change */
			break;

		DPRINTF(("usb_kbd_read: modifier=%#x"
				" key[0..5]=%#x.%#x.%#x.%#x.%#x.%#x\n",
				kbd.modifiers,
				kbd.keycode[0], kbd.keycode[1], kbd.keycode[2],
				kbd.keycode[3], kbd.keycode[4], kbd.keycode[5]));

		for (i = 0; actual < len && i < NKEYCODE; i++)
		{
			k = kbd.keycode[i];

			for (j = 0; j < NKEYCODE; j++)
				if (s->okbd.keycode[j] == k)
					break;

			if (j < NKEYCODE)		/* key is still down */
				continue;

			if (k >= NUM_KEY_CODES || key_codes[k].type == KB_ERROR)
				continue;	/* try again */

			if (key_codes[k].type == KB_LOCK)
			{
				/* locking key */
				s->lock ^= key_codes[k].unshift;
				ret = usb_set_leds(e, usb, s->lock);

				if (ret != NO_ERROR)
					break;

				continue;
			}

			if (kbd.modifiers & (MOD_CONTROL_L | MOD_CONTROL_R))
				c = key_codes[k].ctl;
			else if (kbd.modifiers & (MOD_SHIFT_L | MOD_SHIFT_R))
				c = key_codes[k].shift;
			else
				c = key_codes[k].unshift;

			if ((s->lock & CAPS_LOCK) && c >= 'a' && c <= 'z')
				c -= ('a' - 'A');

			if (kbd.modifiers & (MOD_ALT_L | MOD_ALT_R))
				c |= 0x80;

			addr[actual++] = c;
		}

		memcpy(&s->okbd, &kbd, sizeof kbd);
	}

	if (actual < len)
		addr[actual] = '\0';

	PUSH(e, (actual == 0) ? -2 : actual);
	return ret;
}

CC(f_usb_kbd_open)		/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	USB_self *usb;
	Retcode ret;

	DPRINTF(("usb_kbd_open\n"));

	if (inst->package == NULL)
		return E_NULL_PACKAGE;

	usb = (USB_self*)inst->package->self;
	
	if (usb == NULL)
		return E_BAD_PACKAGE;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	inst->self = (Self*)malloc(sizeof *inst->self);

	if (inst->self == NULL)
		return E_OUT_OF_MEMORY;

	memset(inst->self, 0, sizeof *inst->self);
	inst->self->lock = NUM_LOCK;
	ret = usb_set_leds(e, usb, inst->self->lock);

	IFCKSP(e, 0, 1);
	PUSH(e, FTRUE);
	return ret;
}

CC(f_usb_kbd_close)		/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->self == NULL)
		return E_BAD_INSTANCE;

	free(inst->self);
	return NO_ERROR;
}


const Initentry usb_kbd_methods[] =
{
	{ "open",			f_usb_kbd_open,			INVALID_FCODE },
	{ "close",			f_usb_kbd_close,		INVALID_FCODE },
	{ "selftest",       f_usb_kbd_selftest,		INVALID_FCODE },
	{ "read",			f_usb_kbd_read,			INVALID_FCODE },
	{ NULL,				NULL },
};


Retcode
usb_install_keyboard_driver(Environ *e, Package *pkg, USB_self *usb,
		usb_device_descriptor_t *dev, usb_config_descriptor_t *cfg,
		usb_interface_descriptor_t *ifc)
{
	Retcode ret;
	Byte name[STR_SIZE];

	DPRINTF(("usb_install_keyboard_driver: addr=%d interface=%d\n",
			usb->addr, usb->interface));

	ret = prop_set_str(pkg->props, "device_type", CSTR, "keyboard", CSTR);

	if (ret == NO_ERROR)
		ret = init_entries(e, pkg->dict, usb_kbd_methods);


	/* make keyboard aliases if none already exist */
	if (ret == NO_ERROR)
		(void)get_device_name(e, pkg, name);

	if (ret == NO_ERROR &&
			find_table(e->aliases->props, "keyboard", CSTR) == NULL)
		ret = make_devalias(e, "keyboard", CSTR, name, CSTR);

	if (ret == NO_ERROR &&
			find_table(e->aliases->props, "usbkeyboard", CSTR) == NULL)
		ret = make_devalias(e, "usbkeyboard", CSTR, name, CSTR);


	/* set config descriptor 0 to turn on keyboard */
	ret = usb_setup(e, usb, usb->addr, USB_CONTROL_ENDPOINT,
			UT_WRITE_DEVICE, UR_SET_CONFIG,
			0, cfg->configuration_value, 0, 0, NULL);

	/* set keyboard to boot protocol (0) */
	if (ret == NO_ERROR)
		ret = usb_setup(e, usb, usb->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_INTERFACE, UR_SET_PROTOCOL,
				0, 0, usb->interface, 0, NULL);

	if (ret == NO_ERROR)
		ret = usb_setup(e, usb, usb->addr, USB_CONTROL_ENDPOINT,
				UT_WRITE_CLASS_INTERFACE, UR_SET_IDLE,
				0, 0, usb->interface, 0, NULL);

	return ret;
}
