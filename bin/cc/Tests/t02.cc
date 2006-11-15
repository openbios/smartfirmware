class Foo
{
public:
    int val;

    Foo(int v) { val = v; }
};

void
func(int arg)
{
    Foo *a;

    a = new Foo(arg);
}
