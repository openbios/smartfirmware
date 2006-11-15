//
//	Copyright (C) 1992  Thomas J. Merritt
//

#if defined(MATRIX) || !defined(__MATRIX_H_)

#ifndef __MATRIX_H_
#define __MATRIX_H_
#endif

#ifndef __STDLIBX_H_
#include <stdlib.h>
#include <stdlibx.h>
#endif

//$begincomment
#ifdef MATRIX
#ifndef IMPLEMENT
//$endcomment

#define __matrixrow(TABLE)  name2(__matrix_row, TABLE)

//$begin declare_matrix(MATRIX, TYPE, BUMP, COLBUMP)

class MATRIX;

class __matrixrow(MATRIX)
{
    MATRIX const *_matrix;
    int _row;
public:
    __matrixrow(MATRIX)() { _matrix = NULL; _row = 0; }
    __matrixrow(MATRIX)(MATRIX const &m, int row) { _matrix = &m, _row = row; }
    MATRIX const &matrix() const { return *_matrix; }
    int row() const { return _row; }
    TYPE &col(int c);
    TYPE &operator[](int c) { return col(c); }
};

class MATRIX
{
    TYPE *_matrix;
    int _matlen;
    int _rowmult;
    int _rows;
    int _cols;
    void expand(int rows, int cols);
public:
    MATRIX();
    MATRIX(int rows, int cols);
    MATRIX(MATRIX const &);
    ~MATRIX();
    void assign(MATRIX const &);
    boolean compare(MATRIX const &) const;
    int rows() const { return _rows; }
    int cols() const { return _cols; }
    void reset(int newrows = 0, int newcols = 0);
    __matrixrow(MATRIX) row(int r)
    {
	if (r < 0)
	    return __matrixrow(MATRIX)();

	if (r >= _rows)
	    expand(r + 1, _cols);

	return __matrixrow(MATRIX)(*this, r);
    }
    __matrixrow(MATRIX) getrow(int r) const
	    { return __matrixrow(MATRIX)(*this, r); }
    TYPE &elem(int r, int c)
    {
    	if (r < 0 || c < 0)
	    return *(TYPE *)NULL;

	if (row >= _rows || col >= _cols)
	    expand(row + 1, col + 1);

	return _matrix[_rowmult * r + c];
    }
    TYPE &elt(int r, int c) const { return _matrix[_rowmult * r + c]; }
    TYPE *getarr() const { return _matrix; }
    MATRIX &operator=(MATRIX const &m) { assign(m); return *this; }
    boolean operator==(MATRIX const &m) const { return compare(m); }
    boolean operator!=(MATRIX const &m) const { return !compare(m); }
    __matrixrow(MATRIX) operator[](int r) { return row(r); }
    TYPE *operator()() const { return getarr(); }
};

//$end

//$begincomment
#else
//$endcomment

//$begin implement_matrix(MATRIX, TYPE, BUMP, COLBUMP)

TYPE &
__matrixrow(MATRIX)::col(int c)
{
    if (_matrix == NULL)
	return *(TYPE *)NULL;

    return _matrix->elem(_row, c);
}

void
MATRIX::expand(int rows, int cols)
{
    if (rows >= _rows || cols >= _cols)
    {
    	int nrowmult = _rowmult;

    	if (cols > _rowmult)
	{
	    nrowmult = _rowmult > 0 ? _rowmult : 1;

	    while (nrowmult < cols)
	    	if (nrowmult < COLBUMP)
		    nrowmult <<= 1;
		else
		    nrowmult += COLBUMP;
	}

	int size = nrowmult * _rows;
		
	if (size > _matlen || nrowmult != _rowmult)
	{
	    int nlen = _matlen > 0 ? _matlen : 1;

	    while (nlen < size)
		if (nlen < BUMP)
		    nlen <<= 1;
		else
		    nlen += BUMP;

	    TYPE *nmatrix = new TYPE[nlen];

	    if (_matrix != NULL)
	    {
		for (int r = 0; r < _rows; r++)
		    for (int c = 0; c < _cols; c++)
			nmatrix[r * nrowmult + c] = _matrix[r * _rowmult + c];

		delete[/*_matlen*/] _matrix;
	    }

	    _matrix = nmatrix;
	    _matlen = nlen;
	    _rowmult = nrowmult;
	}

	_rows = rows;
	_cols = cols;
    }
}

MATRIX::MATRIX()
{
    _matrix = NULL;
    _matlen = 0;
    _rowmult = 0;
    _rows = 0;
    _cols = 0;
}

MATRIX::MATRIX(int rows, int cols)
{
    _matrix = NULL;
    _matlen = 0;
    _rowmult = 0;
    _rows = 0;
    _cols = 0;
    expand(rows, cols);
}

MATRIX::MATRIX(MATRIX const &m)
{
    _matrix = NULL;
    _matlen = 0;
    _rowmult = 0;
    _rows = 0;
    _cols = 0;
    assign(m);
}

MATRIX::~MATRIX()
{
    if (_matrix)
	delete[/*_matlen*/] _matrix;
}

void
MATRIX::assign(MATRIX const &m)
{
    int rows = m._rows;
    int cols = m._cols;
    reset(rows, cols);

    for (int r = 0; r < rows; r++)
	for (int c = 0; c < cols; c++)
	    _matrix[r * _rowmult + c] = m._matrix[r * m._rowmult + c];

    return *this;
}

boolean
MATRIX::compare(MATRIX const &m) const
{
    if (_rows != m._rows || _cols != m._cols)
	return FALSE;

    for (int r = 0; r < rows; r++)
	for (int c = 0; c < cols; c++)
	    if (!(_matrix[r * _rowmult + c] == m._matrix[r * m._rowmult + c]))
	    	return FALSE;

    return TRUE;
}

void
MATRIX::reset(int rows, int cols)
{
    int nrows = rows > 0 ? rows : 0;
    int ncols = cols > 0 ? cols : 0;

    if (nrows > 0 || ncols > 0)
	expand(nrows, ncols);
    else
    {
    	if (_matrix != NULL)
	    delete[/*_matlen*/] _matrix;

	_matrix = NULL;
	_matlen = 0;
	_rowmult = 0;
    }

    _rows = nrows;
    _cols = ncols;
}

//$end

//$begincomment
#endif
#undef MATRIX
#undef TYPE
#undef BUMP
#undef COLBUMP
#endif
//$endcomment

#endif // __MATRIX_H_
