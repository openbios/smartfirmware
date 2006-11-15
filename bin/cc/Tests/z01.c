#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <a.out.h>

#ifndef FILHDR
#define FILHDR struct exec
#endif

enum Bool;

enum Bool
{
    False = 0,
    True,
};

enum Bool bozo;

typedef int boolean;
const boolean FALSE = 0;
const boolean TRUE = 1;

extern const TRUE;

struct {
    int a;
    boolean b : 2;
};

typedef int* foo;
void func(int *arg, ...);

void func(int *arg, ...)
{
    arg = 0;
}

void func2(foo arg)
{
    arg = 0;
}

main()
{
    signed short int c = FALSE;
    foo x;
    int *y;
    FILHDR filhdr;

    c = "foo"[2];
    c = 2["foo"];

    func(x);
    func2(y);

    return sizeof filhdr;
}
