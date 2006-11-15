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

#include <stdio.h>
#include <curses.h>

int
main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;

	FILE *fp = fopen(argv[1], "r");

	if (fp == NULL)
		return 2;

	initscr();
	noecho();
	cbreak();
	nonl();
	idlok(stdscr, TRUE);
	typeahead(fileno(stdin));

	int cmd = 0;
	char buf[100];

	for (;;)
	{
		if (cmd == '\n' || cmd == '\r')
		{
			move(0, 0);
			deleteln();
			move(LINES - 1, 0);

			if (fgets(buf, sizeof buf, fp) == NULL)
				break;

			printw("%s", (int)buf);
		}
		else
		{
			int line;

			move(0, 0);

			for (line = 2; line < LINES; line++)
				deleteln();

			for (line = 2; line < LINES; line++)
			{
				if (fgets(buf, sizeof buf, fp) == NULL)
				{
					if (line == LINES - 1)
					{
						move(0, 0);
						deleteln();
						move(LINES - 1, 0);
					}

					goto out;
				}

				move(line, 0);
				printw("%s", (int)buf);
			}
		}

		refresh();

		if ((cmd = getch()) == 'q' || cmd == 'Q')
			break;
	}
out:

	clrtobot();
	move(LINES - 1, 0);
	refresh();
	endwin();
	return 0;
}
