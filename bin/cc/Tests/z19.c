void* malloc(long);

#define NEWCELL(_into,_type) { _into = (char*)malloc(sizeof _type); }

char*
newcell(long t)
{
    char *z;
    NEWCELL(z,t);
    return z;
}
