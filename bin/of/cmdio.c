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

/* All command-line I/O routines, parsing routines, etc. should be here. */

/*#define DEBUG	*/

#include "stdlib.h"
#include "stdarg.h"
#include "defs.h"

#ifndef EOF
#define EOF -1
#endif

#define BEEP(e) 		display_text(e, "\007", 1)


/* this is needed so that we do not get stuck in recursive calls while
   printing debug output */
static Retcode
safe_execute_method(Environ *e, Instance *inst, Byte *str, Int len)
{
	enum Debug_mode debug;
	Bool iscompiling;
	Retcode ret;
	
	debug = e->debug;
	iscompiling = e->iscompiling;
	e->debug = DB_NONE;
	e->iscompiling = FALSE;		
	ret = execute_method_name(e, inst, str, len);
	e->debug = debug;
	e->iscompiling = iscompiling;
	return ret;
}

/* output to a line of text to the screen */
static Int
output_line(Environ *e, Byte *text, Int len)
{
	Instance *screen = e ? (Instance *)(uPtr)e->screen : (Instance *)NULL;
	Int i, actual;

	setstrlen(&text, &len);

	if (e)
	{
		for (i = 0; i < len; i++)
		{
			if (text[i] == '\r')
				e->outcol = 0;
			else if (text[i] == ESC && i + 1 < len && text[i + 1] == '[')
			{
				while (!isalpha(text[i]) && text[i] != '@' && i < len)
					i++;
			}
			else if (text[i] == '\t')
			{
				e->outcol += 8 - e->outcol % 8;
				
				if (e->outcol > e->cols)
					e->outcol = e->cols - 1;
			}
			else if (text[i] == '\b')
			{
				if (e->outcol)
					e->outcol--;
			}
			else if (text[i] != '\n' && (!e->cols || e->outcol < e->cols))
				e->outcol++;
			else
			{
				e->linesout++;
				e->outcol = 0;
				break;
			}
		}

		if (i < len)
			len = i + (text[i] == '\n');
	}

	i = len;

	while (i > 0)
	{
		if (screen)
		{
			PUSHP(e, text);
			PUSH(e, i);
			
			if (safe_execute_method(e, screen, "write", CSTR) == NO_ERROR)
				POP(e, actual);
			else
				actual = failsafe_write(text, i);
		}
		else
			actual = failsafe_write(text, i);

		DPRINTF(("output_lines: write %p, %d -> %d\n", text, i, actual));

		if (actual < 0)
			return actual;

		text += actual;
		i -= actual;
	}

	return len;
}

/* output to the screen - no pagination */
static void
output_text(Environ *e, Byte *text, Int len)
{
	Int actual;

	setstrlen(&text, &len);

	while (len > 0)
	{
		actual = output_line(e, text, len);

		DPRINTF(("output_text: outlines %p, %d -> %d\n", text, len, actual));

		if (actual < 0)
			break;

		text += actual;
		len -= actual;
	}
}


enum ask
{
	ASK_NONE,
	ASK_FLUSH,
	ASK_CAPTURE,
	ASK_NO_PAGING
};
typedef enum ask Ask;

static Ask
do_ask_exit(Environ *e)
{
	Int c;
	Int end = 0;
	Ask ret = ASK_NONE;
	Bool prompt = TRUE;

	if (e->lines <= 0)		/* in case we have not yet been initialized */
		return ASK_NONE;

	/* figure out what to do from our lines/page variable */
	if (e->linesperpage > e->lines || e->linesperpage < -e->lines)
		return ASK_NONE;
	else if (e->linesperpage < 0)
		end = e->lines + e->linesperpage;
	else if (e->linesperpage > 0)
		end = e->linesperpage;
	else	/* linesperpage == 0 */
		prompt = FALSE;

	/* not reached the end of the screen yet, let the user interrupt us */
	if (!prompt || e->linesout < end)
	{
		if (!key_down(e))			/* no key pressed - continue on */
			return ASK_NONE;
		
		c = get_key(e);
		
		/* see if user wants to quit or capture text immediately */
		if (c == 'q' || c == 'Q' || c == CTRL('C') || c == ESC)
		{
			output_text(e, "\r", 1);
			return ASK_FLUSH;
		}
		else if (c == 'f' || c == 'F')
			return ASK_CAPTURE;

		/* force the prompt only if user interrupted the screen */
		prompt = TRUE;
	}
	else if (e->linesout >= end)		/* displayed a screenful */
		prompt = TRUE;

	if (prompt)					/* display the prompt */
	{
		/* if we are not at the beginning of a line, go to the next line */
		if (e->outcol)
			output_text(e, "\r\n", 2);

		output_text(e, "\r\033[7mMore? [<space>,", CSTR);

		if (e->linesperpage)
			output_text(e, "<cr>,c,", CSTR);

		output_text(e, "q,f]\033[0m\033[K", CSTR);
		c = get_key(e);
		output_text(e, "\r\033[K", CSTR);	/* clear the prompt */
		
		if (c == 'q' || c == 'Q' || c == CTRL('C') || c == ESC)		/* quit */
			ret = ASK_FLUSH;
		else if (c == 'c' || c == 'C')	/* continue output without pagination */
			ret = ASK_NO_PAGING;
		else if (c == 'f' || c == 'F')	/* flush output */
			ret = ASK_CAPTURE;
		else
		{
			if (c == '\n' || c == '\r')
				e->linesout = end - 1;	/* display one more line next time */
			else
				e->linesout = 1;		/* display one more screen */
			
			ret = ASK_NONE;
		}
	}
	
	return ret;
}

/* perform pagination as appropriate */
Bool
ask_exit(Environ *e)
{
	Ask ask;
	Int savelpp = e->linesperpage;

	if (e->flush_output == FO_CONTINUE)
		e->linesperpage = 0;

	ask = do_ask_exit(e);
	e->linesperpage = savelpp;

	switch (ask)
	{
	case ASK_CAPTURE:
		output_text(e, "Flushing to capture buffer...\n", CSTR);
		e->flush_output = FO_CAPTURE;
		break;

	case ASK_NO_PAGING:
		e->flush_output = FO_CONTINUE;
		break;

	case ASK_FLUSH:
		e->flush_output = FO_FLUSH;
		return TRUE;

	default:
		if (e->flush_output != FO_CONTINUE)
			e->flush_output = FO_NONE;

		break;
	}

	return FALSE;
}

/* output to the screen, paginating output as desired */
void
display_text(Environ *e, Byte *text, Int len)
{
	setstrlen(&text, &len);

	if (e == NULL)
	{
		failsafe_write(text, len);	/* maybe it'll work - maybe it won't */
		return;
	}

	/* output a line at a time */
	while (len > 0)
	{
		Int actual;

		if (e->flush_output == FO_FLUSH || len == 0)
			return;

		if (e->flush_output == FO_CAPTURE)
		{
			while (len)
			{
				/* append this text to the capture ring buffer */
				Int l;

				l = CAPTURE_BUF_SIZE - e->capture_tail;

				if (l > len)
					l = len;

				memcpy(e->capture_buf + e->capture_tail, text, l);

				if (e->capture_tail + l >= e->capture_head &&
						e->capture_tail < e->capture_head)
					e->capture_head = e->capture_tail + l + 1;

				if (e->capture_head >= CAPTURE_BUF_SIZE)
					e->capture_head -= CAPTURE_BUF_SIZE;

				e->capture_tail += l;

				if (e->capture_tail >= CAPTURE_BUF_SIZE)
					e->capture_tail -= CAPTURE_BUF_SIZE;

				len -= l;
				text += l;
			}

			return;
		}

		actual = output_line(e, text, len);

		DPRINTF(("display_text: outlines %p, %d -> %d\n", text, len, actual));

		if (actual < 0)
			break;

		text += actual;
		len -= actual;

		if (e->paginate || e->debug != DB_NONE)
			(void)ask_exit(e);
	}
}

void
cprintf(Environ *e, const char *fmt, ...)
{
	va_list args;
	Byte *s1, *s2;
	static Byte buf1[2048], buf2[2048];
	extern int vbprintf(char *, const char *, va_list);

	va_start(args, fmt);
	vbprintf(buf1, fmt, args);
	va_end(args);

	/* convert all newlines to carriage-return-newline pairs */
	s1 = buf1;
	s2 = buf2;

	while (*s1)
	{
		if (*s1 == '\n')
			*s2++ = '\r';
		
		*s2++ = *s1++;
	}

	*s2 = '\0';
	display_text(e, buf2, CSTR);
}


/* return FTRUE if a key is down, else FFALSE */
int
key_down(Environ *e)
{
	Instance *keyboard;

	/* already have something in keybuf? */
	if (e->keyloc < e->keylen)
		return FTRUE;

	keyboard = (Instance*)(uPtr)e->keyboard;

	if (e && keyboard)
	{
	#ifdef STANDALONE
		/* non-standard hack to support pipes on standalone Unix hosts */
		if (safe_execute_method(e, keyboard, "is-tty", CSTR) == NO_ERROR)
		{
			Int istty;

			POP(e, istty);

			if (!istty)
				return FFALSE;
		}
	#endif

		PUSHP(e, e->keybuf);
		PUSH(e, sizeof e->keybuf);
		
		if (safe_execute_method(e, keyboard, "read", CSTR) == NO_ERROR)
			POP(e, e->keylen);
		else
			e->keylen = failsafe_read(e->keybuf, sizeof e->keybuf);
	}
	else
		e->keylen = failsafe_read(e->keybuf, sizeof e->keybuf);
	
	e->keyloc = 0;
	return (e->keylen > 0) ? FTRUE : FFALSE;
}

/* return a character from the keyboard */
int
get_key(Environ *e)
{
	Instance *keyboard;

	/* see if we have anything in the input buffer */
	if (e->keyloc < e->keylen)
		return e->keybuf[e->keyloc++];

	keyboard = (Instance*)(uPtr)e->keyboard;

	while (1)
	{
		if (e && keyboard)
		{
			PUSHP(e, e->keybuf);
			PUSH(e, sizeof e->keybuf);

			if (safe_execute_method(e, keyboard, "read", CSTR) == NO_ERROR)
				POP(e, e->keylen);
			else
				e->keylen = failsafe_read(e->keybuf, sizeof e->keybuf);
		}
		else
			e->keylen = failsafe_read(e->keybuf, sizeof e->keybuf);
		
		e->keyloc = 0;

		if (e->keylen > 0)
			return e->keybuf[e->keyloc++];

		/* input devices may return -2 if no input is pending */
		/* the read call should always be non blocking */
		/* ref. Annex A, "serial" */
		if (e->keylen != -2 && e->keylen != 0)
			break;
	}

	return EOF;
}

/* perform command-line matching in a table */
static Int
match_table(Environ *e, Table *tab, Byte *word, Byte *match, Bool show,
		Int *col, Int cols)
{
	Int count = 0;
	Entry *ent;
	Byte *n, *m;
	Int len = strlen(word);
	Int i, l;
	
	for (ent = tab->list; ent; ent = ent->link)
	{
		if (ent->name == NULL || (uByte)ent->name[0] < len)
			continue;
		
		if (compare_strs(ent->name + 1, len, word, len))
		{
			count++;
			
			if (show)
			{
				l = (uByte)ent->name[0];
				
				if (l + *col >= cols)
				{
					cprintf(e, "\n");
					*col = 0;
				}
				
				cprintf(e, "%P", ent->name);
				i = 16 - (l % 16);
				
				if (*col + l + i >= cols)
				{
					cprintf(e, "\n");
					*col = 0;
				}
				else
				{
					*col += l + i;
				
					while (i-- > 0)
						cprintf(e, " ");
				}
			}
			else if (*match == '\0')
				strcpy(match, ent->name + 1);
			else
			{
				m = match + len;
				n = ent->name + 1 + len;
				
				while (*n && *m && *m == *n)
					m++, n++;
				
				*m = '\0';
			}
		}
	}
	
	return count;
}

/* complete a word on the command line or show best matches */
static void
complete_word(Environ *e, Bool show, Byte *addr, Int len, Int *col,
		Int left, Int cols)
{
	Instance *inst = (Instance*)(uPtr)e->currinst;
	Int count = 0, mcol = 0;
	Int c, i, j;
	Byte *s;
	Byte word[STR_SIZE];
	Byte match[STR_SIZE];
	
	/* first find the start of the word */
	for (c = *col; c > 0 && !isspace(addr[c - 1]); c--)
		;
	
	/* now copy it into our temp buf */
	for (s = word; c < *col; c++)
		*s++ = addr[c];
	
	*s = '\0';
	*match = '\0';

	/* no word prefix - do not display every word in the system */
	if (*word == '\0')
	{
		BEEP(e);
		return;
	}
	
	/* find the best matches in the current lookup tables */
	if (inst && inst->dict)
		count += match_table(e, inst->dict, word, match, show, &mcol, cols);
	
	if (e->currpkg && e->currpkg->dict)
		count += match_table(e, e->currpkg->dict, word, match, show, &mcol, cols);
	
	if (e->names)
		count += match_table(e, e->names, word, match, show, &mcol, cols);
	
	if (show)
	{
		if (mcol > 0)
			cprintf(e, "\n");
	}
	else		/* complete the word to the longest match */
	{
		if (count == 1)		/* only one word matched */
			strcat(match, " ");
		
		i = strlen(match) - strlen(word);
		j = strlen(addr);

		if (count > 0 && i > 0 && i + j < len - 1)
		{
			memmove(addr + *col + i, addr + *col, j - *col);
			strncpy(addr + *col, match + strlen(word), i);
			addr[j + i] = '\0';
			*col += i;

			if (*col < cols - left - 2)
			{
				Int savecols = e->cols;
				
				e->cols = cols;
				cprintf(e, "\033[%d@", i);		/* insert some blanks */
				output_text(e, match + strlen(word), i);
				e->cols = savecols;
			}
			
			if (count > 1)
				BEEP(e);
		}
		else
			BEEP(e);
	}
}

#ifndef OK_PROMPT
#define OK_PROMPT "ok "
#endif

#ifndef COMPILE_PROMPT
#define COMPILE_PROMPT "> "
#endif

#ifndef SECURE_PROMPT
#define SECURE_PROMPT "SECURE> "
#endif

/* return column number after displaying prompt for command-line editing */
static Int
prompt(Environ *e)
{
	enum Debug_mode debug = e->debug;
	Bool iscompiling = e->iscompiling;
	Int c = e->outcol;
	
	e->debug = DB_NONE;
	e->iscompiling = FALSE;		

	if (e->showstack)
		display_stack(e);

	/* display one of our standard Forth prompts */
	if (e->security != SM_NONE && !e->got_password)
		output_text(e, SECURE_PROMPT, CSTR);
	else if (iscompiling)
		output_text(e, COMPILE_PROMPT, CSTR);
	else
	{
		/* run defer word to display custom prompt as per OF spec */
		(void)execute_word(e, "status");

		if (e->outcol < c)
			c = e->outcol;

#ifdef CUSTOM_PROMPT
		cprintf(e, CUSTOM_PROMPT);	/* additional user-defined custom prompt */

		if (e->outcol < c)
			c = e->outcol;
#endif

		output_text(e, OK_PROMPT, CSTR);
	}

	e->debug = debug;
	e->iscompiling = iscompiling;
	return e->outcol - c;
}

#ifdef SUN_COMPATIBILITY

/* This version of edit_cmd_line uses as little as possible in the way
   of escape-sequences so that it will work under Sun's CommandTool.
 */

static void
out_nchars(Environ *e, Int c, Int num)
{
	int n;
	Byte buf[STR_SIZE];

	for (; num >= 0; num -= STR_SIZE)
	{
		n = (num < STR_SIZE) ? num : STR_SIZE;
		memset(buf, c, n);
		output_text(e, buf, n);
	}
}

static void
edit_cmd_line(Environ *e, Byte *addr, Int len, Int *elen)
{
	Int c = ' ';
	Int prevc = '\0';
	Int i, j;
	Int col = 0;
	Int startcol = 0;
	Int hist = -1;
	Byte *s;
	Int savelpp = e->linesperpage;
	Int savecols = e->cols;	/* turn off left-to-right scrolling for Sun */

	DPRINTF(("edit_cmd_line: &e=%p e=%p addr=%p len=%d elen=%p\n",
			&e, e, addr, len, elen));
	*addr = '\0';
	e->cols = STR_SIZE;
	e->linesout = 0;		/* reset the line number whenever we display "ok" */
	e->linesperpage = e->lines + 1;		/* shut off pagination */
	
	/* do not overrun our yank buffer */
	if (len > STR_SIZE)
		len = STR_SIZE;

	/* if we are not at the beginning of a line, make it so */
	if (e->outcol)
		output_text(e, "\r\n", 2);

	startcol = prompt(e);

	for (; c != EOF && c != '\r' && c != '\n'; prevc = c)
	{
		c = get_key(e);
		DPRINTF(("edit_cmd_line: &e=%p e=%p c=%#x\n", &e, e, c));

		switch (c)
		{
		case '\n':			/* new line - break line here and move cursor */
		case '\r':
			col = 0;
			output_text(e, "\r\n", 2);		/* move cursor */
			c = '\n';
			break;
		
		case CTRL('K'):		/* kill rest of line, or line-break if at the end */
			strcpy(e->hyankbuf, addr + col);
			addr[col] = '\0';
			output_text(e, "\033[K", CSTR);
			continue;

		case CTRL('U'):		/* kill entire line */
			strcpy(e->hyankbuf, addr);
			addr[0] = '\0';
			col = 0;
			output_text(e, "\r\033[K", CSTR);
			startcol = prompt(e);
			break;
		
		case CTRL('Y'):		/* insert contents of savebuf at cursor */
			i = strlen(e->hyankbuf);
			j = strlen(addr);

			if (i + j < len - 1)
			{
				memmove(addr + col + i, addr + col, j - col);
				strncpy(addr + col, e->hyankbuf, i);
				addr[j + i] = '\0';
				col += i;
				goto redraw_line;
			}
			else
				BEEP(e);

			break;
		
		case CTRL('P'):		/* previous cmd */
		cursor_up:
			if (hist < MAX_CMD_HISTORY - 1 && e->history[hist + 1])
			{
				hist++;
				col = *(uByte*)e->history[hist];
				strcpy(addr, e->history[hist] + 1);

				if (col >= e->cols - startcol - 1)
					col = 0;

				goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('N'):		/* next cmd */
		cursor_down:
			if (hist > 0)
			{
				hist--;
				col = *(uByte*)e->history[hist];
				strcpy(addr, e->history[hist] + 1);

				if (col >= e->cols - startcol - 1)
					col = 0;

				goto redraw_line;
			}
			else if (hist == 0)
			{
				hist = -1;
				col = 0;
				*addr = '\0';
				goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('L'):		/* display entire command history */
			cprintf(e, "\n");
			j = LINE_NUM_START;

			for (i = MAX_CMD_HISTORY - 1; i >= 0; i--)
				if (e->history[i])
					cprintf(e, "%3d: %P\n", j++, e->history[i]);

			goto redraw_line;
			
		case CTRL('X'):		/* redraw the line [non-standard] */
		redraw_line:
			out_nchars(e, '\b', e->outcol);
			output_text(e, "\r\033[K", CSTR);
			startcol = prompt(e);
			i = strlen(addr);

			if (i >= e->cols - startcol)
				i = e->cols - startcol - 1;

			if (i > 0)
				output_text(e, addr, i);

			out_nchars(e, '\b', i - col);
			break;

		case CTRL('B'):		/* backward char */
		cursor_left:
			if (col > 0)
			{
				col--;
				output_text(e, "\b", 1);
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('F'):		/* forward char */
		cursor_right:
			if (col < strlen(addr))
			{
				output_text(e, addr + col, 1);
				col++;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('A'):		/* beginning of line */
		beginning_of_line:
			out_nchars(e, '\b', col);
			col = 0;
			break;
		
		case CTRL('E'):		/* end of line */
		end_of_line:
			i = col;
			col = strlen(addr);
			output_text(e, addr + i, col - i);
			break;
		
		case '\b':			/* kill previous char */
		case DEL:
			if (col == 0)
			{
				BEEP(e);
				continue;
			}

			/* just move the cursor back one */
			col--;

			if (col >= 0)
				output_text(e, "\b", 1);

			goto delete_char;
		
		case CTRL('D'):		/* kill next char */
		delete_char:
			if (col < strlen(addr))
			{
				memmove(addr + col, addr + col + 1, len - col);
				i = strlen(addr + col);
				output_text(e, addr + col, i);
				output_text(e, " ", CSTR);
				out_nchars(e, '\b', i + 1);
			}
			else
				BEEP(e);
			
			break;
		
		case ESC:			/* multi-char escape-sequence */
			switch (get_key(e))
			{
			case 'F':		/* move forward word */
			case 'f':
				for (; col < strlen(addr) && isspace(addr[col]); col++)
					output_text(e, addr + col, 1);

				for (; col < strlen(addr) && !isspace(addr[col]); col++)
					output_text(e, addr + col, 1);

				break;

			case 'B':		/* move backward word */
			case 'b':
				for (; col > 0 && isspace(addr[col - 1]); col--)
					output_text(e, "\b", 1);

				for (; col > 0 && !isspace(addr[col - 1]); col--)
					output_text(e, "\b", 1);

				break;

			case 'D':		/* delete following word into yankbuf */
			case 'd':
			case CTRL('D'):
				for (i = col; i < strlen(addr) && isspace(addr[i]); i++)
					;

				for (; i < strlen(addr) && !isspace(addr[i]); i++)
					;

				strncpy(e->hyankbuf, addr + col, i - col);
				memmove(addr + col, addr + i, strlen(addr + i) + 1);
				goto redraw_line;
				break;

			case 'H':		/* delete previous word into yankbuf */
			case 'h':
			case '\b':
			case DEL:
				i = col;

				for (; col > 0 && isspace(addr[col - 1]); col--)
					;

				for (; col > 0 && !isspace(addr[col - 1]); col--)
					;

				strncpy(e->hyankbuf, addr + col, i - col);
				memmove(addr + col, addr + i, strlen(addr + i) + 1);
				goto redraw_line;
				break;
			
			case ESC:	 		/* complete current word - ksh - non-standard */
				goto complete_word;
				
			case '?':		/* list all matches - ksh - non-standard */
			case '/':
				goto list_matches;

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
					goto beginning_of_line;

				case 'K':	/* end */
					goto end_of_line;

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

		case EOF:
			break;
		
		case CTRL(' '):		/* complete the current word */
		complete_word:
			complete_word(e, FALSE, addr, len, &col, startcol, savecols);
			break;
			
		case '\t':		/* tab is smart completion - bash - not standard */
			if (prevc != c)
			{
				complete_word(e, FALSE, addr, len, &col, startcol, savecols);
				break;
			}
			/* FALLTHROUGH */
		
		case CTRL('?'):		/* list all possible matches */
		case CTRL('/'):
		list_matches:
			cprintf(e, "\n");
			complete_word(e, TRUE, addr, len, &col, startcol, savecols);
			goto redraw_line;

		default:
			if (isprint(c) && strlen(addr) < len - 2 - startcol)
			{
				if (addr[col] != '\0')
				{
					memmove(addr + col + 1, addr + col, len - col);
					addr[col++] = c;
					i = strlen(addr + col - 1);
					output_text(e, addr + col - 1, i);
					out_nchars(e, '\b', i - 1);
				}
				else
				{
					addr[col++] = c;
					addr[col] = '\0';
					output_text(e, addr + col - 1, 1);	/* draw it */
				}
			}
			else
				BEEP(e);

			break;
		}
		
		DPRINTF(("edit_cmd_line: addr='%s'\n", addr));
	}

	for (s = addr; *s && isspace(*s); s++)
		;

	if (*s)		/* user typed in something - anything */
	{
		if (e->history[MAX_CMD_HISTORY - 1])
			free(e->history[MAX_CMD_HISTORY - 1]);

		for (i = MAX_CMD_HISTORY - 1; i > 0; i--)
			e->history[i] = e->history[i - 1];

		e->history[0] = cstrdup(addr);
	}

	DPRINTF(("edit_cmd_line: len=%d addr=%p: %s", len, addr, addr ? addr : ""));

	*elen = (c == EOF) ? -1 : strlen(addr);
	e->linesperpage = savelpp;
	e->linesout = 1;		/* reset the line number again for paging */
	e->cols = savecols;
}

#else /* !SUN_COMPATIBILITY */

/* This version of edit_cmd_line performs left-right scrolling of long
   command lines and uses ANSI terminal sequences specified in IEEE-1275.
 */

static void
edit_cmd_line(Environ *e, Byte *addr, Int len, Int *elen)
{
	Int c = ' ';
	Int prevc = '\0';
	Int i, j;
	Int col = 0;
	Int left = 0;
	Int startcol = 0;
	#define MOVE()		cprintf(e, "\r\033[%dC", \
			(col - left + startcol < 0) ? 0 : (col - left + startcol))
	Int hist = -1;
	Byte *s;
	Int savelpp = e->linesperpage;

	DPRINTF(("edit_cmd_line: &e=%p e=%p addr=%p len=%d elen=%p\n",
			&e, e, addr, len, elen));
	*addr = '\0';
	e->linesout = 0;		/* reset the line number whenever we display "ok" */
	e->linesperpage = e->lines + 1;		/* shut off pagination */
	
	/* do not overrun our yank buffer */
	if (len > STR_SIZE)
		len = STR_SIZE;

	/* if we are not at the beginning of a line, make it so */
	if (e->outcol)
		output_text(e, "\r\n", 2);

	startcol = prompt(e);

	for (; c != EOF && c != '\r' && c != '\n'; prevc = c)
	{
		c = get_key(e);
		DPRINTF(("edit_cmd_line: &e=%p e=%p c=%#x\n", &e, e, c));

		switch (c)
		{
		case '\n':			/* new line - break line here and move cursor */
		case '\r':
			col = 0;
			output_text(e, "\r\n", 2);		/* move cursor */
			c = '\n';
			break;
		
		case CTRL('K'):		/* kill rest of line, or line-break if at the end */
			strcpy(e->hyankbuf, addr + col);
			addr[col] = '\0';
			output_text(e, "\033[K", CSTR);
			continue;

		case CTRL('U'):		/* kill entire line */
			strcpy(e->hyankbuf, addr);
			addr[0] = '\0';
			col = 0;
			left = 0;
			output_text(e, "\r\033[K", CSTR);
			startcol = prompt(e);
			break;
		
		case CTRL('Y'):		/* insert contents of savebuf at cursor */
			i = strlen(e->hyankbuf);
			j = strlen(addr);

			if (i + j < len - 1)
			{
				memmove(addr + col + i, addr + col, j - col);
				strncpy(addr + col, e->hyankbuf, i);
				addr[j + i] = '\0';
				col += i;
				goto redraw_line;
			}
			else
				BEEP(e);

			break;
		
		case CTRL('P'):		/* previous cmd */
		cursor_up:
			if (hist < MAX_CMD_HISTORY - 1 && e->history[hist + 1])
			{
				hist++;
				col = *(uByte*)e->history[hist];
				strcpy(addr, e->history[hist] + 1);
				left = 0;

				if (col >= e->cols - startcol - 1)
					col = 0;

				goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('N'):		/* next cmd */
		cursor_down:
			if (hist > 0)
			{
				hist--;
				col = *(uByte*)e->history[hist];
				strcpy(addr, e->history[hist] + 1);
				left = 0;

				if (col >= e->cols - startcol - 1)
					col = 0;

				goto redraw_line;
			}
			else if (hist == 0)
			{
				hist = -1;
				col = 0;
				left = 0;
				*addr = '\0';
				goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('L'):		/* display entire command history */
			cprintf(e, "\n");
			j = LINE_NUM_START;

			for (i = MAX_CMD_HISTORY - 1; i >= 0; i--)
				if (e->history[i])
					cprintf(e, "%3d: %P\n", j++, e->history[i]);

			goto redraw_line;
			
		case CTRL('X'):		/* redraw the line [non-standard] */
		redraw_line:
			output_text(e, "\r\033[K", CSTR);
			startcol = prompt(e);
			i = strlen(addr);

			if (col < left || col - left >= e->cols - startcol - 2)
				left = col - (e->cols - startcol) / 2;

			if (left < 0)
				left = 0;

			i -= left;

			if (i >= e->cols - startcol)
				i = e->cols - startcol - 1;

			if (i > 0)
				output_text(e, addr + left, i);

			if (col < strlen(addr))
				MOVE();

			break;

		case CTRL('B'):		/* backward char */
		cursor_left:
			if (col > 0)
			{
				col--;

				if (col > left)
					output_text(e, "\b", 1);
				else
					goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('F'):		/* forward char */
		cursor_right:
			if (col < strlen(addr))
			{
				col++;

				if (col - left < e->cols - startcol - 1)
					output_text(e, addr + col - 1, 1);
				else
					goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case CTRL('A'):		/* beginning of line */
		beginning_of_line:
			col = 0;
			
			if (left)
				goto redraw_line;

			MOVE();
			break;
		
		case CTRL('E'):		/* end of line */
		end_of_line:
			col = strlen(addr);

			if (col - left >= e->cols - startcol - 2)
				goto redraw_line;

			MOVE();
			break;
		
		case '\b':			/* kill previous char */
		case DEL:
			if (col == 0)
			{
				BEEP(e);
				continue;
			}
				
			/* just move the cursor back one */
			col--;

			if (col >= left)
				output_text(e, "\b", 1);

			goto delete_char;
		
		case CTRL('D'):		/* kill next char */
		delete_char:
			if (col < strlen(addr))
			{
				memmove(addr + col, addr + col + 1, len - col);

				if (col >= left &&
						strlen(addr) - left < e->cols - startcol - 2)
					output_text(e, "\033[1P", 4);	/* delete 1 character */
				else
					goto redraw_line;
			}
			else
				BEEP(e);
			
			break;
		
		case ESC:			/* multi-char escape-sequence */
			switch (get_key(e))
			{
			case 'F':		/* move forward word */
			case 'f':
				for (; col < strlen(addr) && isspace(addr[col]); col++)
					;

				for (; col < strlen(addr) && !isspace(addr[col]); col++)
					;

				if (col - left >= e->cols - startcol - 2)
					goto redraw_line;

				MOVE();
				break;

			case 'B':		/* move backward word */
			case 'b':
				for (; col > 0 && isspace(addr[col - 1]); col--)
					;

				for (; col > 0 && !isspace(addr[col - 1]); col--)
					;

				if (col < left)
					goto redraw_line;

				MOVE();
				break;

			case 'D':		/* delete following word into yankbuf */
			case 'd':
			case CTRL('D'):
				for (i = col; i < strlen(addr) && isspace(addr[i]); i++)
					;

				for (; i < strlen(addr) && !isspace(addr[i]); i++)
					;

				strncpy(e->hyankbuf, addr + col, i - col);
				memmove(addr + col, addr + i, strlen(addr + i) + 1);

				if (strlen(addr + col) >= e->cols - startcol - 2 + left)
					goto redraw_line;

				cprintf(e, "\033[%dP", i - col);
				break;

			case 'H':		/* delete previous word into yankbuf */
			case 'h':
			case '\b':
			case DEL:
				i = col;

				for (; col > 0 && isspace(addr[col - 1]); col--)
					;

				for (; col > 0 && !isspace(addr[col - 1]); col--)
					;

				strncpy(e->hyankbuf, addr + col, i - col);
				memmove(addr + col, addr + i, strlen(addr + i) + 1);

				if (col < left ||
						strlen(addr + col) >= e->cols - startcol - 2 + left)
					goto redraw_line;

				MOVE();
				cprintf(e, "\033[%dP", i - col);
				break;
			
			case ESC:	 		/* complete current word - ksh - non-standard */
				goto complete_word;
				
			case '?':		/* list all matches - ksh - non-standard */
			case '/':
				goto list_matches;

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
					goto beginning_of_line;

				case 'K':	/* end */
					goto end_of_line;

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

		case EOF:
			break;
		
		case CTRL(' '):		/* complete the current word */
		complete_word:
			complete_word(e, FALSE, addr, len, &col, left + startcol, e->cols);

			if (col - left >= e->cols - startcol - 2)
				goto redraw_line;

			break;
			
		case '\t':		/* tab is smart completion - bash - not standard */
			if (prevc != c)
			{
				complete_word(e, FALSE, addr, len, &col,
						left + startcol, e->cols);

				if (col - left >= e->cols - startcol - 2)
					goto redraw_line;

				break;
			}
			/* FALLTHROUGH */
		
		case CTRL('?'):		/* list all possible matches */
		case CTRL('/'):
		list_matches:
			cprintf(e, "\n");
			complete_word(e, TRUE, addr, len, &col, left + startcol, e->cols);
			goto redraw_line;

		default:
			if (isprint(c) && strlen(addr) < len - 2 - startcol)
			{
				if (addr[col] != '\0')
				{
					memmove(addr + col + 1, addr + col, len - col);
					addr[col++] = c;

					if (col - left >= e->cols - startcol - 2)
						goto redraw_line;

					cprintf(e, "\033[1@%c", c);		/* insert a character */
				}
				else
				{
					addr[col++] = c;
					addr[col] = '\0';

					if (col - left >= e->cols - startcol - 2)
						goto redraw_line;

					output_text(e, addr + col - 1, 1);	/* draw it */
				}
			}
			else
				BEEP(e);

			break;
		}
		
		DPRINTF(("edit_cmd_line: addr='%s'\n", addr));
	}

	for (s = addr; *s && isspace(*s); s++)
		;

	if (*s)		/* user typed in something - anything */
	{
		if (e->history[MAX_CMD_HISTORY - 1])
			free(e->history[MAX_CMD_HISTORY - 1]);

		for (i = MAX_CMD_HISTORY - 1; i > 0; i--)
			e->history[i] = e->history[i - 1];

		e->history[0] = cstrdup(addr);
	}

	DPRINTF(("edit_cmd_line: len=%d addr=%p: %s", len, addr, addr ? addr : ""));

	*elen = (c == EOF) ? -1 : strlen(addr);
	e->linesperpage = savelpp;
	e->linesout = 1;		/* reset the line number again for paging */
}

#endif /* !SUN_COMPATIBILITY */

/* return an edited line from the keyboard */
void
expect_line(Environ *e, Byte *addr, Int len, Int *elen, Bool prompt)
{
	int c = ' ';
	Byte b;
	int l;
	Byte *a;
	Cell echo = FTRUE;	/* assume we should echo text as we read it */
	
	if (e->flush_output == FO_CAPTURE && e->capture_head != e->capture_tail)
	{
		if (e->capture_head > e->capture_tail)
		{
			output_text(e, e->capture_buf + e->capture_head,
					CAPTURE_BUF_SIZE - e->capture_head);
			e->capture_head = 0;
		}

		output_text(e, e->capture_buf + e->capture_head,
				e->capture_tail - e->capture_head);
		e->capture_head = e->capture_tail;
	}

	e->flush_output = FO_NONE;		/* do not flush any more output */
	e->linesout = 0;		/* reset the line number whenever we display "ok" */
	l = 0;
	a = addr;

#ifdef STANDALONE
	/* "is-tty" is a non-standard hack to let us run in standalone mode on Unix
	   hosts and let us redirect output, etc. and be a proper Unix tool */
	{
		Instance *keyboard = (Instance*)(uPtr)e->keyboard;

		if (prompt &&
			safe_execute_method(e, keyboard, "is-tty", CSTR) == NO_ERROR)
		POP(e, echo);
	}
#endif

	/* perform command-line editing and history buffer stuff */
	if (prompt && echo)
	{
		edit_cmd_line(e, addr, len, elen);
		return;
	}

	if (e->showstack)
		display_stack(e);

	DPRINTF(("expect_line: &e=%p e=%p addr=%p len=%d elen=%p prompt=%d\n",
			&e, e, addr, len, elen, prompt));
		
	while (c != EOF && c != '\r' && c != '\n')
	{
		c = get_key(e);
		DPRINTF(("expect_line: &e=%p e=%p c=%#x\n", &e, e, c));

		/* perform minimal line-editing */
		switch (c)
		{
		case '\b':		/* back-space */
		case 0177:		/* delete */
			if (l)
			{
				if (echo)
					output_text(e, "\b \b", CSTR);

				a--;
				l--;
			}
			
			continue;
		
		case CTRL('U'):		/* control-U */
			l = 0;
			a = addr;

			if (echo)
				output_text(e, "\r\033[K", CSTR);	/* clear the line */

			continue;
		
		case '\r':	/* convert carriage-return to newline after printing it */
			if (echo)
			{
				b = c;
				output_text(e, &b, 1);
			}

			c = '\n';
			break;
		
		case '\t':		/* convert tab to space */
			c = ' ';
			break;
			
		default:
			break;
		}

		if (echo)
		{
			b = c;
			output_text(e, &b, 1);
		}

		if (c == EOF || c == '\n')
			break;

		if (l < len)
		{
			*a++ = c;
			l++;
		}

		DPRINTF(("expect_line: &e=%p e=%p addr=%p len=%d elen=%p"
				" prompt=%d l=%d c=%#x\n",
				&e, e, addr, len, elen, prompt, l, c));
	}

	DPRINTF(("expect_line: len=%d addr=%p: %s", len, addr, addr ? addr : ""));

	if (c == EOF)
		l = -1;

	*elen = l;
	e->linesout = 1;		/* reset the line number again for paging */
}

/* return an line from the keyboard without echoing it */
void
get_password(Environ *e, Byte *addr, Int len, Int *elen)
{
	int c = ' ';
	Byte b;
	int l;
	Byte *a;
	
	l = 0;
	a = addr;

	DPRINTF(("get_password: &e=%p e=%p addr=%p len=%d elen=%p\n",
			&e, e, addr, len, elen));
		
	while (l < len && c != EOF && c != '\r' && c != '\n')
	{
		c = get_key(e);
		DPRINTF(("get_password: &e=%p e=%p c=%#x\n", &e, e, c));

		/* perform minimal line-editing */
		switch (c)
		{
		case '\b':		/* back-space */
		case 0177:		/* delete */
			if (l)
			{
				output_text(e, "\b \b", CSTR);
				a--;
				l--;
			}
			
			continue;
		
		case CTRL('U'):		/* control-U */
			l = 0;
			a = addr;
			output_text(e, "\r\033[K", CSTR);	/* clear the line */
			continue;
		
		case '\r':			/* carriage-return */
		case '\n':			/* new-line */
			continue;
		
		default:
			break;
		}

		if (c == EOF)
			break;

		b = '*';
		output_text(e, &b, 1);
		*a++ = c;
		l++;
		DPRINTF(("get_password: &e=%p e=%p addr=%p len=%d elen=%p l=%d c=%#x\n",
				&e, e, addr, len, elen, l, c));
	}

	b = '\r';
	output_text(e, &b, 1);
	b = '\n';
	output_text(e, &b, 1);
	DPRINTF(("get_password: len=%d addr=%p: %s", len, addr, addr ? addr : ""));

	if (c == EOF)
		l = -1;

	/* IEEE-1275 mandates no more than 8 chars in a password */
	if (l > 8)
		l = 8;

	*elen = l;
}

Bool
check_password(Environ *e)
{
	Byte *str;
	Int len;
	Byte pass[STR_SIZE];

	if (e->outcol)
		output_text(e, "\r\n", 2);

	cprintf(e, "Enter password: ");
	get_password(e, pass, sizeof pass - 1, &len);
	pass[len] = '\0';
	str = get_nvram(e, "security-password", CSTR);

	if (strcmp(str, pass) == 0)
	{
		cprintf(e, "Password accepted - exiting secure-mode.\n");
		return e->got_password = TRUE;
	}

	len = get_config_int(e, "security-#badlogins", CSTR);
	len++;
	bprintf(pass, "%d", len);
	save_config(e, "security-#badlogins", CSTR, pass, CSTR, TRUE);
	cprintf(e, "Incorrect password - still in secure-mode.\n");
	return e->got_password = FALSE;
}

void
display_char(Environ *e, int c)
{
	uByte b = c;
	
	display_text(e, (Byte*)&b, 1);
}

static Retcode
display_num(Environ *e, Int radix, uCell num, Bool sign, Int width)
{
	Int d;

	if (e->numlen != -1)
		return E_PICNUM_FORMAT;

	if (radix < 2 || radix > 36)	/* sanity check */
		radix = 16;

	e->numptr = e->numbuf + STR_SIZE - 1;
	*e->numptr = '\0';
	e->numlen = 0;

	if (num == 0)
	{
		e->numlen++;
		e->numptr--;
		*e->numptr = '0';
	}
	else
	{
		if (sign && (Cell)num < 0)
			num = -(Cell)num;
		else
			sign = FALSE;

		while (num)
		{
			if (e->numlen >= STR_SIZE)
				return E_PICNUM_OVERFLOW;

			d = num % radix;
			e->numlen++;
			e->numptr--;
			*e->numptr = d + (d < 10 ? '0' : HEX_A - 10);
			num /= radix;
		}

		if (sign)
		{
			e->numlen++;
			e->numptr--;
			*e->numptr = '-';
		}
	}

	/* right justify in field width, if specified */
	for (; width > 0 && width > e->numlen; width--)
		display_char(e, ' ');

	display_text(e, e->numptr, e->numlen);
	e->numlen = -1;
	return NO_ERROR;
}

void
display_stack(Environ *e)
{
	Cell *sp;

	/* e->stack[0] is never used since we always incr-push, then pop-decr */

#ifdef SUN_COMPATIBILITY

	/* display single line of numbers using the current radix */
	if (e->sp == e->stack && !e->showstack)
		cprintf(e, "Empty\n");
	else
	{
		for (sp = e->stack + 1; sp <= e->sp; sp++)
		{
			display_num(e, e->radix, *sp, e->radix == 10 ? TRUE : FALSE, FALSE);
			cprintf(e, " ");
		}

		cprintf(e, "\n");
	}

#else

	cprintf(e, "STACK:");

	if (e->sp == e->stack)
		cprintf(e, "\n");
	else
		for (sp = e->sp; sp > e->stack; sp--)
			cprintf(e, "\t[%#0*Cx] %Cd\n", sizeof *sp * 2, *sp, *sp);

#endif
}

void
display_rstack(Environ *e)
{
	Cell *sp;

	cprintf(e, "RETURN-STACK:\n");

	/* e->rstack[0] is never used since we always incr-push, then pop-decr */
	for (sp = e->rsp; sp > e->rstack; sp--)
		cprintf(e, "\t[%#0*Cx] %Cd\n", sizeof *sp * 2, *sp, *sp);
}

void
display_ftrace(Environ *e)
{
	Entry **code;

	cprintf(e, "FTRACE:\n");

	for (code = e->estack; *code; code++)
		cprintf(e, "\t%s [%#x]\n",
				(*code)->name ? (*code)->name + 1 : "<null>", (*code)->xtok);
}

void
display_exec_stack(Environ *e)
{
	Exec_stack *es;

	cprintf(e, "EXEC-STACK:\n");

	for (es = e->estk; es; es = es->prev)
		cprintf(e, "\t%s [%#x]\n",
				es->code->name ? es->code->name + 1 : "<null>", es->code->xtok);
}

void
display_memory(Environ *e, Byte *saddr, Int len)
{
    #define LINELEN	16
    int i, l;
    Byte *addr;

    for (addr = saddr; (uPtr)addr & 0xF; addr--)
	len++;

    // "%-*s     0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  "
    cprintf(e, "%-8s     0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  "
	    "01234567.89ABCDEF\n", "");

    while (len > 0)
    {
	l = len < LINELEN ? len : LINELEN;
	// cprintf(e, "%0*p  ", sizeof addr * 2, addr);
	cprintf(e, "%08p  ", addr);

	for (i = 0; i < l; i++)
	{
	    if (i == 8)
		cprintf(e, " ");

	    if (addr + i < saddr)
		cprintf(e, "   ");
	    else
		cprintf(e, "%02x ", (uByte)addr[i]);
	}

	for (; i < LINELEN; i++)
	{
	    if (i == 8)
		cprintf(e, " ");

	    if (i >= l)
		cprintf(e, "   ");
	    else
		cprintf(e, "%02x ", (uByte)addr[i]);
	}

	cprintf(e, " ");

	for (i = 0; i < LINELEN; i++)
	{
	    if (i == 8)
		cprintf(e, " ");

	    if (addr + i < saddr || i >= l)
		cprintf(e, " ");
	    else
		cprintf(e, "%c", isprint(addr[i]) ?
			addr[i] : '.');
	}

	for (; i < LINELEN; i++)
	{
	    if (i == 8)
		cprintf(e, " ");

	    cprintf(e, "%c", isprint(addr[i]) ? addr[i] : '.');
	}

	cprintf(e, "\n");
	len -= l;
	addr += l;
    }
}

Retcode
display_number(Environ *e, Int radix, Bool sign, Bool dowidth)
{
	Int width;
	uCell num;
	Retcode ret;

	if (dowidth)
	{
		IFCKSP(e, 1, 0);
		POP(e, width);
	}
	else
		width = 0;

	IFCKSP(e, 1, 0);
	POP(e, num);
	ret = display_num(e, radix, num, sign, width);
	display_char(e, ' ');
	return ret;
}

static Input *
select_input(Environ *e, Bool refill)
{
	Input *i = &e->in;

	/* search for the most recent text input stream */
	while (i->type == I_FCODE)
		i = i->link;

	if (i == NULL || i->type == I_NONE)
	{
		error(E_INPUT_ERROR);
		return NULL;
	}

	if (refill && i->type == I_STREAM)
		while (i->loc >= i->len)
		{
			expect_line(e, i->buf, STR_SIZE - 2, &i->len, TRUE);
			i->loc = 0;

			/* EOF? */
			if (i->len < 0)
				return NULL;

			i->buf[STR_SIZE - 1] = '\0';
		}
	
	return i;
}

Int
read_char(Environ *e)
{
	Input *i = select_input(e, TRUE);
	
	if (i == NULL)
		return '\0';

	if (i->buf[i->loc] == '\0' || i->loc >= i->len)
		return '\0';
	
	return i->buf[i->loc++];
}

/* like read_char() but do not automatically continue past the end of
   an I_STREAM by getting another line
 */
Retcode
parse(Environ *e, Int delim, Bool whsp, Byte **rstr, Int *rlen)
{
	Input *i = select_input(e, FALSE);
	Byte *str;
	Int len;

	*rstr = NULL;
	*rlen = 0;

	if (i == NULL)
		return R_END;

	if (i->loc >= i->len)
		return NO_ERROR;

	str = i->buf + i->loc;

	/* skip white space if so requested */
	if (whsp)
		for (; *str == delim && i->loc < i->len; i->loc++)
			str++;

	/* parse the next word */
	for (len = 0; str[len] != delim && i->loc + len < i->len; len++)
		;

	i->loc += len + 1;
	*rlen = len;
	*rstr = str;
	return NO_ERROR;
}

void
unparse_char(Environ *e)
{
	Input *i = select_input(e, FALSE);

	if (i && i->loc > 0)
		i->loc--;
}

/* return the rest of the current line, skipping leading white-space */
Retcode
parse_line_len(Environ *e, Byte **rstr, Int *rlen)
{
	Input *i = select_input(e, FALSE);
	Byte *str;
	Int len;

	*rstr = NULL;
	*rlen = 0;

	if (i == NULL)
		return R_END;

	if (i->loc >= i->len)
		return NO_ERROR;

	str = i->buf + i->loc;
	len = 0;

	/* skip leading white space */
	while (i->loc < i->len && isblank(*str))
		str++, i->loc++;

	while (i->loc + len < i->len &&
			str[len] != '\n' && str[len] != '\r')
		len++;

	i->loc += len + 1;
	*rstr = str;
	*rlen = len;
	return NO_ERROR;
}

/* semi-compatible to old version - new code should use parse_line_len()
 */
Byte *
parse_line(Environ *e)
{
	Byte *str;
	Int len;
	Retcode ret;
	static Byte buf[STR_SIZE];

	ret = parse_line_len(e, &str, &len);

	if (ret != NO_ERROR)
		return NULL;

	memcpy(buf + 1, str, len);
	buf[len + 1] = '\0';
	*(uByte*)buf = len;
	return buf;
}

/* cannot call parse() above with ' ' as the delimiter -
	need to handle newlines here
 */
Retcode
parse_word_len_refill(Environ *e, Byte **rstr, Int *rlen, Bool refill)
{
	Input *i = select_input(e, refill);
	Byte *str;
	Int len;

	*rstr = NULL;
	*rlen = 0;

	if (i == NULL)
		return R_END;

	if (i->loc >= i->len)
		return NO_ERROR;
	
	str = i->buf + i->loc;

	/* skip leading white space */
	for (; i->loc < i->len && isspace(*str); i->loc++)
		str++;
	
	/* parse the next word */
	for (len = 0; i->loc + len < i->len && !isspace(str[len]); len++)
		;
	
	i->loc += len;

	if (isblank(str[len]))		/* skip space but not newline */
		i->loc++;

	*rstr = str;
	*rlen = len;
	DPRINTF(("parse_word_len: rstr=[%S]\n", *rstr, *rlen));
	return NO_ERROR;
}

Retcode
parse_word_len(Environ *e, Byte **rstr, Int *rlen)
{
	return parse_word_len_refill(e, rstr, rlen, FALSE);
}

/* semi-compatible to old version - new code should use parse_word_len()
 */
Byte *
parse_word(Environ *e)
{
	Byte *str;
	Int len = 0;
	Retcode ret;
	static Byte buf[STR_SIZE];

	ret = parse_word_len(e, &str, &len);

	if (ret != NO_ERROR)
		return NULL;

	memcpy(buf + 1, str, len);
	buf[len + 1] = '\0';
	*(uByte*)buf = len;
	return buf;
}

void
parse_number(Int radix, Byte **str, Int *len, Cell *val,
		Cell *error, Bool dosign)
{
	Cell s = 0;
	Bool err = FALSE;
	Bool sign = FALSE;
	Byte *p = *str;
	Int b;
	Int i = *len;
	
	setstrlen(&p, &i);
	
	if (radix > 36)
	{
		*error = FTRUE;
		return;
	}
	
	if (dosign && *p == '-' && i > 0)
	{
		sign = TRUE;
		i--;
		p++;
	}
	
	for (; i > 0; p++, i--)
	{
		if (*p >= '0' && *p <= '9')
			b = *p - '0';
		else if (*p >= 'A' && *p <= 'Z')
			b = *p - 'A' + 10;
		else if (*p >= 'a' && *p <= 'z')
			b = *p - 'a' + 10;
		else if (dosign && (*p == '.' || *p == ','))
			continue;	/* OpenFirmware spec has us ignore these in a number */
		else
		{
			err = TRUE;
			break;
		}
		
		if (b < 0 || b >= radix)
		{
			err = TRUE;
			break;
		}
		
		s = s * radix + b;
	}
	
	if (sign)
		s = -s;

	*str = p;
	*len = i;
	*val = s;
	*error = err ? FTRUE : FFALSE;
}
