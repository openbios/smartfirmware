struct S
{
    int b1 : 2;
    int b2 : 3;
    //int : 0;
    char c[3];
    short s;
    int b3 : 4;
    //int i;
};

struct S foo = { 1, 2, };
int b = sizeof (struct S);

func(int c, float d)
{
    int a = 5;
    char z[] = "foo";
    double b;

    while (1)
    {
	static int q = 14;

	b += 2;
	goto out;
    }

out:
    return a = b += c * 7 + d;
}
