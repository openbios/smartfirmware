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


// This is the usual obligatory calculator sample.  It even handles the
// precedence of C operators properly while demonstrating how to use
// most of wacco's features.

{
#include <stdio.h>
#include <stdlib.h>
}

expr<double>
	: andand { $_ = $andand; }
		("||" andand { $$ = $$ || $andand; })* { $$ = $_; }
	;

andand<double>
	: or { $_ = $or; }
		("&&" or { $$ = $$ && $or; })* { $$ = $_; }
	;

or<double>
	: xor { $_ = $xor; }
		('|' xor { $$ = (long)$$ | (long)$xor; })* { $$ = $_; }
	;

xor<double>
	: and { $_ = $and; }
		('^' and { $$ = (long)$$ ^ (long)$and; })* { $$ = $_; }
	;

and<double>
	: equal { $_ = $equal; }
		('&' equal { $$ = (long)$$ & (long)$equal; })* { $$ = $_; }
	;

equal<double>
	: compare { $_ = $compare; }
		( "==" compare { $$ = $$ == $compare; }
		| "!=" compare { $$ = $$ != $compare; }
		)* { $$ = $_; }
	;

compare<double>
	: plus { $_ = $plus; }
		( '<' plus { $$ = $$ < $plus; }
		| '>' plus { $$ = $$ > $plus; }
		| "<=" plus { $$ = $$ <= $plus; }
		| ">=" plus { $$ = $$ >= $plus; }
		)* { $$ = $_; }
	;

plus<double>
	: mult { $_ = $mult; }
		( '+' mult { $$ += $mult; }
		| '-' mult { $$ -= $mult; }
		)* { $$ = $_; }
	;

mult<double>
	: unary { $_ = $unary; }
		( '*' unary { $$ *= $unary; }
		| '/' unary { $$ /= $unary; }
		| '%' unary { $$ = (long)$$ % (long)$unary; }
		)* { $$ = $_; }
	;

unary<double>
	: DOUBLE { $$ = atof((char *)yytext); }
	| '-' expr { $$ = -$expr; }
	| '~' expr { $$ = ~(long)$expr; }
	| '!' expr { $$ = !$expr; }
	| '(' expr ')' { $$ = $expr; }
	;

calc %export
	: ( expr (':' | []) ('=' | ';' | ',' )? { printf("%f\n", $expr); } )*
	;

{
	int
	main()
	{
		w_setfile(stdin);
		return !calc();
	}
}

$$

D	[0-9]
L	[_A-Za-z]

%%

"."		{ return (int)W_EOI; }

$DOUBLE		({D}+)|({D}+\.{D}*)|({D}+[Ee]-?{D}+)|({D}+\.{D}*[Ee]-?{D}+)

"#".*$		;

[ \t\v\n\f]	;
.	{ w_scanerr("Illegal character %d (%c)", yytext[0], yytext[0]); }
