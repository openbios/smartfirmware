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

/* This package is used by devices of type "display" to implement a
   subset of an ANSI terminal emulator as specified by OpenFirmware.
   An additional word ("term") connects the console to a serial device,
   making a complete terminal emulator.
 */

#include "defs.h"


/* used by the terminal emualator package - package-specific vars */
struct display
{
	Int openxt;		/* execution token to open the device */
	Int closext;
	Int testxt;
};
typedef struct display Display;


/* 5.3.6  Display device management */

CC(f_default_font)	/* (-- addr width height advance min-char #glyphs) */
{
	EC(machine_font);
	
	return machine_font(e);
}

CC(f_set_font)	/* set-font (addr width height advance min-char #glyphs --) */
{
	IFCKSP(e, 6, 0);	
	POP(e, e->glyphs);
	POP(e, e->minchar);
	POP(e, e->fontbytes);
	POP(e, e->cheight);
	POP(e, e->cwidth);
	POPT(e, e->font, Byte*);
	return NO_ERROR;
}

CC(f_tofont)			/* >font (char -- addr) */
{
	Int c;
	
	IFCKSP(e, 1, 1);
	POP(e, c);
	
	if (c < e->minchar || c - e->minchar >= e->glyphs)
	{
		/* out-of-range - try getting the glyph for a question mark */
		c = '?';
		
		if (c < e->minchar || c - e->minchar >= e->glyphs)
		{
			/* still out of range - push the 1st character, whatever it is */
			PUSHP(e, e->font);
			return NO_ERROR;
		}
	}

	c -= e->minchar;
	c *= (e->cheight - 1) * e->fontbytes;
	PUSHP(e, e->font + c);
	return NO_ERROR;
}


/* 16-color text extensions */

C(f_foreground_color)		/* foreground-color (-- color-number) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, e->fg);
	return NO_ERROR;
}

C(f_background_color)		/* background-color (-- color-number) */
{
	IFCKSP(e, 0, 1);
	PUSH(e, e->bg);
	return NO_ERROR;
}


/* the following methods implement the terminal emulator package
   for a display device
 */

C(f_is_open)		/* open (-- okay?) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	
	if (inst == NULL || inst->package == NULL ||
			inst->package->disp == NULL)
		return E_BAD_INSTANCE;
	
	ret = execute_xtok(e, inst->package->disp->openxt);
	PUSH(e, (ret == NO_ERROR) ? FTRUE : FFALSE);
	return ret;
}

C(f_is_close)		/* close (--) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;

	if (inst == NULL || inst->package == NULL ||
			inst->package->disp == NULL)
		return E_BAD_INSTANCE;
	
	return execute_xtok(e, inst->package->disp->closext);
}

C(f_is_test)		/* selftest (-- 0 | error-code) */
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Retcode ret;
	
	if (inst == NULL || inst->package == NULL ||
			inst->package->disp == NULL)
		return E_BAD_INSTANCE;
	
	ret = execute_xtok(e, inst->package->disp->testxt);
	PUSH(e, ret);
	return ret;
}

static void
bump_line(Environ *e)
{
	if (e->curline < e->lines - 1)
		e->curline++;
	else
	{
		Int line = e->curline;
		Int scroll = e->scrollstep;

		if (scroll <= 0 || scroll >= e->lines)
			scroll = 1;

		e->curline = 0;
		PUSH(e, scroll);
		execute_xtok(e, 0x160);		/* delete-lines */
		e->curline = line - scroll + 1;
	}
}

static void
bump_col(Environ *e)
{
	if (e->curcol < e->cols - 1)
		e->curcol++;
	else
	{
		e->curcol = 0;
		bump_line(e);
	}
}

/* write method implements a subset of an ANSI terminal as required by
   the OpenFirmware spec
 */
CC(f_is_write)		/* write (addr len -- actual) */
{
	Byte *addr;
	Int len, savcol, savline;
	Retcode ret;
	Int savecols = e->cols;

	IFCKSP(e, 2, 1);
	POP(e, len);
	POPT(e, addr, Byte*);
	PUSH(e, len);			/* actual should always == len for us */

	if (e->cols == STR_SIZE)	/* hack for Sun CommandTool */
		e->cols = get_config_int(e, "screen-#columns", CSTR);
	
	ret = execute_xtok(e, 0x159);			/* toggle-cursor off */
	
	/* perform terminal emulation here */
	for (; len-- > 0; addr++)
	{
		switch (e->emstate)
		{
		case S_NONE:
			switch (*addr)
			{
			case CTRL('G'):		/* ^G bell */
				ret = execute_xtok(e, 0x15B);		/* blink-screen */
				break;
				
			case '\b':		/* ^H backspace */
			case DEL:		/* delete */
				if (e->curcol > 0)
					e->curcol--;
				else if (e->curline > 0)
				{
					/* wrap to the end of the previous line for sun-compat */
					e->curline--;
					e->curcol = e->cols - 1;
				}
				
				break;
			
			case '\t':		/* ^I tab */
				e->curcol += 8 - e->curcol % 8;
				
				if (e->curcol > e->cols)
					e->curcol = e->cols - 1;
				
				break;
			
			case '\n':		/* ^J linefeed also does a return */
				bump_line(e);		/* linefeed */
				break;
				
			case '\r':		/* ^M return */
				e->curcol = 0;		/* return */
				break;
			
			case CTRL('K'):		/* ^K reverse linefeed */
				e->curline--;
				
				if (e->curline < 0)
					e->curline = 0;
				
				break;
			
			case CTRL('L'):		/* ^L form-feed */
				e->curline = 0;
				e->curcol = 0;
				ret = execute_xtok(e, 0x15A);		/* erase-screen */
				break;
			
			case ESC:		/* ESC escape */
				e->emstate = S_ESCAPE;
				break;
			
			default:
				if (isprint(*addr))
				{
					PUSH(e, *addr);	
					ret = execute_xtok(e, 0x157);		/* draw-character */
				}
				else
				{
					PUSH(e, '?');
					e->inverse = ~e->inverse;
					ret = execute_xtok(e, 0x157);		/* draw-character */
					e->inverse = ~e->inverse;
				}

				bump_col(e);
				continue;
			}
			
			break;
			
		case S_ESCAPE:
			if (*addr == '[')		/* ANSI escape sequence? */
			{
				e->emval = 0;
				e->emstate = S_NUM1;
			}
			else
				e->emstate = S_NONE;
				
			break;
			
		case S_NUM1:
			if (isdigit(*addr))
			{
				e->emval = e->emval * 10 + (*addr - '0');
				continue;
			}
			
			e->emstate = S_NONE;
			
			switch (*addr)
			{
			case 'A':		/* cursor up: ESC [ # A */
				e->curline -= (e->emval ? e->emval : 1);
				
				if (e->curline < 0)
					e->curline = 0;
				
				break;

			case 'B':		/* cursor down: ESC [ # B */
			case 'E':		/* next line: ESC [ # E - TODO? */
				e->curline += (e->emval ? e->emval : 1);
				
				if (e->curline >= e->lines)
					e->curline = e->lines - 1;
				
				break;
			
			case 'C':		/* cursor forward: ESC [ # C */
				e->curcol += (e->emval ? e->emval : 1);
				
				if (e->curcol >= e->cols)
					e->curcol = e->cols - 1;
				
				break;
			
			case 'D':		/* cursor backward: ESC [ # D */
				e->curcol -= (e->emval ? e->emval : 1);
				
				if (e->curcol < 0)
					e->curcol = 0;
				
				break;
			
			case ';':
				e->emval2 = 0;
				e->emstate = S_NUM2;
				continue;
				
			case 'H':		/* home cursor */
				e->curline = 0;
				e->curcol = 0;
				break;

			case 'J':	/* erase in display: ESC [ # J */
				switch (e->emval)
				{
				case 0:	/* kill the rest of the line and all lines following */
					PUSH(e, e->cols - e->curcol);
					ret = execute_xtok(e, 0x15E);		/* delete-characters */

					if (e->curline < e->lines - 1)
					{
						e->curline++;
						PUSH(e, e->lines - e->curline);
						ret = execute_xtok(e, 0x160);		/* delete-lines */
						e->curline--;
					}

					break;

				/* kill from beginning of screen to cursor - not in OF spec */
				case 1:
					savcol = e->curcol;
					e->curcol = 0;
					PUSH(e, savcol);
					ret = execute_xtok(e, 0x15E);		/* delete-characters */
					PUSH(e, savcol);
					ret = execute_xtok(e, 0x15D);		/* insert-characters */

					if (e->curline > 0)
					{
						savline = e->curline;
						e->curline = 0;
						PUSH(e, savline);
						ret = execute_xtok(e, 0x160);		/* delete-lines */
						PUSH(e, savline);
						ret = execute_xtok(e, 0x15F);		/* insert-lines */
						e->curline = savline;
					}

					e->curcol = savcol;
					break;
				
				case 2:		/* erase entire screen - not in OF spec */
					e->curline = 0;
					e->curcol = 0;
					ret = execute_xtok(e, 0x15A);		/* erase-screen */
					break;
				}
				
				break;
			
			case 'K':	/* erase in line: ESC [ # K */
				switch (e->emval)
				{
				case 0:		/* kill to end of line */
					PUSH(e, e->cols - e->curcol);
					ret = execute_xtok(e, 0x15E);		/* delete-characters */
					break;
				
				case 1:		/* kill from beginning of line - not in OF spec */
					savcol = e->curcol;
					e->curcol = 0;
					PUSH(e, savcol);
					ret = execute_xtok(e, 0x15E);		/* delete-characters */
					PUSH(e, savcol);
					ret = execute_xtok(e, 0x15D);		/* insert-characters */
					e->curcol = savcol;
					break;
				}

				break;
			
			case 'L':	/* insert lines: ESC [ # L */
				PUSH(e, (e->emval ? e->emval : 1));
				ret = execute_xtok(e, 0x15F);		/* insert-lines */
				break;
			
			case 'M':	/* delete lines: ESC [ # M */
				PUSH(e, (e->emval ? e->emval : 1));
				ret = execute_xtok(e, 0x160);		/* delete-lines */
				break;
			
			case '@':	/* insert characters: ESC [ # @ */
				PUSH(e, (e->emval ? e->emval : 1));
				ret = execute_xtok(e, 0x15D);		/* insert-characters */
				break;
			
			case 'P':	/* delete characters: ESC [ # P */
				PUSH(e, (e->emval ? e->emval : 1));
				ret = execute_xtok(e, 0x15E);		/* delete-characters */
				break;
			
			case 'm':	/* select graphics rendition: ESC [ # m */
				switch (e->emval)
				{
					static int cmap[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
					
				case 0:
					e->inverse = FFALSE;
					e->embold = 0;
					break;
				
				case 1:
					e->embold = 8;
					break;
				
				case 2:
					e->embold = 0;
					break;
				
				case 7:
					e->inverse = FTRUE;
					e->embold = 0;
					break;
				
				case 27:
					e->inverse = FFALSE;
					e->embold = 0;
					break;
				
				/* use cmap to map escape-sequence color to 16-bit color */
				case 30: case 31: case 32:
				case 34: case 35: case 36: case 37:
					e->fg = cmap[e->emval - 30] + e->embold;
					e->embold = 0;
					break;
				
				case 33:			/* no dim/bright yellow */
					e->fg = 14;
					e->embold = 0;
					break;
				
				case 40: case 41: case 42:
				case 44: case 45: case 46: case 47:
					e->bg = cmap[e->emval - 40] + e->embold;
					e->embold = 0;
					break;
				
				case 43:			/* no dim/bright yellow */
					e->bg = 14;
					e->embold = 0;
					break;
				}
				
				break;
			
			case 'p':	/* normal text colors */
			
				if (e->invscreen == FTRUE)
				{
					e->invscreen = FFALSE;
					e->inverse = ~e->inverse;
					ret = execute_xtok(e, 0x15C);		/* invert-screen */
				}
				
				break;
			
			case 'q':	/* inverse text colors */
			
				if (e->invscreen == FFALSE)
				{
					e->invscreen = FTRUE;
					e->inverse = ~e->inverse;
					ret = execute_xtok(e, 0x15C);		/* invert-screen */
				}
				
				break;
			
			case 's':	/* reset screen */
				ret = execute_xtok(e, 0x158);		/* reset-screen */
				break;
				
			default:
				break;
			}		/* switch for ANSI escape-sequence */

			break;
			
		case S_NUM2:
			if (isdigit(*addr))
			{
				e->emval2 = e->emval2 * 10 + (*addr - '0');
				continue;
			}
			
			e->emstate = S_NONE;
			
			switch (*addr)
			{
			case 'f':	/* cursor position: ESC [ #1 ; #2 f - TODO? */
			case 'H':	/* cursor position: ESC [ #1 ; #2 H */
				e->curline = e->emval - 1;
				e->curcol = e->emval2 - 1;
				
				if (e->curcol < 0)
					e->curcol = 0;
				else if (e->curcol >= e->cols)
					e->curcol = e->cols - 1;
				
				if (e->curline < 0)
					e->curline = 0;
				else if (e->curline >= e->lines)
					e->curline = e->lines - 1;
				
				break;
			
			default:
				break;
			}		/* switch for two-argument sequences */

			break;
		
		default:
			e->emstate = S_NONE;
			e->cols = savecols;
			return E_EMULATOR_BUG;
		}		/* switch (e->emstate) */
	}		/* for loop */
	
	ret = execute_xtok(e, 0x159);			/* toggle-cursor back on */
	e->cols = savecols;
	return NO_ERROR;
}

C(f_is_draw_logo)	/* draw-logo (line# addr width height --) */
{
	return execute_xtok(e, 0x161);
}

C(f_is_restore)		/* restore (--) */
{
	return execute_xtok(e, 0x158);
}

static const Initentry is_open_methods[] =
{
	{ "open", f_is_open, INVALID_FCODE },
	{ "write", f_is_write, INVALID_FCODE },
	{ "draw-logo", f_is_draw_logo, INVALID_FCODE },
	{ "restore", f_is_restore, INVALID_FCODE },
	{ NULL, NULL }
};

static const Initentry is_close_methods[] =
{
	{ "close", f_is_close, INVALID_FCODE },
	{ NULL, NULL }
};

static const Initentry is_test_methods[] =
{
	{ "selftest", f_is_test, INVALID_FCODE },
	{ NULL, NULL }
};

static Retcode
setup_is_methods(Environ *e, const Initentry methods[])
{
	IFCKSP(e, 1, 0);
	
	if (e->currpkg == NULL)
		return E_NULL_PACKAGE;
	
	if (e->currpkg->disp == NULL)
	{
		e->currpkg->disp = (Display*)malloc(sizeof (Display));
	
		if (e->currpkg->disp == NULL)
			return E_OUT_OF_MEMORY;

		e->currpkg->disp->openxt = INVALID_FCODE;
		e->currpkg->disp->closext = INVALID_FCODE;
		e->currpkg->disp->testxt = INVALID_FCODE;
	}
		
	return init_entries(e, e->currpkg->dict, methods);
}

CC(f_is_install)		/* is-install (xt --) */
{
	Retcode ret = setup_is_methods(e, is_open_methods);
	
	if (ret == NO_ERROR)
		POP(e, e->currpkg->disp->openxt);
	
	return ret;
}

CC(f_is_remove)		/* is-remove (xt --) */
{
	Retcode ret = setup_is_methods(e, is_close_methods);
	
	if (ret == NO_ERROR)
		POP(e, e->currpkg->disp->closext);
	
	return ret;
}

CC(f_is_selftest)		/* is-selftest (xt --) */
{
	Retcode ret = setup_is_methods(e, is_test_methods);
	
	if (ret == NO_ERROR)
		POP(e, e->currpkg->disp->testxt);
	
	return ret;
}


/* terminal emulator - connect console to a serial port and watch for an escape
   character to kick back to the Forth prompt
 */
C(f_terminal)				/* term ("{serial-device-path}<eol>"--) */
{
	Int len, c;
	Retcode ret;
	Byte *name, *str;
	Int slen;
	Instance *serial;				/* serial device */
	Int savelpp = e->linesperpage;
	Byte buf[STR_SIZE];

	IFCKSP(e, 0, 3);
	name = parse_line(e);		/* parse command-line options */

	if (name == NULL || *name == 0)
		return E_EMPTY_STRING;

	/* open serial device - options contain tty settings */
	PUSHP(e, name + 1);
	PUSH(e, (uChar)*name);
	ret = execute_word(e, "open-dev");

	if (ret != NO_ERROR)
		return ret;

	POPT(e, serial, Instance*);

	if (serial == NULL)
		return E_BAD_DEVICE;

	/* make sure this device has type "serial" */
	ret = prop_get_str(serial->package->props, "device_type", CSTR,
			&str, &slen);

	if (ret != NO_ERROR)
		goto done;

	if (!compare_strs(str, slen, "serial", CSTR))
	{
		ret = E_BAD_DEVICE;
		goto done;
	}

	e->linesperpage = e->lines + 1;		/* shut off pagination */
	cprintf(e, "entering terminal emulator - size %Cdx%Cd - type ",
			e->cols, e->lines);

	if (isprint(e->escape))
		cprintf(e, "'%c'", (Int)e->escape);
	else if (e->escape == DEL)
		cprintf(e, "^?");
	else
		cprintf(e, "^%c", (Int)e->escape + HEX_A - 1);

	cprintf(e, " to exit\n");

	/* loop moving data between console, keyboard, and serial port */
	for (;;)
	{
		/* read a character from the console input */
		if (key_down(e))
		{
			c = get_key(e);

			if (c < 0 || c == e->escape)
			{
				ret = NO_ERROR;
				break;
			}

			/* send it through the serial port */
			buf[0] = c;
			buf[1] = '\0';
			PUSHP(e, buf);
			PUSH(e, 1);
			
			ret = execute_method_name(e, serial, "write", CSTR);

			if (ret != NO_ERROR)
				break;
				
			POP(e, len);

			if (len != 1)
			{
				cprintf(e, "term: serial port write error - "
						"failed to write 1 byte\n");
				ret = E_WRITE_ERROR;
				break;
			}
		}

		/* get anything pending from the serial port */
		PUSHP(e, buf);
		PUSH(e, sizeof buf - 1);
		
		ret = execute_method_name(e, serial, "read", CSTR);

		if (ret != NO_ERROR)
			break;

		POP(e, len);

		/* check for errors - a -2 means no input pending */
		if (len < -2)
		{
			cprintf(e, "serial port read error %d\n", len);
			ret = E_READ_ERROR;
			break;
		}

		/* and display it on the console */
		if (len > 0)
			display_text(e, buf, len);
	}

	e->linesperpage = savelpp;

done:
	PUSHP(e, serial);			/* close serial device */
	execute_word(e, "close-dev");
	return ret;
}

CC(f_push_config_bool);

static const Initentry display_words[] =
{
	{ "line#", (Command)offsetof(Environ, curline), 0x152, F_NONE, T_ADDRVAL
			HELP("(-- line#) return current cursor line number") },
	{ "column#", (Command)offsetof(Environ, curcol), 0x153, F_NONE, T_ADDRVAL
			HELP("(-- column#) return current cursor column number") },
	{ "inverse?", (Command)offsetof(Environ, inverse), 0x154, F_NONE, T_ADDRVAL
			HELP("(-- white-on-black?) indicates how to paint characters") },
	{ "inverse-screen?", (Command)offsetof(Environ, invscreen), 0x155,
			F_NONE, T_ADDRVAL
			HELP("(-- black?) indiciated how to paint background") },
	{ "#lines", (Command)offsetof(Environ, lines), 0x150, F_NONE, T_ADDRVAL
			HELP("(-- rows) number of lines of text in text window") },
	{ "#columns", (Command)offsetof(Environ, cols), 0x151, F_NONE, T_ADDRVAL
			HELP("(-- columns) number of columns of text in text window") },
	{ "scroll-step", (Command)offsetof(Environ, scrollstep), INVALID_FCODE,
			F_NONE, T_ADDRVAL
			HELP("(-- step) jump-scroll by step lines at bottom of screen") },
			/* [non-standard] */

	{ "draw-character", (Command)INVALID_FCODE, 0x157, F_NONE, T_DEFER
			HELP("(char --) draw a character at current cursor position") },
	{ "reset-screen", (Command)INVALID_FCODE, 0x158, F_NONE, T_DEFER
			HELP("(--) perform frame-buffer device initialization") },
	{ "toggle-cursor", (Command)INVALID_FCODE, 0x159, F_NONE, T_DEFER
			HELP("(--) toggle the state of the text cursor") },
	{ "erase-screen", (Command)INVALID_FCODE, 0x15A, F_NONE, T_DEFER
			HELP("(--) clear the screen") },
	{ "blink-screen", (Command)INVALID_FCODE, 0x15B, F_NONE, T_DEFER
			HELP("(--) flash the screen") },
	{ "invert-screen", (Command)INVALID_FCODE, 0x15C, F_NONE, T_DEFER
			HELP("(--) exchange foreground and background colors") },
	{ "insert-characters", (Command)INVALID_FCODE, 0x15D, F_NONE, T_DEFER
			HELP("(n --) insert n spaces to the right of the cursor") },
	{ "delete-characters", (Command)INVALID_FCODE, 0x15E, F_NONE, T_DEFER
			HELP("(n --) delete n characters to the right of cursor") },
	{ "insert-lines", (Command)INVALID_FCODE, 0x15F, F_NONE, T_DEFER
			HELP("(n --) insert n lines at and below cursor line") },
	{ "delete-lines", (Command)INVALID_FCODE, 0x160, F_NONE, T_DEFER
			HELP("(n --) delete n lines at and below cursor line") },
	{ "draw-logo", (Command)INVALID_FCODE, 0x161, F_NONE, T_DEFER
			HELP("(line# addr width height --)\n"
					"\tdraw the logo stored at addr at line#") },
	
	{ "foreground-color", f_foreground_color, 0x168, F_NONE, T_FUNC
			HELP("(-- color-number) return current foreground color") },
	{ "background-color", f_background_color, 0x169, F_NONE, T_FUNC
			HELP("(-- color-number) return current background color") },

	{ "default-font", f_default_font, 0x16A, F_NONE, T_FUNC
			HELP("(-- addr width height advance min-char #glyphs)\n" \
					"\treturn font parameters of default system font") },
	{ "set-font", f_set_font, 0x16B, F_NONE, T_FUNC
			HELP("(addr width height advance min-char #glyphs --)\n" \
					"\tset the current font as specified") },
	{ ">font", f_tofont, 0x16E, F_NONE, T_FUNC
			HELP("(char -- addr) " \
					"return beginning addr for char in current font") },

	{ "frame-buffer-adr", (Command)offsetof(Environ, framebuf), 0x162,
			F_NONE, T_ADDRVAL
			HELP("(-- addr) return current frame-buffer virtual address") },
	{ "screen-width", (Command)offsetof(Environ, swidth), 0x164,
			F_NONE, T_ADDRVAL
			HELP("(-- width) return width of display in pixels") },
	{ "screen-height", (Command)offsetof(Environ, sheight), 0x163,
			F_NONE, T_ADDRVAL
			HELP("(-- height) return height of display in pixels") },
	{ "window-top", (Command)offsetof(Environ, wtop), 0x165,
			F_NONE, T_ADDRVAL
			HELP("(-- border-height) return window top border in pixels") },
	{ "window-left", (Command)offsetof(Environ, wleft), 0x166,
			F_NONE, T_ADDRVAL
			HELP("(-- border-width) return window left border in pixels") },
	{ "char-width", (Command)offsetof(Environ, cwidth), 0x16D,
			F_NONE, T_ADDRVAL
			HELP("(-- width) return width of a font character in pixels") },
	{ "char-height", (Command)offsetof(Environ, cheight), 0x16C,
			F_NONE, T_ADDRVAL
			HELP("(-- height) return height of font character in pixels") },
	{ "fontbytes", (Command)offsetof(Environ, fontbytes), 0x16F,
			F_NONE, T_ADDRVAL
		HELP("(-- bytes) return interval between entries in the font table") },
	
	{ "is-install", f_is_install, 0x11C, F_NONE, T_FUNC
		HELP("(xt --) create open and other methods for this display device") },
	{ "is-remove", f_is_remove, 0x11D, F_NONE, T_FUNC
			HELP("(xt --) create close method for this display device") },
	{ "is-selftest", f_is_selftest, 0x11E, F_NONE, T_FUNC
			HELP("(xt --) create selftest method for this display device") },

	{ "term-escape", (Command)offsetof(Environ, escape), INVALID_FCODE,
			F_NONE, T_ADDRVAL
	HELP("(-- char) character to use to end terminal emulation, usually ^]") },
			/* [non-standard] */
	{ "term", f_terminal, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{serial-device-path}<eol>\"--)\n"
			"\tstart terminal emulator connecting console to serial device") },
			/* [non-standard] */

	{ "inverse-video?", f_push_config_bool, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(-- inverted?) NVRAM var to set initial display mode") },
			/* [non-standard] */

	{ NULL, NULL }
};


CC(install_display)
{
	Package *pkg = new_pkg_name(e->packages, "terminal-emulator");
	
	prop_set_str(pkg->props, "iso6429-1983-colors", CSTR, "", CSTR);
	return init_entries(e, e->names, display_words);
}
