extern int arr[];

int *gp = &arr[1];

void
func(int *p = &arr[1])
{
    p = 0;
}
