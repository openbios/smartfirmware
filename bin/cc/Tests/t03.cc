class Foo
{
public:
    int val;
};

void
func(Foo &arg)
{
    Foo *var = &arg;

    if (var)
	var->val = 22;
}
