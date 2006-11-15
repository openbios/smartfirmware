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

/* (c) Copyright 1996-1998 by CodeGen, Inc.  All Rights Reserved. */

#ifndef __ERRS_H_
#define __ERRS_H_

/* Only error codes go in this file.  By appending them to the end, only errs.c
   and the file using the new error code need to be recompiled.
 */


/*	Exception word set limits error numbers from -4095..-1, with -255..-1
	reserved by the ANS Forth standard.  The following ensures that our
	additional errors fall within this range.
 */
enum retcode
{
	NO_ERROR = 0,
	
	/* special magic values for special return codes */
	RET_CODES = -4090,
	R_END,
	E_USER_ABORT,
	
	/* errors that aren't in the Forth spec */
	E_ERRORS = -3500,
	E_INVALID_MEMORY,
	E_INPUT_ERROR,
	E_ILLEGAL_XTOK,
	E_ILLEGAL_FCODE,
	E_OFFSET_RANGE,
	E_INVALID_POINTER,
	E_PICNUM_FORMAT,
	E_OUT_OF_MEMORY,
	E_OUT_OF_STORAGE,
	E_ILLEGAL_TYPE,
	E_TABLE_ERROR,
	E_NEWDEF_ERROR,
	E_COMPILATION_ERROR,
	E_DEFER_ERROR,
	E_INVALID_XTOKEN,
	E_BAD_FCODE_FORMAT,
	E_BAD_SYMBOL,
	E_NULL_PACKAGE,
	E_NO_METHOD,
	E_UNIMPLEMENTED,
	E_NO_PROPERTY,
	E_BAD_PROPERTY_TYPE,
	E_PROPERTY_ERROR,
	E_PACKAGE_ERROR,
	E_EXECUTION_ERROR,
	E_NO_DEVICE,
	E_INIT_ERROR,
	E_BAD_INSTANCE,
	E_BLOCKSIZE,
	E_READ_ERROR,
	E_WRITE_ERROR,
	E_SEEK_ERROR,
	E_EMPTY_STRING,
	E_NO_SELFTEST,
	E_EMULATOR_BUG,
	E_BAD_PIXELSIZE,
	E_TOKENIZER_ERROR,
	E_OUT_OF_USER_FCODES,
	E_NO_FILE,
	E_FILE_IO,
	E_NO_CALLBACK,
	E_CALLBACK_ERROR,
	E_NO_CONSOLE,
	E_NO_KEYBOARD,
	E_BOOT_ERROR,
	E_NO_CONFIG_VAR,
	E_OUT_OF_PROM,
	E_PROM_WRITE_ERROR,
	E_PROM_READ_ERROR,
	E_MEM_TEST_FAIL,
	E_BAD_NUM_FORMAT,
	E_SELFTEST_FAILED,
	E_NO_MOUSE,
	E_BAD_DEVICE,
	E_UNCOMPRESS_FAILED,
	E_BAD_IMAGE,
	E_NO_IMAGE,
	E_UNSUPPORTED,
	E_BAD_PACKAGE,
	E_NO_FCODE,
	E_NO_DRIVER,
	E_NULL_INSTANCE,
	E_BAD_LOAD_IMAGE,
	E_BAD_ARGUMENT,
	E_NO_FILESYS,
	E_BAD_COLOR,
	E_NUM_OUT_OF_RANGE,
	E_PKG_STACK_OVERFLOW,
	E_PKG_STACK_UNDERFLOW,
	E_UNSUPPORTED_FILESYS,
	E_SECURE_CONFIG_VAR,
	E_NET_TIMEOUT,
	E_NET_ERROR,
	E_NET_PROTOCOL_ERROR,
	E_NO_INTR_MAP,

#ifdef DEBUG_MALLOC
	E_HEAD_GUARD_TRASHED,
	E_TAIL_GUARD_TRASHED,
#endif

#ifdef macintosh
	E_MAC_NO_MBAR,
	E_MAC_NO_MENU,
	E_MAC_NO_CLUT,
	E_MAC_NO_WINDOW,
#endif

#ifdef CUSTOM_ERRORS_H
	#include CUSTOM_ERRORS_H
#endif

	/* specific errors from Forth spec go here
	   - not all are used by SmartFirmware
	 */
	FORTH_ERRORS = -255,
	E_ABORT = -1,
	E_ABORTQUOTE = -2,
	E_STACK_OVERFLOW = -3,
	E_STACK_UNDERFLOW = -4,
	E_RSTACK_OVERFLOW = -5,
	E_RSTACK_UNDERFLOW = -6,
	E_DO_LOOP_NESTED_TOO_DEEP = -7,
	E_DICTIONARY_OVERFLOW = -8,
	E_INVALID_MEMORY_ADDR = -9,
	E_DIVIDE_BY_ZERO = -10,
	E_RESULT_OUT_OF_RANGE = -11,
	E_ARG_TYPE_MISMATCH = -12,
	E_UNDEFINED_WORD = -13,
	E_CANNOT_INTERP_COMPILE_WORD = -14,
	E_INVALID_FORGET = -15,
	E_ZERO_LENGTH_STRING_AS_NAME = -16,
	E_PICNUM_OVERFLOW = -17,
	E_PARSED_STRING_OVERFLOW = -18,
	E_DEFINITION_TOO_LONG = -19,
	E_WRITE_TO_READ_ONLY_LOC = -20,
	E_UNSUPPORTED_OPERATION = -21,
	E_CONTROL_STRUCTURE_MISMATCH = -22,
	E_ADDR_ALIGN_ERR = -23,
	E_INVALID_NUMERIC_ARG = -24,
	E_RSTACK_IMBALANCE = -25,
	E_LOOP_PARAMS_UNAVAILABLE = -26,
	E_INVALID_RECURSION = -27,
	E_USER_INTERRUPT = -28,
	E_COMPILER_NESTING = -29,
	E_OBSOLESCENT_FEATURE = -30,
	E_BODY_USED_ON_NON_CREATE = -31,
	E_INVALID_NAME_ARG = -32,
	E_BLOCK_READ_ERR = -33,
	E_BLOCK_WRITE_ERR = -34,
	E_INVALID_BLOCK_NUM = -35,
	E_INVALID_FILE_POS = -36,
	E_FILE_IO_ERR = -37,
	E_NONEXISTENT_FILE = -38,
	E_UNEXPECTED_EOF = -39,
	E_INVALID_BASE_FOR_FLOAT_CONV = -40,
	E_LOSS_OF_PRECISION = -41,
	E_FP_DIVIDE_BY_ZERO = -42,
	E_FP_RESULT_OUT_OF_RANGE = -43,
	E_FP_STACK_OVERFLOW = -44,
	E_FP_STACK_UNDERFLOW = -45,
	E_FP_INVALID_ARG = -46,
	E_COMPILATION_WORD_LIST_DELETED = -47,
	E_INVALID_POSTPONE = -48,
	E_SEARCH_ORDER_OVERFLOW = -49,
	E_SEARCH_ORDER_UNDERFLOW = -50,
	E_COMPILATION_WORD_LIST_CHANGED = -51,
	E_CONTROL_FLOW_STACK_OVERFLOW = -52,
	E_EXCEPTION_STACK_OVERFLOW = -53,
	E_FP_UNDERFLOW = -54,
	E_FP_UNIDENTIFIED_FAULT = -55,
	E_QUIT = -56,
	E_SEND_RECV_CHAR_ERR = -57,
	E_IF_ELSE_THEN_ERR = -58,
	
	END_RETCODES
};
typedef enum retcode Retcode;


extern const char *err2str(Retcode r);


#endif /* __ERRS_H_ */
