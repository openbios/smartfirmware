#include <sys/cdefs.h>
#include <stdio.h>

class Foo
{
    int a;

public:
    Foo() { a = 0; }
    Foo(int x) { a = x; }
    Foo(int x, int y) { a = x + y; }
    int operator++() { return a++; }
};

//Foo g(21);

int
func()
{
    Foo f(21);

    return ++f;
}
