
struct foo
{
    int a;
    int b;
};

struct bar
{
    struct foo x;
    struct foo y;
};

extern struct bar s1, s2;

void baz()
{
    s1.x.a = s2.y.b;
}
