
struct foo
{
    int a;
    int b;
};

void
main()
{
    struct foo c;
    int d = c.a;
#pragma unused(d)
    int e = (&c)->b;
}
