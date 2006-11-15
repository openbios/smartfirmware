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

#ifndef	_WINDOW_H_
#define	_WINDOW_H_


class Window
{
	Buffer &buf;
	Mark bloc;
	Display &disp;
	Displine *desired;
	long dline, dcol;
	int dlines, dcols;
	int scol, sline, eline, ecol;
	Window *prev, *next;

	int tabsize;
	boolean enable, border, number, redraw;

	void mkwinline(int line);
	const char *makemode(boolean top);
	void updatedline(int ln, int cl, int dcols, long bloc, Disptext *str,
			long len, long id, long dcol, int inv, int last);
	void updateline(long ln, int dl, int numcol);
	int numbercolumn();
public:

	void settabsize(int tab = 8) { tabsize = tab; redraw = TRUE; }
	int gettabsize() { return tabsize; }
	void setenable(boolean state) { enable = state; redraw = TRUE; }
	boolean getenable() { return enable; }
	void setborder(boolean state) { border = state; redraw = TRUE; }
	boolean getborder() { return border; }
	void setnumber(boolean state) { number = state; redraw = TRUE; }
	boolean getnumber() { return number; }

	Window(Display &disp, Buffer &buf);
	~Window();

	void setsize(int scol, int sline, int ecol, int eline);
	void getsize(int &scol, int &sline, int &ecol, int &eline);
	void setdisploc(int dline, int dcol = 0);
	void getdisploc(long &dline, long &dcol);

	int numlines(void) { return eline - sline; }

	void tobot();
	void totop();
	void draw();

	Buffer &getbuf() { return buf; }
	long getloc() { return bloc(); }
	void setloc(long loc) { bloc = loc; }

	Window *nextwin() { return next; }
	Window *prevwin() { return prev; }

	long colcalc();
	void restorecursor();

	friend void drawwindows(boolean garbage = FALSE);
	friend Window *topwindow();
};



#endif	/* _WINDOW_H_ */
