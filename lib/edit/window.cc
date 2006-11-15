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
#include "window.h"


static Window *topwin = NULL;
const char BEGINBUFCH = '?';
const char ENDBUFCH = '~';


Window *
topwindow()
{
	return topwin;
}


Window::Window(Display & d, Buffer & b): buf(b), bloc(b), disp(d)
{
	desired = d.getdesired();

	redraw = enable = border = TRUE;
	number = FALSE;
	tabsize = 4;

	dline = b.getline();
	dcol = 0;

	scol = sline = 0;
	ecol = dcols = disp.numcols();
	eline = dlines = disp.numlines();

	if (topwin == NULL)
		topwin = next = prev = this;
	else
	{
		next = topwin;
		prev = topwin->prev;
		topwin = topwin->prev = topwin->prev->next = this;
	}
}

Window::~Window()
{
	next->prev = prev;
	prev->next = next;

	if (topwin == this)
		topwin = next;

	if (topwin == this)
		topwin = NULL;
}

void
Window::tobot()
{
	topwin = next;
}

void
Window::totop()
{
	topwin = this;
}

void
Window::getsize(int &sc, int &sl, int &ec, int &el)
{
	sc = scol;
	sl = sline;
	ec = ecol;
	el = eline;
}

void
Window::setsize(int sc, int sl, int ec, int el)
{
	scol = sc;
	sline = sl;
	ecol = ec;
	eline = el;

	if (scol < 0)
		scol = 0;
	else if (scol >= dcols)
		scol = dcols - 1;

	if (ecol < 0)
		ecol = 1;
	else if (ecol >= dcols)
		ecol = dcols;

	if (ecol < scol)
	{
		int t = scol;
		scol = ecol;
		ecol = t;
	}

	if (sline < 0)
		sline = 0;
	else if (sline >= dlines)
		sline = dlines - 1;

	if (eline < 0)
		eline = 1;
	else if (eline >= dlines)
		eline = dlines;

	if (eline < sline)
	{
		int t = sline;
		sline = eline;
		eline = t;
	}
}

void
Window::setdisploc(int dln, int dcl)
{
	long buflines = buf.getnumlines() - 1;
	dline = dln > buflines ? buflines : dln;
	dcol = dcl;
}

void
Window::getdisploc(long &dln, long &dcl)
{
	dln = dline;
	dcl = dcol;
}


void
drawwindows(boolean garbage)
{
	Window *w = topwin;
	w->disp.cleardesired();

	do
	{
		w = w->prev;
		w->draw();
	} while (w != topwin);

	if (garbage)
		w->disp.redraw();
	else
		w->disp.refresh();

	w->restorecursor();
}


void
Window::updatedline(int ln, int cl, int dcols, long bloc, Disptext * str,
    long len, long id, long dcol, int inv, int last)
{
	if (desired[ln].id == ID_WINDOW)
	{
		if (desired[ln].lwin >= cl && desired[ln].rwin <= dcols)
			desired[ln].id = id;
	}
	else
		desired[ln].id = id;

	desired[ln].changed = FALSE;

	Disptext *des = desired[ln].text;
	long col = 0;
	int d = cl;
	long i;

	for (i = 0; i < len && d < dcols; i++)
	{
		int bufchar;

		if (str == NULL)
			bufchar = buf[bloc++];
		else
			bufchar = *str++;

		int chr = bufchar & T_CHAR;
		int mask = (bufchar & T_ENHANCE) | inv;

		if (chr == '\t')
		{
			long t;

			for (t = col; t >= tabsize; t -= tabsize)
				;

			for (; d < dcols && t < tabsize; t++)
				if (col++ >= dcol)
					des[d++] = ' ' | mask;
		}
		else if (chr & 0x80)
		{
			if (col++ >= dcol)
				des[d++] = '\\' | mask;

			int c = chr & 0x7F;

			if (iscntrl(c))
			{
				if (col++ >= dcol && d < dcols)
					des[d++] = '^' | mask;

				if (col++ >= dcol && d < dcols)
					des[d++] = ((c == DEL) ? '?' : c + '@') | mask;
			}
			else if (col++ >= dcol && d < dcols)
				des[d++] = c | mask;
		}
		else if (iscntrl(chr))
		{
			if (col++ >= dcol)
				des[d++] = '^' | mask;

			if (col++ >= dcol && d < dcols)
				des[d++] = ((bufchar == DEL) ? '?' : bufchar + '@') | mask;
		}
		else if (col++ >= dcol)
			des[d++] = chr | mask;
	}

	// if (last && d < dcols)
	if (last && d == cl)
	{
		des[d++] = ENDBUFCH | T_BOLD;
		desired[ln].id = ID_ENDBUF;
	}

	for (int j = d; j < dcols; j++)
		des[j] = ' ';

	if (dcol > 0)
		des[cl] = '<' | T_STANDOUT;

	if (i < len)
		des[dcols - 1] = '>' | T_STANDOUT;
}

long
Window::colcalc()
{
	long col = 0;
	long loc = bloc();
	long b = buf.getlineloc(buf.getlocline(loc));

	while (b < loc)
	{
		int chr = buf[b++];

		if (chr == '\t')
		{
			long t = col;

			for (; t >= tabsize; t -= tabsize)
				;

			col += tabsize - t;
		}
		else if (chr & 0x80)
		{
			if (iscntrl(chr & 0x7F))
				col += 3;
			else
				col += 2;
		}
		else if (iscntrl(chr))
			col += 2;
		else
			col++;
	}

	return col;
}

int
Window::numbercolumn()
{
	if (!number)
		return 0;

	int len = 0;

	for (int num = eline; num > 0; num /= 10)
		len++;

	if (len > 0)
		len++;

	return len < 4 ? 4 : len;
}

void
Window::updateline(long ln, int dl, int numcol)
{
	long id, len, nln, dcl;
	int inv, last;
	Disptext *str, cbuf[1];
	long lptr;

	nln = buf.getnumlines();

	if (ln < 0 || ln >= nln)
	{
		*cbuf = ln < 0 ? BEGINBUFCH : ENDBUFCH;
		str = cbuf;
		len = 1;
		id = ID_ENDBUF;
		inv = T_BOLD;
		dcl = 0;
		last = FALSE;
		lptr = -1;
	}
	else
	{
		lptr = buf.getlineloc(ln);
		len = buf.getlinelen(ln);
		id = buf.getlineid(ln);
		str = NULL;
		inv = T_NONE;
		dcl = dcol;
		last = ln == nln - 1;

		if (!last)
			len--;
	}

	updatedline(dl, scol + numcol, ecol, lptr, str, len, id, dcl, inv, last);
}

static
void itoa(Disptext *buf, long num, int len)
{
	while (len-- > 0)
	{
		buf[len] = ('0' + (int)(num % 10)) | T_BOLD;
		num /= 10;

		if (num == 0)
			break;
	}

	while (len-- > 0)
		buf[len] = ' ';
}


static
void padchar(Disptext *buf, int ch, int len)
{
	while (len-- > 0)
		buf[len] = ch;
}


void
Window::mkwinline(int l)
{
	if (desired[l].id == ID_WINDOW)
	{
		if (scol < desired[l].lwin)
			desired[l].lwin = scol;

		if (ecol > desired[l].rwin)
			desired[l].rwin = ecol;

		return;
	}

	desired[l].id = ID_WINDOW;
	desired[l].lwin = scol;
	desired[l].rwin = ecol;
}

const char *
Window::makemode(boolean top)
{
	// hard-wired for now...
	if (top)
		return strbldf(" Line:%ld  Col:%ld",
				buf.getline() + 1, colcalc() + 1);
	else
	{
		long num = buf.getnumlines();

		return strbldf(" NumLines:%ld  Size:%ld",
				buf.getlinelen(num - 1) > 0 ? num : num - 1,
				buf.length());
	}
}

void
Window::draw()
{
	if (!enable)
		return;

	int i;
	long n;

	long dln = dline;
	long col = colcalc();
	int hor = '-';
	int ver = '|';
	int cor = '+';
	int sep = ' ';
	int enh = T_NONE;

	if (disp.canenhance(T_STANDOUT))
	{
		hor = ver = cor = ' ' | T_STANDOUT;
		enh = T_STANDOUT;
	}

	int numcol = numbercolumn();

	if (col >= dcol + ecol - scol - numcol - 1
			|| col < dcol
			|| (dcol > 0 && col == dcol))
	{
		dcol = col - ((ecol - scol - numcol) >> 1);

		if (dcol < 0)
			dcol = 0;

		redraw = TRUE;
	}

	long bline = buf.getlocline(bloc());

	// if (bline >= dln + eline - sline || bline < dln)
	// dline = bline - ((eline - sline) >> 1);

	if (bline >= dln + eline - sline)
		dline = bline - (eline - sline) + 1;

	if (bline < dln)
		dline = bline;

	for (i = sline; i < eline; i++)
		updateline(dline + i - sline, i, numcol);

	if (border)
	{
		if (sline > 0)
		{
			const char *m = makemode(TRUE);

			for (i = scol; i < ecol && *m != '\0'; i++)
				desired[sline - 1].text[i] = *m++ | enh;

			for (; i < ecol; i++)
				desired[sline - 1].text[i] = hor;

			mkwinline(sline - 1);
		}

		if (eline < dlines)
		{
			const char *m = makemode(FALSE);

			for (i = scol; i < ecol && *m != '\0'; i++)
				desired[eline].text[i] = *m++ | enh;

			for (; i < ecol; i++)
				desired[eline].text[i] = hor;

			mkwinline(eline);
		}

		if (scol > 0)
		{
			for (i = sline; i < eline; i++)
			{
				desired[i].text[scol - 1] = ver;
				mkwinline(i);
			}

			if (sline > 0)
				desired[sline - 1].text[scol - 1] = cor;

			if (eline < dlines)
				desired[eline].text[scol - 1] = cor;
		}

		if (ecol < dcols)
		{
			for (i = sline; i < eline; i++)
			{
				desired[i].text[ecol] = ver;
				mkwinline(i);
			}

			if (sline > 0)
				desired[sline - 1].text[ecol] = cor;

			if (eline < dlines)
				desired[eline].text[ecol] = cor;
		}
	}

	if (numcol > 0)
	{
		for (i = sline; i < eline; i++)
		{
			Disptext *s = &desired[i].text[scol];

			n = dline + i - sline;

			if (n >= 0 && n < buf.getnumlines())
			{
				if (n < buf.getnumlines() - 1 ||
						buf.getlinelen(buf.getnumlines() - 1) > 0)
					itoa(s, n + 1, numcol - 1);
				else
					padchar(s, ' ', numcol - 1);
			}
			else
				padchar(s, ' ', numcol - 1);

			s[numcol - 1] = sep;
			desired[i].changed = TRUE;
		}
	}
	else if (redraw)
		for (i = sline; i < eline; i++)
			desired[i].changed = TRUE;

	redraw = FALSE;
}


void
Window::restorecursor()
{
	disp.move((int)(buf.getline() - dline) + sline,
			(int)(colcalc() - dcol) + scol + numbercolumn());
}
