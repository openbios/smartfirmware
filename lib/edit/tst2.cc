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
#include <stdarg.h>
#include "buffer.h"
#include "terminal.h"
#include "display.h"
#include "window.h"
#include "keymap.h"


static Buffer *currbuf;
static Display *currdisp;
static Keymap *currkmap;
static Window *currwin;
static boolean getout = FALSE;


#define readchar()		getc(stdin)
#define pushback(c)		ungetc((c), stdin)

static int
prefixarg(Keyinfo &k)
{
	auto int ch, p;
	auto boolean neg = FALSE;

	k.newarg = TRUE;

	if ((p = k.lastchar) == '-')
		neg = TRUE;
	else
		k.arg = isdigit(p) ? p - '0' : 4;

	auto boolean first = TRUE;

	for (ch = readchar(); isdigit(ch) || ch == p || ch == '-';
			ch = readchar())
	{
		if (ch == '-')
			neg = !neg;
		else if (isdigit(ch))
		{
			if (first)
				k.arg = 0;

			k.arg = k.arg * 10 + ch - '0';
			first = FALSE;
		}
		else
			k.arg *= 4;
	}

	k.arg2 = k.arg = neg ? -k.arg : k.arg;
	k.numargs = 1;

	if (ch == ',')
	{
		k.arg2 = 0;
		neg = FALSE;

		for (ch = readchar(); isdigit(ch) || ch == p || ch == '-';
				ch = readchar())
		{
			if (ch == '-')
				neg = !neg;
			else
				k.arg2 = isdigit(ch) ? k.arg2 * 10 + ch - '0' : k.arg2 * 4;
		}

		k.arg2 = neg ? -k.arg2 : k.arg2;
		k.numargs = 2;
	}

	pushback(ch);
	k.thiscmd = k.lastcmd;
	return k.error;
}

static int
abortexit(Keyinfo &k)
{
	getout = TRUE;
	return k.error = ERR;
}

static int
cleanexit(Keyinfo &k)
{
	getout = TRUE;
	return k.error;
}

static int
selfinsert(Keyinfo &k)
{
	auto Buftext str[1];
	str[0] = k.lastchar;
	currbuf->insert(str, 1);
	return k.error;
}

static int
newline(Keyinfo &k)
{
	currbuf->insert("\n", 1);
	return k.error;
}

static int
killprev(Keyinfo &k)
{
	currbuf->killprev(k.numargs ? k.arg : 1);
	return k.error;
}

static int
killnext(Keyinfo &k)
{
	currbuf->kill(k.numargs ? k.arg : 1);
	return k.error;
}

static int
movebol(Keyinfo &k)
{
	currbuf->moveto(currbuf->getlineloc(currbuf->getline()));
	return k.error;
}

static int
moveeol(Keyinfo &k)
{
	currbuf->moveto(currbuf->getlineloc(currbuf->getline())
			+ currbuf->getlinelen() - 1);

	if (currbuf->getline() == currbuf->getnumlines() - 1)
		currbuf->moverel(1);

	return k.error;
}

static int
nextline(Keyinfo &k)
{
	auto long numlines = k.numargs ? k.arg : 1;

	if (currbuf->getline() < currbuf->getnumlines() - numlines)
		currbuf->moveto(currbuf->getlineloc(currbuf->getline() + numlines));
	else
		currbuf->moveto(currbuf->getlineloc(currbuf->getnumlines() - 1));

	return k.error;
}

static int
prevline(Keyinfo &k)
{
	currbuf->moveto(currbuf->getlineloc(currbuf->getline() -
				(k.numargs ? k.arg : 1)));
	return k.error;
}


static int
nextpage(Keyinfo &k)
{
	auto long numlines = (k.numargs ? k.arg : 1) * (currwin->numlines() - 1);

	if (currbuf->getline() < currbuf->getnumlines() - numlines)
		currbuf->moveto(currbuf->getlineloc(currbuf->getline() + numlines));
	else
		currbuf->moveto(currbuf->getlineloc(currbuf->getnumlines() - 1));

	return k.error;
}

static int
prevpage(Keyinfo &k)
{
	currbuf->moveto(currbuf->getlineloc(currbuf->getline() -
				(k.numargs ? k.arg : 1) * (currwin->numlines() - 1)));
	return k.error;
}

static int
forward(Keyinfo &k)
{
	currbuf->moverel(k.numargs ? k.arg : 1);
	return k.error;
}

static int
backward(Keyinfo &k)
{
	currbuf->moverel(-(k.numargs ? k.arg : 1));
	return k.error;
}

static int
redraw(Keyinfo &k)
{
	currdisp->redraw();
	return k.error;
}

static int
kmapdebug(Keyinfo &k)
{
	currkmap->debugdump();
	currbuf->debugdump();
	return k.error;
}

static int
togglenumber(Keyinfo &k)
{
	currwin->setnumber(!currwin->getnumber());
	return k.error;
}

static int
openwin(Keyinfo &k)
{
	currwin = new Window(*currdisp, *currbuf);
	currwin->setsize(5, 3, 70, 15);
	return k.error;
}

static int
nextwin(Keyinfo &k)
{
	currwin = currwin->nextwin();
	currwin->totop();
	currbuf->moveto(currwin->getloc());
	return k.error;
}

static int
undo(Keyinfo &k)
{
	auto int c;

	if (currbuf->startundo())
		return k.error = ERR;

	if (currbuf->prevundo() == ERR)
		currdisp->beep();

	currwin->setloc(currbuf->getloc());
	drawwindows();

	while (TRUE)
	{
		switch (c = readchar())
		{
		case '-':
			if (currbuf->nextundo() == ERR)
				currdisp->beep();

			// error("No more undos left to undo!");
			break;

		case ' ':
			if (currbuf->prevundo() == ERR)
				currdisp->beep();

			// error("No more to undo!");
			break;

		default:
			if (c != '\r' && c != '\n' && c != ESC)
				pushback(c);

			currbuf->endundo();
			return OK;
		}

		currwin->setloc(currbuf->getloc());
		drawwindows();
	}

	return k.error;
}


int
main(int argc, const char *argv[])
{
#ifdef DLD
	dldquit = quit;
	dlderror = error;
	dldinit(argv[0]);
#endif

	auto Buffer buf;
	auto Display disp;
	auto Keymap kmap;

	currbuf = &buf;
	currdisp = &disp;
	currkmap = &kmap;
	currwin = new Window(disp, buf);

	if (argc > 1)
		if (!buf.readfile(argv[1]))
			disp.beep();

	buf.setundo(ON);

	// win.setsize(5,3,70,15);

	auto boolean kret;
	kret = kmap.defaultbind(selfinsert);
	kret = kmap.bind(newline, '\n');
	kret = kmap.bind(newline, '\r');
	kret = kmap.bind(abortexit, CTRL('C'));
	kret = kmap.bind(cleanexit, CTRL('X'), CTRL('C'));
	kret = kmap.bind(kmapdebug, CTRL('X'), CTRL('K'));
	kret = kmap.bind(togglenumber, CTRL('X'), CTRL('T'));
	kret = kmap.bind(openwin, CTRL('X'), CTRL('O'));
	kret = kmap.bind(openwin, CTRL('X'), 'o');
	kret = kmap.bind(nextwin, CTRL('X'), CTRL('N'));
	kret = kmap.bind(nextwin, CTRL('X'), 'n');
	kret = kmap.bind(killprev, CTRL('H'));
	kret = kmap.bind(killprev, DEL);
	kret = kmap.bind(killnext, CTRL('D'));
	kret = kmap.bind(movebol, CTRL('A'));
	kret = kmap.bind(moveeol, CTRL('E'));
	kret = kmap.bind(nextline, CTRL('N'));
	kret = kmap.bind(prevline, CTRL('P'));
	kret = kmap.bind(nextpage, CTRL('Z'));
	kret = kmap.bind(prevpage, CTRL('Y'));
	kret = kmap.bind(forward, CTRL('F'));
	kret = kmap.bind(backward, CTRL('B'));
	kret = kmap.bind(redraw, CTRL('L'));
	kret = kmap.bind(prefixarg, CTRL('U'));
	kret = kmap.bind(undo, CTRL('X'), CTRL('U'));

	if (kret)
		return error("cannot bind keysequences\n");

	currwin->setloc(buf.getloc());
	drawwindows(TRUE);

	auto int ch;
	auto boolean err = FALSE;

	while ((ch = readchar()) >= 0)
	{
		auto Keychar str[2];
		str[0] = ch;

		if ((err = kmap.eval(str, 1)) == ERR)
			disp.beep();

		if (getout)
			break;

		currwin->setloc(buf.getloc());
		drawwindows();
	}

	disp.toend();

	if (err != ERR && argc > 1)
		if (buf.ismodified() && !buf.writefile(argv[1]))
			error("cannot write file %s\n", argv[1]);

	return 0;
}
