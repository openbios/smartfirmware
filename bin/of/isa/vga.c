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

#include "defs.h"
#include "isa.h"


/* VGA package for ISA displays - lots of work left to do, including
   monochrome support, proper initialization and selection of text
   modes, size of text window, etc.
 */


#define MONO_BASE	0x3B4		/* crtc index register address mono */
#define CGA_BASE	0x3D4		/* crtc index register address color */

#define CURSHAPEH		0x0A		/* cursor shape high */
#define CURSHAPEL		0x0B		/* cursor shape low */
#define CURSORH			0x0E		/* cursor address high */
#define CURSORL			0x0F		/* cursor address low */


#ifndef COLS
#define	COLS		80
#endif
#ifndef ROWS
#define	ROWS		25
#endif
#define	CHR			(sizeof (uShort))
#define MONO_BUF	0xB0000
#define CGA_MEM		0xB8000

#define CURSHAPE	0x0010


static uShort *cga_mem = NULL;


static void
mmove(uShort *d, uShort *s, size_t len)
{
	if (d == NULL || s == NULL)
		return;

	if (d < s)		/* assume src overlaps end of dst */
	{
		while (len-- > 0)
			*d++ = *s++;
	}
	else			/* assume dst overlaps end of src */
	{
		s += len;
		d += len;

		while (len-- > 0)
			*--d = *--s;
	}
}


/* I/O routines to talk to ISA ports */

#if 0
static uByte
inb(int port)
{
	uByte val = 0;

	isa_io_read(port, &val, 1);
	return val;
}
#endif

static void
outb(int port, uByte val)
{
	isa_io_write(port, val, 1);
}

static void
cput(Environ *e, Int loc, Int chr)
{
	Int fg, bg;

	if (e->inverse)
	{
		fg = e->bg;
		bg = e->fg;
	}
	else
	{
		fg = e->fg;
		bg = e->bg;
	}

	cga_mem[loc] = ((bg & 0x7) << 12) | ((fg & 0x8F) << 8) | (chr & 0xFF);
}


C(f_cga_open)			/* cga-open (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Isa_device *dev;
	Int curs, i;
	Cell addr;
	Retcode ret;

	IFCKSP(e, 0, 1);

	if (inst == NULL)
		return E_NULL_INSTANCE;

	if (inst->package->self == NULL)
		return E_NULL_PACKAGE;

	dev = (Isa_device*)inst->package->self;
	
	if (dev == NULL)
		return E_BAD_INSTANCE;
		
	ret = isa_map_in(ISA_MEMORY_ADDR, CGA_MEM, ROWS * COLS * CHR, &addr);

	if (ret != NO_ERROR)
		return ret;

	cga_mem = (uShort*)(uPtr)addr;
	DPRINTF(("cga_open\n"));

	e->pixsize = 8;
	e->lines = ROWS;
	e->cols = COLS;

#if 0
	/* set fg/bg based on colors of 1st char on screen */
	e->fg = (cga_mem[0] >> 8) & 0x0F;
	e->bg = (cga_mem[0] >> 12) & 0x0F;

	if (e->fg == e->bg)		/* same colors?!? - make them reasonable */
	{
		e->fg = 0;			/* black */
		e->bg = 15;			/* white */
	}
#endif

	/* default colors as specified by 16-color text-color extensions */
	e->fg = 0;		/* black */
	e->bg = 15;		/* white */
	curs = 0;		/* home the cursor */

#if 0
	/* read the cursor location */
	outb(CGA_BASE, CURSORH);
	curs = inb(CGA_BASE + 1) << 8;
	outb(CGA_BASE, CURSORL);
	curs |= inb(CGA_BASE + 1);
#endif

	/* set the cursor style */
	outb(CGA_BASE, CURSHAPEH);
	outb(CGA_BASE + 1, CURSHAPE >> 8);
	outb(CGA_BASE, CURSHAPEL);
	outb(CGA_BASE + 1, CURSHAPE & 0xFF);

	/* clear the screen */
	for (i = 0; i < ROWS * COLS; i++)
		cput(e, i, ' ');

	/* move the cursor */
	e->curline = curs / COLS;
	e->curcol = curs % COLS;
	e->cursor = TRUE;
	outb(CGA_BASE, CURSORH);
	outb(CGA_BASE + 1, (e->curline * COLS + e->curcol) >> 8);
	outb(CGA_BASE, CURSORL);
	outb(CGA_BASE + 1, (e->curline * COLS + e->curcol) & 0xFF);

	/* set the display words to point to our versions */
	ret = set_defer(e, "draw-character", "cga-draw-character");
	ret = set_defer(e, "reset-screen", "reset-screen");
	ret = set_defer(e, "toggle-cursor", "cga-toggle-cursor");
	ret = set_defer(e, "erase-screen", "cga-erase-screen");
	ret = set_defer(e, "blink-screen", "cga-blink-screen");
	ret = set_defer(e, "invert-screen", "cga-invert-screen");
	ret = set_defer(e, "insert-characters", "cga-insert-characters");
	ret = set_defer(e, "delete-characters", "cga-delete-characters");
	ret = set_defer(e, "insert-lines", "cga-insert-lines");
	ret = set_defer(e, "delete-lines", "cga-delete-lines");
	ret = set_defer(e, "draw-logo", "cga-draw-logo");

	return NO_ERROR;
}

C(f_cga_close)			/* cga-close (--) */
{
	isa_map_out((uPtr)cga_mem, ROWS * COLS * CHR);
	return NO_ERROR;
}

C(f_cga_selftest)		/* selftest (-- 0|error-code) */
{
	Bool diag = diagnostic_mode(e);

	if (diag)
	{
		cprintf(e, "cga selftest\n");
		/* do something someday */
	}

	PUSH(e, 0);			/* successful */
	return NO_ERROR;
}

/* cga-draw-character ( char -- ) */
C(f_cga_draw_character)
{
	Int ch;

	IFCKSP(e, 1, 0);
	POP(e, ch);

	/* no ">font" for CGA */
	cput(e, e->curline * COLS + e->curcol, ch);

	return NO_ERROR;
}

/* cga-reset-screen ( -- ) */
C(f_cga_reset_screen)
{
	Int i;

	/* clear the screen */
	for (i = 0; i < ROWS * COLS; i++)
		cput(e, i, ' ');

	return NO_ERROR;
}

/* cga-toggle-cursor ( -- ) */
C(f_cga_toggle_cursor)
{
	Int curs;
	
	e->cursor = !e->cursor;

	if (e->cursor)
		curs = e->curline * COLS + e->curcol;
	else
		curs = COLS * ROWS;		/* move to end-of-screen */

	/* move the cursor */
	outb(CGA_BASE, CURSORH);
	outb(CGA_BASE + 1, curs >> 8);
	outb(CGA_BASE, CURSORL);
	outb(CGA_BASE + 1, curs & 0xFF);
	
	return NO_ERROR;
}

/* cga-erase-screen ( -- ) */
C(f_cga_erase_screen)
{
	return f_cga_reset_screen(e);
}

/* cga-invert-screen ( -- ) */
C(f_cga_invert_screen)
{
	int i, a;

	for (i = 0; i < ROWS * COLS; i++)
	{
		a = cga_mem[i] >> 8;
		a = ((a >> 4) & 0x0F) | ((a << 4) & 0xF0);
		cga_mem[i] = (a << 8) | (cga_mem[i] & 0xFF);
	}

	return NO_ERROR;
}

/* cga-blink-screen ( -- ) */
C(f_cga_blink_screen)
{
	IFCKSP(e, 0, 1);
	f_cga_invert_screen(e);
	u_sleep(20000);
	f_cga_invert_screen(e);
	return NO_ERROR;
}

/* cga-insert-characters ( n -- ) */
C(f_cga_insert_characters)
{
	Int l, w, n;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = COLS - e->curcol - n;
	l = e->curline * COLS + e->curcol;

	if (w > 0)
		mmove(&cga_mem[l + n], &cga_mem[l], w);

	while (n--)
		cput(e, l++, ' ');

	return NO_ERROR;
}

/* cga-delete-characters ( n -- ) */
C(f_cga_delete_characters)
{
	Int l, n, w;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	w = COLS - e->curcol - n;
	l = e->curline * COLS + e->curcol;

	if (w > 0)
	{
		mmove(&cga_mem[l], &cga_mem[l + n], w);
		l += w;
	}

	while (n--)
		cput(e, l++, ' ');

	return NO_ERROR;
}

/* cga-insert-lines ( n -- ) */
C(f_cga_insert_lines)
{
	Int h, l, n;
	
	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;
		
	h = (ROWS - e->curline - n) * COLS;
	l = e->curline * COLS;
	n *= COLS;

	if (h > 0)
		mmove(&cga_mem[l + n], &cga_mem[l], h);

	while (n--)
		cput(e, l++, ' ');

	return NO_ERROR;
}

/* cga-delete-lines ( n -- ) */
C(f_cga_delete_lines)
{
	Int h, l, n;

	IFCKSP(e, 1, 0);
	POP(e, n);

	if (n < 1)
		return NO_ERROR;

	h = (ROWS - e->curline - n) * COLS;
	l = e->curline * COLS;
	n *= COLS;

	if (h > 0)
	{
		mmove(&cga_mem[l], &cga_mem[l + n], h);
		l += h;
	}

	while (n--)
		cput(e, l++, ' ');

	return NO_ERROR;
}

/* cga-draw-logo ( line# addr width height -- ) */
C(f_cga_draw_logo)
{
	Int line, width, height;
	Byte *addr;
	
	IFCKSP(e, 4, 0);
	POP(e, line);
	POPT(e, addr, Byte*);
	POP(e, width);
	POP(e, height);

	/* cannot draw pixmap in CGA text graphics */

	return NO_ERROR;
}

/* defer methods for display */
static const Initentry cga_names[] =
{
	{ "cga-draw-character", f_cga_draw_character, INVALID_FCODE },
	{ "cga-reset-screen", f_cga_reset_screen, INVALID_FCODE },
	{ "cga-toggle-cursor", f_cga_toggle_cursor, INVALID_FCODE },
	{ "cga-erase-screen", f_cga_erase_screen, INVALID_FCODE },
	{ "cga-blink-screen", f_cga_blink_screen, INVALID_FCODE },
	{ "cga-invert-screen", f_cga_invert_screen, INVALID_FCODE },
	{ "cga-insert-characters", f_cga_insert_characters, INVALID_FCODE },
	{ "cga-delete-characters", f_cga_delete_characters, INVALID_FCODE },
	{ "cga-insert-lines", f_cga_insert_lines, INVALID_FCODE },
	{ "cga-delete-lines", f_cga_delete_lines, INVALID_FCODE },
	{ "cga-draw-logo", f_cga_draw_logo, INVALID_FCODE },

	{ NULL, NULL }
};

static const Initentry cga_methods[] =
{
	{ "cga-open",      f_cga_open,          INVALID_FCODE },
	{ "cga-close",     f_cga_close,         INVALID_FCODE },
	{ "selftest",      f_cga_selftest,      INVALID_FCODE },
	{ NULL,            NULL },
};


ISA(cga_probe)
{
	uShort org, val;

	/* if we can write and read a known 16-bit value from the standard
	   CGA address space, assume we have a CGA-compatible card */

	if (isa_mem_read(CGA_MEM, &org, 2) == NO_ERROR &&
			isa_mem_write(CGA_MEM, 0xA569, 2) == NO_ERROR &&
			isa_mem_read(CGA_MEM, &val, 2) == NO_ERROR &&
			isa_mem_write(CGA_MEM, org, 2) == NO_ERROR &&
			val == 0xA569)
		return NO_ERROR;

	return E_ABORT;
}

ISA(cga_install)
{
	Retcode ret = NO_ERROR;
	Package *savepkg = e->currpkg;
	Package *pkg;
	Entry *ent;
	EC(f_is_install);
	EC(f_is_remove);

	dev->self = NULL;
	ret = new_isa_device(e, dev);

	if (ret == NO_ERROR)
		ret = init_entries(e, e->names, cga_names);
	
	if (ret != NO_ERROR)
		return ret;
	
	pkg = dev->pkg;
	ret = prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);

	if (ret != NO_ERROR)
		return ret;

	/* need to pass in the xtok for our open routine to is-install */
	ent = find_table(pkg->dict, "cga-open", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	e->currpkg = pkg;
	PUSH(e, ent->xtok);
	/* is-install adds open, write, draw-logo, and restore */
	ret = f_is_install(e);
	e->currpkg = savepkg;
	
	if (ret != NO_ERROR)
		return ret;
	
	/* need to pass in the xtok for our close routine to is-remove */
	ent = find_table(pkg->dict, "cga-close", CSTR);
	
	if (ent == NULL)
		return E_NO_METHOD;
	
	e->currpkg = pkg;
	PUSH(e, ent->xtok);
	ret = f_is_remove(e);		/* is-remove adds a close method */
	e->currpkg = savepkg;

	if (ret != NO_ERROR)
		return ret;

	return ret;
}

static Int cga_reg[] = { ISA_MEMORY_ADDR, CGA_MEM, COLS * ROWS * CHR };

Isa_device vga_display =
{
	"display",
	"display",
	ISA_IO_ADDRESS,
	CGA_BASE,
	2,
	{ 0, 0 },		/* IRQ */
	{ -1, },		/* DMA */
	0x0,			/* BIOS */
	1, cga_reg, 	/* pointer to display memory */
	0,			/* clock freq, N/A here */
	cga_probe,
	cga_install,
	cga_methods
};
