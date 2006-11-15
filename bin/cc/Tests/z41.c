void
func(void)
{
    char *s1 = "bozo";
    const char *s2 = "test";

    if (s1 == s2)
	return;

    if (s1 < s2)
	return;
}
