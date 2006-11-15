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

#if defined(TABLE) || !defined(__HASHTABLE_H_)

#ifndef __HASHTABLE_H_
#define __HASHTABLE_H_
#endif

#include <stdlibx.h>

extern long _hashtable_bumpfunc(long);

#define __elemtype(TABLE)  name2(__hashtable_elem, TABLE)

#define declare_hashtable(TABLE, ITER, ELEM, KEY) \
 \
class __elemtype(TABLE) : public ELEM \
{ \
public: \
     __elemtype(TABLE) *_link; \
     __elemtype(TABLE)(KEY const &k) : ELEM(k) { _link = NULL; } \
}; \
 \
class ITER; \
 \
class TABLE \
{ \
    __elemtype(TABLE) **_chains; \
    long _numchains; \
    long _size; \
    __elemtype(TABLE) *_cur; \
    ITER *_iters; \
    void bump(long chains = 0); \
    __elemtype(TABLE) *lookup(KEY const &) const; \
    friend class ITER; \
public: \
    TABLE(); \
    TABLE(TABLE const &); \
    ~TABLE(); \
    void clear(); \
    long size() const { return _size; } \
    boolean find(KEY const &k) { _cur = lookup(k); return _cur != NULL; } \
    boolean insert(KEY const &); \
    boolean remove(KEY const &); \
    ELEM &elt(KEY const &); \
    ELEM &cur() const { return *_cur; } \
    void assign(TABLE const &); \
    boolean compare(TABLE const &) const; \
    void insert(TABLE const &); \
    void remove(TABLE const &); \
    void intersect(TABLE const &); \
    boolean operator+=(KEY const &k) { return insert(k); } \
    boolean operator-=(KEY const &k) { return remove(k); } \
    TABLE &operator=(TABLE const &t) { assign(t); return *this; } \
    boolean operator==(TABLE const &t) const { return compare(t); } \
    boolean operator!=(TABLE const &t) const { return !compare(t); } \
    TABLE &operator+=(TABLE const &t) { insert(t); return *this; } \
    TABLE &operator-=(TABLE const &t) { remove(t); return *this; } \
    TABLE &operator&=(TABLE const &t) { intersect(t); return *this; } \
    ELEM &operator[](KEY const &k) { return elt(k); } \
    ELEM &operator()() const { return cur(); } \
    ELEM &operator*() const { return cur(); } \
    ELEM *operator->() const { return &cur(); } \
    long numchains() { return _numchains; } \
}; \
 \
class ITER \
{ \
    TABLE *_t; \
    ITER *_n; \
    ITER *_p; \
    long _hv; \
     __elemtype(TABLE) *_cur; \
    void attach(TABLE &); \
    void detach(); \
    friend class TABLE; \
public: \
    ITER(); \
    ITER(ITER const &); \
    ITER(TABLE &); \
    ~ITER(); \
    void assign(ITER const &); \
    void assign(TABLE &); \
    boolean compare(ITER const &) const; \
    ELEM &cur() const { return *_cur; } \
    boolean finished() const { return _cur == NULL; } \
    ELEM &advance(); \
    boolean remove(); \
    ITER &operator=(ITER const &i) { assign(i); return *this; } \
    ITER &operator=(TABLE &t) { assign(t); return *this; } \
    boolean operator==(ITER const &i) const { return compare(i); } \
    boolean operator!=(ITER const &i) const { return !compare(i); } \
    operator boolean() { return !finished(); } \
    ELEM &operator()() const { return cur(); } \
    ELEM &operator*() const { return cur(); } \
    ELEM *operator->() const { return &cur(); } \
    ELEM &operator++() { return advance(); } \
    ELEM &operator++(int) { return advance(); } \
    long chain() { return _hv; } \
}; \



#define implement_hashtable(TABLE, ITER, ELEM, KEY, HASHFUNC) \
 \
void \
TABLE::bump(long chains) \
{ \
    if (_iters != NULL) \
    	return; \
 \
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
    __elemtype(TABLE) **nchains = new __elemtype(TABLE) *[chains]; \
    long i; \
 \
    for (i = 0; i < chains; i++) \
    	nchains[i] = NULL; \
 \
    __elemtype(TABLE) *list = NULL; \
    __elemtype(TABLE) *last = NULL; \
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
    	unsigned long hv = HASHFUNC((KEY)*list) % chains; \
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
TABLE::TABLE() \
{ \
    _chains = NULL; \
    _numchains = 0; \
    _size = 0; \
    _cur = NULL; \
    _iters = NULL; \
    bump(); \
} \
 \
TABLE::TABLE(TABLE const &t) \
{ \
    _chains = NULL; \
    _numchains = 0; \
    _size = 0; \
    _cur = NULL; \
    _iters = NULL; \
    bump(); \
    insert(t); \
} \
 \
TABLE::~TABLE() \
{ \
    clear(); \
    delete[/*_numchains*/] _chains; \
} \
 \
__elemtype(TABLE) * \
TABLE::lookup(KEY const &k) const \
{ \
    unsigned long hv = HASHFUNC(k) % _numchains; \
 \
    for (__elemtype(TABLE) *p = _chains[hv]; p != NULL; p = p->_link) \
    	if (*p == k) \
	    return p; \
 \
    return NULL; \
} \
 \
boolean \
TABLE::insert(KEY const &k) \
{ \
    unsigned long hv = HASHFUNC(k) % _numchains; \
    __elemtype(TABLE) *p; \
 \
    for (p = _chains[hv]; p != NULL; p = p->_link) \
    	if (*p == k) \
	{ \
	    _cur = NULL; \
	    return FALSE; \
	} \
 \
    p = new __elemtype(TABLE)(k); \
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
TABLE::remove(KEY const &k) \
{ \
    unsigned long hv = HASHFUNC(k) % _numchains; \
 \
    for (__elemtype(TABLE) *p = _chains[hv], *q = NULL; p != NULL; p = p->_link) \
    	if (*p == k) \
	{ \
	    if (_cur == p) \
		_cur = NULL; \
 \
	    for (ITER *_iter = _iters; _iter != NULL; _iter = _iter->_n) \
	    	if (_iter->_cur == p) \
		    _iter->advance(); \
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
TABLE::clear() \
{ \
    for (ITER *_iter = _iters; _iter != NULL; _iter = _iter->_n) \
    { \
    	_iter->_cur = NULL; \
	_iter->_hv = -1; \
    } \
 \
    for (long i = 0; i < _numchains; i++) \
    { \
    	for (__elemtype(TABLE) *p = _chains[i], *q = NULL; p != NULL; p = q) \
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
TABLE::assign(TABLE const &t) \
{ \
    clear(); \
    insert(t); \
} \
 \
boolean \
TABLE::compare(TABLE const &t) const \
{ \
    if (_size != t._size) \
	return FALSE; \
 \
    for (long i = 0; i < _numchains; i++) \
    	for (__elemtype(TABLE) *p = _chains[i]; p != NULL; p = p->_link) \
	{ \
	    ELEM *e = t.lookup((KEY)*p); \
 \
	    if (e == NULL || !(*e == *p)) \
		return FALSE; \
	} \
 \
    return TRUE; \
} \
 \
void \
TABLE::insert(TABLE const &t) \
{ \
    for (long i = 0; i < t._numchains; i++) \
    	for (__elemtype(TABLE) *p = t._chains[i]; p != NULL; p = p->_link) \
	    if (insert((KEY)*p)) \
		*_cur = *p; \
} \
 \
void \
TABLE::remove(TABLE const &t) \
{ \
    for (long i = 0; i < t._numchains; i++) \
	for (__elemtype(TABLE) *p = t._chains[i]; p != NULL; p = p->_link) \
	    remove((KEY)*p); \
} \
 \
void \
TABLE::intersect(TABLE const &t) \
{ \
    for (long i = 0; i < _numchains; i++) \
    	for (__elemtype(TABLE) *p = _chains[i]; p != NULL; p = p->_link) \
	    if (t.lookup((KEY)*p) == NULL) \
		remove((KEY)*p); \
} \
 \
ELEM & \
TABLE::elt(KEY const &k) \
{ \
    if (!find(k)) \
		insert(k); \
 \
    return cur(); \
} \
 \
void \
ITER::attach(TABLE &t) \
{ \
    _t = &t; \
    _n = _t->_iters; \
    _p = NULL; \
    if (_n != NULL) \
	_n->_p = this; \
    _t->_iters = this; \
 \
    for (_hv = 0; _hv < _t->_numchains; _hv++) \
    	if (_t->_chains[_hv] != NULL) \
	    break; \
 \
    if (_hv < _t->_numchains) \
    	_cur = _t->_chains[_hv]; \
    else \
	_cur = NULL; \
} \
 \
void \
ITER::detach() \
{ \
    if (_t != NULL) \
    { \
    	if (_p == NULL) \
	{ \
	    _t->_iters = _n; \
 \
	    if (_n) \
		_n->_p = NULL; \
	} \
	else \
	{ \
	    _p->_n = _n; \
 \
	    if (_n) \
		_n->_p = NULL; \
	} \
    } \
} \
 \
ITER::ITER() \
{ \
    _t = NULL; \
    _n = NULL; \
    _p = NULL; \
    _hv = 0; \
    _cur = NULL; \
} \
 \
ITER::ITER(TABLE &t) \
{ \
    attach(t); \
} \
 \
ITER::ITER(ITER const &i) \
{ \
    attach(*i._t); \
    _hv = i._hv; \
    _cur = i._cur; \
} \
 \
ITER::~ITER() \
{ \
    detach(); \
} \
 \
void \
ITER::assign(ITER const &i) \
{ \
    detach(); \
    attach(*i._t); \
    _hv = i._hv; \
    _cur = i._cur; \
} \
 \
void \
ITER::assign(TABLE &t) \
{ \
    detach(); \
    attach(t); \
} \
 \
boolean \
ITER::compare(ITER const &i) const \
{ \
    if (_t == i._t && _hv == i._hv && _cur == i._cur) \
	return TRUE; \
 \
    return FALSE; \
} \
 \
ELEM & \
ITER::advance() \
{ \
    __elemtype(TABLE) *cur = _cur; \
 \
    if (_cur != NULL && _cur->_link != NULL) \
	_cur = _cur->_link; \
    else \
    { \
    	if (_cur == NULL) \
	    _hv = -1; \
 \
	for (_hv++; _hv < _t->_numchains; _hv++) \
	    if (_t->_chains[_hv] != NULL) \
	    	break; \
 \
    	if (_hv < _t->_numchains) \
	    _cur = _t->_chains[_hv]; \
	else \
	    _cur = NULL; \
    } \
 \
    return *cur; \
} \
 \
boolean \
ITER::remove() \
{ \
    __elemtype(TABLE) *prev = _t->_chains[_hv]; \
    __elemtype(TABLE) *cur = _cur; \
 \
    if (cur == NULL) \
	return FALSE; \
 \
    if (prev == cur) \
    	_t->_chains[_hv] = cur->_link; \
    else \
    { \
	while (prev != NULL && prev->_link != NULL) \
	    if (prev->_link == cur) \
		break; \
 \
	if (prev == NULL) \
	    return FALSE; \
 \
	prev->_link = cur->_link; \
    } \
 \
    for (ITER *_iter = _t->_iters; _iter != NULL; _iter = _iter->_n) \
	if (_iter->_cur == cur) \
	    _iter->advance(); \
 \
    delete cur; \
    return TRUE; \
} \



#endif
