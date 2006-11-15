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

/* Copyright (c) 1991,1997 by Thomas J. Merritt and Parag Patel.
   All Rights Reserved.
 */

#if defined(SPOOL) || defined(POOL) || !defined(__POOL_H_)

#ifndef __POOL_H_
#define __POOL_H_
#endif

#include <stdlibx.h>

long _hashtable_bumpfunc(long);


#define __poolelemtype(POOL)  name2(__pool_elem, POOL)

#define declare_pool(POOL, ELEM) \
 \
class __poolelemtype(POOL) : public ELEM \
{ \
public: \
     int _refs; \
     __poolelemtype(POOL) *_link; \
     __poolelemtype(POOL)(ELEM const &k) : ELEM(k) { _link = NULL; _refs = 0; } \
}; \
 \
class POOL \
{ \
    __poolelemtype(POOL) **_chains; \
    long _numchains; \
    long _size; \
    __poolelemtype(POOL) *_cur; \
    void bump(long chains = 0); \
    __poolelemtype(POOL) *lookup(ELEM const &) const; \
    boolean find(ELEM const &e) { _cur = lookup(e); return _cur != NULL; } \
    boolean insert(ELEM const &); \
    boolean remove(__poolelemtype(POOL) *); \
public: \
    POOL(); \
    POOL(POOL const &); \
    ~POOL(); \
    void clear(); \
    void assign(POOL const &); \
    boolean compare(POOL const &) const; \
    void add(POOL const &); \
    long size() const { return _size; } \
    ELEM &get(ELEM const &k) { insert(k); _cur->_refs++; return *_cur; } \
    ELEM &get(ELEM const &k, boolean &x) \
	   { x = !insert(k); _cur->_refs++; return *_cur; } \
    void put(ELEM const &); \
    int refs(ELEM const &k) { return ((__poolelemtype(POOL) *)&k)->_refs; } \
    void ref(ELEM const &k) { ((__poolelemtype(POOL) *)&k)->_refs++; } \
    void unref(ELEM const &k); \
    POOL &operator=(POOL const &p) { assign(p); return *this; } \
    POOL &operator+=(POOL const &p) { add(p); return *this; } \
    boolean operator==(POOL const &p) const { return compare(p); } \
    boolean operator!=(POOL const &p) const { return !compare(p); } \
}; \


#define implement_pool(POOL, ELEM, HASHFUNC) \
 \
void \
POOL::bump(long chains) \
{ \
    if (chains == 0) \
    { \
	chains = _hashtable_bumpfunc(_size); \
 \
	if (chains == 0) \
	    chains = 1; \
    } \
 \
    if (_numchains >= chains) \
	return; \
 \
    __poolelemtype(POOL) **nchains = new __poolelemtype(POOL) *[chains]; \
    long i; \
 \
    for (i = 0; i < chains; i++) \
    	nchains[i] = NULL; \
 \
    __poolelemtype(POOL) *list = NULL; \
    __poolelemtype(POOL) *last = NULL; \
 \
    for (i = 0; i < _numchains; i++) \
    	if (_chains[i] != NULL) \
	{ \
	    if (last != NULL) \
		last->_link = _chains[i]; \
	    else \
		list = _chains[i]; \
 \
	    for (last = _chains[i]; last->_link != NULL; last = last->_link) \
	    	; \
	} \
 \
    for (last = list; last != NULL; ) \
    { \
    	list = last; \
	last = last->_link; \
    	unsigned long hv = HASHFUNC(*list) % chains; \
	list->_link = nchains[hv]; \
	nchains[hv] = list; \
    } \
 \
    if (_chains != NULL) \
	delete[/*_numchains*/] _chains; \
 \
    _chains = nchains; \
    _numchains = chains; \
} \
 \
POOL::POOL() \
{ \
    _chains = NULL; \
    _numchains = 0; \
    _size = 0; \
    _cur = NULL; \
    bump(); \
} \
 \
POOL::POOL(POOL const &t) \
{ \
    _chains = NULL; \
    _numchains = 0; \
    _size = 0; \
    _cur = NULL; \
    bump(); \
    add(t); \
} \
 \
POOL::~POOL() \
{ \
    clear(); \
    delete[/*_numchains*/] _chains; \
} \
 \
__poolelemtype(POOL) * \
POOL::lookup(ELEM const &k) const \
{ \
    unsigned long hv = HASHFUNC(k) % _numchains; \
 \
    for (__poolelemtype(POOL) *p = _chains[hv]; p != NULL; p = p->_link) \
    	if (*p == k) \
	    return p; \
 \
    return NULL; \
} \
 \
boolean \
POOL::insert(ELEM const &k) \
{ \
    unsigned long hv = HASHFUNC(k) % _numchains; \
    __poolelemtype(POOL) *p; \
 \
    for (p = _chains[hv]; p != NULL; p = p->_link) \
    	if (*p == k) \
	{ \
	    _cur = p; \
	    return FALSE; \
	} \
 \
    p = new __poolelemtype(POOL)(k); \
    p->_link = _chains[hv]; \
    _chains[hv] = p; \
    _cur = p; \
    _size++; \
 \
    if (_size << 1 > (_numchains << 1) + _numchains) \
    	bump(); \
 \
    return TRUE; \
} \
 \
boolean \
POOL::remove(__poolelemtype(POOL) *e) \
{ \
    unsigned long hv = HASHFUNC(*e) % _numchains; \
 \
    for (__poolelemtype(POOL) *p = _chains[hv], *q = NULL; p != NULL; \
	    p = p->_link) \
    	if (p == e) \
	{ \
	    if (_cur == p) \
		_cur = NULL; \
 \
	    if (q != NULL) \
		q->_link = p->_link; \
	    else \
		_chains[hv] = p->_link; \
 \
	    delete p; \
	    _size--; \
	    return TRUE; \
	} \
	else \
	    q = p; \
 \
    return FALSE; \
} \
 \
void \
POOL::clear() \
{ \
    for (long i = 0; i < _numchains; i++) \
    { \
    	for (__poolelemtype(POOL) *p = _chains[i], *q = NULL; p != NULL; p = q) \
	{ \
	    q = p->_link; \
	    delete p; \
	} \
 \
	_chains[i] = NULL; \
    } \
 \
    _size = 0; \
    _cur = NULL; \
} \
 \
void \
POOL::assign(POOL const &t) \
{ \
    clear(); \
    add(t); \
} \
 \
boolean \
POOL::compare(POOL const &t) const \
{ \
    if (_size != t._size) \
	return FALSE; \
 \
    for (long i = 0; i < _numchains; i++) \
    	for (__poolelemtype(POOL) *p = _chains[i]; p != NULL; p = p->_link) \
	{ \
	    __poolelemtype(POOL) *e = t.lookup(*p); \
 \
	    if (e == NULL || !(*e == *p)) \
		return FALSE; \
	} \
 \
    return TRUE; \
} \
 \
void \
POOL::add(POOL const &t) \
{ \
    for (long i = 0; i < t._numchains; i++) \
    	for (__poolelemtype(POOL) *p = t._chains[i]; p != NULL; p = p->_link) \
	    if (insert(*p)) \
		*_cur = *p; \
} \
 \
void \
POOL::put(ELEM const &k) \
{ \
    if (!find(k)) \
	return; \
 \
    if (--(_cur->_refs) == 0) \
    	remove(_cur); \
} \
 \
 \
 \
 \
 \
void \
POOL::unref(ELEM const &k) \
{ \
    __poolelemtype(POOL) *elem = (__poolelemtype(POOL) *)&k; \
 \
    if (--(elem->_refs) == 0) \
	remove(elem); \
} \



extern char const *strget(char const *);

#endif
