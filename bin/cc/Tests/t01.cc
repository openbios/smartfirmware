class Foo
{
public:
    int val;

    void memfunc(int a) { val = a; }
};

void
func()
{
    Foo a;

    a.memfunc(122);
}
