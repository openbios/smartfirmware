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

/* Copyright (c) 1991,1993,1997 by Thomas J. Merritt and Parag Patel.
   All Rights Reserved.
 */

#if defined(ARRAY) || !defined(__DYNARRAY_H_)

#ifndef __DYNARRAY_H_
#define __DYNARRAY_H_
#endif

#ifndef __STDLIBX_H_
#include <stdlib.h>
#include <stdlibx.h>
#endif

#ifndef NULL
#define NULL 0
#endif

#define declare_array(ARRAY, TYPE, BUMP) \
 \
class ARRAY \
{ \
    TYPE *_array; \
    int _arrlen; \
    int _size; \
    void doexpand(int leng); \
    void expand(int leng) \
	{ if (leng > _arrlen) doexpand(leng); \
	  else if (leng > _size) _size = leng; } \
public: \
    ARRAY(); \
    ARRAY(int length); \
    ARRAY(ARRAY const &); \
    ~ARRAY(); \
    void assign(ARRAY const &); \
    void swap(ARRAY &); \
    boolean compare(ARRAY const &) const; \
    int size() const { return _size; } \
    void reset(int newlen = 0); \
    void resize(int newlen = 0); \
    TYPE &elem(int e) \
    { \
	if (e < 0) crash("negative dynarray reference"); \
	else if (e >= _size) expand(e + 1); \
	return _array[e]; \
    } \
    TYPE &elt(int e) const { return _array[e]; } \
    TYPE &end() { int sz = _size; expand(sz + 1); return _array[sz]; } \
    TYPE *getarr() const { return _array; } \
    ARRAY &operator=(ARRAY const &a) { assign(a); return *this; } \
    boolean operator==(ARRAY const &a) const { return compare(a); } \
    boolean operator!=(ARRAY const &a) const { return !compare(a); } \
    TYPE &operator[](int e) { return elem(e); } \
    TYPE *operator()() const { return getarr(); } \
}; \
 \


#define implement_array(ARRAY, TYPE, BUMP) \
 \
void \
ARRAY::doexpand(int size) \
{ \
    if (size > _size) \
    { \
	if (size > _arrlen) \
	{ \
	    int nlen = _arrlen > 0 ? _arrlen : 1; \
 \
	    while (nlen < size) \
		if (nlen < BUMP) \
		    nlen <<= 1; \
		else \
		    nlen += BUMP; \
 \
	    TYPE *narray = new TYPE[nlen]; \
 \
	    if (_array != NULL) \
	    { \
		for (int i = 0; i < _arrlen; i++) \
		    narray[i] = _array[i]; \
 \
		delete[/*_arrlen*/] _array; \
	    } \
 \
	    _array = narray; \
	    _arrlen = nlen; \
	} \
 \
	_size = size; \
    } \
} \
 \
ARRAY::ARRAY() \
{ \
    _array = NULL; \
    _arrlen = 0; \
    _size = 0; \
} \
 \
ARRAY::ARRAY(int length) \
{ \
    _array = NULL; \
    _arrlen = 0; \
    _size = 0; \
    expand(length); \
} \
 \
ARRAY::ARRAY(ARRAY const &a) \
{ \
    _array = NULL; \
    _arrlen = 0; \
    _size = 0; \
    int sz = a.size(); \
    expand(sz); \
 \
    for (int i = 0; i < sz; i++) \
    	_array[i] = a._array[i]; \
} \
 \
ARRAY::~ARRAY() \
{ \
    if (_array) \
	delete[/*_arrlen*/] _array; \
} \
 \
void \
ARRAY::assign(ARRAY const &a) \
{ \
    reset(a.size()); \
 \
    for (int i = 0; i < _size; i++) \
    	_array[i] = a._array[i]; \
} \
 \
void \
ARRAY::swap(ARRAY &x) \
{ \
    int s = _size; \
    int l = _arrlen; \
    TYPE *arr = _array; \
    _size = x._size; \
    _arrlen = x._arrlen; \
    _array = x._array; \
    x._size = s; \
    x._arrlen = l; \
    x._array = arr; \
} \
 \
boolean \
ARRAY::compare(ARRAY const &a) const \
{ \
    if (_size != a._size) \
	return FALSE; \
 \
    for (int i = 0; i < _size; i++) \
    	if (!(_array[i] == (TYPE)a._array[i])) \
	    return FALSE; \
 \
    return TRUE; \
} \
 \
void \
ARRAY::reset(int newlen) \
{ \
    int len = newlen > 0 ? newlen : 0; \
 \
    if (len > 0) \
	expand(len); \
    else \
    { \
    	if (_array != NULL) \
	    delete[/*_arrlen*/] _array; \
 \
	_array = NULL; \
	_arrlen = 0; \
    } \
 \
    _size = len; \
} \
 \
void \
ARRAY::resize(int newlen) \
{ \
    int len = newlen > 0 ? newlen : 0; \
    expand(len); \
    _size = len; \
} \



declare_array(Charbuf, char, 1024);

#endif
