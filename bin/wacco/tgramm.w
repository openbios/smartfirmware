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

// Copyright (c) 2001 by Parag Patel.  All Rights Reserved.

%opt -O -l Modula-3 -h Wtest.i3 -p Wtest.m3

program
	: Program Id '(' idlist ')' semi
			declarations subprogdecls compoundstat '.'
	;

semi
	: ';'[semi+]
	;

idlist
	: Id (',' Id)*
	;

declarations
	: Var (decllist | semi)
	| []
	;

decllist
	: idlist ':' type semi decllist*
	;

type
	: Integer
	| Array '[' Int Dotdot Int ']' Of Integer
	;

subprogdecls
	: subproghead declarations subprogdecls
			compoundstat semi subprogdecls
	| []
	;

subproghead
	: Function Id arguments ':' Integer semi
	| Procedure Id arguments semi
	;

arguments
	: '(' paramlist ')'
	| []
	;

paramlist
	: idlist ':' type (semi paramlist)*
	;

compoundstat
	: Begin statlist End [+semi+]
	;

statlist
	: statement semi statlist
	| []
	;

statement
	: Id assignment
	| compoundstat
	| If expression Then statement (Else statement)?
	| While expression Do statement
	| For varref Assign expression (To | Downto)
			expression Do statement
	;

assignment
	: '[' expression ']' Assign expression
	| '(' exprlist ')'
	| Assign expression
	| []
	;

varref
	: Id ('[' expression ']')?
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
	: Id ('(' exprlist ')' | '[' expression ']' [')'])?
	| Int
	| '(' expression ')'
	| Not expression
	| ('+' | '-') expression
	;

{
	VAR
		fd : Rd.T := Stdio.stdin;
		line : Text.T;
		unget : CHAR := '\000';
		linenum : INTEGER := 0;

	PROCEDURE w_input() : CHAR =
	VAR
		c : CHAR;
	BEGIN
		IF unget # '\000' THEN
			c := unget;
			unget := '\000';
			RETURN c;
		END;

		IF Rd.EOF(fd) THEN
			RETURN '\000';
		END;

		c := Rd.GetChar(fd);
		(* Wr.PutChar(Stdio.stderr, c); *)

		IF c = '\n' THEN
			linenum := linenum + 1;
		END;

		RETURN c;
	END w_input;

	PROCEDURE w_unput(c : CHAR) =
	BEGIN
		unget := c;
	END w_unput;

	PROCEDURE w_gettoken() : INTEGER =
	VAR
		c : CHAR;
	BEGIN
		c := w_input();

		IF c = '\000' THEN
			RETURN W_EOI;
		END;

		WHILE c = ' ' OR c = '\t' OR c = '\n' OR c = '\r' DO
			c := w_input();
		END;

		IF c = '\000' THEN
			RETURN W_EOI;
		END;

		CASE c OF
		'A'..'Z', 'a'..'z' =>
			line := "";

			WHILE (c >= 'A' AND c <= 'Z') OR (c >= 'a' AND c <= 'z') DO
				(* TODO - really inefficient but who cares *)
				line := line & Text.FromChar(c);
				c := w_input();
			END;

			w_unput(c);

			IF Text.Equal(line, "program") THEN
				RETURN Program;
			ELSIF Text.Equal(line, "var") THEN
				RETURN Var;
			ELSIF Text.Equal(line, "array") THEN
				RETURN Array;
			ELSIF Text.Equal(line, "integer") THEN
				RETURN Integer;
			ELSIF Text.Equal(line, "of") THEN
				RETURN Of;
			ELSIF Text.Equal(line, "function") THEN
				RETURN Function;
			ELSIF Text.Equal(line, "procedure") THEN
				RETURN Procedure;
			ELSIF Text.Equal(line, "begin") THEN
				RETURN Begin;
			ELSIF Text.Equal(line, "end") THEN
				RETURN End;
			ELSIF Text.Equal(line, "if") THEN
				RETURN If;
			ELSIF Text.Equal(line, "then") THEN
				RETURN Then;
			ELSIF Text.Equal(line, "else") THEN
				RETURN Else;
			ELSIF Text.Equal(line, "while") THEN
				RETURN While;
			ELSIF Text.Equal(line, "do") THEN
				RETURN Do;
			ELSIF Text.Equal(line, "for") THEN
				RETURN For;
			ELSIF Text.Equal(line, "to") THEN
				RETURN To;
			ELSIF Text.Equal(line, "downto") THEN
				RETURN Downto;
			ELSIF Text.Equal(line, "not") THEN
				RETURN Not;
			END;

			RETURN Id;

		| '0'..'9' =>
			line := "";

			WHILE c >= '0' AND c <= '9' DO
				(* TODO - really inefficient but who cares *)
				line := line & Text.FromChar(c);
				c := w_input();
			END;

			w_unput(c);
			RETURN Int;

		| '.' =>
			c := w_input();

			IF c # '.' THEN
				w_unput(c);
				RETURN ORD('.');
			END;

			RETURN Dotdot;

		| ':' =>
			c := w_input();

			IF c # '=' THEN
				w_unput(c);
				RETURN ORD(':');
			END;

			RETURN Assign;
		ELSE
		END;

		(* ELSE *)
		RETURN ORD(c);
	END w_gettoken;

	PROCEDURE w_err_expected(str : TEXT) =
	BEGIN
		Wr.PutText(Stdio.stderr, Fmt.Int(linenum) & ": expected " & str & "\n");
	END w_err_expected;

	PROCEDURE w_err_illegal(str : TEXT) =
	BEGIN
		Wr.PutText(Stdio.stderr, Fmt.Int(linenum) & ": illegal " & str & "\n");
	END w_err_illegal;

	PROCEDURE w_err_skip() =
	BEGIN
	END w_err_skip;

	PROCEDURE w_flusherrs() =
	BEGIN
	END w_flusherrs;
}
