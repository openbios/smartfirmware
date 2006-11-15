union foo
{
    int a;
    float f;
};

typedef union foo foo;

void
func(foo *f)
{
    foo ff = *f;
}
