
#define MAKEINTFUNC(NAME, STORAGE, TYPE) \
TYPE \
NAME() \
{ \
    STORAGE TYPE a = 1; \
    STORAGE TYPE b = 2; \
    STORAGE TYPE c = 3; \
    STORAGE TYPE d = 4; \
    STORAGE int x = 5; \
 \
    c = -a; \
    c = ~a; \
    x = !a; \
    c = a + b; \
    c = a - b; \
    c = a * b; \
    c = a / b; \
    c = a % b; \
    c = a | b; \
    c = a ^ b; \
    c = a & b; \
    c = a << b; \
    c = a >> b; \
    x = a && b; \
    x = a || b; \
    x = a == b; \
    x = a != b; \
    x = a < b; \
    x = a <= b; \
    x = a > b; \
    x = a >= b; \
    c = (a, b); \
    c = a ? b : d; \
    c += b; \
    c -= b; \
    c *= b; \
    c /= b; \
    c %= b; \
    c |= b; \
    c ^= b; \
    c &= b; \
    c <<= b; \
    c >>= b; \
    return c; \
}

#define MAKEFPFUNC(NAME, STORAGE, TYPE) \
TYPE \
NAME() \
{ \
    STORAGE TYPE a = 1.0; \
    STORAGE TYPE b = 2.0; \
    STORAGE TYPE c = 3.0; \
    STORAGE TYPE d = 4.0; \
    STORAGE int x = 5; \
 \
    c = -a; \
    x = !a; \
    c = a + b; \
    c = a - b; \
    c = a * b; \
    c = a / b; \
    x = a && b; \
    x = a || b; \
    x = a == b; \
    x = a != b; \
    x = a < b; \
    x = a <= b; \
    x = a > b; \
    x = a >= b; \
    c = (a, b); \
    c = a ? b : d; \
    c += b; \
    c -= b; \
    c *= b; \
    c /= b; \
    return c; \
}

MAKEINTFUNC(dochar, static, char)
MAKEINTFUNC(doshort, static, short)
MAKEINTFUNC(doint, static, int)
MAKEINTFUNC(dolong, static, long)
MAKEINTFUNC(dolonglong, static, long long)

MAKEINTFUNC(docharlocal, auto, char)
MAKEINTFUNC(doshortlocal, auto, short)
MAKEINTFUNC(dointlocal, auto, int)
MAKEINTFUNC(dolonglocal, auto, long)
MAKEINTFUNC(dolonglonglocal, auto, long long)

MAKEINTFUNC(docharreg, register, char)
MAKEINTFUNC(doshortreg, register, short)
MAKEINTFUNC(dointreg, register, int)
MAKEINTFUNC(dolongreg, register, long)
MAKEINTFUNC(dolonglongreg, register, long long)

MAKEFPFUNC(dofloat, static, float)
MAKEFPFUNC(doshortdouble, static, short double)
MAKEFPFUNC(dodouble, static, double)
MAKEFPFUNC(dolongdouble, static, long double)

MAKEFPFUNC(dofloatlocal, auto, float)
MAKEFPFUNC(doshortdoublelocal, auto, short double)
MAKEFPFUNC(dodoublelocal, auto, double)
MAKEFPFUNC(dolongdoublelocal, auto, long double)

MAKEFPFUNC(dofloatreg, register, float)
MAKEFPFUNC(doshortdoublereg, register, short double)
MAKEFPFUNC(dodoublereg, register, double)
MAKEFPFUNC(dolongdoublereg, register, long double)

#if 0
int
dobitfield()
{
    struct {
    	int a : 4;
    	int b : 4;
    	int c : 4;
    	int d : 4;
    } s;
    static int x;

    s.c = -s.a;
    s.c = ~s.a;
    x = !s.a;
    s.c = s.a + s.b;
    s.c = s.a - s.b;
    s.c = s.a * s.b;
    s.c = s.a / s.b;
    s.c = s.a % s.b;
    s.c = s.a | s.b;
    s.c = s.a ^ s.b;
    s.c = s.a & s.b;
    s.c = s.a << s.b;
    s.c = s.a >> s.b;
    x = s.a && s.b;
    x = s.a || s.b;
    x = s.a == s.b;
    x = s.a != s.b;
    x = s.a < s.b;
    x = s.a <= s.b;
    x = s.a > s.b;
    x = s.a >= s.b;
    s.c = (s.a, s.b);
    s.c = s.a ? s.b : s.d;
    s.c += s.b;
    s.c -= s.b;
    s.c *= s.b;
    s.c /= s.b;
    s.c %= s.b;
    s.c |= s.b;
    s.c ^= s.b;
    s.c &= s.b;
    s.c <<= s.b;
    s.c >>= s.b;
    return s.c;
}
#endif
