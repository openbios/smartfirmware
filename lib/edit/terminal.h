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

#ifndef	_TERMINAL_H_
#define	_TERMINAL_H_


#include <fcntl.h>

#if defined SGTTY	/* BSD sgtty */

#ifdef CTRL
#undef CTRL
#endif

#include <sgtty.h>
#define TGET TIOCGETP
#define TSET TIOCSETP
#define TSETW TIOCSETP
typedef struct sgttyb TERMIO;

#elif defined TERMIO /* SysV termio */

#include <termio.h>
#define TGET TCGETA
#define TSET TCSETA
#define TSETW TCSETAW
typedef struct termio TERMIO;

#else	/* POSIX termios */

#include <termios.h>
#define TGET TIOCGETA
#define TSET TIOCSETA
#define TSETW TIOCSETAW
typedef struct termios TERMIO;

#endif	/* terms */


typedef short Disptext;

const int T_NONE		= 0;
const int T_CHAR		= 0x00FF;
const int T_ENHANCE		= 0x7F00;
const int T_SPECIAL		= 0x8000;

const int T_STANDOUT	= 0x0100;
const int T_BOLD		= 0x0200;
const int T_DIM			= 0x0400;
const int T_ITALIC		= 0x0800;
const int T_UNDERLINE	= 0x1000;
const int T_UNDEF2		= 0x2000;
const int T_UNDEF3		= 0x4000;


extern boolean typeahead(FILE *fp);

class Terminal
{
	char buf[1025],
	*tc_cm, *tc_ti, *tc_te, *tc_ce, *tc_cd, *tc_up, *tc_do,
	*tc_le, *tc_nd, *tc_dl, *tc_al, *tc_im, *tc_ei, *tc_dc,
	*tc_ic, *tc_ip, *tc_ks, *tc_ke, *tc_so, *tc_se, *tc_us,
	*tc_ue, *tc_bold, *tc_ital, *tc_dim;
	int cm_len, tc_co, tc_li, tc_mi, tc_xs, tc_db;
	int ocol, oln;
	boolean insert, standout, optimize;
	int enhance;

	FILE *fp;
	TERMIO oldterm, rawterm;

public:
	Terminal(char *tty = NULL, char *termtype = NULL);
	~Terminal();

	void init();
	void fini();

	void beep();
	void move(int ln, int col);
	void reset(int ln, int col);
	void newline();
	void put(int c);
	void putf(int ln, int col, char *fmt, ...);
	void puts(int ln, int col, char *str);
	void puttext(Disptext *str, int len);

	void clearline();
	void clearscreen();

	boolean caninsdelline();
	void deleteline(int ln);
	void insertline(int ln);

	boolean caninsdelchars();
	void deletechars(int num);
	void setinsert(int mode);

	boolean membelow();
	boolean canenhance(int enh);
	void setenhance(int enh);

	int numlines() { return tc_li; }
	int numcols() { return tc_co - 1; }
	void getloc(int &line, int &col);
};


#endif	/* _TERMINAL_H_ */
