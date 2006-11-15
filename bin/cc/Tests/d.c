
struct foo
{
    int a;
    int b;
};

struct foo bar = { 1, 2 };
int x;

void
baz()
{
    static struct foo goo = { 3, 4 };
    struct foo zoo = { 5, 6 };
    x = zoo.b;
    x += goo.a;
}
