int func(int bozo);
//int mpw_inline(int f) = 0x34;

typedef struct S T;

#define bozo foo

const long x = 2;
int y = (int)&x + 2;
char *const *z;

struct S
{
    int a:3;
    char *b;
    union
    {
	float x;
	int y;
    };
    //int fun();
};

#if defined(__STDC__)
  typedef void (*sigfunc_t)(int);
#elif defined(BOZO)
  typedef void (*sigfunc_t)();
#else
  typedef int (*sigfunc_t)();
#endif

enum
{
    TIC_SORT = 1,
    CALL_SORT,
    ADDR_SORT,
    NAME_SORT,
} sort = TIC_SORT;

char *messages[] =
{
    "message1",
    [7] "error",
    "crash",
};

int
func(argc)
int argc;
{
    unsigned long a = 39;
    double b;
    char d[] = (char*)L"eatme";
    char *e;// = { &"foobar"[2] };
    T z = { sizeof (z), __FILE__ };

    while (b == z.x)
    {
	const int c[][2] = { 32, 11, { 12, 24, }, };
	//z = (T){13, "foo"};
	/*a = 'x';*/
	a = x;
	/*b = 3;*/
	b = /*q =*/ 12.2;
	asm(__FUNC__);
	if (!a)
	    break;
	switch (a)
	{
	case 32:
	    bozo(c[a][1]);
	    break;
	case (int)19.32:
	case 33 ... 50:
	    e = "v";
	    break;
	default:
	    continue;
	    break;
	}
    }

    return a + b;
}
