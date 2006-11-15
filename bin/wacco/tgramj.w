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

// Copyright (c) 1991,2000 by Parag Patel.  All Rights Reserved.

%opt -l Java -h Wtoken.java -p Wparse.java

{
	import java.io.*;
}

program
	: PROGRAM ID '(' idlist ')' semi
			declarations subprogdecls compoundstat '.'
	;

semi
	: ';'[semi+]
	;

idlist
	: ID (',' ID)*
	;

declarations
	: VAR (decllist | semi)
	| []
	;

decllist
	: idlist ':' type semi decllist*
	;

type
	: INTEGER
	| ARRAY '[' INT DOTDOT INT ']' OF INTEGER
	;

subprogdecls
	: subproghead declarations subprogdecls
			compoundstat semi subprogdecls
	| []
	;

subproghead
	: FUNCTION ID arguments ':' INTEGER semi
	| PROCEDURE ID arguments semi
	;

arguments
	: '(' paramlist ')'
	| []
	;

paramlist
	: idlist ':' type (semi paramlist)*
	;

compoundstat
	: BEGIN statlist END [+semi+]
	;

statlist
	: statement semi statlist
	| []
	;

statement
	: ID assignment
	| compoundstat
	| IF expression THEN statement (ELSE statement)?
	| WHILE expression DO statement
	| FOR varref ASSIGN expression (TO | DOWNTO)
			expression DO statement
	;

assignment
	: '[' expression ']' ASSIGN expression
	| '(' exprlist ')'
	| ASSIGN expression
	| []
	;

varref
	: ID ('[' expression ']')?
	;

exprlist
	: expression (',' exprlist)?
	;

expression
	: term (('+' | '-') expression)?
	;

term
	: expr (('*' | '/') term)?
	;

expr
	: ID ('(' exprlist ')' | '[' expression ']' [')'])?
	| INT
	| '(' expression ')'
	| NOT expression
	| ('+' | '-') expression
	;

{
	static InputStreamReader in;
	static StringBuffer line = new StringBuffer();
	static char unget = 0;
	static int linenum = 0;

    public static void
    main(String[] argv)
    {
	    Wparse wparse = new Wparse();

		in = new InputStreamReader(System.in);
		wparse.program();
    }

	static char
	w_input()
	{
		char c;

		if (unget != 0)
		{
			c = unget;
			unget = 0;
			return c;
		}

		try
		{
			int i = in.read();

			if (i == -1)		// appears to be EOF
				i = 0;
			
			c = (char)i;
		}
		catch (IOException ioe)
		{
			return 0;
		}

		if (c == '\n')
			linenum++;

		return c;
	}

	static void
	w_unput(char c)
	{
		unget = c;
	}

	public static int
	w_gettoken()
	{
		char c;
		int loc = 0;

		for (c = w_input(); c != 0; c = w_input())
		{
			while (Character.isWhitespace(c))
				c = w_input();

			if (Character.isLetter(c))
			{
				for (line.setLength(0); Character.isLetter(c); c = w_input())
					line.append(c);

				w_unput(c);

				String s = line.toString();

				s.toLowerCase();

				if (s.compareTo("program") == 0)
					return Wtoken.PROGRAM;
				else if (s.compareTo("var") == 0)
					return Wtoken.VAR;
				else if (s.compareTo("array") == 0)
					return Wtoken.ARRAY;
				else if (s.compareTo("integer") == 0)
					return Wtoken.INTEGER;
				else if (s.compareTo("of") == 0)
					return Wtoken.OF;
				else if (s.compareTo("function") == 0)
					return Wtoken.FUNCTION;
				else if (s.compareTo("procedure") == 0)
					return Wtoken.PROCEDURE;
				else if (s.compareTo("begin") == 0)
					return Wtoken.BEGIN;
				else if (s.compareTo("end") == 0)
					return Wtoken.END;
				else if (s.compareTo("if") == 0)
					return Wtoken.IF;
				else if (s.compareTo("then") == 0)
					return Wtoken.THEN;
				else if (s.compareTo("else") == 0)
					return Wtoken.ELSE;
				else if (s.compareTo("while") == 0)
					return Wtoken.WHILE;
				else if (s.compareTo("do") == 0)
					return Wtoken.DO;
				else if (s.compareTo("for") == 0)
					return Wtoken.FOR;
				else if (s.compareTo("to") == 0)
					return Wtoken.TO;
				else if (s.compareTo("downto") == 0)
					return Wtoken.DOWNTO;
				else if (s.compareTo("not") == 0)
					return Wtoken.NOT;

				return Wtoken.ID;
			}
			else if (Character.isDigit(c))
			{
				for (line.setLength(0); Character.isDigit(c); c = w_input())
					line.append(c);

				w_unput(c);
				return Wtoken.INT;
			}
			else if (c == '.')
			{
				c = w_input();

				if (c != '.')
				{
					w_unput(c);
					return '.';
				}

				return Wtoken.DOTDOT;
			}
			else if (c == ':')
			{
				c = w_input();

				if (c != '=')
				{
					w_unput(c);
					return ':';
				}

				return Wtoken.ASSIGN;
			}
			else
				return c;
		}

		return Wtoken.W_EOI;
	}

	public static void
	w_err_expected(String str)
	{
		System.out.println(linenum + ": expected " + str);
	}

	public static void
	w_err_illegal(String str)
	{
		System.out.println(linenum + ": illegal " + str);
	}

	public static void
	w_err_skip()
	{
	}

	public static void
	w_flusherrs()
	{
	}

}
