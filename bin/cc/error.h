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

// Copyright (c) 1992,1998,2000-2001 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/error.h,v 1.67 2001/01/21 18:29:31 parag Exp $
#ifndef __ERROR_H_
#define __ERROR_H_

// trailing comments specify types and number of expected arguments
// most args are strings, # are integers, no comment means no args
//
enum Error
{
	// preprocessor warnings and errors:
	CANNOT_OPEN_FILE,						// filename
	CANNOT_GET_FILE_LENGTH,				// filename
	CANNOT_READ_FILE,						// filename
	MISSING_NEWLINE_AT_EOF,				// filename
	UNTERMINATED_C_COMMENT,
	UNTERMINATED_BCPL_COMMENT,
	NON_ISO_BCPL_COMMENT,

	ILLEGAL_DIRECTIVE,						// directive-name
	MISSING_ENDIF,
	MISSING_ENDIFS,
	ILLEGAL_HASH_TOKENS,
	ILLEGAL_DOTDOT_TOKEN,
	UNTERMINATED_STRING_OR_CHAR_CONST,
	ILLEGAL_DOLLAR_CHAR_IN_IDENT,
	OLD_STYLE_LINE_DIRECTIVE,

	ILLEGAL_FORM_FOR_INCLUDE_MACRO,
	CANNOT_OPEN_INCLUDE_FILE,				// filename
	MISSING_NAME_AFTER_DEFINE_MACRO,		// string
	BAD_ARG_LIST_FOR_DEFINE_MACRO,		// macro
	EXPECTED_COMMA_IN_DEFINE_MACRO,		// macro
	EXPECTED_SPACE_AFTER_MACRO_NAME,		// macro
	CANNOT_UNDEF_BUILTIN_MACRO,				// macro
	MACRO_REDEFINED,						// macro, line#
	MACRO_REDEFINED_IN_FILE,				// macro, filename, line#

	CANNOT_UNDEF_NON_IDENT,				// string
	EXTRA_STUFF_AFTER_UNDEF,
	BADLY_FORMED_LINE_DIRECTIVE,
	UNDEF_OF_BUILTIN_MACRO,				// macro
	ERROR_DIRECTIVE,						// error-string

	DIVIDE_BY_ZERO_IN_IFDEF,
	MODULO_BY_ZERO_IN_IFDEF,
	MISSING_PAREN_IN_IFDEF,
	UNEXPECTED_LONGLONG_IN_IFDEF,
	UNEXPECTED_ITEM_IN_IFDEF,				// string
	MISSING_COLON_IN_IFDEF,
	MISSING_DEFINED_PAREN_IN_IFDEF,
	MISSING_MACRO_NAME_IN_IFDEF,
	BAD_DEFINED_USAGE_IN_IFDEF,
	UNEXPECTED_STUFF_IN_IFDEF,				// string
	TOO_MANY_ELSES_IN_IFDEF,
	ELIF_AFTER_ELSE_IN_IFDEF,
	MISSING_ENDIF_FOR_IFDEF,
	EXPECTED_MACRO_NAME_AFTER_IFDEF,		// string
	UNMATCHED_ELIF_WITHOUT_IFDEF,
	UNMATCHED_ELSE_WITHOUT_IFDEF,
	UNMATCHED_ENDIF_WITHOUT_IFDEF,

	EXPECTED_CLOSE_PAREN_OR_COMMA,
	EXPECTED_CLOSE_PAREN,
	HASHHASH_EXPANDS_TO_ONE_TOKEN,		// macro, #tokens
	MACRO_ARGUMENT_COUNT_MISMATCH,		// macro
	HASHHASH_AT_BOL,
	HASHHASH_AT_EOL,
	STRINGIFY_MISSING_ARG,
	PARAM_MUST_FOLLOW_STRINGIFY,		// string

	LONGLONG_IS_NOT_ISO,
	SHORTDOUBLE_IS_NOT_ISO,
	BADLY_FORMED_NUMBER,				// character

	// compiler warnings and errors:
	WACCO_EXPECTED,						// token-name
	WACCO_ILLEGAL,						// rule-name

	EXPECTED_CONST_EXPR,
	SYMBOL_NOT_DEFINED,						// symbol
	ARRAY_EXPRESSION_IS_BACKWARDS,
	DEREF_OF_NON_ARRAY,
	NON_INT_ARRAY_DEREF,
	ILLEGAL_ESCAPE_IN_STRING,				// character
	ILLEGAL_ESCAPE_IN_CHAR,				// character
	EXTRA_JUNK_IN_CHAR,
	ILLEGAL_STUFF_IN_STRUCT,

	ATTEMPT_TO_CALL_NON_FUNCTION,
	NOT_ENOUGH_ARGS_FOR_FUNC_CALL,		// function
	FUNC_ARGUMENT_TYPE_MISMATCH,		// argument#, function
	TOO_MANY_ARGS_FOR_FUNC_CALL,		// function

	STRUCT_REF_OF_NON_STRUCT,				// member-name
	NO_SUCH_MEMBER_IN_UNION,				// member, union-name
	NO_SUCH_MEMBER_IN_STRUCT,				// member, struct-name
	CANNOT_TAKE_ADDR_OF_OBJ,
	DEREF_OF_NON_POINTER,

	CANNOT_INCR_DECR_CONST,
	SHOULD_NOT_INCR_DECR_VOLATILE,
	EXPECTED_INT_OR_FLOAT,
	EXPECTED_INT,
	EXPECTED_ARITH_EXPR,
	EXPECTED_SCALAR_EXPR,
	EXPECTED_INT_EXPR,
	INCOMPATIBLE_POINTER_TYPES,
	ILLEGAL_TYPES_IN_ASSIGN,
	MISMATCHED_EXPRS,

	DUPLICATE_TYPE_QUALIFIERS,
	DUPLICATE_TYPE_SPECIFIERS,
	DUPLICATE_FUNC_TYPE_SPECIFIERS,
	DUPLICATE_STORAGE_CLASSES,
	STRUCT_ALREADY_DEFINED,				// struct/union/enum, line#
	NEGATIVE_VALUE_FOR_BITFIELD,
	ILLEGAL_VALUE_FOR_BITFIELD,
	SYMBOL_ALREADY_DEFINED,				// symbol, line#
	LABEL_ALREADY_DEFINED,				// label, line#
	LABEL_NOT_DEFINED,						// label, line#
	LABEL_NOT_USED,						// label, line#

	CASE_OUTSIDE_SWITCH,
	DEFAULT_OUTSIDE_SWITCH,
	RANGED_CASE_IS_NOT_ISO,
	CONTINUE_OUTSIDE_LOOP,
	BREAK_OUTSIDE_LOOP_OR_SWITCH,
	MISSING_FINAL_BREAK_IN_SWITCH,

	ARRAY_MUST_HAVE_POSITIVE_SIZE,
	INCOMPATIBLE_TYPE_ELEMENTS,
	TOO_MANY_STORAGE_CLASSES,
	SYMBOL_MUST_BE_FUNCTION_TYPE,
	CANNOT_ASSIGN_TO_CONST_OBJECT,

	SHORT_LONG_MEANINGLESS_FOR_CHAR,
	SPECIFIERS_MEANINGLESS_FOR_FLOAT,
	SPECIFIERS_MEANINGLESS_FOR_VOID,
	CANNOT_DECLARE_VOID_TYPE,
	CANNOT_DECLARE_ARRAY_OF_VOIDS,
	INCOMPATIBLE_TYPES_FOR_IDENT,		// symbol, filename, line#

	ISO_BITFIELD_MUST_BE_INT,
	BITFIELD_MUST_BE_INTEGRAL,
	BITFIELD_ALLOWED_ONLY_IN_STRUCTS,
	BITFIELD_TOO_LARGE,
	BITFIELD_MUST_BE_INT,

	FIELD_ALREADY_IN_STRUCT,				// member, line#
	ONLY_REGISTER_ALLOWED_FOR_PARAMS,
	REDECLARATION_OF_PARAMETER,				// parameter
	NON_ISO_COMMA_ENDS_ENUM_LIST,
	FUNCTION_ALREADY_DEFINED,				// function, line#, file
	PARAMETER_CANNOT_BE_VOID,				// parameter
	PARAMETER_ALREADY_DEFINED,				// parameter
	NO_NAME_FOR_FUNC_PARAMETER,				// parameter#, function

	ILLEGAL_TYPE_IN_SIZEOF,
	ILLEGAL_TYPE_FOR_CAST,
	ONLY_ONE_DEFAULT_IN_SWITCH,				// line#
	CASE_EXPRESSION_MUST_BE_CONST,		// line#
	DUPLICATE_CASES_IN_SWITCH,				// case-value str, line#
	RANGE_OVERLAPS_CASE_IN_SWITCH,		// case-value str, line#
	RANGE_ERROR_IN_CASE,				// line#
	NO_CASES_IN_SWITCH,

	OLD_STYLE_FUNCTION_DECLARATOR,
	OLD_STYLE_ARG_DECL_NOT_SPECIFIED,		// parameter-name

	TYPE_MISMATCH_IN_INITIALIZER,
	INITIALIZING_INCOMPLETE_STRUCT,		// struct
	INITIALIZING_INCOMPLETE_UNION,		// union
	TOO_MANY_INITIALIZERS_FOR_ARRAY,
	TOO_MANY_INITIALIZERS_FOR_STRUCT,		// struct
	TOO_MANY_INITIALIZERS_FOR_UNION,		// union
	NOT_ENOUGH_INITIALIZERS_FOR_ARRAY,
	TOO_MANY_INITIALIZERS,
	EXPECTED_CONST_INITIALIZERS,		// symbol

	CANNOT_ASSIGN_TO_NON_LVALUE,
	SYMBOL_NEVER_USED_IN_FUNC,				// symbol, line#, function
	SYMBOL_NEVER_USED_IN_SCOPE,				// symbol, line#
	SYMBOL_USED_BEFORE_INITIALIZED,		// symbol

	RETURN_EXPR_FOR_VOID_FUNC,				// function
	RETURN_FOR_NON_VOID_FUNC,				// function
	RETURN_FOR_INT_FUNC,				// function
	BAD_TYPE_FOR_RETURN_EXPR,				// function
	USELESS_RETURN_AT_FUNC_END,		 // function
	MISSING_RETURN_AT_FUNC_END,				// function

	REF_OF_REGISTER_SYMBOL,				// symbol
	CANNOT_MAKE_REGISTER_SYMBOL,		// symbol
	BAD_STORAGE_FOR_FUNCTION,				// function
	INCOMPATIBLE_STORAGE_FOR_IDENT,		// function, line#, filename
	EXTERN_SYMBOL_IS_INITIALIZED,		// symbol
	BOGUS_STORAGE_FOR_MEMBER,				// member-name
	BOGUS_TYPE_FOR_MEMBER,				// member-name

	CONST_SYMBOL_IS_NOT_INITIALIZED,		// symbol
	FUNCTION_NOT_DECLARED,				// function
	ILLEGAL_EXPR_IN_INITIALIZER,
	EXPECTED_INT_EXPR_FOR_SWITCH,
	CANNOT_CAST_TO_SWITCH_EXPR,				// line#
	ENUM_ALREADY_DEFINED,				// enum-name line#

	SYMBOL_IS_ALREADY_INITIALIZED,		// symbol, line#
	CANNOT_AUTO_A_GLOBAL,				// symbol
	CANNOT_GET_SIZE_OF_INCOMPLETE,
	STRUCT_REF_OF_INCOMPLETE,				// member-name
	CANNOT_DECLARE_INCOMPLETE_VAR,		// symbol

	DECLARATION_SYNTAX_NOT_ISO,
	UNNAMED_SUBSTRUCT_IS_NOT_ISO,
	ONLY_UNNAMED_SUBSTRUCTS_ALLOWED,
	CANNOT_MAKE_INCOMPLETE_UNNAMED,
	STRUCT_DISPLAY_IS_NOT_ISO,
	INIT_INDEX_IS_NOT_ISO,
	ILLEGAL_INIT_INDEX,						// index#
	DUPLICATE_INIT_INDEX,				// index#
	MISSING_INIT_ENTRY,						// index#
	MISSING_INIT_ENTRIES,				// index#, index2#

	NON_ISO_PASCAL_STRING,
	HEX_CHARACTER_TOO_BIG,				// bytes-in-char#
	PASCAL_STRING_TOO_LONG,				// max-bytes#

	MUST_DECLARE_A_VARIABLE_NAME,
	EXPECTED_BITFIELD_OR_DECL,
	BOGUS_FUNCTION_DEFINITION,
	MUST_DECLARE_A_MEMBER_NAME,
	MISSING_TYPE_FOR_DECLARATION,
	FUNCTION_PROTOTYPE_MISMATCH,

	MISSING_COMMA_BEFORE_DOTDOTDOT,
	DECL_AS_STMT_IS_NOT_ISO,
	NO_MATCH_FOR_OVERLOADED_FUNC,		// symbol

	REFERENCE_IS_NOT_INITIALIZED,		// symbol
	FRIEND_OUTSIDE_OF_CLASS,
	BAD_FRIEND_FOR_CLASS,				// symbol
	MISSING_BASE_ACCESS_SPECIFIER,
	CAN_ONLY_INHERIT_FROM_CLASS,
	REFERENCES_ARE_NOT_ISO,
	MEMFUNCS_ARE_NOT_ISO,
	DEFAULT_PARAM_IS_NOT_ISO,
	DECL_IN_FOR_IS_NOT_ISO,
	MULTIPLE_INHERITANCE_NOT_SUPPORTED,

	EXPECTED_CLASS_INSTANCE,				// symbol
	NO_CONSTRUCTORS_FOR_CLASS,				// classname
	NO_MATCH_FOR_CONSTRUCTOR,				// classname
	NO_DESTRUCTORS_FOR_CLASS,				// classname
	EXPECTED_CLASS_TYPE,
	EXPECTED_POINTER_TYPE,
	ILLEGAL_EXPR_IN_CTOR,
	CTOR_INIT_IN_NON_MEMBER,
	NON_BASE_CLASS_INIT,				// classname
	NO_MEMBER_FOR_CLASS,				// member name, classname
	NO_MATCH_FOR_MEMBER,				// member name, classname

	// extras to be sorted above later
	UNEXPECTED_LABEL,
	SYMBOL_NOT_DECLARED,				// symbol, line#
	ILLEGAL_TYPE_IN_CAST_OPERATOR,
	COMPARING_POINTER_CONSTANTS,
	BAD_PRAGMA_FORMAT,
	STRUCT_FORTH_STRING_NOT_DEFINED,

	LAST_ERROR_WITHOUT_COMMA,
	NUM_ERRORS								// no comma here
};

extern void warn(Error e, ...);
extern void error(Error e, ...);
extern void fatal(Error e, ...);
extern void eprintf(const char *fmt, ...);
extern int num_errors();
extern void reset_errors();

// bug messages are hardwired throughout the code - these are fatal
void bug(const char *fmt, ...);


// define DEBUG before including this file to use this macro
#ifdef DEBUG
#define DPRINTF(args) dprintf args
#else
#define DPRINTF(args)
#endif
extern void dprintf(const char *fmt, ...);


#endif // __ERROR_H_
