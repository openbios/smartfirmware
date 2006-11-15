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


main(int argc, char *argv[])
{
#ifdef DLD
	dldquit = quit;
	dlderror = error;
	dldinit(argv[0]);
#endif

	Buffer buf;
	// Terminal term;
	Display disp;
	Window win(disp, buf);
	buf.doundo = TRUE;

	buf.insert("This is a test.\n");
	buf.insert("This is ONLY a test.\n");
	buf.dump();

	Mark mark(buf);
	mark = 20;

	buf.moveto(13);
	debug("line 0 == %d\n", buf.getlineloc(0));
	debug("line 1 == %d\n", buf.getlineloc(1));
	debug("line 2 == %d\n", buf.getlineloc(2));
	debug("line 3 == %d\n", buf.getlineloc(3));
	buf.kill(10);

	buf.dump();
	debug("mark() == %d\n", mark());
	debug("length == %d\n", buf.length());
	debug("char[5] == %c   char[19] == %c\n", buf[5], buf[19]);

	if (buf.startundo())
		debug("err start\n");

	if (buf.prevundo())
		debug("err prev\n");

	buf.endundo();
	buf.dump();
	debug("mark() == %d\n\n", mark());

	win.setloc(buf.getloc());
	drawwindows();

	int ch;

	while ((ch = getchar()) >= 0)
	{
		Buftext str[2];

		if (isprint(ch))
		{
			str[0] = ch;
			buf.insert(str, 1);
		}
		else
			switch (ch)
			{
			Case '\n':
			case '\r':
				str[0] = '\n';
				buf.insert(str, 1);

			Case CTRL(H):
			case DEL:
				buf.killprev(1);

			Case CTRL(D):
				buf.kill(1);

			Case CTRL(A):
				buf.moveto(buf.getlineloc(buf.getline()));

			Case CTRL(E):
				buf.moveto(buf.getlineloc(buf.getline()) +
						buf.getlinelen() - 1);

			Case CTRL(N):
				buf.moveto(buf.getlineloc(buf.getline() + 1));

			Case CTRL(P):
				buf.moveto(buf.getlineloc(buf.getline() - 1));

			Case CTRL(F):
				buf.moverel(1);

			Case CTRL(B):
				buf.moverel(-1);

			Case CTRL(L):
				disp.redraw();

			Case CTRL(X):
				buf.dump();

			Case ESC:
				buf.dump();
				return 0;

			default:
				disp.beep();
			}

		win.setloc(buf.getloc());
		drawwindows();
	}
}
