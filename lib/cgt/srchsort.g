
//
//	Copyright (C) 1991  Thomas J. Merritt
//

#ifndef __SRCHSORT_H_
#define __SRCHSORT_H_

//$begincomment
#ifndef IMPLEMENT
//$endcomment

#include <stdlib.h>

extern void merge(void *dest, int elsz, void const *src1, int len1,
	void const *src2, int len2, int (*cmpfunc)(void const *, void const *));

//$begin declare_searchsort(TYPE)

inline void
qsort(TYPE *arr, size_t len, int (*fcmp)(TYPE const *, TYPE const *))
{
    if (len > 2)
	qsort((void *)arr, len, sizeof (TYPE),
		(int (*)(const void *, const void *))fcmp);
    else if (len == 2 && (*fcmp)(&arr[0], &arr[1]) > 0)
    {
    	TYPE x = arr[0];
	arr[0] = arr[1];
	arr[1] = x;
    }
}

int
bsearch(TYPE const &key, TYPE const *arr, size_t len,
	int (*fcmp)(TYPE const *, TYPE const *));

inline void
merge(TYPE *dest, TYPE const *src1, size_t len1, TYPE const *src2, size_t len2,
	int (*cmpfunc)(TYPE const *, TYPE const *))
{
    merge((void *)dest, sizeof (TYPE), (void const *)src1, len1,
	    (void const *)src2, len2,
	    (int (*)(void const *, void const *))cmpfunc);
}

//$end

//$begincomment
#else
//$endcomment

//$begin implement_searchsort(TYPE)

int
bsearch(TYPE const &key, TYPE const *arr, size_t len,
	int (*fcmp)(TYPE const *, TYPE const *))
{
    if (len == 0)
	return -1;

    TYPE const *el = (TYPE const *)bsearch((void const *)&key,
	    (void const *)arr, len, sizeof (TYPE),
	    (int (*)(void const *, void const *))fcmp);

    if (el == NULL)
	return -1;

    return el - arr;
}

//$end

//$begincomment
#endif
#undef TYPE
//$endcomment

#endif // __SRCHSORT_H_

