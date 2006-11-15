%{
// Copyright (c) 1992 by Parag Patel.  All Rights Reserved.
// Portions Copyright (c) 1989,1990 James A. Roskind
// $Header: /u/cgt/cvs/bin/cc/ccgram.y,v 1.59 1998/01/25 19:34:30 parag Exp $

#define IN_YACC_FILE
#include "cpp.h"
#include "cc.h"
#include "cctype.h"
#include "ccsym.h"
#include "ccnode.h"

//#define YYDEBUG 1
%}

%token DOTDOTDOT
%token ARROW
%token INCR DECR
%token LSHIFT RSHIFT
%token LESSEQ GREATEQ EQUAL NOTEQ
%token ANDAND OROR
%token MULTEQ DIVEQ MODEQ
%token PLUSEQ MINUSEQ
%token LSHIFTEQ RSHIFTEQ
%token ANDEQ XOREQ OREQ

%token INTVAL UINTVAL LONGVAL ULONGVAL LONGLONGVAL ULONGLONGVAL
%token DOUBLEVAL FLOATVAL LONGDOUBLEVAL SHORTDOUBLEVAL
%token CHARACTER WCHARACTER
%token STRING WSTRING

%token IDENTIFIER TYPENAME

%token SIZEOF TYPEDEF EXTERN STATIC AUTO REGISTER VOID CHAR
%token SHORT INT LONG FLOAT DOUBLE SIGNED UNSIGNED STRUCT
%token UNION ENUM CONST VOLATILE CASE DEFAULT IF ELSE
%token SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token ASM PASCAL FORTRAN

%start translation_unit

%union
{
    Token token;
    Node *node;
    Expr_node *expr;
    String_node *string;
    Initializer_node *init;
    Node_arr *nodearr;
    Expr_arr *exprarr;
    Type *type;
    Type_struct *type_struct;
    Type_enum *type_enum;
    const char *sym;
    const char *cstr;
    short integer;
    Storage_thing storage;
    struct Charvec *charvec;
    Param_arr *paramarr;
    Symbol *symbol;
}

%right <expr> '?' ':'
%left <expr> OROR
%left <expr> ANDAND
%left <expr> '|'
%left <expr> '^'
%left <expr> '&'
%left <expr> EQUAL NOTEQ
%left <expr> '<' '>' LESSEQ GREATEQ
%left <expr> LSHIFT RSHIFT
%left <expr> '+' '-'
%left <expr> '*' '/' '%'

%type <token> IDENTIFIER TYPENAME INTVAL UINTVAL LONGVAL ULONGVAL
%type <token> LONGLONGVAL ULONGLONGVAL
%type <token> DOUBLEVAL FLOATVAL LONGDOUBLEVAL SHORTDOUBLEVAL
%type <token> CHARACTER WCHARACTER
%type <token> STRING WSTRING CASE DEFAULT GOTO ASM
%type <token> identifier

%type <expr> postfix_expr unary_expr cast_expr base_expr
%type <expr> expression expr opt_expr opt_expression constant_expr
%type <expr> assignment_expr
%type <string> string_literal wstring_literal
%type <exprarr> argument_expr_list
%type <init> init_list
%type <expr> initializer initializer_opt initializer_list

%type <storage> declaration_qualifier declaration_qualifier_list
%type <integer> type_qualifier storage_class
%type <storage> type_qualifier_list type_name

%type <node> statement comp_stmt asm_statement
%type <node> labeled_statement compound_statement expression_statement
%type <node> selection_statement iteration_statement jump_statement
%type <nodearr> statement_list

%type <storage> declaring_list default_declaring_list
%type <storage> sue_declaration_specifier sue_type_specifier
%type <cstr> paren_identifier_declarator
%type <storage> declarator identifier_declarator abstract_declarator
%type <storage> unary_identifier_declarator postfix_identifier_declarator
%type <type> postfixing_abstract_declarator array_abstract_declarator
%type <storage> declaration_specifier type_specifier basic_type_name
%type <storage> basic_declaration_specifier basic_type_specifier

%type <storage> typedef_declaration_specifier typedef_type_specifier
%type <storage> elaborated_type_name aggregate_name
%type <type_struct> aggregate_key
%type <type_enum> enum
%type <storage> member_default_declaring_list member_declaring_list
%type <storage> member_declarator member_identifier_declarator
%type <integer> bit_field_size_opt bit_field_size
%type <storage> enum_name

%type <paramarr> parameter_type_list parameter_list identifier_list
%type <storage> parameter_declaration
%type <storage> parameter_typedef_declarator clean_typedef_declarator
%type <storage> typedef_declarator paren_typedef_declarator
%type <storage> clean_postfix_typedef_declarator
%type <storage> paren_postfix_typedef_declarator
%type <cstr> simple_paren_typedef_declarator
%type <symbol> function_head
%type <storage> old_function_head old_function_declarator
%type <storage> postfix_old_function_declarator unary_abstract_declarator
%type <storage> postfix_abstract_declarator

%%

// A.2.1  Expressions

// must translate strings before catenating
string_literal
	: STRING
	    { $$ = new String_node(g_curr_scope, $1.text); }
	| string_literal STRING
	    {
		$$ = $1;
		$1->append($2.text);
	    }
	;

wstring_literal
	: WSTRING
	    { $$ = new String_node(g_curr_scope, $1.text, TRUE); }
	| wstring_literal WSTRING
	    {
		$$ = $1;
		$1->append($2.text);
	    }
	;

// 3.2.1 ... 3.3.2
postfix_expr
	: IDENTIFIER
	    { $$ = new Symbol_node(g_curr_scope, $1.text, $1.file, $1.line); }
	| string_literal
	    { $$ = $1; $1->finish(); }
	| wstring_literal
	    { $$ = $1; $1->finish(); }
	| '(' expr ')'
	    { $$ = $2; }
	| INTVAL
	    { $$ = new Integer_node(g_int_type, g_curr_scope, $1.text); }
	| UINTVAL
	    { $$ = new Integer_node(g_uint_type, g_curr_scope, $1.text); }
	| LONGVAL
	    { $$ = new Integer_node(g_long_type, g_curr_scope, $1.text); }
	| ULONGVAL
	    { $$ = new Integer_node(g_ulong_type, g_curr_scope, $1.text); }
	| CHARACTER
	    { $$ = new Integer_node(g_char_type, g_curr_scope,
		    strtochar($1.text)); }
	| WCHARACTER
	    { $$ = new Integer_node(g_wchar_t_type, g_curr_scope,
		    strtochar($1.text, TRUE)); }
	| FLOATVAL
	    { $$ = new Float_node(g_float_type, g_curr_scope, $1.text); }
	| DOUBLEVAL
	    { $$ = new Float_node(g_double_type, g_curr_scope, $1.text); }
	| LONGDOUBLEVAL
	    { $$ = new Float_node(g_longdouble_type, g_curr_scope, $1.text); }
	// !ISO - short doubles
	| SHORTDOUBLEVAL
	    { $$ = new Float_node(g_shortdouble_type, g_curr_scope, $1.text); }
	// !ISO - long long integers
	| LONGLONGVAL
	    { $$ = new Integer_node(g_longlong_type, g_curr_scope, $1.text); }
	| ULONGLONGVAL
	    { $$ = new Integer_node(g_ulonglong_type, g_curr_scope, $1.text); }
	| postfix_expr  '[' expr ']'
	    { $$ = new Array_ref_node(g_curr_scope, $1, $3); }
	| postfix_expr '(' ')'
	    { $$ = new Func_call_node(g_curr_scope, $1, NULL); }
	| postfix_expr '(' argument_expr_list ')'
	    { $$ = new Func_call_node(g_curr_scope, $1, $3); }
	| postfix_expr '.' identifier
	    { $$ = new Struct_ref_node(g_curr_scope, $1, $3.text, $3.file,
		    $3.line, FALSE); }
	| postfix_expr ARROW identifier
	    { $$ = new Struct_ref_node(g_curr_scope, $1, $3.text, $3.file,
		    $3.line, TRUE); }
	| postfix_expr INCR
	    { $$ = new Operator_node(g_curr_scope, INCR, $1, FALSE); }
	| postfix_expr DECR
	    { $$ = new Operator_node(g_curr_scope, DECR, $1, FALSE); }
	;

identifier
	: IDENTIFIER { $$ = $1; }
	| TYPENAME { $$ = $1; }
	;

argument_expr_list
	: assignment_expr { $$ = new Expr_arr; $$->end() = $1; }
	| argument_expr_list ',' assignment_expr { $$ = $1; $$->end() = $3; }
	;

// 3.3.3
unary_expr
	: postfix_expr { $$ = $1; }
	| INCR unary_expr
		{ $$ = new Operator_node(g_curr_scope, INCR, $2); }
	| DECR unary_expr
		{ $$ = new Operator_node(g_curr_scope, DECR, $2); }
	| '&' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '&', $2); }
	| '*' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '*', $2); }
	| '+' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '+', $2); }
	| '-' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '-', $2); }
	| '~' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '~', $2); }
	| '!' cast_expr
		{ $$ = new Operator_node(g_curr_scope, '!', $2); }
	| SIZEOF unary_expr
		{ $$ = new Sizeof_node(g_curr_scope, $2); }
	| SIZEOF '(' type_name ')'
		{ $$ = new Sizeof_node(g_curr_scope, $3.type); }
	;

// 3.3.4
cast_expr
	: unary_expr { $$ = $1; }
	| '(' type_name ')' cast_expr
	    { $$ = new Cast_node(g_curr_scope, $2.type, $4); }
	// !ISO - structure displays
	| '(' type_name ')' initializer_list
	    {
		if (g_strict_iso)
		    warn(STRUCT_DISPLAY_IS_NOT_ISO);

		// create a temp var in the current scope which is inited
		// to this init list, then return a symbol to the new var
		const char *id = make_unnamed_name();
		Storage_thing st, stid;
		stid.init();
		stid.sym = id;
		st.do_declaration($2, stid);
		$$ = do_initializer(st.symbol, $4);
		do_tree(st.symbol);

		if ($$ == NULL)
		    $$ = new Symbol_node(g_curr_scope, st.symbol,
			    g_cpp.filename(), g_cpp.linenum());
	    }
	;

// 3.3.5 ... 3.3.15
base_expr
	: cast_expr { $$ = $1; }
	| base_expr '*' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '*', $1, $3); }
	| base_expr '/' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '/', $1, $3); }
	| base_expr '%' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '%', $1, $3); }
	| base_expr '+' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '+', $1, $3); }
	| base_expr '-' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '-', $1, $3); }
	| base_expr LSHIFT base_expr
	    { $$ = new Operator2_node(g_curr_scope, LSHIFT, $1, $3); }
	| base_expr RSHIFT base_expr
	    { $$ = new Operator2_node(g_curr_scope, RSHIFT, $1, $3); }
	| base_expr '<' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '<', $1, $3); }
	| base_expr '>' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '>', $1, $3); }
	| base_expr LESSEQ base_expr
	    { $$ = new Operator2_node(g_curr_scope, LESSEQ, $1, $3); }
	| base_expr GREATEQ base_expr
	    { $$ = new Operator2_node(g_curr_scope, GREATEQ, $1, $3); }
	| base_expr EQUAL base_expr
	    { $$ = new Operator2_node(g_curr_scope, EQUAL, $1, $3); }
	| base_expr NOTEQ base_expr
	    { $$ = new Operator2_node(g_curr_scope, NOTEQ, $1, $3); }
	| base_expr '&' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '&', $1, $3); }
	| base_expr '^' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '^', $1, $3); }
	| base_expr '|' base_expr
	    { $$ = new Operator2_node(g_curr_scope, '|', $1, $3); }
	| base_expr ANDAND base_expr
	    { $$ = new Operator2_node(g_curr_scope, ANDAND, $1, $3); }
	| base_expr OROR base_expr
	    { $$ = new Operator2_node(g_curr_scope, OROR, $1, $3); }
	| base_expr '?' expr ':' base_expr
	    { $$ = new Operator3_node(g_curr_scope, '?', $1, $3, $5); }
	;

// 3.3.16
assignment_expr
	: base_expr { $$ = $1; }
	| unary_expr '=' assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, '=', $1, $3); }
	| unary_expr MULTEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, MULTEQ, $1, $3); }
	| unary_expr DIVEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, DIVEQ, $1, $3); }
	| unary_expr MODEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, MODEQ, $1, $3); }
	| unary_expr PLUSEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, PLUSEQ, $1, $3); }
	| unary_expr MINUSEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, MINUSEQ, $1, $3); }
	| unary_expr LSHIFTEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, LSHIFTEQ, $1, $3); }
	| unary_expr RSHIFTEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, RSHIFTEQ, $1, $3); }
	| unary_expr ANDEQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, ANDEQ, $1, $3); }
	| unary_expr XOREQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, XOREQ, $1, $3); }
	| unary_expr OREQ assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, OREQ, $1, $3); }
	;

// 3.3.17 - split into 2 rules to mark symbols properly
expr
	: assignment_expr
	    { $$ = $1; }
	| expr ',' assignment_expr
	    { $$ = new Operator2_node(g_curr_scope, ',', $1, $3); }
	;

// "expression" is always "read" by something, so mark it that way
expression
	: expr
	    {
		$$ = $1;
		$1->mark_symbol(TRUE, FALSE, FALSE);
	    }
	;

// this is only used in the FOR statement rule below:
opt_expression
	: /*[]*/ { $$ = NULL; }
	| expression { $$ = $1; }
	;

// this is only used in the FOR statement and expression_statement below:
opt_expr
	: /*[]*/ { $$ = NULL; }
	| expr
	    {
		$1->mark_symbol(FALSE, FALSE, FALSE);
		$$ = implicit_cast($1, g_void_type);
	    }
	;

// 3.3.18
constant_expr
	: base_expr
	    {
		$$ = $1;

		if (!$1->is_int_expr())
		    error(EXPECTED_CONST_EXPR);

		// no need to mark_symbols, as there cannot be any here
	    }
	;


// A.2.2  Declarations
// The following rules are cribbed from James Roskind's ANSI C grammar
// since he did such a great job removing ambiguities from it.

declaration
	: sue_declaration_specifier ';'
	| sue_type_specifier ';'
	| declaring_list ';'
	| default_declaring_list ';'
	;

    /* Note that if a typedef were  redeclared,  then  a  declaration
    specifier must be supplied */

default_declaring_list  /* Can't  redeclare typedef names */
	: declaration_qualifier_list identifier_declarator
		{ $$.do_declaration($1, $2); }
	    initializer_opt
		{ do_initializer($3.symbol, $4); do_tree($3.symbol); }
	| type_qualifier_list identifier_declarator
		{ $$.do_declaration($1, $2); }
	    initializer_opt
		{ do_initializer($3.symbol, $4); do_tree($3.symbol); }
	| default_declaring_list ',' identifier_declarator
		{ $$.do_declaration($1, $3); }
	    initializer_opt
		{ do_initializer($4.symbol, $5); do_tree($4.symbol); }
	;

declaring_list
	: declaration_specifier declarator
		{ $$.do_declaration($1, $2); }
	    initializer_opt
		{ do_initializer($3.symbol, $4); do_tree($3.symbol); }
	| type_specifier declarator 
		{ $$.do_declaration($1, $2); }
	    initializer_opt
		{ do_initializer($3.symbol, $4); do_tree($3.symbol); }
	| declaring_list ',' declarator 
		{ $$.do_declaration($1, $3); }
	    initializer_opt
		{ do_initializer($4.symbol, $5); do_tree($4.symbol); }
	;

declaration_specifier
	: basic_declaration_specifier { $$ = $1; }  /* Arithmetic or void */
	| sue_declaration_specifier { $$ = $1; }    /* struct/union/enum */
	| typedef_declaration_specifier { $$ = $1; } /* typedef */
	;

type_specifier
	: basic_type_specifier { $$ = $1; }   /* Arithmetic or void */
	| sue_type_specifier { $$ = $1; }     /* Struct/Union/Enum */
	| typedef_type_specifier { $$ = $1; } /* Typedef */
	;


declaration_qualifier_list  /* const/volatile, AND storage class */
	: storage_class
	    {
		$$.init();
		$$.storage  = $1;
	    }
	| type_qualifier_list storage_class
	    {
		$$.init();
		$$.flags = $1.flags;
		$$.storage = $2;
	    }
	| declaration_qualifier_list declaration_qualifier
	    {
		$$.init();
		$$.flags = set_flags($1.flags, $2.flags);
		$$.set_storage($1.storage, $2.storage);
	    }
	;

type_qualifier_list
	: type_qualifier { $$.init(); $$.flags = $1; }
	| type_qualifier_list type_qualifier
	    { $$ = $1; $$.flags = set_flags($1.flags, $2); }
	;

declaration_qualifier
	: storage_class { $$.init(); $$.storage = $1; }
	| type_qualifier { $$.init(); $$.flags = $1; }	/* const or volatile */
	;

type_qualifier
	: CONST { $$ = S_CONST; }
	| VOLATILE { $$ = S_VOLATILE; }
	;

basic_declaration_specifier      /* Storage Class+Arithmetic or void */
	: declaration_qualifier_list basic_type_name
	    { $$.set_basic_type($1, $2); }
	| basic_type_specifier  storage_class
	    { $$ = $1; $$.set_storage($1.storage, $2); }
	| basic_declaration_specifier declaration_qualifier
	    { $$.set_basic_type($1, $2); }
	| basic_declaration_specifier basic_type_name
	    { $$.set_basic_type($1, $2); }
	;

basic_type_specifier
	: basic_type_name { $$ = $1; }            /* Arithmetic or void */
	| type_qualifier_list basic_type_name
	    { $$ = $2; $$.flags = set_flags($1.flags, $2.flags); }
	| basic_type_specifier type_qualifier
	    { $$ = $1; $$.flags = set_flags($1.flags, $2); }
	| basic_type_specifier basic_type_name
	    { $$.set_basic_type($1, $2); }
	;

sue_declaration_specifier          /* Storage Class + struct/union/enum */
	: declaration_qualifier_list  elaborated_type_name
	    {
		$$ = $2;
		$$.storage = $1.storage;
		$$.flags = set_flags($1.flags, $2.flags);
	    }
	| sue_type_specifier        storage_class
	    {
		$$ = $1;
		$$.storage = $2;
	    }
	| sue_declaration_specifier declaration_qualifier
	    {
		$$ = $1;
		$$.set_storage($1.storage, $2.storage);
		$$.flags = set_flags($1.flags, $2.flags);
	    }
	;

sue_type_specifier
	: elaborated_type_name              /* struct/union/enum */
	    { $$ = $1; }
	| type_qualifier_list elaborated_type_name
	    {
		$$ = $2;
		$$.flags = set_flags($1.flags, $2.flags);
	    }
	| sue_type_specifier  type_qualifier
	    {
		$$ = $1;
		$$.flags = set_flags($1.flags, $2);
	    }
	;


typedef_declaration_specifier       /*Storage Class + typedef types */
	: typedef_type_specifier          storage_class
	    {
		$$ = $1;
		$$.set_storage($1.storage, $2);
	    }
	| declaration_qualifier_list    TYPENAME
	    {
		$$ = $1;
		$$.type = $2.sym->type();
	    }
	| typedef_declaration_specifier declaration_qualifier
	    {
		$$ = $1;
		$$.flags = set_flags($1.flags, $2.flags);
		$$.set_storage($1.storage, $2.storage);
	    }
	;

typedef_type_specifier              /* typedef types */
	: TYPENAME
	    {
		$$.init();
		$$.type = $1.sym->type();
	    }
	| type_qualifier_list    TYPENAME
	    {
		$$.init();
		$$.flags = $1.flags;
		$$.type = $2.sym->type();
	    }
	| typedef_type_specifier type_qualifier
	    {
		$$ = $1;
		$$.flags = set_flags($1.flags, $2);
	    }
	;

storage_class
	: TYPEDEF { $$ = TYPEDEF; }
	| EXTERN { $$ = EXTERN; }
	| STATIC { $$ = STATIC; }
	| AUTO { $$ = AUTO; }
	| REGISTER { $$ = REGISTER; }
	// !ISO - for compatibility to MPW C and other C compilers
	| PASCAL { $$ = PASCAL; }
	| FORTRAN { $$ = FORTRAN; }
	;

basic_type_name
	: INT { $$.init(); $$.basic = INT; }
	| CHAR { $$.init(); $$.basic = CHAR; }
	| FLOAT { $$.init(); $$.basic = FLOAT; }
	| DOUBLE { $$.init(); $$.basic = DOUBLE; }
	| VOID { $$.init(); $$.basic = VOID; }
	| SHORT { $$.init(); $$.flags = S_SHORT; }
	| LONG { $$.init(); $$.flags = S_LONG; }
	| SIGNED { $$.init(); $$.flags = S_SIGNED; }
	| UNSIGNED { $$.init(); $$.flags = S_UNSIGNED; }
	;

elaborated_type_name
	: aggregate_name { $$ = $1; }
	| enum_name { $$ = $1; }
	;

aggregate_name
	: aggregate_key '{'  member_declaration_list '}'
	    {
		$$.do_unnamed_sue($1);
		g_type_top--;
	    }
	| aggregate_key identifier '{'  member_declaration_list '}'
	    {
		$$.do_sue_definition($1, $2);
		g_type_top--;
	    }
	| aggregate_key identifier
	    {
		$$.do_sue_declaration($1, $2);
		g_type_top--;
	    }
	;

aggregate_key
	: STRUCT
	    {
		$$ = new Type_struct;
		g_type_stack[++g_type_top] = $$;
	    }
	| UNION
	    {
		$$ = new Type_union;
		g_type_stack[++g_type_top] = $$;
	    }
	;

member_declaration_list
	: member_declaration
	| member_declaration_list member_declaration
	;

member_declaration
	: member_declaring_list ';'
	| member_default_declaring_list ';'
	// !ISO - unnamed substructures
	| aggregate_key '{'  member_declaration_list '}' ';'
	    {
		Storage_thing st;
		st.do_unnamed_sue($1);
		g_type_top--;
		do_unnamed_substruct(st.type, g_type_stack[g_type_top]);
	    }
	| aggregate_key identifier ';'
	    {
		Storage_thing st;
		st.do_sue_declaration($1, $2);
		g_type_top--;
		do_unnamed_substruct(st.type, g_type_stack[g_type_top]);
	    }
	;

member_default_declaring_list        /* doesn't redeclare typedef*/
	: type_qualifier_list member_identifier_declarator
		{
		    Storage_thing st;
		    st.init();
		    st.flags = $1.flags;
		    $$.do_member_decl(st, $2);
		}
	| member_default_declaring_list ',' member_identifier_declarator
		{ $$.do_member_decl($1, $3); }
	;

member_declaring_list
	: type_specifier member_declarator
		{ $$.do_member_decl($1, $2); }
	| member_declaring_list ',' member_declarator
		{ $$.do_member_decl($1, $3); }
	;


member_declarator
	: declarator bit_field_size_opt
	    {
		$$ = $1;

		if ($2 == 0)
		    error(ILLEGAL_VALUE_FOR_BITFIELD);
		else if ($2 > 0)
		    $$.bitfield = $2;
	    }
	| bit_field_size
	    {
		$$.init();
		$$.bitfield = $1;
	    }
	;

member_identifier_declarator
	: identifier_declarator bit_field_size_opt
	    {
		$$ = $1;

		if ($2 == 0)
		    error(ILLEGAL_VALUE_FOR_BITFIELD);
		else if ($2 > 0)
		    $$.bitfield = $2;
	    }
	| bit_field_size
	    {
		$$.init();
		$$.bitfield = $1;
	    }
	;

bit_field_size_opt
	: /*[]*/ { $$ = -1; }
	| bit_field_size { $$ = $1; }
	;

bit_field_size
	: ':' constant_expr
	    {
		Const_expr width;
		$2->eval_const_expr(width);
		delete $2;

		if (width.ival != NULL)
		    $$ = (short)width.ival->get();

		if ($$ < 0)
		{
		    error(NEGATIVE_VALUE_FOR_BITFIELD);
		    $$ = -1;
		}
	    }
	;

enum_name
	: enum '{' enumerator_list '}'
	    {
		$$.do_unnamed_sue($1);
		g_type_top--;
	    }
	| enum identifier '{' enumerator_list '}'
	    {
		$$.do_sue_definition($1, $2);
		g_type_top--;
	    }
	| enum identifier
	    {
		$$.do_sue_declaration($1, $2);
		g_type_top--;
	    }
	;

enum
	: ENUM
	    {
		$$ = new Type_enum;
		g_type_stack[++g_type_top] = $$;
	    }
	;

enumerator_list
	: enum_list
	| enum_list ','
	    {
		if (g_strict_iso)
		    warn(NON_ISO_COMMA_ENDS_ENUM_LIST);
	    }
	;

enum_list
	: identifier
	    { ((Type_enum*)g_type_stack[g_type_top])->add($1.text,
		    $1.file, $1.line); }
	| identifier '=' constant_expr
	    {
		Const_expr val;
		val.new_ival(g_int_type->ic());
		$3->eval_const_expr(val);
		delete $3;
		((Type_enum*)g_type_stack[g_type_top])->add($1.text,
			$1.file, $1.line, *val.ival);
	    }
	| enum_list ',' identifier
	    { ((Type_enum*)g_type_stack[g_type_top])->add($3.text,
		    $3.file, $3.line); }
	| enum_list ',' identifier '=' constant_expr
	    {
		Const_expr val;
		val.new_ival(g_int_type->ic());
		$5->eval_const_expr(val);
		delete $5;
		((Type_enum*)g_type_stack[g_type_top])->add($3.text,
			$3.file, $3.line, *val.ival);
	    }
	;

parameter_type_list
	: parameter_list
	    {
		if ($1->size() == 1 && $1->elt(0).type()->isvoid())
		{
		    if ($1->elt(0).symbol() != NULL)
			error(PARAMETER_CANNOT_BE_VOID, $1->elt(0).symbol());

		    $1->reset(0);
		}

		$$ = $1;
	    }
	| parameter_list ',' DOTDOTDOT
	    {
		$$ = $1;
		Param &p = $$->end();
		p.storage(DOTDOTDOT);
		p.type(g_unknown_type);
		p.symbol(NULL);
	    }
	;

parameter_list
	: parameter_declaration
	    {
		$$ = new Param_arr;
		Param &p = $$->end();
		p.type($1.type);
		p.storage($1.storage);
		p.symbol($1.sym);
	    }
	| parameter_list ',' parameter_declaration
	    {
		$$ = $1;
		Param &p = $$->end();
		p.type($3.type);
		p.storage($3.storage);
		p.symbol($3.sym);
	    }
	;

parameter_declaration
	: declaration_specifier
	    { $$.do_param_decl($1); }
	| declaration_specifier abstract_declarator
	    { $$.do_param_decl($1, $2); }
	| declaration_specifier identifier_declarator
	    { $$.do_param_decl($1, $2); }
	| declaration_specifier parameter_typedef_declarator
	    { $$.do_param_decl($1, $2); }
	| declaration_qualifier_list
	    { $$.do_param_decl($1); }
	| declaration_qualifier_list abstract_declarator
	    { $$.do_param_decl($1, $2); }
	| declaration_qualifier_list identifier_declarator
	    { $$.do_param_decl($1, $2); }
	| type_specifier
	    { $$.do_param_decl($1); }
	| type_specifier abstract_declarator
	    { $$.do_param_decl($1, $2); }
	| type_specifier identifier_declarator
	    { $$.do_param_decl($1, $2); }
	| type_specifier parameter_typedef_declarator
	    { $$.do_param_decl($1, $2); }
	| type_qualifier_list
	    { $$.do_param_decl($1); }
	| type_qualifier_list abstract_declarator
	    { $$.do_param_decl($1, $2); }
	| type_qualifier_list identifier_declarator
	    { $$.do_param_decl($1, $2); }
	;

    /*  ANSI  C  section  3.7.1  states  "An identifier declared as a
    typedef name shall not be redeclared as a parameter".  Hence  the
    following is based only on IDENTIFIERs */

type_name
	: type_specifier { $$.make_type($1); }
	| type_specifier abstract_declarator
		{ $$.make_type($1, $2); }
	| type_qualifier_list { $$.make_type($1); }
	| type_qualifier_list abstract_declarator
		{ $$.make_type($1, $2); }
	;

initializer_opt
	: /*[]*/ { $$ = NULL; }
	| '=' initializer { $$ = $2; }
	;

// end of cribbed code


// 3.5.7
initializer
	: assignment_expr
	    {
		$$ = $1;
		$1->mark_symbol(TRUE, FALSE, FALSE);
	    }
	| initializer_list { $$ = $1; }
	;

initializer_list
	: '{' init_list '}' { $$ = $2; }
	| '{' init_list ',' '}' { $$ = $2; }
	;

init_list
	: initializer
	    {
		$$ = new Initializer_node(g_curr_scope);
		$$->append($1);
	    }
	| init_list ',' initializer
	    {
		$$ = $1;
		$$->append($3);
	    }
	// !ISO - initialisation indexes
	| '[' constant_expr ']' initializer
	    {
		$$ = new Initializer_node(g_curr_scope);
		$$->append($4, $2);
	    }
	| init_list ',' '[' constant_expr ']' initializer
	    {
		$$ = $1;
		$$->append($6, $4);
	    }
	;

// A.2.3  Statements

// 3.6
statement
	: labeled_statement { $$ = $1; }
	| compound_statement { $$ = $1; }
	| expression_statement { $$ = $1; }
	| selection_statement { $$ = $1; }
	| iteration_statement { $$ = $1; }
	| jump_statement { $$ = $1; }
	// !ISO
	| asm_statement { $$ = $1; }
	;

// 3.6.1
labeled_statement
	: identifier ':' statement
	    {
		if (g_func_scope == NULL)
		    bug("yacc: labeled_statement g_func_scope==NULL");

		if (g_func_scope->labels().find($1.text))
		{
		    if (g_func_scope->labels()().scope() != NULL)
		    {
			error(LABEL_ALREADY_DEFINED, g_func_scope->file(),
				g_func_scope->labels()().line());
			$$ = $3;
		    }
		}

		Symbol &sym = g_func_scope->labels()[$1.text];
		sym.scope(g_func_scope);
		sym.file($1.file);
		sym.line($1.line);
		sym.node($3);
		$$ = new Label_node(g_curr_scope, &sym, $3);
	    }
	| CASE constant_expr ':' statement
	    {
		Switch_node *sw = get_switch();

		if (sw == NULL)
		{
		    error(CASE_OUTSIDE_SWITCH);
		    $$ = NULL;
		}
		else
		{
		    $$ = new Case_node(g_curr_scope, $2, NULL, $4, sw, $1.line);
		    sw->cases().end() = $$;
		}
	    }
	| DEFAULT ':' statement
	    {
		Switch_node *sw = get_switch();

		if (sw == NULL)
		{
		    error(DEFAULT_OUTSIDE_SWITCH);
		    $$ = NULL;
		}
		else
		{
		    $$ = new Case_node(g_curr_scope, NULL, NULL, $3, sw,
			    $1.line);
		    sw->cases().end() = $$;
		}
	    }
	// !ISO - ranged case statement
	| CASE constant_expr DOTDOTDOT constant_expr ':' statement
	    {
		if (g_strict_iso)
		    warn(RANGED_CASE_IS_NOT_ISO);

		Switch_node *sw = get_switch();

		if (sw == NULL)
		{
		    error(CASE_OUTSIDE_SWITCH);
		    $$ = NULL;
		}
		else
		{
		    $$ = new Case_node(g_curr_scope, $2, $4, $6, sw, $1.line);
		    sw->cases().end() = $$;
		}
	    }
	;

// 3.6.2
compound_statement
	: /*[]*/
	    {
		g_curr_scope = new Scope(g_cpp.filename(), g_cpp.linenum(),
			g_curr_scope);
	    }
	comp_stmt
	    {
		$$ = $2;
		Scope *s = g_curr_scope;
		g_curr_scope = s->parent();
		s->node($2);

		if (g_curr_scope->children() == NULL)
		    g_curr_scope->new_children();

		g_curr_scope->children()->end() = s;
		check_unused_vars(s);
		s->cleanup();
	    }
	;

comp_stmt
        : '{' '}'
		{ $$ = new Compound_node(g_curr_scope, new Node_arr); }
	| '{' declaration_list '}'
		{ $$ = new Compound_node(g_curr_scope, new Node_arr); }
        | '{' statement_list '}'
		{ $$ = new Compound_node(g_curr_scope, $2); }
	| '{' declaration_list statement_list '}'
		{ $$ = new Compound_node(g_curr_scope, $3); }
	;

declaration_list
	: declaration
	| declaration_list declaration
	;

statement_list
	: statement
	    {
		$$ = new Node_arr;

		if ($1 != NULL)
		    $$->end() = $1;
	    }
	| statement_list statement
	    {
		$$ = $1;

		if ($2 != NULL)
		    $$->end() = $2;
	    }
	;

// 3.6.3
expression_statement
	: opt_expr ';' { $$ = $1; }
	;

// 3.6.4
selection_statement
	: IF '(' expression ')' statement
	    { $$ = new If_node(g_curr_scope, $3, $5, NULL); }
	| IF '(' expression ')' statement ELSE statement
	    { $$ = new If_node(g_curr_scope, $3, $5, $7); }
	| SWITCH
	    { g_node_stack[++g_node_top] = new Switch_node(g_curr_scope); }
	'(' expression ')' statement // NOT compound_statement
	    {
		Switch_node *sw = (Switch_node*)g_node_stack[g_node_top--];
		// we assume that the top of the stack is still a SWITCH
		sw->expr($4);
		sw->stmt($6);
		sw->check_cases();
		$$ = sw;
	    }
	;

// 3.6.5
iteration_statement
	: WHILE 
	    {
		g_node_stack[++g_node_top] = new While_node(g_curr_scope);
		g_loop_count++;
	    }
	'(' expression ')' statement
	    {
		// assume that the top of the stack has a WHILE
		While_node *w = (While_node*)g_node_stack[g_node_top--];
		w->expr($4);
		w->stmt($6);
		$$ = w;
		g_loop_count--;
	    }
	| DO
	    {
		g_node_stack[++g_node_top] = new Dowhile_node(g_curr_scope);
		g_loop_count++;
	    }
	statement WHILE '(' expression ')' ';'
	    {
		// assume that the top of the stack has a DOWHILE
		Dowhile_node *d = (Dowhile_node*)g_node_stack[g_node_top--];
		d->stmt($3);
		d->expr($6);
		$$ = d;
		g_loop_count--;
	    }
	| FOR
	    {
		g_node_stack[++g_node_top] = new For_node(g_curr_scope);
		g_loop_count++;
	    }
	'(' opt_expr ';' opt_expression ';' opt_expr ')' statement
	    {
		// assume that the top of the stack has a FOR
		For_node *f = (For_node*)g_node_stack[g_node_top--];
		f->expr1($4);
		f->expr2($6);
		f->expr3($8);
		f->stmt($10);
		$$ = f;
		g_loop_count--;
	    }
	;

// 3.6.6
jump_statement
	: GOTO identifier ';'
	    {
		Symbol &sym = g_func_scope->labels()[$2.text];
		$$ = new Goto_node(g_curr_scope, &sym, $1.line);
	    }
	| CONTINUE ';'
	    {
		int st = g_node_top;

		// skip the switch nodes on the stack
		while (st >= 0 && g_node_stack[st]->nodetype() == N_SWITCH)
		    st--;

		if (st < 0)
		{
		    error(CONTINUE_OUTSIDE_LOOP);
		    $$ = NULL;
		}
		else
		    $$ = new Continue_node(g_curr_scope, g_node_stack[st]);
	    }
	| BREAK ';'
	    {
		if (g_node_top < 0)
		{
		    error(BREAK_OUTSIDE_LOOP_OR_SWITCH);
		    $$ = NULL;
		}
		else
		    $$ = new Break_node(g_curr_scope, g_node_stack[g_node_top]);
	    }
	| RETURN expression ';'
	    { $$ = new Return_node(g_curr_scope, $2); }
	| RETURN ';'
	    { $$ = new Return_node(g_curr_scope, NULL); }
	;

// !ISO
asm_statement
	: ASM '(' string_literal ')' ';'
	    {
		$3->finish();
		$$ = new Asm_node(g_curr_scope, $3, $1.file, $1.line);
	    }
	;

// A.2.4  External Definitions

// 3.7
translation_unit
	//: external_declaration
	//| translation_unit external_declaration
	: /*[]*/
	| translation_unit external_declaration
	;

external_declaration
	: function_definition
	| declaration
	// !ISO - global ASM statements
	| asm_statement { do_tree($1); }
	;

// 3.7.1
// The following are cribbed from James Roskind's ANSI C grammar
// and then modified somewhat - especially the function definitions.

function_head
	:                            identifier_declarator
	    { $$ = do_function_def($1); }
	| declaration_specifier      identifier_declarator
	    { $$ = do_function_def($1, $2); }
	| type_specifier             identifier_declarator
	    { $$ = do_function_def($1, $2); }
	| declaration_qualifier_list identifier_declarator
	    { $$ = do_function_def($1, $2); }
	| type_qualifier_list        identifier_declarator
	    { $$ = do_function_def($1, $2); }
	| old_function_head
	    { $$ = check_old_function_def($1); }
	| old_function_head declaration_list
	    { $$ = check_old_function_def($1); }
	;

old_function_head
	:                            old_function_declarator
	    { $$ = do_old_function_def($1); }
	| declaration_specifier      old_function_declarator
	    { $$ = do_old_function_def($1, $2); }
	| type_specifier             old_function_declarator
	    { $$ = do_old_function_def($1, $2); }
	| declaration_qualifier_list old_function_declarator
	    { $$ = do_old_function_def($1, $2); }
	| type_qualifier_list        old_function_declarator
	    { $$ = do_old_function_def($1, $2); }
	;

function_definition
	: function_head comp_stmt
	    {
		if ($1 != NULL)
		{
		    Scope *s = g_curr_scope;
		    g_curr_scope = s->parent();
		    g_func_scope = NULL;
		    s->node($2);

		    if (g_curr_scope->children() == NULL)
			g_curr_scope->new_children();

		    g_curr_scope->children()->end() = s;
		    $1->node($2);
		    $1->scope(s);
		    check_unused_vars(s, $1);
		    check_for_return($1);
		    s->cleanup();
		    do_tree($1);
		}
	    }
	;

declarator
	: identifier_declarator { $$ = $1; }
	| typedef_declarator { $$ = $1; }
	;

typedef_declarator
	: paren_typedef_declarator        /* would be ambiguous as parameter*/
		{ $$ = $1; }
	| parameter_typedef_declarator    /* not ambiguous as param*/
		{ $$ = $1; }
	;

parameter_typedef_declarator
	: TYPENAME
	    {
		$$.init();
		$$.sym = $1.text;
	    }
	| TYPENAME postfixing_abstract_declarator
	    {
		$$.init();
		$$.type = $2;
		$$.sym = $1.text;
	    }
	| clean_typedef_declarator { $$ = $1; }
	;

    /*  The  following have at least one '*'. There is no (redundant)
    '(' between the '*' and the TYPENAME. */

clean_typedef_declarator
	: clean_postfix_typedef_declarator { $$ = $1; }
	| '*' parameter_typedef_declarator
	    {
		$$ = $2;
		Type *t = new Type_pointer(NULL);
		$$.point_type_to(t);
	    }
	| '*' type_qualifier_list parameter_typedef_declarator
	    {
		$$ = $3;
		Type *t = new Type_pointer(NULL);
		t->isconst($2.flags & S_CONST);
		t->isvolatile($2.flags & S_VOLATILE);
		$$.point_type_to(t);
	    }
	;

clean_postfix_typedef_declarator
	: '(' clean_typedef_declarator ')' { $$ = $2; }
	| '(' clean_typedef_declarator ')' postfixing_abstract_declarator
		{ $$ = $2; $$.point_type_to($4); }
	;

    /* The following have a redundant '(' placed immediately  to  the
    left of the TYPENAME */

paren_typedef_declarator
	: paren_postfix_typedef_declarator { $$ = $1; }
	| '*' '(' simple_paren_typedef_declarator ')' /* redundant paren */
	    {
		$$.init();
		$$.type = new Type_pointer(NULL);
		$$.sym = $3;
	    }
	| '*' type_qualifier_list
	        '(' simple_paren_typedef_declarator ')' /* redundant paren */
	    {
		$$.init();
		$$.type = new Type_pointer(NULL);
		$$.type->isconst($2.flags & S_CONST);
		$$.type->isvolatile($2.flags & S_VOLATILE);
		$$.sym = $4;
	    }
	| '*' paren_typedef_declarator
	    {
		$$ = $2;
		Type *t = new Type_pointer(NULL);
		$$.point_type_to(t);
	    }
	| '*' type_qualifier_list paren_typedef_declarator
	    {
		$$ = $3;
		Type *t = new Type_pointer(NULL);
		t->isconst($2.flags & S_CONST);
		t->isvolatile($2.flags & S_VOLATILE);
		$$.point_type_to(t);
	    }
	;

paren_postfix_typedef_declarator /* redundant paren to left of tname*/
	: '(' paren_typedef_declarator ')' { $$ = $2; }
	| '(' simple_paren_typedef_declarator postfixing_abstract_declarator
			')' /* redundant paren */
		{ $$.init(); $$.sym = $2; $$.type = $3; }
	| '(' paren_typedef_declarator ')' postfixing_abstract_declarator
		{ $$ = $2; $$.point_type_to($4); }
	;

simple_paren_typedef_declarator
	: TYPENAME { $$ = $1.text; }
	| '(' simple_paren_typedef_declarator ')' { $$ = $2; }
	;

identifier_declarator
	: unary_identifier_declarator { $$ = $1; }
	| paren_identifier_declarator { $$.init(); $$.sym = $1; }
	;

unary_identifier_declarator
	: postfix_identifier_declarator { $$ = $1; }
	| '*' identifier_declarator
	    {
		$$ = $2;
		Type *t = new Type_pointer(NULL);
		$$.point_type_to(t);
	    }
	| '*' type_qualifier_list identifier_declarator
	    {
		$$ = $3;
		Type *t = new Type_pointer(NULL);
		t->isconst($2.flags & S_CONST);
		t->isvolatile($2.flags & S_VOLATILE);
		$$.point_type_to(t);
	    }
	;

postfix_identifier_declarator
	: paren_identifier_declarator postfixing_abstract_declarator
	    { $$.init(); $$.sym = $1; $$.type = $2; }
	| '(' unary_identifier_declarator ')' { $$ = $2; }
	| '(' unary_identifier_declarator ')' postfixing_abstract_declarator
	    {
		$$ = $2;

		if ($4 != NULL)
		    $$.point_type_to($4);
	    }
	;

paren_identifier_declarator
	: IDENTIFIER { $$ = $1.text; }
	| '(' paren_identifier_declarator ')' { $$ = $2; }
	;

// g_curr_scope should already be setup for a new function, and this
// rule is only used for old-style function definitions.
identifier_list
	: IDENTIFIER
	    {
		$$ = new Param_arr;
		Param &p = $$->end();
		p.symbol($1.text);
	    }
	| identifier_list ',' IDENTIFIER
	    {
		$$ = $1;
		Param &p = $$->end();
		p.symbol($3.text);
	    }
	;

old_function_declarator
	: postfix_old_function_declarator { $$ = $1; }
	| '*' old_function_declarator
	    {
		$$ = $2;
		Type *t = new Type_pointer(NULL);
		$$.point_type_to(t);
	    }
	| '*' type_qualifier_list old_function_declarator
	    {
		Type *t = new Type_pointer(NULL);
		t->isconst($2.flags & S_CONST);
		t->isvolatile($2.flags & S_VOLATILE);
		$$ = $3;
		$$.point_type_to(t);
	    }
	;

postfix_old_function_declarator
	:  paren_identifier_declarator '(' identifier_list ')'
		{
		    $$.init();
		    $$.sym = $1;
		    $$.type = new Type_function($3);

		    if (g_strict_iso)
			warn(OLD_STYLE_FUNCTION_DECLARATOR);
		}
	| '(' old_function_declarator ')' { $$ = $2; }
	| '(' old_function_declarator ')' postfixing_abstract_declarator
		{ $$ = $2; $$.point_type_to($4); }
	;

abstract_declarator
	: unary_abstract_declarator { $$ = $1; }
	| postfix_abstract_declarator { $$ = $1; }
	| postfixing_abstract_declarator { $$.init(); $$.type = $1; }
	;

postfixing_abstract_declarator
	: array_abstract_declarator { $$ = $1; }
	| '(' ')' { $$ = new Type_function; }
	| '(' parameter_type_list ')' { $$ = new Type_function($2); }
	;

array_abstract_declarator
	: '[' ']'
	    {
		Type_array *a = new Type_array(NULL,
			g_size_t_type->ic().zero());
		a->complete(FALSE);
		$$ = a;
	    }
	| '[' constant_expr ']'
	    {
		Const_expr isize;
		isize.new_ival(g_size_t_type->ic());
		$2->eval_const_expr(isize);
		delete $2;

		if (isize.ival->isPos())
		    $$ = new Type_array(NULL, *isize.ival);
		else
		{
		    error(ARRAY_MUST_HAVE_POSITIVE_SIZE);
		    $$ = NULL;
		}
	    }
	| array_abstract_declarator '[' constant_expr ']'
	    {
		$$ = $1;

		if ($1 != NULL)
		{
		    Const_expr isize;
		    isize.new_ival(g_size_t_type->ic());
		    $3->eval_const_expr(isize);
		    delete $3;

		    if (isize.ival->isPos())
		    {
			Type_array *nt = new Type_array(NULL, *isize.ival);
			Type_array *t = (Type_array*)$$;

			while (t->type() != NULL)
			    t = (Type_array*)t->type();

			t->type(nt);
		    }
		    else
		    {
			error(ARRAY_MUST_HAVE_POSITIVE_SIZE);
			$$->free();
			$$ = NULL;
		    }
		}
	    }
	;

unary_abstract_declarator
	: '*'
	    {
		$$.init();
		$$.type = new Type_pointer(NULL);
	    }
	| '*' type_qualifier_list
	    {
		$$.init();
		$$.type = new Type_pointer(NULL);
		$$.type->isconst($2.flags & S_CONST);
		$$.type->isvolatile($2.flags & S_VOLATILE);
	    }
	| '*' abstract_declarator
	    {
		$$ = $2;
		Type *t = new Type_pointer(NULL);
		$$.point_type_to(t);
	    }
	| '*' type_qualifier_list abstract_declarator
	    {
		Type *t = new Type_pointer(NULL);
		t->isconst($2.flags & S_CONST);
		t->isvolatile($2.flags & S_VOLATILE);
		$$ = $3;
		$$.point_type_to(t);
	    }
	;

postfix_abstract_declarator
	: '(' unary_abstract_declarator ')' { $$ = $2; }
	| '(' postfix_abstract_declarator ')' { $$ = $2; }
	| '(' postfixing_abstract_declarator ')' { $$.init(); $$.type = $2; }
	| '(' unary_abstract_declarator ')' postfixing_abstract_declarator
	    {
		$$ = $2;

		if ($4 != NULL)
		    $$.point_type_to($4);
	    }
	;

// end of cribbed code

%%

void
yyerror(const char *str)
{
    error(YACC_ERROR, str);
}

#if YYDEBUG
extern char *yyname[];
extern int yydebug;

static void
dumptok()
{
    static int line = 0;
    static const char *fname = NULL;

    if (g_cpp.filename() != fname)
    {
	line = 1;
	fname = g_cpp.filename();
    }

    if (g_cpp.linenum() > line)
    {
	line = g_cpp.linenum();
	printf("\n");
    }

    int tok = g_cpp.getnode().token;

    //if (tok < 255)
	//printf(" <%c>", tok); else
    if (tok == IDENTIFIER || tok == STRING || tok == WSTRING)
	printf(" %s", g_cpp.getnode().text);
    else if (tok == CHARACTER || tok == WCHARACTER)
	printf(" @%s", g_cpp.getnode().text);
    else if (tok == INTVAL || tok == UINTVAL 
	    || tok == LONGVAL || tok == ULONGVAL)
	printf(" #%s", g_cpp.getnode().text);
    else if (tok == DOUBLEVAL || tok == FLOATVAL 
	    || tok == LONGDOUBLEVAL || tok == SHORTDOUBLEVAL)
	printf(" ##%s", g_cpp.getnode().text);
    else
	printf(" %s", yyname[tok]);
}
#endif // YYDEBUG

static Cpp_node_list lextokbuf;
static int lexloc = 0;

inline int
yylex()
{
    if (lexloc >= lextokbuf.size())
    {
	lexloc = 0;
	lextokbuf.resize();

	if (g_cpp.scan(&lextokbuf) == END)
	    return END;
    }

    int tok = lextokbuf.elt(lexloc).token;
    yylval.token.text = lextokbuf.elt(lexloc++).text;
    yylval.token.file = g_cpp.filename();
    yylval.token.line = g_cpp.linenum();

    if (tok == IDENTIFIER)
    {
	if (g_keywords.find(yylval.token.text))
	    tok = g_keywords().token();
	else
	{
	    Symbol *sym = g_curr_scope->lookup_sym(yylval.token.text);

	    if (sym != NULL && sym->storage() == TYPEDEF)
		tok = TYPENAME;

	    yylval.token.sym = sym;
	}
    }

#if YYDEBUG
    if (!yydebug)
	dumptok();
#endif // YYDEBUG
    return tok;
}
