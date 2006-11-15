
struct foo
{
    int a;
    int b;
};

struct foo s1;
struct foo s2;

void
bar()
{
    struct foo s3 = { 5, 7 };
    s2.b = (s1  = s3).a;
    s1.a = s2.b;
}
