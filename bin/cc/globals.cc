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
// $Header: /u/cgt/cvs/bin/cc/globals.cc,v 1.28 2000/12/24 04:33:01 parag Exp $

#include <stddef.h>
#include <stdlibx.h>
#include "cpp.h"
#include "globals.h"

Cpp g_cpp;

boolean g_debug = FALSE;
boolean g_verbose = FALSE;
boolean g_cplusplus = FALSE;
boolean g_strict_iso = FALSE;
boolean g_macintosh = FALSE;
boolean g_pascal_strings = FALSE;
boolean g_warn2error = FALSE;
boolean g_nowarnings = FALSE;
boolean g_const_strings = FALSE;
boolean g_sloppy_args = FALSE;
boolean g_do_tree = TRUE;

int g_min_addressable = 8;
int g_size_pointer = 32;
int g_align_pointer = 32;
int g_size_wchar_t = 16;
int g_size_size_t = BITSPERLONG;
int g_size_char = 8;
int g_align_char = 8;
int g_size_short = 16;
int g_align_short = 16;
int g_size_int = 32;
int g_align_int = 32;
int g_size_long = 32;
int g_align_long = 32;
int g_size_longlong = 64;
int g_align_longlong = 32;
//int g_size_float = 32;
int g_size_float_mantissa = 24;
int g_size_float_exponent = 8;
int g_align_float = 32;
//int g_size_shortdouble = 32;
int g_size_shortdouble_mantissa = 24;
int g_size_shortdouble_exponent = 8;
int g_align_shortdouble = 32;
//int g_size_double = 64;
int g_size_double_mantissa = 53;
int g_size_double_exponent = 11;
int g_align_double = 32;
//int g_size_longdouble = 128;
int g_size_longdouble_mantissa = 113;
int g_size_longdouble_exponent = 15;
int g_align_longdouble = 32;

Int_type *g_wchar_t_type = NULL;
Int_type *g_string_type = NULL;
Int_type *g_wstring_type = NULL;
Int_type *g_pascal_string_type = NULL;
Type *g_forth_string_type = NULL;
Int_type *g_size_t_type = NULL;
Int_type *g_ptrdiff_t_type = NULL;

Type *g_unknown_type = NULL;
Type *g_void_type = NULL;
Type *g_voidptr_type = NULL;

Int_type *g_char_type = NULL;
Int_type *g_uchar_type = NULL;
Int_type *g_short_type = NULL;
Int_type *g_ushort_type = NULL;
Int_type *g_int_type = NULL;
Int_type *g_uint_type = NULL;
Int_type *g_long_type = NULL;
Int_type *g_ulong_type = NULL;
Int_type *g_longlong_type = NULL;
Int_type *g_ulonglong_type = NULL;

Float_type *g_float_type = NULL;
Float_type *g_double_type = NULL;
Float_type *g_shortdouble_type = NULL;
Float_type *g_longdouble_type = NULL;

Integer *g_min_addr_size = NULL;

Node *g_null_node = NULL;
Expr_node *g_null_expr = NULL;
