
struct foo
{
    char a;			// test padding
    short b;
    int c;
    long d;
    long long e;
    int f : 8;			// test bit fields
    unsigned int g : 1;
    int h : 6;
    int : 5;
    int j : 12;
    long long k;		// test packing
    long l;
    int m;
    short n;
    char o;
};

struct foo fooinit =
{
    129, 32769, 2147483649, 2147483649L, 9223372036854775809LL,
    129, 1, 33, 2049, 
    9223372036854775809LL, 2147483649L, 2147483649, 32769, 129, 
};

void
bar()
{
    int i;
    long l;
    long long ll;
    i = fooinit.a;
    i = fooinit.b;
    i = fooinit.c;
    l = fooinit.d;
    ll = fooinit.e;
    i = fooinit.f;
    i = fooinit.g;
    i = fooinit.h;
    i = fooinit.j;
    ll = fooinit.k;
    l = fooinit.l;
    i = fooinit.m;
    i = fooinit.n;
    i = fooinit.o;
}
