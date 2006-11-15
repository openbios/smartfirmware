

struct foo
{
    int a;
    int b;
};

struct foo s1 = { 5, 7 };
struct foo s2;

void
bar()
{
    struct foo s3;
    s2.b = (s3  = s1).a;
    s1.a = s2.b;
}
