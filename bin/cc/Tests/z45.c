struct instance
{
    int val;
    struct iself self;
};
typedef struct instance instance;

struct environ
{
    int instance;
    struct eself self;
};
