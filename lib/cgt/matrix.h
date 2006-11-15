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



#if defined(MATRIX) || !defined(__MATRIX_H_)

#ifndef __MATRIX_H_
#define __MATRIX_H_
#endif

#ifndef __STDLIBX_H_
#include <stdlib.h>
#include <stdlibx.h>
#endif






#define __matrixrow(TABLE)  name2(__matrix_row, TABLE)

#define declare_matrix(MATRIX, TYPE, BUMP, COLBUMP) \
 \
class MATRIX; \
 \
class __matrixrow(MATRIX) \
{ \
    MATRIX const *_matrix; \
    int _row; \
public: \
    __matrixrow(MATRIX)() { _matrix = NULL; _row = 0; } \
    __matrixrow(MATRIX)(MATRIX const &m, int row) { _matrix = &m, _row = row; } \
    MATRIX const &matrix() const { return *_matrix; } \
    int row() const { return _row; } \
    TYPE &col(int c); \
    TYPE &operator[](int c) { return col(c); } \
}; \
 \
class MATRIX \
{ \
    TYPE *_matrix; \
    int _matlen; \
    int _rowmult; \
    int _rows; \
    int _cols; \
    void expand(int rows, int cols); \
public: \
    MATRIX(); \
    MATRIX(int rows, int cols); \
    MATRIX(MATRIX const &); \
    ~MATRIX(); \
    void assign(MATRIX const &); \
    boolean compare(MATRIX const &) const; \
    int rows() const { return _rows; } \
    int cols() const { return _cols; } \
    void reset(int newrows = 0, int newcols = 0); \
    __matrixrow(MATRIX) row(int r) \
    { \
	if (r < 0) \
	    return __matrixrow(MATRIX)(); \
 \
	if (r >= _rows) \
	    expand(r + 1, _cols); \
 \
	return __matrixrow(MATRIX)(*this, r); \
    } \
    __matrixrow(MATRIX) getrow(int r) const \
	    { return __matrixrow(MATRIX)(*this, r); } \
    TYPE &elem(int r, int c) \
    { \
    	if (r < 0 || c < 0) \
	    return *(TYPE *)NULL; \
 \
	if (row >= _rows || col >= _cols) \
	    expand(row + 1, col + 1); \
 \
	return _matrix[_rowmult * r + c]; \
    } \
    TYPE &elt(int r, int c) const { return _matrix[_rowmult * r + c]; } \
    TYPE *getarr() const { return _matrix; } \
    MATRIX &operator=(MATRIX const &m) { assign(m); return *this; } \
    boolean operator==(MATRIX const &m) const { return compare(m); } \
    boolean operator!=(MATRIX const &m) const { return !compare(m); } \
    __matrixrow(MATRIX) operator[](int r) { return row(r); } \
    TYPE *operator()() const { return getarr(); } \
}; \
 \






#define implement_matrix(MATRIX, TYPE, BUMP, COLBUMP) \
 \
TYPE & \
__matrixrow(MATRIX)::col(int c) \
{ \
    if (_matrix == NULL) \
	return *(TYPE *)NULL; \
 \
    return _matrix->elem(_row, c); \
} \
 \
void \
MATRIX::expand(int rows, int cols) \
{ \
    if (rows >= _rows || cols >= _cols) \
    { \
    	int nrowmult = _rowmult; \
 \
    	if (cols > _rowmult) \
	{ \
	    nrowmult = _rowmult > 0 ? _rowmult : 1; \
 \
	    while (nrowmult < cols) \
	    	if (nrowmult < COLBUMP) \
		    nrowmult <<= 1; \
		else \
		    nrowmult += COLBUMP; \
	} \
 \
	int size = nrowmult * _rows; \
 \
	if (size > _matlen || nrowmult != _rowmult) \
	{ \
	    int nlen = _matlen > 0 ? _matlen : 1; \
 \
	    while (nlen < size) \
		if (nlen < BUMP) \
		    nlen <<= 1; \
		else \
		    nlen += BUMP; \
 \
	    TYPE *nmatrix = new TYPE[nlen]; \
 \
	    if (_matrix != NULL) \
	    { \
		for (int r = 0; r < _rows; r++) \
		    for (int c = 0; c < _cols; c++) \
			nmatrix[r * nrowmult + c] = _matrix[r * _rowmult + c]; \
 \
		delete[/*_matlen*/] _matrix; \
	    } \
 \
	    _matrix = nmatrix; \
	    _matlen = nlen; \
	    _rowmult = nrowmult; \
	} \
 \
	_rows = rows; \
	_cols = cols; \
    } \
} \
 \
MATRIX::MATRIX() \
{ \
    _matrix = NULL; \
    _matlen = 0; \
    _rowmult = 0; \
    _rows = 0; \
    _cols = 0; \
} \
 \
MATRIX::MATRIX(int rows, int cols) \
{ \
    _matrix = NULL; \
    _matlen = 0; \
    _rowmult = 0; \
    _rows = 0; \
    _cols = 0; \
    expand(rows, cols); \
} \
 \
MATRIX::MATRIX(MATRIX const &m) \
{ \
    _matrix = NULL; \
    _matlen = 0; \
    _rowmult = 0; \
    _rows = 0; \
    _cols = 0; \
    assign(m); \
} \
 \
MATRIX::~MATRIX() \
{ \
    if (_matrix) \
	delete[/*_matlen*/] _matrix; \
} \
 \
void \
MATRIX::assign(MATRIX const &m) \
{ \
    int rows = m._rows; \
    int cols = m._cols; \
    reset(rows, cols); \
 \
    for (int r = 0; r < rows; r++) \
	for (int c = 0; c < cols; c++) \
	    _matrix[r * _rowmult + c] = m._matrix[r * m._rowmult + c]; \
 \
    return *this; \
} \
 \
boolean \
MATRIX::compare(MATRIX const &m) const \
{ \
    if (_rows != m._rows || _cols != m._cols) \
	return FALSE; \
 \
    for (int r = 0; r < rows; r++) \
	for (int c = 0; c < cols; c++) \
	    if (!(_matrix[r * _rowmult + c] == m._matrix[r * m._rowmult + c])) \
	    	return FALSE; \
 \
    return TRUE; \
} \
 \
void \
MATRIX::reset(int rows, int cols) \
{ \
    int nrows = rows > 0 ? rows : 0; \
    int ncols = cols > 0 ? cols : 0; \
 \
    if (nrows > 0 || ncols > 0) \
	expand(nrows, ncols); \
    else \
    { \
    	if (_matrix != NULL) \
	    delete[/*_matlen*/] _matrix; \
 \
	_matrix = NULL; \
	_matlen = 0; \
	_rowmult = 0; \
    } \
 \
    _rows = nrows; \
    _cols = ncols; \
} \
 \











#endif
