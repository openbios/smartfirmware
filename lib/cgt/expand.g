
//
//	Copyright (C) 1991  Thomas J. Merritt
//

#ifndef __EXPAND_H_
#define __EXPAND_H_

#include <stdlibx.h>

extern void _expand(void **buf, int *buflen, int len, int eltsz = 1,
	int bump = 32);

inline void
expand(void *buf, int *buflen, int len, int eltsz = 1, int bump = 32)
{
    if (len > *buflen)
	_expand((void**)buf, buflen, len, eltsz, bump);
}

extern char *bufcpy(char *&buf, int &buflen, char const *str);

//$begincomment
#ifndef IMPLEMENT
//$endcomment

//$begin declare_expand(TYPE)
inline void
expand(TYPE *&buf, int &buflen, int len, int bump = 32)
{
    if (len > buflen)
	_expand((void**)&buf, &buflen, len, sizeof (TYPE), bump);
}
//$end

//$begincomment
#else
//$endcomment

//$begin implement_expand(TYPE)
//$end

//$begincomment
#endif
#undef TYPE
//$endcomment

#endif // __EXPAND_H_
