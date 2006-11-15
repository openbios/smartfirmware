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

// Copyright (c) 1992,1999-2000 by Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/bin/cc/globals.h,v 1.29 2000/12/24 04:33:02 parag Exp $

#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#include <stdlibx.h>

class Cpp;
extern Cpp g_cpp;

extern boolean g_debug;
extern boolean g_verbose;
extern boolean g_cplusplus;
extern boolean g_strict_iso;
extern boolean g_macintosh;
extern boolean g_pascal_strings;
extern boolean g_warn2error;
extern boolean g_nowarnings;
extern boolean g_const_strings;
extern boolean g_sloppy_args;
extern boolean g_do_tree;

extern int g_min_addressable;
extern int g_size_pointer;
extern int g_align_pointer;
extern int g_size_wchar_t;
extern int g_size_size_t;
extern int g_size_char;
extern int g_align_char;
extern int g_size_short;
extern int g_align_short;
extern int g_size_int;
extern int g_align_int;
extern int g_size_long;
extern int g_align_long;
extern int g_size_longlong;
extern int g_align_longlong;
extern int g_size_float_mantissa;
extern int g_size_float_exponent;
extern int g_align_float;
extern int g_size_double_mantissa;
extern int g_size_double_exponent;
extern int g_align_double;
extern int g_size_shortdouble_mantissa;
extern int g_size_shortdouble_exponent;
extern int g_align_shortdouble;
extern int g_size_longdouble_mantissa;
extern int g_size_longdouble_exponent;
extern int g_align_longdouble;

class Type;
class Int_type;
class Float_type;
class Integer;

extern Int_type *g_wchar_t_type;
extern Int_type *g_string_type;
extern Int_type *g_wstring_type;
extern Int_type *g_pascal_string_type;
extern Type *g_forth_string_type;
extern Int_type *g_size_t_type;
extern Int_type *g_ptrdiff_t_type;

extern Type *g_unknown_type;
extern Type *g_void_type;
extern Type *g_voidptr_type;

extern Int_type *g_char_type;
extern Int_type *g_uchar_type;
extern Int_type *g_short_type;
extern Int_type *g_ushort_type;
extern Int_type *g_int_type;
extern Int_type *g_uint_type;
extern Int_type *g_long_type;
extern Int_type *g_ulong_type;
extern Int_type *g_longlong_type;
extern Int_type *g_ulonglong_type;

extern Float_type *g_float_type;
extern Float_type *g_double_type;
extern Float_type *g_shortdouble_type;
extern Float_type *g_longdouble_type;

extern Integer *g_min_addr_size;

class Node;
class Expr_node;
extern Node *g_null_node;
extern Expr_node *g_null_expr;

#endif // __GLOBALS_H_
