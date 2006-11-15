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

/* Copyright (c) 1990,1999 by Parag Patel.  All Rights Reserved. */

#include "defs.h"
#include "buffer.h"
#include "terminal.h"
#include "display.h"


Display::Display(char *tty, char *termtype): Terminal(tty, termtype)
{
	dcols = numcols();
	dlines = numlines();
	needclear = FALSE;
	// oneline = moveonly = garbage = FALSE;

	desired = new Displine[dlines];
	screen = new Displine[dlines];

	for (int i = 0; i < dlines; i++)
	{
		desired[i].text = new Disptext[dcols + 1];
		screen[i].text = new Disptext[dcols + 1];

		for (int j = 0; j < dcols; j++)
			desired[i].text[j] = screen[i].text[j] = ' ';

		desired[i].text[dcols] = screen[i].text[dcols] = '\0';
		desired[i].id = screen[i].id = ID_UNDEF;
		desired[i].changed = screen[i].changed = FALSE;
		desired[i].lwin = screen[i].lwin = -1;
		desired[i].rwin = screen[i].rwin = dcols;
	}

	init();
	reset(0, 0);
	clearscreen();
}

Display::~ Display()
{
	toend();
	fini();

	for (int i = 0; i < dlines; i++)
	{
		delete desired[i].text;
		delete screen[i].text;
	}

	delete[/*dlines*/] desired;
	delete[/*dlines*/] screen;
}

void
Display::toend()
{
	setinsert(OFF);
	reset(dlines - 1, 0);
	newline();
	clearscreen();
}


void
Display::movecursor(int line, int col)
{
	setinsert(OFF);
	move(line, col);
}


void
Display::redrawline(int ln)
{
	if (desired[ln].id == screen[ln].id && desired[ln].id > 0
			&& !desired[ln].changed)
		return;

	long sid = screen[ln].id;

	screen[ln].id = desired[ln].id;
	desired[ln].changed = FALSE;

	Disptext *scr = screen[ln].text;
	Disptext *des = desired[ln].text;

	if (memcmp((char *)des, (char *)scr, dcols * sizeof (Disptext)) == 0)
		return;

	int slen = dcols - 1;
	int dlen = slen;

	while (slen >= 0 && scr[slen] == ' ')
		slen--;

	while (dlen >= 0 && des[dlen] == ' ')
		dlen--;

	if (scr[slen] != ' ')
		slen++;

	if (des[dlen] != ' ')
		dlen++;

	if (slen > 0 && caninsdelchars())
	{
		int beg = 0;

		for (; beg < dlen && beg < slen; beg++)
			if (scr[beg] != des[beg])
				break;

		int dend = dlen - 1, send = slen - 1;

		for (; dend >= beg && send >= beg; dend--, send--)
			if (scr[send] != des[dend])
				break;

		dend++;
		send++;

		if (dend > send)
		{
			setinsert(OFF);
			move(ln, beg);
			puttext(&des[beg], send - beg);

			if (dend - send < (dcols >> 3))
			{
				if (send < slen)
					setinsert(ON);

				puttext(&des[send], dend - send);
			}
			else
			{
				puttext(&des[send], dlen - send);

				if (dlen < slen)
					clearline();
			}
		}
		else if (dend < send)
		{
			setinsert(OFF);
			move(ln, beg);

			if (send < slen && send - dend < (dcols >> 3))
			{
				puttext(&des[beg], dend - beg);
				deletechars(send - dend);
			}
			else
			{
				puttext(&des[beg], dlen - beg);

				if (dlen < slen)
					clearline();
			}
		}
		else					/* dend == send */
		{
			setinsert(OFF);
			boolean didit = FALSE;

			if (sid > 0)
				for (int n = beg; n < dend && n < beg + (dcols >> 3); n++)
					if (memcmp((char *)&des[n], (char *)&scr[beg],
							(dend - n) * sizeof (Disptext)) == 0)
					{
						move(ln, dend - (n - beg));
						deletechars(n - beg);
						move(ln, beg);
						setinsert(ON);
						puttext(&des[beg], n - beg);
						didit = TRUE;
						break;
					}
					else if (memcmp((char *)&scr[n], (char *)&des[beg],
							(send - n) * sizeof (Disptext)) == 0)
					{
						move(ln, beg);
						deletechars(n - beg);
						move(ln, send - (n - beg));
						setinsert(ON);
						puttext(&des[dend - (n - beg)], n - beg);
						didit = TRUE;
						break;
					}

			if (!didit)
			{
				move(ln, beg);
				puttext(&des[beg], dend - beg);
			}
		}
	}
	else if (dlen > 0)
	{
		setinsert(OFF);
		move(ln, 0);
		puttext(des, dlen);

		if (dlen < slen)
			clearline();
	}
	else if (slen > 0)
	{
		move(ln, 0);
		clearline();
	}

	memcpy((char *)screen[ln].text, (char *)desired[ln].text,
		dcols *sizeof (Disptext));
}


void
Display::refresh()
{
	int i;
	boolean again = caninsdelline();

	while (again)
	{
		again = FALSE;

		int b = 0;

		/* find first differing line number */
		for (; b < dlines; b++)
			if (screen[b].id != desired[b].id && desired[b].id > 0)
				break;

		for (; b < dlines; b++)
		{
			/* find last line that matches on screen and in desired */
			int e = b;

			for (; e < dlines; e++)
				if (screen[e].id == desired[e].id || screen[e].id <= 0)
					break;

			e -= 1;

			if (e <= b)
				continue;

			for (int l = b + 1; l <= e && l < b + (dlines >> 1); l++)
			{
				/* try to find a line with the right properties */
				if (desired[l].id == ID_UNDEF || screen[l].id == ID_UNDEF
						|| desired[l].id == ID_WINDOW
						|| screen[l].id == ID_WINDOW)
					break;

				int j, d;
				Displine dln;

				if (desired[l].id == screen[b].id)
				{
					/* drop lines on screen to desired position */
					again = TRUE;
					setinsert(OFF);

					for (d = e + 1; d < dlines; d++)
						if (screen[d].id != ID_ENDBUF)
							break;

					e = d - 1;

					if (e < dlines - 1)
						for (i = b; i < l; i++)
							deleteline(e - (l - b) + 1);
					else
						needclear = TRUE;

					for (i = b; i < l; i++)
					{
						insertline(b);
						dln = screen[e];

						for (j = e; j > b; j--)
							screen[j] = screen[j - 1];

						for (j = 0; j < dcols; j++)
							dln.text[j] = ' ';

						dln.id = ID_UNDEF;
						screen[b] = dln;
					}

					break;
				}
				else if (desired[b].id == screen[l].id)
				{
					/* raise lines on screen to desired position */
					again = TRUE;
					setinsert(OFF);

					for (d = b - 1; d >= 0; d--)
						if (screen[d].id != ID_ENDBUF)
							break;

					d += 1;

					if (d == 0 && e == dlines - 1)
					{
						/* entire screen scrolls - let terminal do it */
						move(dlines - 1, 0);

						for (i = b; i < l; i++)
						{
							dln = screen[0];

							for (j = 0; j < e; j++)
								screen[j] = screen[j + 1];

							screen[e] = dln;
							newline();

							if (needclear && membelow() && i == b)
							{
								clearscreen();
								needclear = FALSE;
							}

							int t = e - l + 1 + i;

							d = dcols - 1;

							while (d >= 0 && desired[t].text[d] == ' ')
								d--;

							puttext(desired[t].text, d + 1);
							memcpy((char *)screen[e].text,
									(char *)desired[t].text,
									dcols *sizeof (Disptext));
							screen[e].id = desired[t].id;
						}

						break;
					}

					for (i = b; i < l; i++)
					{
						deleteline(d);
						dln = screen[d];

						for (j = d; j < e; j++)
							screen[j] = screen[j + 1];

						for (j = 0; j < dcols; j++)
							dln.text[j] = ' ';

						dln.id = ID_UNDEF;
						screen[e] = dln;
					}

					if (e < dlines - 1)
						for (i = b; i < l; i++)
							insertline(e - (l - b) + 1);
					else if (membelow())
					{
						move(dlines - (l - b), 0);
						clearscreen();
						needclear = FALSE;
					}

					break;
				}
			}

			if (again == TRUE)
				break;
		}
	}

	/* redraw every line on screen if necessary */
	for (i = 0; i < dlines; i++)
		redrawline(i);
}

void
Display::cleardesired()
{
	for (int i = 0; i < dlines; i++)
	{
		desired[i].id = ID_UNDEF;
		desired[i].changed = FALSE;
		desired[i].lwin = -1;
		desired[i].rwin = dcols;

		for (int j = 0; j < dcols; j++)
			desired[i].text[j] = ' ';
	}
	// oneline = moveonly = garbage = FALSE;
}

void
Display::redraw()
{
	setinsert(OFF);
	reset(0, 0);
	clearscreen();

	for (int i = 0; i < dlines; i++)
	{
		if (i > 0)
			newline();

		move(i, 0);

		int dlen = dcols - 1;

		while (dlen >= 0 && desired[i].text[dlen] == ' ')
			dlen--;

		puttext(desired[i].text, dlen + 1);
		memcpy((char *)screen[i].text, (char *)desired[i].text,
			dcols *sizeof (Disptext));
		screen[i].id = desired[i].id;
	}
}
