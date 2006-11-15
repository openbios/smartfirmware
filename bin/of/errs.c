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

/* only routine in here is err2str to convert a Retcode to a string. */

#include "errs.h"


const char *
err2str(Retcode r)
{
	switch (r)
	{
	case NO_ERROR:
		return "no error";

	case R_END:
		return "end";

	case E_USER_ABORT:
		return "user abort";

	case E_INVALID_MEMORY:
		return "invalid memory access";

	case E_INPUT_ERROR:
		return "unexpected error in input stream";
		
	case E_ILLEGAL_XTOK:
		return "illegal execution-token number out of range";
		
	case E_ILLEGAL_FCODE:
		return "illegal fcode number out of range";
	
	case E_OFFSET_RANGE:
		return "branch offset out of range of Short";
		
	case E_INVALID_POINTER:
		return "invalid pointer";
		
	case E_PICNUM_FORMAT:
		return "picture/numeric conversion format error";
		
	case E_OUT_OF_MEMORY:
		return "out of memory";
		
	case E_OUT_OF_STORAGE:
		return "no storage available to claim";
		
	case E_ILLEGAL_TYPE:
		return "internal illegal/unexpected type of Entry";
		
	case E_TABLE_ERROR:
		return "internal hashtable error of some sort";
		
	case E_NEWDEF_ERROR:
		return "internal error while trying to create new definition";
		
	case E_COMPILATION_ERROR:
		return "internal Forth compilation state error";
		
	case E_DEFER_ERROR:
		return "attempt to set non-defer/value word";
		
	case E_INVALID_XTOKEN:
		return "invalid execution-token value - undefined?";
		
	case E_BAD_FCODE_FORMAT:
		return "bad fcode format";
		
	case E_BAD_SYMBOL:
		return "no such symbol in tables";
	
	case E_NULL_PACKAGE:
		return "unexpected NULL (pointer to) package";
	
	case E_NO_METHOD:
		return "call to non-existent method";
	
	case E_UNIMPLEMENTED:
		return "call to unimplemented function/method";
	
	case E_NO_PROPERTY:
		return "no such property";
	
	case E_BAD_PROPERTY_TYPE:
		return "bad type for property";
	
	case E_PROPERTY_ERROR:
		return "unexpected problem encoding/decoding property data";
	
	case E_PACKAGE_ERROR:
		return "unexpected problem with package";
	
	case E_EXECUTION_ERROR:
		return "unexpected problem while executing/evaluating - infinite loop?";
	
	case E_NO_DEVICE:
		return "no such device";
	
	case E_INIT_ERROR:
		return "fatal error during initialization!";
	
	case E_BAD_INSTANCE:
		return "bad instance";
	
	case E_BLOCKSIZE:
		return "incorrect block number or size";
	
	case E_READ_ERROR:
		return "unexpected problem  while reading from a device";
	
	case E_WRITE_ERROR:
		return "unexpected problem while writing to a device";
	
	case E_SEEK_ERROR:
		return "unexpected problem while seeking on a device";
	
	case E_EMPTY_STRING:
		return "expected a string here";
	
	case E_NO_SELFTEST:
		return "no selftest method, or some other error";
	
	case E_EMULATOR_BUG:
		return "bug in terminal emulator implementation";
	
	case E_BAD_PIXELSIZE:
		return "bad pixel size - must be power of two between 1 and 64";

	case E_TOKENIZER_ERROR:
		return "tokenizer error";

	case E_OUT_OF_USER_FCODES:
		return "no more fcodes available for allocation";
	
	case E_NO_FILE:
		return "cannot open file for reading or writing";
	
	case E_FILE_IO:
		return "file I/O error - seek/read/write";
	
	case E_NO_CALLBACK:
		return "no callback routine defined";

	case E_CALLBACK_ERROR:
		return "error in callback function";

	case E_NO_CONSOLE:
		return "cannot open a console for I/O";

	case E_NO_KEYBOARD:
		return "cannot open keyboard for console input";

	case E_BOOT_ERROR:
		return "error while trying to load or boot";

	case E_NO_CONFIG_VAR:
		return "no such configuration variable";

	case E_OUT_OF_PROM:
		return "out of nvram space (or other nvram error)";

	case E_PROM_WRITE_ERROR:
		return "nvram write error";

	case E_PROM_READ_ERROR:
		return "nvram read error";

	case E_MEM_TEST_FAIL:
		return "memory test failure";

	case E_BAD_NUM_FORMAT:
		return "bad format for a number - cannot convert from string to num";
	
	case E_SELFTEST_FAILED:
		return "selftest failure";
	
	case E_NO_MOUSE:
		return "cannot open mouse device";
	
	case E_BAD_DEVICE:
		return "device is of wrong type or cannot be opened";

	case E_UNCOMPRESS_FAILED:
		return "could not uncompress image";

	case E_BAD_IMAGE:
		return "bootimage format is of an unknown format";

	case E_NO_IMAGE:
		return "no matching boot-image found";
	
	case E_UNSUPPORTED:
		return "no hardware support";

	case E_BAD_PACKAGE:
		return "bad package";

	case E_NO_FCODE:
		return "no identifiable Fcode in expansion ROM";

	case E_NO_DRIVER:
		return "no builtin driver for device";
	
	case E_NULL_INSTANCE:
		return "unexpected NULL (pointer to) instance";
	
	case E_BAD_LOAD_IMAGE:
		return "bad or non-existent image to load/run";
	
	case E_BAD_ARGUMENT:
		return "bad argument format for open method";

	case E_NO_FILESYS:
		return "no filesystem recognized";

	case E_BAD_COLOR:
		return "bad color specification";
		
	case E_NUM_OUT_OF_RANGE:
		return "numeric constant is out-of-range";

	case E_PKG_STACK_OVERFLOW:
		return "package stack overflow";
		
	case E_PKG_STACK_UNDERFLOW:
		return "package stack underflow";

	case E_UNSUPPORTED_FILESYS:
		return "filesystem not supported";

	case E_SECURE_CONFIG_VAR:
		return "no defaults for security config variables";

	case E_NET_TIMEOUT:
		return "network timeout";

	case E_NET_ERROR:
		return "unknown network error";

	case E_NET_PROTOCOL_ERROR:
		return "network protocol error";

	case E_NO_INTR_MAP:
		return "no interrupt mapping or cannot map";

#ifdef DEBUG_MALLOC
	case E_HEAD_GUARD_TRASHED:
		return "malloc memory block head guard trashed!";
	
	case E_TAIL_GUARD_TRASHED:
		return "malloc memory block tail guard trashed!";
#endif

#ifdef macintosh
	case E_MAC_NO_MBAR:
		return "no Mac menubar resource!";
	
	case E_MAC_NO_MENU:
		return "missing Mac menu resource!";
	
	case E_MAC_NO_CLUT:
		return "cannot get color table!";
	
	case E_MAC_NO_WINDOW:
		return "cannot create main window!";
#endif

#ifdef CUSTOM_ERRORS_C
	#include CUSTOM_ERRORS_C
#endif

	/* specific errors from Forth spec - not all are used by SmartFirmware */
	case E_ABORT:
	case E_ABORTQUOTE:
		return "abort";

	case E_STACK_OVERFLOW:
		return "data stack overflow";
		
	case E_STACK_UNDERFLOW:
		return "data stack underflow";
		
	case E_RSTACK_OVERFLOW:
		return "return stack overflow"; 
		
	case E_RSTACK_UNDERFLOW:
		return "return stack underflow";

	case E_DO_LOOP_NESTED_TOO_DEEP:
		return "do loops nested too deep during recursion";

	case E_DICTIONARY_OVERFLOW:
		return "dictionary overflo";

	case E_INVALID_MEMORY_ADDR:
		return "invalid memory address";

	case E_DIVIDE_BY_ZERO:
		return "division by zero";

	case E_RESULT_OUT_OF_RANGE:
		return "result out of range";

	case E_ARG_TYPE_MISMATCH:
		return "argument type mismatch";

	case E_UNDEFINED_WORD:
		return "undefined word";

	case E_CANNOT_INTERP_COMPILE_WORD:
		return "interpreting a compile-only word";

	case E_INVALID_FORGET:
		return "invalid FORGET";

	case E_ZERO_LENGTH_STRING_AS_NAME:
		return "attempt to use zero-length string as a name";

	case E_PICNUM_OVERFLOW:
		return "pictured numeric output string overflow";

	case E_PARSED_STRING_OVERFLOW:
		return "parsed string overflow";

	case E_DEFINITION_TOO_LONG:
		return "definition name too long";

	case E_WRITE_TO_READ_ONLY_LOC:
		return "write to a read-only location";

	case E_UNSUPPORTED_OPERATION:
		return "unsupported operation";

	case E_CONTROL_STRUCTURE_MISMATCH:
		return "control structure mismatch";

	case E_ADDR_ALIGN_ERR:
		return "address alignment exception";

	case E_INVALID_NUMERIC_ARG:
		return "invalid numeric argument";

	case E_RSTACK_IMBALANCE:
		return "return stack imbalance";

	case E_LOOP_PARAMS_UNAVAILABLE:
		return "loop parameters unavailable";

	case E_INVALID_RECURSION:
		return "invalid recursion";

	case E_USER_INTERRUPT:
		return "user interrupt";

	case E_COMPILER_NESTING:
		return "compiler nesting";

	case E_OBSOLESCENT_FEATURE:
		return "obsolescent feature";

	case E_BODY_USED_ON_NON_CREATE:
		return ">BODY used on non-CREATEd definition";

	case E_INVALID_NAME_ARG:
		return "invalid name argument";

	case E_BLOCK_READ_ERR:
		return "block read exception";

	case E_BLOCK_WRITE_ERR:
		return "block write exception";

	case E_INVALID_BLOCK_NUM:
		return "invalid block number";

	case E_INVALID_FILE_POS:
		return "invalid file position";

	case E_FILE_IO_ERR:
		return "file I/O exception";

	case E_NONEXISTENT_FILE:
		return "non-existent file";

	case E_UNEXPECTED_EOF:
		return "unexpected end of file";

	case E_INVALID_BASE_FOR_FLOAT_CONV:
		return "invalid BASE for floating point conversion";

	case E_LOSS_OF_PRECISION:
		return "loss of precision";

	case E_FP_DIVIDE_BY_ZERO:
		return "floating-point divide by zero";

	case E_FP_RESULT_OUT_OF_RANGE:
		return "floating-point result out of range";

	case E_FP_STACK_OVERFLOW:
		return "floating-point stack overflow";

	case E_FP_STACK_UNDERFLOW:
		return "floating-point stack underflow";

	case E_FP_INVALID_ARG:
		return "floating-point invalid argument";

	case E_COMPILATION_WORD_LIST_DELETED:
		return "compilation word list deleted";

	case E_INVALID_POSTPONE:
		return "invalid POSTPONE";

	case E_SEARCH_ORDER_OVERFLOW:
		return "search-order overflow";

	case E_SEARCH_ORDER_UNDERFLOW:
		return "search-order underflow";

	case E_COMPILATION_WORD_LIST_CHANGED:
		return "compilation word list changed";

	case E_CONTROL_FLOW_STACK_OVERFLOW:
		return "control-flow stack overflow";

	case E_EXCEPTION_STACK_OVERFLOW:
		return "exception stack overflow";

	case E_FP_UNDERFLOW:
		return "floating-point underflow";

	case E_FP_UNIDENTIFIED_FAULT:
		return "floating-point unidentified fault";
	
	case E_QUIT:
		return "QUIT";

	case E_SEND_RECV_CHAR_ERR:
		return "exception in sending or receiving a character";

	case E_IF_ELSE_THEN_ERR:
		return "[IF], [ELSE], or [THEN] exception";

	default:
		break;
	}
	
	return "unknown error";
}
