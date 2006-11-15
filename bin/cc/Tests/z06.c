struct GlobalAttrib
{
    char const *name;
    unsigned long mask;
    unsigned long value;
};

char const foo_str[] = "foo";

struct GlobalAttrib attrib = { "test", 1, 2 };

extern struct GlobalAttrib const globalattribs[];

struct GlobalAttrib const globalattribs[] =
{
    { foo_str, 1, 1 },
    { &foo_str[1], 2, 2 },
    { foo_str + 2, 3, 3 },
};
