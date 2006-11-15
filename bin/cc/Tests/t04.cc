class Foo
{
    int ival;

    void func();
};

class Bar : public Foo
{
    int bval;

    void test();
};

void
Bar::test()
{
    func();
}
