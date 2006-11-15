int     cmp (a, b)
char *a;
char   *b;
{
    register char  *ta,
                   *tb;

    ta = (char*)a;
    tb = b;

    while (*ta) {
        if (*ta > *tb)
            return (1);
        if (*ta < *tb)
            return (-1);
        ++ta;
        ++tb;
    }
    if (*tb)
        return (1);
    else
        return (0);
};

int
eqin (str1, str2)
char   *str1;
char   *str2;
{
    register char  *s1; /* local pointer into first string */
    register char  *s2; /* local pointer into second string */

    s1 = str1;
    s2 = str2;
    while (*s1) {              /* compare no further than end of first
                                  string */
        if (*s2 == 0)          /* check that second string isn't too short 
                               */
            return (0);
        if (*s1++ != *s2++)
            return (0);
    }

    return (1);
};
