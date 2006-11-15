void
func()
{
    int arr[12];
    char *s;

    s = (char*)&arr;
    s = (char*)&arr[12];
}
