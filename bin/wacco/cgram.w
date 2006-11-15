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

// Copyright (c) 1993,2000-2001 by Parag Patel.  All Rights Reserved.

/* This is a sample grammar that almost parses ANSI C.  It is based on
   the official ANSI C grammar as documented in Appendix A of the C89
   standard.  The rules are as similar to the sample grammar as possible.

   For example purposes, a TYPENAME is any identifier that begins with a
   capital letter.  Otherwise it is a plain IDENTIFIER.  A complete
   scanner would lookup an identifier in a symbol-table and return the
   appropriate token value.

   Much of the lex section below is likely to have bugs.  It is appended
   only as an example and is not intended to correctly identify numbers,
   floating-point value, or strings.  It should correctly tokenize the
   explicit character and string tokens in the grammar rules below.

   In most places, a single token lookahead is sufficient to parse the
   language.  There are two specific cases of typecasts and labels that
   must lookahead one additional token (LL(2)).  These are labeled
   "X_TYPENAME" and "X_LABEL" in the grammar below.

   A fully-operational parser would need to perform additional lookahead
   and return one of these fake tokens at that point in the rule before
   the rule is fully evaluated.  (These bogus tokens are given values in
   the lex section at the bottom of the file only for demonstration
   purposes - they should not be real tokens.)

   The expression parsing code can be much faster than shown below by
   eliminating most of the function calls and creating a pseudo-precedence
   grammar.  This is left as an exercise for the student. :)

   Known non-standard features of the grammar are marked with "!ISO".
   Unknown ones are left as another exercise.
 */

{
#include <stdio.h>
}

// A.2.1  Expressions

// 3.3.1
primary_expr
	//: identifier
	//| constant
	: IDENTIFIER
	| string_literal
	| '(' expression ')'
	| INTVAL
	| UINTVAL
	| LONGVAL
	| ULONGVAL
	| CHARACTER
	| WCHARACTER
	| FLOATVAL
	| DOUBLEVAL
	| LONGDOUBLEVAL
	// !ISO - short doubles
	| SHORTDOUBLEVAL
	// !ISO - long long integers
	| LONGLONGVAL
	| ULONGLONGVAL
	;

identifier
	: IDENTIFIER
	| TYPENAME
	;

string_literal
	: STRING+
	| WSTRING+
	;

// 3.3.2
postfix_expr
	: primary_expr
		( '[' expression ']'
		| '(' argument_expr_list ')'
		| '.' identifier
		| "->" identifier
		| "++"
		| "--"
		)*
	;

argument_expr_list
	: assignment_expr (',' assignment_expr)*
	| []
	;

// 3.3.3
unary_expr
	: postfix_expr
	| "++" unary_expr
	| "--" unary_expr
	|
		( '&' 
	    | '*'
	    | '+'
	    | '-'
	    | '~'
	    | '!'
	    ) cast_expr
	| "sizeof"
	    ( X_TYPENAME '(' type_name ')'
	    | unary_expr
	    )
	;

// 3.3.4
cast_expr
	:
	    ( X_TYPENAME '(' type_name ')'
			( '{' initializer_list '}'
			| []
			)
	    )*
	    unary_expr
	;

// 3.3.5
multiplicative_expr 
	: cast_expr (('*' | '/' | '%') cast_expr)* 
	;

// 3.3.6
additive_expr
	: multiplicative_expr ( ('+' | '-') multiplicative_expr )* 
	;

// 3.3.7
shift_expr
	: additive_expr ( ("<<" | ">>") additive_expr )*
	;

// 3.3.8
relational_expr
	: shift_expr (('<' | '>' | "<=" | ">=") shift_expr )*
	;

// 3.3.9
equality_expr
	: relational_expr (("==" | "!=") relational_expr)*
	;

// 3.3.10
bitand_expr
	: equality_expr ('&' equality_expr)*
	;

// 3.3.11
bitxor_expr
	: bitand_expr ('^' bitand_expr)*
	;

// 3.3.12
bitor_expr
	: bitxor_expr ('|' bitxor_expr)*
	;

// 3.3.13
logical_and_expr
	: bitor_expr ("&&" bitor_expr)*
	;

// 3.3.14
logical_or_expr
	: logical_and_expr ("||" logical_and_expr)*
	;

// 3.3.15
conditional_expr
	: logical_or_expr ('?' expression ':' logical_or_expr)*
	;

// 3.3.16
assignment_expr	// TODO
	//: conditional_expr
	//| cast_expr ('=' | "*=" | "/=" | "%=" | "+="
	//	| "-=" | "<<=" | ">>="
	//	| "&=" | "^=" | "|=") assignment_expr
	: conditional_expr
		(
			('=' | "*=" | "/=" | "%=" | "+="
			| "-=" | "<<=" | ">>="
			| "&=" | "^=" | "|=") assignment_expr
		| []
		)
	;

// 3.3.17
expression
	: assignment_expr (',' assignment_expr)*
	;

// 3.3.18
constant_expr
	: conditional_expr
	;


// A.2.2  Declarations

// 3.5
declaration
	: declaration_specifiers init_declarator_list?  ';'
	;

declaration_specifiers
	: ( storage_class_specifier
	  | type_specifier
	  | type_qualifier
	  )+
	;

init_declarator_list
	: init_declarator ( ',' init_declarator )*
	;

init_declarator
	: declarator
	    ( '=' initializer 
	    | []
	    )
	;

// 3.5.1
storage_class_specifier
	: "typedef"
	| "extern"
	| "static"
	| "auto"
	| "register"
	;

// 3.5.2
type_specifier
	: "void"
	| "char"
	| "short"
	| "int"
	| "long"
	| "float"
	| "double"
	| "signed"
	| "unsigned"
	| struct_union_specifier
	| enum_specifier
	//| typedef_name
	| TYPENAME
	;

// 3.5.2.1
struct_union_specifier
	: struct_or_union
	    ( identifier
			( '{' struct_declaration_list '}'
			| []
			)
	    | '{' struct_declaration_list '}'
	    )
	;

struct_or_union
	: "struct"
	| "union"
	;

struct_declaration_list
	: struct_declaration+
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list? ';'
	;

specifier_qualifier_list
	:
	  ( type_specifier
	  | type_qualifier
	  )+
	;

struct_declarator_list
	: struct_declarator (',' struct_declarator)*
	;

struct_declarator
	: declarator?
	    ( ':' constant_expr
	    | []
	    )
	;

// 3.5.2.2
enum_specifier
	: "enum"
	    ( identifier
			( '{' enumerator_list '}'
			| []
			)
	    | '{' enumerator_list '}'
	    )
	;

enumerator_list
	//: enumerator (',' ( enumerator | []))* ;
	: enumerator (',' enumerator )*
	;

enumerator
	//: enumeration_const ('=' constant_expr | [])
	: identifier
	    ( '=' constant_expr
	    | []
	    )
	;

//enumeration_const
//	: identifier
//	;

// 3.5.3
type_qualifier
	: "const"
	| "volatile"
	;

// 3.5.4
declarator
	: pointer direct_declarator
	;

direct_declarator
	:
		( identifier
	    | '(' declarator ')'
	    )

	    ( array
	    //| '(' (parameter_type_list | identifier_list | []) ')'
	    | '('
			( parameter_type_list
			| []
			)
			')'
	    | []
	    )
	;

pointer
	: ( '*' type_qualifier_list )*
	;

array
	: '[' constant_expr? ']' array?
	;

type_qualifier_list
	: type_qualifier*
	;

/***
parameter_type_list
	:
	    parameter_list
	    (',' "..."
	    | []
	    )
	;
parameter_list
***/

parameter_type_list
	: parameter_declaration
	    ( ','
			( parameter_declaration
			| "..."
			)
	    )*
	;

parameter_declaration
	//: declaration_specifiers (declarator | abstract_declarator | [])
	: declaration_specifiers param_declarator?
	;

param_declarator
	: pointer direct_param_declarator
	;

direct_param_declarator
	:
		( identifier
		| '(' param_declarator ')'
		| []	// abstract declarator
	    )
		( array
		| '(' parameter_type_list? ')'
		| []
		)
	;

identifier_list
	//: identifier (',' identifier)*
	: IDENTIFIER (',' IDENTIFIER )*
	; 

// 3.5.5
type_name
	: specifier_qualifier_list abstract_declarator?
	;

abstract_declarator
	: pointer direct_abstract_declarator
	;

direct_abstract_declarator
	:
		( '(' abstract_declarator ')'
		| []
		)
		( array
		| '(' parameter_type_list?  ')'
		| []
		)

	;

// 3.5.6
//typedef_name
//	: TYPENAME //identifier
//	;

// 3.5.7
initializer
	: assignment_expr
	| '{' initializer_list '}'
	;

initializer_list
	:
		( '[' constant_expr ']'
	    | []
	    )
	    initializer
	    ( ','
			( '[' constant_expr ']'
			| []
			)
			initializer
	    )*
	;


// A.2.3  Statements

// 3.6
statement
	: labeled_statement
	| compound_statement
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	// !ISO
	| asm_statement
	;

// 3.6.1
labeled_statement

	: X_LABEL identifier ':' statement
	| "case" constant_expr
		// !ISO
		( "..." constant_expr
		| []
		)
		':' statement
	| "default" ':' statement
	;

// 3.6.2
compound_statement
	//: '{' (declaration_list | []) (statement_list | []) '}'
	: '{' declaration_list statement_list '}'
	;

declaration_list
	: declaration*
	;

statement_list
	: statement*
	;

// 3.6.3
expression_statement
	: opt_expr ';'
	;

// 3.6.4
selection_statement
	: "if" '(' expression ')' statement
		( "else" statement
		| []
		)
	| "switch" '(' expression ')' statement
	;

// 3.6.5
iteration_statement
	: "while" '(' expression ')' statement
	| "do" statement "while" '(' expression ')' ';'
	| "for" '(' opt_expr ';' opt_expr ';' opt_expr ')' statement
	;

opt_expr
	: expression?
	;

// 3.6.6
jump_statement
	: "goto" identifier ';'
	| "continue" ';'
	| "break" ';'
	| "return" opt_expr ';'
	;

// !ISO
asm_statement
	: "asm" '(' string_literal ')' ';'
	;

// A.2.4  External Definitions

// 3.7
translation_unit %export
	: external_declaration+
	;

// 3.7.1
external_declaration
	:
	    ( declaration_specifiers ';'?
	    | []
	    )
	    ext_declarator
		( declaration_list
			//compound_statement
			'{' declaration_list statement_list '}'
		| ( '=' initializer 
			| []
			)
			(',' init_declarator_list 
			| []
			) ';'
		)
	// !ISO
	| asm_statement
	;

ext_declarator
	: pointer direct_ext_declarator
	;

direct_ext_declarator
	:
		( identifier
	    | '(' ext_declarator ')'
	    )
	    ( array
	    | '('
			( parameter_type_list
			| identifier_list
			| []
			)
			')'
	    | []
	    )
	;

/*****
external_declaration
	: function_definition
	| declaration
	;

// 3.7.1
function_definition
	: (declaration_specifiers | []) declarator (declaration_list | [])
		compound_statement
	;
*****/

{
    int
    main(int argc, const char *argv[])
    {
	    w_setfile(stdin);

	    if (!translation_unit())
		    return 1;

	    return 0;
    }
}

$$

D	[0-9]
OD	[0-7]
HD	[0-9A-Fa-f]
INT	("0x"{HD}+|"0"{OD}+|{D}+)
FLT	({D}+"."{D}+|{D}+"."|"."{D}+)([Ee]{D}+)?

L	[A-Za-z_$]
UC	[A-Z]
LC	[a-z_]

OC	"\\"([0-7]+|.)

%%

$X_TYPENAME		"typeof"
$X_LABEL		"label"

$IDENTIFIER		{LC}({L}|{D})*
$TYPENAME		{UC}({L}|{D})*

$INTVAL			{INT}
$UINTVAL		{INT}[Uu]
$LONGVAL		{INT}[Ll]
$ULONGVAL		{INT}([Uu][Ll]|[Ll][Uu])
$LONGLONGVAL	{INT}[Ll][Ll]
$ULONGLONGVAL	{INT}([Uu][Ll][Ll]|[Ll][Ll][Uu])

$FLOATVAL		{FLT}[Ff]
$DOUBLEVAL		{FLT}
$LONGDOUBLEVAL	{FLT}[Ll]
$SHORTDOUBLEVAL	{FLT}[Ss]

$CHARACTER		\'([^']|{OC})*\'
$WCHARACTER		L\'([^']|{OC})*\'
$STRING			\"([^"]|{OC})*\"
$WSTRING		L\"([^"]|{OC})*\"

^[ \t]*"#".*$	;
[ \t\v\n\f]	;
.			{ w_scanerr("Illegal character %d (%c)", yytext[0], yytext[0]); }

