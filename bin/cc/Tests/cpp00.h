/* test1: */
#define	x	3
#define	f(a)	f(x * (a))
#undef	x
#define	x	2
#define	g	f
#define	z	z[0]
#define h	g(~
#define	m(a)	a(w)
#define	w	0,1
#define t(a)	a

f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);
g(x+(3,4)-w) | h 5) & m
	(f)^m(m);


// test2:
#define str(s)		# s
#define xstr(s)		str(s)
#define debug(s, t)	printf("x" # s "= %d, x" # t "= %s",  \
				x ## s, x ## t)
#define INCFILE(n)	vers ## n
#define glue(a, b)	a ## b
#define xglue(a, b)	glue(a, b)
#define HIGHLOW		"hello"
#define LOW		LOW ", world"

debug(1, 2);
fputs(str(strncmp("abc\0d", "abc", '\4') /\
* this goes away */ == 0) str(: @\n), s);
include xstr(INCFILE(2).hdr)
glue(HIGH, LOW)
xglue(HIGH, LOW)


// test\
3
#define OBJ_LIKE	(1-1)
#define OBJ_LIKE	/* white space */ (1-1) /* other */
#define OBJ_LIKE	(0)
#define FTN_LIKE(a)	( a )
#define FTN_LIKE( a )(	/* note the white space */ \
			a /* other stuff on this line
			  */ )
#define FTN_LIKE(a)	( b )



// regression test #1
#define tok1() part1
#define gluetoks(a, b)  a ## b
#define gluemacs(a, b)  gluetoks(a, b)
#define fulltok  gluemacs(tok1(), part2)

fulltok
fulltok



// regression test #2
#define one(arg)	func(arg + 1)
#define two(arg)	one(arg + 2)

one(two(77))
