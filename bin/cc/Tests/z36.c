typedef struct token {
	unsigned char	type;
	unsigned char 	flag;
	unsigned short	hideset;
	unsigned int	wslen;
	unsigned int	len;
	unsigned char	*t;
} Token;

void func(register Token *tp)
{
	int nmac;

	nmac = tp->t[0];
}
