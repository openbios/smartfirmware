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

#include <stdarg.h>
#include <math.h>
#include "defs.h"
#include "terminal.h"


/* externals for "termcap" routines */
short ospeed;
char PC, *BC, *UP;
extern "C"
{
	int tgetent(char *, char *);
	int tgetflag(char *);
	int tgetnum(char *);
	char *tgetstr(char *, char **);
	char *tgoto(char *, int, int);
	char *tparm(char *, int,...);
	int tputs(char *, int, int (*)(char));
}


Terminal::Terminal(char *tty, char *termtype)
{
	if (tty == NULL)
	{
		fp = stdout;
		tty = "<stdout>";
	}
	else
		fp = fopen(tty, "w");

	if (fp == NULL)
		quit("Cannot open %s for writing", tty);

	if (ioctl(fileno(fp), TGET, &oldterm) < 0)
	{
		if (fp != stdout)
			fclose(fp);

		fp = NULL;
		quit("Sorry - %s must be a terminal", tty);
	}

	rawterm = oldterm;

#if defined SGTTY
	/* BSD sgtty */
	ospeed = rawterm.sg_ospeed;
	rawterm.sg_flags &= ~(CRMOD | ECHO);
	rawterm.sg_flags |= RAW;
#elif defined TERMIO
	/* SysV termio */
	ospeed = rawterm.c_cflag & CBAUD;	/* get baud rate */
	/* rawterm.c_cc[VQUIT] = -1; */
	rawterm.c_cc[VEOF] = 1;		/* num. chars per read */
	rawterm.c_cc[VEOL] = 0;		/* time delay between chars */
	rawterm.c_lflag &= ~(ICANON | ECHO | ISIG);
	rawterm.c_iflag &= ~(INLCR | ICRNL);
	rawterm.c_oflag &= ~(TAB3 | ONLCR | OCRNL);
#else
	/* Posix termios */
	ospeed = rawterm.c_ospeed;
	rawterm.c_cc[VMIN] = 1;		/* num. chars per read */
	rawterm.c_cc[VTIME] = 0;	/* time delay between chars */
	rawterm.c_iflag &= ~(INLCR | ICRNL);
	rawterm.c_oflag &= ~(OPOST | ONLCR | ONOEOT);
	rawterm.c_lflag &= ~(ICANON | ECHO | ISIG);
#endif	/* term types */

	/* get termcap stuff */
	if (termtype == NULL)
		termtype = getenv("TERM");

	char termbuf[1025];

	if (tgetent(termbuf, termtype) != 1)
	{
		if (fp != stdout)
			fclose(fp);

		fp = NULL;
		quit("Cannot get termcap info for %s", termtype);
	}

	char *ptr = buf;			/* point to start of termcap buffer */
	tc_ce = tgetstr("ce", &ptr);/* clear line */
	tc_cd = tgetstr("cd", &ptr);/* clear screen */
	tc_cm = tgetstr("cm", &ptr);/* cursor motion */
	tc_ti = tgetstr("ti", &ptr);/* terminal init */
	tc_te = tgetstr("te", &ptr);/* terminal end */
	cm_len = tc_cm ? strlen(tc_cm) >> 1 : 0;	/* length of cm string */
	tc_up = tgetstr("up", &ptr);/* cursor up */
	tc_do = tgetstr("do", &ptr);/* cursor down */
	tc_le = tgetstr("le", &ptr);/* cursor left */

	if (tc_le == NULL || *tc_le == '\0')
		tc_le = "\010";

	tc_nd = tgetstr("nd", &ptr);/* cursor right (nondestructive) */

	if (tc_nd == NULL || *tc_nd == '\0')
		tc_nd = " ";

	char *tmp = tgetstr("pc", &ptr);	/* pad character if not '\0' */

	if (tmp == NULL)
		PC = '\0';
	else
		PC = *tmp;

	BC = tc_le;
	UP = tc_up;

	/* set initial number of lines/columns */
	tc_co = tgetnum("co");	/* number of columns on terminal */
	tc_li = tgetnum("li");	/* number of lines */

#ifdef TIOCGWINSZ
	/* override number of lines/columns if this bit of magic is supported */
	struct winsize win;

	if (ioctl(fileno(fp), TIOCGWINSZ, &win) == 0)
	{
		tc_co = win.ws_col;
		tc_li = win.ws_row;
	}
#endif

	/* finally let user override everything with environment vars */
	tmp = getenv("COLUMNS");

	if (tmp && *tmp)
		tc_co = atoi(tmp);

	tmp = getenv("LINES");

	if (tmp && *tmp)
		tc_li = atoi(tmp);

	tc_dl = tgetstr("dl", &ptr);/* delete line */
	tc_al = tgetstr("al", &ptr);/* add line */
	tc_im = tgetstr("im", &ptr);/* insert mode */
	tc_ei = tgetstr("ei", &ptr);/* end insert mode */
	tc_mi = tgetflag("mi");		/* safe to move in insert mode? */
	insert = FALSE;				/* not in insert mode right now */

	if (tc_im == NULL)
		tc_ic = tgetstr("ic", &ptr);	/* insert character */
	else
		tc_ic = NULL;

	tc_ip = tgetstr("ip", &ptr);/* insert padding */
	tc_dc = tgetstr("dc", &ptr);/* delete character */
	tc_ks = tgetstr("ks", &ptr);/* keypad transmit start */
	tc_ke = tgetstr("ke", &ptr);/* keypad transmit end */
	tc_xs = tgetflag("xs");		/* standout overwritten? */
	tc_db = tgetflag("db");		/* memory retained below? */
	standout = FALSE;

	if (tc_xs)
		tc_so = tc_se = tc_us = tc_ue = tc_bold = tc_ital = tc_dim = NULL;
	else
	{
		tc_so = tgetstr("so", &ptr);	/* standout mode */
		tc_se = tgetstr("se", &ptr);	/* standout end */
		tc_us = tgetstr("us", &ptr);	/* underline mode */
		tc_ue = tgetstr("ue", &ptr);	/* underline end */
		tc_bold = tgetstr("md", &ptr);	/* bold mode */

		if (tc_bold == NULL)
			tc_bold = tc_so;

		tc_ital = tgetstr("mr", &ptr);	/* inverse */
		tc_dim = tgetstr("mh", &ptr);	/* dim */

		if (tc_ital == NULL)
			tc_ital = tc_dim;

		if (tc_ital == NULL)
			tc_ital = tc_so;
	}

	if (tc_cm == NULL || tc_cd == NULL || tc_ce == NULL)
	{
		if (fp != stdout)
			fclose(fp);

		fp = NULL;
		quit("Terminal %s is not powerful enough.", tty);
	}

	optimize = tc_up != NULL && tc_do != NULL &&
		tc_le != NULL || tc_nd != NULL;

	enhance = 0;
}

Terminal::~ Terminal()
{
	fini();
	fclose(fp);
	fp = NULL;
}


static FILE *filep = NULL;

static int
putchr(char c)
{
	return putc(c, filep);
}


void
Terminal::init()
{
	filep = fp;
	ioctl(fileno(fp), TSETW, &rawterm);

	if (tc_ti != NULL)
		tputs(tc_ti, 1, putchr);

	if (tc_ks != NULL)
		tputs(tc_ks, 1, putchr);
}


void
Terminal::fini()
{
	filep = fp;
	setinsert(FALSE);
	setenhance(T_NONE);

	if (tc_ke != NULL)
		tputs(tc_ke, 1, putchr);

	if (tc_te != NULL)
		tputs(tc_te, 1, putchr);

	ioctl(fileno(fp), TSETW, &oldterm);
}


void
Terminal::beep()
{
	putc(BELL, fp);
}


void
Terminal::move(int ln, int col)
{
	filep = fp;

	//if (!tc_mi && insert && tc_ei != NULL)
		//tputs(tc_ei, 1, putchr);

	if (!tc_mi && insert)
		setinsert(OFF);

	/* if the length of the cursor motion string is greater than the *
	   number of positions to move the cursor, send up/down/left/right *
	   cursor commands instead */
	if (col < 0)
		col = 0;
	else if (col >= tc_co)
		col = tc_co - 1;

	if (ln < 0)
		ln = 0;
	else if (ln > tc_li)
		ln = tc_li;

	if (optimize && abs(ocol - col) + abs(oln - ln) < cm_len)
	{
		int i;

		if (col > ocol)
			for (i = ocol; i < col; i++)
				tputs(tc_nd, 1, putchr);
		else if (col < ocol)
			for (i = col; i < ocol; i++)
				tputs(tc_le, 1, putchr);

		if (ln > oln)
			for (i = oln; i < ln; i++)
				tputs(tc_do, 1, putchr);
		else if (ln < oln)
			for (i = ln; i < oln; i++)
				tputs(tc_up, 1, putchr);
	}
	else if (col == 0 && ln == oln)
		putchar('\r');
	else if (col == ocol && ln == oln + 1)
		putchar('\n');
	else if (col == 0 && ln == oln + 1)
	{
		putchar('\r');
		putchar('\n');
	}
	else if (ocol != col || oln != ln)
		tputs(tgoto(tc_cm, col, ln), 1, putchr);

	ocol = col;
	oln = ln;

	//if (!tc_mi && insert && tc_ei != NULL)
		//tputs(tc_im, 1, putchr);
}


void
Terminal::reset(int ln, int col)
{
	filep = fp;
	oln = ocol = 30000;
	move(ln, col);
}


void
Terminal::newline()
{
	filep = fp;
	ocol = 0;

	if (oln < tc_li - 1)
		oln++;

	putc('\r', fp);
	putc('\n', fp);
}


void
Terminal::put(int ch)
{
	filep = fp;
	ocol++;

	if (insert && tc_ic != NULL)
		tputs(tc_ic, 1, putchr);

	putc(ch, fp);

	if (insert && tc_ip != NULL)
		tputs(tc_ip, 1, putchr);
}


void
Terminal::putf(int ln, int col, char *fmt,...)
{
	move(ln, col);

	va_list ap;
	va_start(ap, fmt);
	char str[1024];
	vsprintf(str, fmt, ap);
	va_end(ap);

	str[tc_co] = '\0';

	if (tc_ic != NULL || tc_ip != NULL)
		for (char *ptr = str; *ptr != '\0'; ptr++)
			put(*ptr);
	else
	{
		ocol += strlen(str);
		tputs(str, 1, putchr);
	}
}


void
Terminal::puts(int ln, int col, char *str)
{
	move(ln, col);
	str[tc_co] = '\0';

	if (tc_ic != NULL || tc_ip != NULL)
		for (; *str != '\0'; str++)
			put(*str & 0x7F);
	else
	{
		ocol += strlen(str);
		tputs(str, 1, putchr);
	}
}


void
Terminal::puttext(Disptext *str, int len)
{
	int inv = T_NONE;

	for (; len-- > 0 && ocol < tc_co; str++)
	{
		if ((*str & T_ENHANCE) != inv)
			setenhance(inv = (*str & T_ENHANCE));

		put(*str & T_CHAR);
	}

	setenhance(T_NONE);
}


void
Terminal::clearline()
{
	if (tc_ce == NULL)
		return;

	filep = fp;
	tputs(tc_ce, 1, putchr);
}


void
Terminal::clearscreen()
{
	if (tc_cd == NULL)
		return;

	filep = fp;
	tputs(tc_cd, 1, putchr);
}


boolean
Terminal::caninsdelline()
{
	return tc_dl != NULL && tc_al != NULL;
}


void
Terminal::deleteline(int ln)
{
	if (tc_dl == NULL)
		return;

	move(ln, 0);
	tputs(tc_dl, 0, putchr);
}


void
Terminal::insertline(int ln)
{
	if (tc_al == NULL)
		return;

	move(ln, 0);
	tputs(tc_al, 0, putchr);
}


boolean
Terminal::caninsdelchars()
{
	return tc_dc != NULL && ((tc_im != NULL && tc_ei != NULL)
			|| tc_ic != NULL);
}


void
Terminal::deletechars(int num)
{
	if (tc_dc == NULL)
		return;

	filep = fp;

	while (num-- > 0)
		tputs(tc_dc, 0, putchr);
}


void
Terminal::setinsert(int mode)
{
	if (insert == mode || (tc_im == NULL && tc_ei == NULL && tc_ic == NULL))
		return;

	filep = fp;
	insert = mode;

	if (tc_im != NULL && tc_ei != NULL)
		tputs(mode ? tc_im : tc_ei, 0, putchr);
}


boolean
Terminal::membelow()
{
	return tc_db;
}


boolean
Terminal::canenhance(int enh)
{
	switch (enh)
	{
	case T_UNDERLINE:
		return tc_us != NULL && tc_ue != NULL;

	case T_ITALIC:
		return tc_ital != NULL && tc_se != NULL;

	case T_DIM:
		return tc_dim != NULL && tc_se != NULL;

	case T_BOLD:
		return tc_bold != NULL && tc_se != NULL;

	case T_STANDOUT:
		return tc_so != NULL && tc_se != NULL;

	default:
		return FALSE;
	}
}


void
Terminal::setenhance(int mode)
{
	if (enhance == mode)
		return;

	filep = fp;
	int a, b;

	a = mode & T_UNDERLINE;
	b = enhance & T_UNDERLINE;

	if (a != b && tc_us != NULL && tc_ue != NULL)
		tputs(a ? tc_us : tc_ue, 0, putchr);

	a = mode & (T_ITALIC | T_BOLD | T_DIM | T_STANDOUT);
	b = enhance & (T_ITALIC | T_BOLD | T_DIM | T_STANDOUT);

	if (a != b && tc_se != NULL)
		tputs(tc_se, 0, putchr);

	if ((mode & T_ITALIC) && tc_ital != NULL)
		tputs(tc_ital, 0, putchr);

	if ((mode & T_BOLD) && tc_bold != NULL)
		tputs(tc_bold, 0, putchr);

	if ((mode & T_DIM) && tc_dim != NULL)
		tputs(tc_dim, 0, putchr);

	if ((mode & T_STANDOUT) && tc_so != NULL)
		tputs(tc_so, 0, putchr);

	enhance = mode;
}


void
Terminal::getloc(int &line, int &col)
{
	line = oln;
	col = ocol;
}


boolean
typeahead(FILE * fp)
{
	int fd = fileno(fp);
	int f = fcntl(fd, F_GETFL, 0);

	fcntl(fd, F_SETFL, f | O_NDELAY);

	int c = getc(fp);
	fcntl(fd, F_SETFL, f & ~O_NDELAY);

	if (c < 0)
		return FALSE;

	ungetc(c, fp);
	return TRUE;
}
