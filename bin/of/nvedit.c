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


/* 7.4.4.2  The script */

/* [most nvramrc stuff is in admin.c but the editing stuff is all here] */


/* The text editor buffer is simply a list of lines each of which is
   as long as the number of columns - 1, to avoid the problems of
   dealing with end-of-line wrapping.  The array is grown as needed.
 */

#define BEEP(e) display_text(e, "\007", 1)
#define NUMWIDTH	7
#define WIDTH(e)	((e)->cols - NUMWIDTH)
#define MOVENUM(e, line)	\
		cprintf(e, "\033[%d;0H", (Int)(line) + 1 - e->etop)
#define MOVE(e, line, col)	\
		cprintf(e, "\033[%d;%dH", (Int)(line) + 1 - e->etop, \
				(Int)(col) + 1 - e->eleft + NUMWIDTH)

/* grow the edit buffer to hold at least the requested number of lines
 */
static Retcode
grow_edit_buf(Environ *e, Int num)
{
	Int i;

	if (num < e->emax)
		return NO_ERROR;

	/* allocate array of lines leaving some additional room to grow */
	num += e->lines;
	e->editbuf = (Byte**)realloc(e->editbuf, num * sizeof (Byte*));

	if (e->editbuf == NULL)
		return E_OUT_OF_MEMORY;

	/* allocate and null-terminate each additional line */
	for (i = e->emax; i < num; i++)
	{
		e->editbuf[i] = (Byte*)malloc(STR_SIZE + 2);

		if (e->editbuf[i] == NULL)
			return E_OUT_OF_MEMORY;

		e->editbuf[i][0] = '\0';
	}

	e->emax = num;
	return NO_ERROR;
}

static Retcode
init_edit(Environ *e)
{
	if (e->editbuf != NULL)
		return NO_ERROR;

	/* sanity checks in case these have not been initialized */
	if (e->lines == 0)
		e->lines = 24;

	if (e->cols == 0)
		e->cols = 80;

	e->elines = 0;
	e->etop = 0;
	e->eleft = 0;
	e->editbuf = NULL;
	e->emax = 0;
	return grow_edit_buf(e, e->lines);
}

/* parse a string and insert it into the text-editing buffer
   - long lines are wrapped 
 */
static Retcode
fill_edit_buf(Environ *e, Byte *s, Int len)
{
	Int line = 0;
	Int col = 0;
	Retcode ret;

	setstrlen(&s, &len);

	while (len-- > 0)
	{
		if (*s == '\n' || *s == '\r')
		{
			/* newline - goto the next line */
			e->editbuf[line][col] = '\0';
			ret = grow_edit_buf(e, line + 1);

			if (ret != NO_ERROR)
				return ret;

			line++;
			col = 0;
			s++;
			continue;
		}

		e->editbuf[line][col++] = *s++;		/* add the character */

		if (col >= STR_SIZE - 1)
		{
			/* wrapped around the end of the line */
			line++;
			col = 0;
		}
	}

	e->editbuf[line][col] = '\0';

	/* total number of lines we parsed */
	e->elines = col == 0 ? line : line + 1;
	e->etop = 0;
	e->eleft = 0;
	return NO_ERROR;
}

/* puts edited text into e->fbrk area as a null-terminated C string
 */
static Retcode
save_edit_buf(Environ *e)
{
	Int line;
	Byte *s = e->fbrk;

	for (line = 0; line < e->elines; line++)
	{
		strcpy(s, e->editbuf[line]);
		s += strlen(e->editbuf[line]);
		*s++ = '\n';
	}

	*s = '\0';
	return NO_ERROR;
}

/* clear the screen from the cursor and redraw the remainder
 */
static void
redraw_after(Environ *e, Int line, Int col)
{
	Int i;
	Int end = (e->etop + e->lines >= e->elines) ?
			e->elines : e->etop + e->lines;
	Int len;

	if (col < e->eleft || col >= e->eleft + WIDTH(e) - 1)
	{
		e->eleft = col - WIDTH(e) / 2;

		if (e->eleft < 0)
			e->eleft = 0;
	}

	MOVENUM(e, line);

	/* erase-screen or reset-screen - whichever works */
	display_text(e, "\033[J", CSTR);

	for (i = line; i < end; i++)
	{
		cprintf(e, "%*d: ", NUMWIDTH - 2, i + LINE_NUM_START);
		len = strlen(e->editbuf[i]);

		if (len > e->eleft)
		{
			len -= e->eleft;
			display_text(e, e->editbuf[i] + e->eleft,
					(len < WIDTH(e) - 1) ? len : WIDTH(e) - 1);
		}

		if (i < end - 1)
			display_text(e, "\r\n", 2);
	}

	if (e->etop == end)
		cprintf(e, "%*d: ", NUMWIDTH - 2, LINE_NUM_START);

	MOVE(e, line, col);		/* restore cursor */
}

/* clear the screen redraw the entire editor buffer
 */
static void
redraw_edit_buf(Environ *e, Int line, Int col)
{
	redraw_after(e, e->etop, col);
	MOVE(e, line, col);		/* restore cursor */
}

static void
move(Environ *e, Int line, Int col)
{
	if (col >= e->eleft && col < e->eleft + WIDTH(e) - 1)
		MOVE(e, line, col);
	else
		redraw_edit_buf(e, line, col);
}

/* scroll window up one line - deletes first line on screen
 */
static Retcode
scroll_up(Environ *e, Int line, Int col)
{
	Int len;

	if (e->etop + e->lines >= e->elines)
		return E_ABORT;

	/* delete first line, scrolls everything up */
	MOVENUM(e, e->etop);
	display_text(e, "\033[1M", CSTR);	/* delete line */
	e->etop++;
	MOVENUM(e, e->etop + e->lines - 1);
	cprintf(e, "%*d: ", NUMWIDTH - 2,
			e->etop + (Int)e->lines - 1 + LINE_NUM_START);
	len = strlen(e->editbuf[e->etop + e->lines - 1]) - e->eleft;

	if (len > 0)
	{
		if (len > WIDTH(e) - 1)
			len = WIDTH(e) - 1;

		display_text(e, e->editbuf[e->etop + e->lines - 1] + e->eleft, len);
	}

	move(e, line, col);
	return NO_ERROR;
}

/* scroll window down one line - deletes last line on screen
 */
static Retcode
scroll_down(Environ *e, Int line, Int col)
{
	Int len;

	if (e->etop == 0)
		return E_ABORT;

	/* insert first line, scrolls everything down */
	MOVENUM(e, e->etop);
	display_text(e, "\033[1L", CSTR);			/* insert line */
	cprintf(e, "%*d: ", NUMWIDTH - 2, e->etop);
	e->etop--;
	len = strlen(e->editbuf[e->etop]) - e->eleft;

	if (len > 0)
	{
		if (len > WIDTH(e) - 1)
			len = WIDTH(e) - 1;

		display_text(e, e->editbuf[e->etop], len);
	}

	move(e, line, col);
	return NO_ERROR;
}

/* break a line at col and scrolls the following lines down
 */
static Retcode
break_line(Environ *e, Int *line, Int *col, Bool movecursor)
{
	Int i;
	Byte *s;
	Retcode ret;

	/* make sure there is enough room in the buffer */
	ret = grow_edit_buf(e, e->elines + 1);

	if (ret != NO_ERROR)
		return ret;

	if (*line < e->elines)		/* not the last line */
	{
		/* save the last line's buffer */
		s = e->editbuf[e->elines];

		/* move all lines following the current one down */
		for (i = e->elines; i > *line + 1; i--)
			e->editbuf[i] = e->editbuf[i - 1];

		/* and use the saved buffer for the next line */
		e->editbuf[*line + 1] = s;
	}
	else
		s = e->editbuf[*line + 1];

	/* copy text from col of current line to end to the following line */
	strcpy(s, e->editbuf[*line] + *col);

	/* and kill the rest of the line following col */
	e->editbuf[*line][*col] = '\0';
	e->elines++;

	/* user typed a CR - move the cursor to the next line */
	if (movecursor)
	{
		*line += 1;
		*col = 0;

		if (e->eleft > 0)
		{
			if (*line >= e->etop + e->lines)
				e->etop++;

			e->eleft = 0;
			redraw_edit_buf(e, *line, *col);
			return NO_ERROR;
		}

		if (*line >= e->etop + e->lines)
			scroll_up(e, *line, *col);
		else
		{
			redraw_after(e, *line - 1, *col);
			MOVE(e, *line, *col);
		}
	}
	else
		redraw_after(e, *line, *col);

	return NO_ERROR;
}

/* delete the implicit newline at col in line - scrolls the rest of lines up */
static Retcode
join_line(Environ *e, Int line, Int col)
{
	Int i, next;
	Byte *s;

	/* see if we are at the end of a line, it isn't the last line, and there
	   are at least 2 lines in the buffer */
	if (line >= e->elines - 1 ||
			col < strlen(e->editbuf[line]) ||
			e->elines <= 1)
		return E_ABORT;

	next = line + 1;		/* next line number */

	/* if the joined line is too long to fit, forget it */
	if (strlen(e->editbuf[line]) + strlen(e->editbuf[next]) >= STR_SIZE - 1)
		return E_ABORT;

	/* save the next line's buffer */
	s = e->editbuf[next];

	for (i = next; i < e->elines - 1; i++)
		e->editbuf[i] = e->editbuf[i + 1];

	/* append the next line to the current line at col */
	strncat(e->editbuf[line] + col, s, STR_SIZE - col);

	/* need this if next line was too long */
	e->editbuf[line][STR_SIZE - 1] = '\0';

	/* and finally use the saved buffer as the last line's buffer */
	e->editbuf[e->elines - 1] = s;
	*s = '\0';
	e->elines--;

	/* redraw the display where the line numbers have changed */
	redraw_after(e, line, col);
	return NO_ERROR;
}

/* performs minimal Emacs-style text editing on screen */
static Retcode
do_edit(Environ *e)
{
	Byte c;				/* key from user */
	Int line = 0;		/* cursor line */
	Int col = 0;		/* cursor column */
	Int savelpp = e->linesperpage;
	Byte *savebuf;
	Int i, j;

	/* sanity checks in case these have not been initialized */
	if (e->lines == 0)
		e->lines = 24;

	if (e->cols == 0)
		e->cols = 80;

	e->linesperpage = e->lines + 1;		/* shut off pagination */
	e->flush_output = FO_NONE;			/* do not flush any output */
	e->etop = 0;
	e->eleft = 0;
	redraw_edit_buf(e, 0, 0);
	savebuf = (Byte*)malloc(STR_SIZE + 2);		/* allocate yank-buffer */

	if (savebuf == NULL)
		return E_OUT_OF_MEMORY;

	savebuf[0] = '\0';

	while ((c = get_key(e)) != CTRL('C'))
	{
		switch (c)
		{
		case '\n':			/* new line - break line here and move cursor */
		case '\r':
			if (break_line(e, &line, &col, TRUE) != NO_ERROR)
				BEEP(e);

			break;

		case CTRL('O'):		/* open a new line - cursor stays where it is */
			if (break_line(e, &line, &col, FALSE) != NO_ERROR)
				BEEP(e);

			break;

		case CTRL('K'):		/* kill rest of line, or line-break if at the end */
			if (col + e->eleft < strlen(e->editbuf[line]))
			{
				strcpy(savebuf, e->editbuf[line] + col + e->eleft);
				e->editbuf[line][col + e->eleft] = '\0';
				display_text(e, "\033[K", CSTR);
				continue;
			}

			if (join_line(e, line, col) != NO_ERROR)
				BEEP(e);

			break;

		case CTRL('U'):		/* kill entire line */
			strcpy(savebuf, e->editbuf[line]);
			e->editbuf[line][0] = '\0';
			col = 0;
			display_text(e, "\r\033[K", CSTR);
			cprintf(e, "%*d: ", NUMWIDTH - 2, line + LINE_NUM_START);

			if (e->eleft)
			{
				e->eleft = 0;
				redraw_edit_buf(e, line, col);
			}

			break;

		case CTRL('Y'):		/* insert contents of savebuf at cursor */
			i = strlen(savebuf);
			j = strlen(e->editbuf[line]);

			if (i + j < STR_SIZE - 1)
			{
				memmove(e->editbuf[line] + col + i, e->editbuf[line] + col,
						j - col);
				strncpy(e->editbuf[line] + col, savebuf, i);
				e->editbuf[line][j + i] = '\0';
				col += i;

				if (col >= e->eleft + WIDTH(e) - 1 ||
						i > WIDTH(e) - 1 - col - e->eleft)
				{
					redraw_edit_buf(e, line, col);
				}
				else
				{
					cprintf(e, "\033[%d@", i);		/* insert some blanks */
					display_text(e, savebuf + e->eleft, i);
				}
			}
			else
				BEEP(e);

			break;

		case CTRL('N'):		/* next line */
		cursor_down:
			if (line < e->elines - 1)
			{
				line++;

				if (col > strlen(e->editbuf[line]))
					col = strlen(e->editbuf[line]);

				if (line >= e->etop + e->lines)
					scroll_up(e, line, col);
				else
					move(e, line, col);
			}
			else
				BEEP(e);

			break;

		case CTRL('P'):		/* previous line */
		cursor_up:
			if (line > 0)
			{
				line--;

				if (col > strlen(e->editbuf[line]))
					col = strlen(e->editbuf[line]);

				if (line < e->etop)
					scroll_down(e, line, col);
				else
					move(e, line, col);	/* restore cursor */
			}
			else
				BEEP(e);

			break;

		case CTRL('X'):		/* page forward [non-standard] */
			if (e->etop + e->lines < e->elines)
			{
				e->etop += e->lines - 1;
				e->eleft = 0;
				line = e->etop;
				col = 0;
				redraw_edit_buf(e, line, col);
			}

			break;

		case CTRL('Z'):		/* page backward [non-standard] */
			if (e->etop > 0)
			{
				e->etop -= e->lines - 1;

				if (e->etop < 0)
					e->etop = 0;

				e->eleft = 0;
				line = e->etop;
				col = 0;
				redraw_edit_buf(e, line, col);
			}

			break;

		case CTRL('L'):		/* redraw the screen */
			redraw_edit_buf(e, line, col);
			break;

		case CTRL('B'):		/* backward char */
		cursor_left:
			if (col > 0)
			{
				col--;

				if (col < e->eleft)
					redraw_edit_buf(e, line, col);
				else
					display_text(e, "\b", 1);
			}
			else if (line > 0)
			{
				col = strlen(e->editbuf[line - 1]);
				goto cursor_up;
			}
			else
				BEEP(e);

			break;

		case CTRL('F'):		/* forward char */
		cursor_right:
			if (col < strlen(e->editbuf[line]))
			{
				col++;

				if (col >= WIDTH(e) - e->eleft - 1)
					redraw_edit_buf(e, line, col);
				else
					display_text(e, e->editbuf[line] + col - 1, 1);
			}
			else if (line < e->elines - 1)
			{
				col = 0;
				goto cursor_down;
			}
			else
				BEEP(e);

			break;

		case CTRL('A'):		/* beginning of line */
			col = 0;
			move(e, line, col);
			break;

		case CTRL('E'):		/* end of line */
			col = strlen(e->editbuf[line]);
			move(e, line, col);
			break;

		/* kill previous char, or newline if at beginning of a line */
		case '\b':
		case DEL:
			if (col == 0)
			{
				if (line == 0)
				{
					BEEP(e);
					continue;
				}

				/* move the cursor to the end of the previous line */
				line--;
				col = strlen(e->editbuf[line]);

				if (line < e->etop)
					scroll_down(e, line, col);

				move(e, line, col);
			}
			else
			{
				/* just move the cursor back one */
				col--;

				if (col < e->eleft)
					redraw_edit_buf(e, line, col);
				else
					display_text(e, "\b", 1);
			}

			goto delete_char;

		case CTRL('D'):		/* kill next char, or newline if at end of a line */
		delete_char:
			if (col < strlen(e->editbuf[line]))
			{
				memmove(e->editbuf[line] + col, e->editbuf[line] + col + 1,
						STR_SIZE - col);
				display_text(e, "\033[1P", CSTR);		/* delete 1 character */

				if (strlen(e->editbuf[line]) >= WIDTH(e) - 1)
					redraw_edit_buf(e, line, col);
			}
			else if (join_line(e, line, col) != NO_ERROR)
				BEEP(e);

			break;

		case ESC:		/* multi-char escape-sequence, from user or keyboard */
			switch (get_key(e))
			{
			case 'F':		/* move forward word */
			case 'f':
				for (; col < strlen(e->editbuf[line]) &&
						isspace(e->editbuf[line][col]); col++)
					;

				for (; col < strlen(e->editbuf[line]) &&
						!isspace(e->editbuf[line][col]); col++)
					;

				if (col >= e->eleft + WIDTH(e) + 1)
					redraw_edit_buf(e, line, col);
				else
					move(e, line, col);

				break;

			case 'B':		/* move backward word */
			case 'b':
				for (; col > 0 && isspace(e->editbuf[line][col]); col--)
					;

				for (; col > 0 && !isspace(e->editbuf[line][col]); col--)
					;

				if (col < e->eleft)
					redraw_edit_buf(e, line, col);
				else
					move(e, line, col);

				break;

			case 'D':		/* delete following word into savebuf */
			case 'd':
			case CTRL('D'):
				for (i = col; i < strlen(e->editbuf[line]) &&
						isspace(e->editbuf[line][i]); i++)
					;

				for (; i < strlen(e->editbuf[line]) &&
						!isspace(e->editbuf[line][i]); i++)
					;

				strncpy(savebuf, e->editbuf[line] + col, i - col);
				memmove(e->editbuf[line] + col, e->editbuf[line] + i,
						strlen(e->editbuf[line] + i) + 1);

				if (strlen(e->editbuf[line]) >= WIDTH(e) - 1)
					redraw_edit_buf(e, line, col);
				else
					cprintf(e, "\033[%dP", i - col);

				break;

			case 'H':		/* delete previous word into savebuf */
			case 'h':
			case '\b':
			case DEL:
				i = col;

				for (; col > 0 && isspace(e->editbuf[line][col - 1]); col--)
					;

				for (; col > 0 && !isspace(e->editbuf[line][col - 1]); col--)
					;

				strncpy(savebuf, e->editbuf[line] + col, i - col);
				memmove(e->editbuf[line] + col, e->editbuf[line] + i,
						strlen(e->editbuf[line] + i) + 1);
				MOVE(e, line, col);

				if (col < e->eleft)
					redraw_edit_buf(e, line, col);
				else
					cprintf(e, "\033[%dP", i - col);

				break;

			case 'O':		/* ANSI cursor-motion */
				switch (get_key(e))
				{
				case 'A':	/* cursor up */
					goto cursor_up;

				case 'B':	/* cursor down */
					goto cursor_down;

				case 'C':	/* cursor right */
					goto cursor_right;

				case 'D':	/* cursor left */
					goto cursor_left;

				case 'H':	/* home */
					line = 0;
					col = 0;
					break;

				case 'K':	/* end */
					line = e->lines - 1;
					col = 0;
					break;

				case 'P':	/* delete next char */
					goto delete_char;

				default:
					BEEP(e);
					break;
				}

			default:
				BEEP(e);
				break;
			}

			break;

		default:
			if (isprint(c) && strlen(e->editbuf[line]) < STR_SIZE - 2)
			{
				if (e->editbuf[line][col] != '\0')
				{
					memmove(e->editbuf[line] + col + 1, e->editbuf[line] + col,
							STR_SIZE - col);
					e->editbuf[line][col++] = c;

					if (col < e->eleft + WIDTH(e) - 1)
						cprintf(e, "\033[1@%c", c);		/* insert a character */
					else
						move(e, line, col);
				}
				else
				{
					e->editbuf[line][col++] = c;
					e->editbuf[line][col] = '\0';

					if (col < e->eleft + WIDTH(e) - 1)
						display_text(e, &c, 1);			/* draw the character */
					else
						move(e, line, col);
				}
			}
			else
				BEEP(e);

			break;
		}

		/* wrap the cursor to the next line, if necessary */
		if (col >= STR_SIZE - 1)
		{
			col = 0;
			line++;

			if (line - e->etop < e->lines)
				move(e, line, col);
			else
				scroll_up(e, line, col);
		}

		if (line - e->etop > e->lines - 1)
		{
			line = 0;
			move(e, 0, 0);
		}

		/* sanity check in case cursor was moved and we have a bug */
		if (line >= e->elines)
			e->elines = line + 1;
	}

	MOVENUM(e, e->etop + e->lines - 1);
	display_text(e, "\033[K", CSTR);	/* move to last line and clear it */
	free(savebuf);
	e->linesperpage = savelpp;
	return NO_ERROR;
}

C(f_nvedit)			/* nvedit (--) */
{
	Retcode ret;
	Byte *str;
	Int slen;

	/* first initialize the editor buffer if needed */
	if (e->editbuf == NULL)
	{
		ret = init_edit(e);

		if (ret != NO_ERROR)
			return ret;
	}

	/* reload the buffer only if the editor is completely empty */
	if (e->elines < 0 || (e->editbuf[0][0] == '\0' && (e->elines == 0 ||
			(e->elines == 1 && e->editbuf[1][0] == '\0'))))
	{
		ret = get_config_val(e, "nvramrc", CSTR, &str, &slen);

		if (ret == NO_ERROR)
			ret = fill_edit_buf(e, str, slen);

		if (ret != NO_ERROR)
			return ret;
	}

	return do_edit(e);
}

C(f_nvstore)			/* nvstore (--) */
{
	Retcode ret;

	if (e->editbuf == NULL || e->elines < 0)		/* nothing to store */
		return NO_ERROR;

	ret = save_edit_buf(e);

	if (ret != NO_ERROR)
		return ret;

	ret = save_config(e, "nvramrc", CSTR, e->fbrk, CSTR, TRUE);

	/* mark the edit buffer as discardable but don't erase it yet */
	if (ret == NO_ERROR)
		e->elines = -e->elines - 1;

	return ret;
}

C(f_nvquit)				/* nvquit (--) */
{
	Int len;
	Byte line[STR_SIZE];

	display_text(e,
			"Discard contents of nvedit buffer (not recoverable)? (yes/no): ",
			CSTR);
	expect_line(e, line, sizeof line - 2, &len, FALSE);

	if (len && line[len - 1] == '\n')
		len--;

	if (compare_strs(line, len, "yes", CSTR))
	{
		if (e->editbuf == NULL)
		{
			Retcode ret = init_edit(e);

			if (ret != NO_ERROR)
				return ret;
		}

		/* mark the edit buffer as discardable but don't erase it yet */
		e->elines = -e->elines - 1;
		display_text(e, "nvedit buffer contents discarded\r\n", CSTR);
	}

	return NO_ERROR;
}

C(f_nvrecover)			/* nvrecover (--) */
{
	if (e->editbuf == NULL || e->elines >= 0)
	{
		cprintf(e, "Sorry, unable to recover nvedit buffer.\n");
		return NO_ERROR;
	}

	/* restore e->lines as buffer has not been erased yet */
	e->elines = -e->elines - 1;
	return do_edit(e);
}

C(f_nvrun)				/* nvrun (--) */
{
	Byte *str;	
	Retcode ret = save_edit_buf(e);

	if (ret != NO_ERROR)
		return ret;

	str = cstrdup(e->fbrk);
	e->security = SM_NVRAMRC;
	ret = interp_text(e, str + 1, CSTR);
	e->security = security_mode(e);
	free(str);
	return ret;
}


static Retcode
edit_alias(Environ *e, Byte *name, Int namelen, Byte *dev,
		Int devlen, Bool del)
{
	Retcode ret;
	Int line, i;
	Byte *str;

	setstrlen(&name, &namelen);
	setstrlen(&dev, &devlen);

	/* first initialize the editor buffer if needed */
	if (e->editbuf == NULL)
	{
		ret = init_edit(e);

		if (ret != NO_ERROR)
			return ret;
	}

	/* edit buffer only if it is completely empty, saved, or discarded */
	if (!(e->elines < 0 || (e->editbuf[0][0] == '\0' && (e->elines == 0 ||
			(e->elines == 1 && e->editbuf[1][0] == '\0')))))
	{
		cprintf(e, "nvramrc script still needs to be stored or discarded\n");
		return NO_ERROR;
	}

	ret = get_config_val(e, "nvramrc", CSTR, &str, &i);

	if (ret == NO_ERROR)
		ret = fill_edit_buf(e, str, i);

	if (ret != NO_ERROR)
		return ret;

	/* find a line matching "[ \t]*devalias[ \t][ \t]*name[ \t]..." in script */
	for (line = 0; line < e->elines; line++)
	{
		str = e->editbuf[line];

		/* skip white space */
		while (*str && isspace(*str))
			str++;

		/* if str is less than 8, it will have a null in it, so this is OK */
		if (!compare_strs(str, 8, "devalias", 8))
			continue;

		str += 8;

		/* better have whitespace here */
		if (!isspace(*str))
			continue;

		/* skip white space */
		while (*str && isspace(*str))
			str++;

		/* if str is less than namelen, it will have a null, so this is OK */
		if (!compare_strs(str, namelen, name, namelen))
			continue;

		/* if whitespace follows the alias name, we found a matching line! */
		if (isspace(str[namelen]))
			break;
	}

	/* no matching line - insert new line at the top */
	if (line >= e->elines)
	{
		/* deleting the line - already gone */
		if (del)
			return NO_ERROR;

		/* make sure we have space */
		ret = grow_edit_buf(e, e->elines + 1);

		if (ret != NO_ERROR)
			return ret;

		if (e->elines > 1)
		{
			/* shuffle all lines down by one */
			str = e->editbuf[e->elines];

			for (i = e->elines; i > 0; i--)
				e->editbuf[i] = e->editbuf[i - 1];

			e->editbuf[0] = str;
			*str = '\0';
		}

		e->elines++;
		line = 0;
	}

	/* found a line but it has to be deleted */
	if (del)
	{
		/* move all the lines up by one to erase this line */
		str = e->editbuf[line];

		for (i = line; i < e->elines - 1; i++)
			e->editbuf[i] = e->editbuf[i + 1];

		e->editbuf[e->elines - 1] = str;
		*str = '\0';
		e->elines--;
	}
	else
		/* put into the edit line as a devalias command */
		bprintf(e->editbuf[line], "devalias %S %S", name, namelen, dev, devlen);

	/* make the new alias */
	ret = make_devalias(e, name, namelen, dev, devlen);

	/* store the new edited nvramrc script */
	if (ret == NO_ERROR)
		ret = f_nvstore(e);

	/* and set use-nvramrc? if needed */
	if (ret == NO_ERROR && !get_config_bool(e, "use-nvramrc?", CSTR))
		save_config(e, "use-nvramrc?", CSTR, "true", CSTR, TRUE);

	return ret;
}


C(f_dnvalias)			/* $nvalias (name-str name-len dev-str dev-len --) */
{
	Byte *name, *dev;
	Int namelen, devlen;

	IFCKSP(e, 4, 0);
	POP(e, devlen);
	POPT(e, dev, Byte*);
	POP(e, namelen);
	POPT(e, name, Byte*);
	return edit_alias(e, name, namelen, dev, devlen, FALSE);
}

C(f_nvalias)		/* nvalias ("{alias-name}< >{device-specifier}<cr>" --) */
{
	Byte *alias, *device;

	alias = parse_word(e);

	if (*alias)
	{
		device = parse_line(e);

		/* do not check for valid device as it would operate differently
		   than devalias command - sigh */

		if (*device)
			return edit_alias(e, alias, PSTR, device, PSTR, FALSE);
	}

	return E_EMPTY_STRING;
}

C(f_dnvunalias)			/* $nvunalias (name-str name-len --) */
{
	Byte *name;
	Int namelen;

	IFCKSP(e, 2, 0);
	POP(e, namelen);
	POPT(e, name, Byte*);
	return edit_alias(e, name, namelen, NULL, 0, TRUE);
}

C(f_nvunalias)			/* nvunalias ("alias-name< >" --) */
{
	Byte *alias;

	alias = parse_word(e);

	if (*alias)
		return edit_alias(e, alias, PSTR, NULL, 0, TRUE);

	return E_EMPTY_STRING;
}



const Initentry init_nvedit[] =
{
	{ "nvedit", f_nvedit, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) enter nvramrc script editor, exit with ^C") },
	{ "nvstore", f_nvstore, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) copy nvedit buffer into nvramrc script") },
	{ "nvquit", f_nvquit, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) discard contents of nvedit buffer") },
	{ "nvrecover", f_nvrecover, INVALID_FCODE, F_NONE, T_FUNC
			HELP("(--) try to recover lost nvramrc contents") },
	{ "nvrun", f_nvrun, INVALID_FCODE, F_NONE, T_FUNC
			HELP("execute contents of nvedit buffer") },

	{ "nvalias",       f_nvalias,       INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"{alias-name}< >{device-specifier}<cr>\" --)\n" \
					"\tcreate nonvolatile device alias; edit nvramrc script") },
	{ "$nvalias",      f_dnvalias,      INVALID_FCODE, F_NONE, T_FUNC
			HELP("(name-str name-len dev-str dev-len --)\n" \
					"\tcreate nonvolatile device alias; edit nvramrc script") },
	{ "nvunalias",     f_nvunalias,     INVALID_FCODE, F_NONE, T_FUNC
			HELP("(\"alias-name< >\" --)\n" \
					"\tdelete nonvolatile device alias; edit nvramrc script") },
	{ "$nvunalias",    f_dnvunalias,    INVALID_FCODE, F_NONE, T_FUNC
			HELP("(name-str name-len --)\n" \
					"\tdelete nonvolatile device alias; edit nvramrc script") },

	{ NULL, NULL }
};
