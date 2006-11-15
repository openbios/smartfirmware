
typedef int size_t; 

#define offsetof(TYPE, MEMBER) \
     ((size_t)(&(((TYPE *)0)->MEMBER)))

struct foo
{
    int a;
    int b;
};

int c = offsetof(struct foo, b);
