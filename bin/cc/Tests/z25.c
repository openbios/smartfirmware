struct Foo
{
    int a;
};

typedef struct Foo Type;

void
func(const Type arg)
{
    arg.a = 0;
}

void
test(Type val)
{
    func(val);
}
