struct syment
{
	union
	{
		char		_n_name[8];	/* old COFF version */
		struct
		{
			long	_n_zeroes;	/* new == 0 */
			long	_n_offset;	/* offset into string table */
		} _n_n;
		char		*_n_nptr[2];	/* allows for overlaying */
	} _n;
	long			n_value;	/* value of symbol */
	short			n_scnum;	/* section number */
	unsigned short		n_type;		/* type and derived type */
	char			n_sclass;	/* storage class */
	char			n_numaux;	/* number of aux. entries */
};

typedef int size_t;
#define offsetof(s, m) ((size_t) &(((s *) 0)->m))

void
func()
{
    struct syment sym;
    static size_t x = offsetof(struct syment, _n._n_n._n_offset);
    char *y;

    x = sym._n._n_n._n_zeroes;
    y = sym._n._n_nptr[3];
}
