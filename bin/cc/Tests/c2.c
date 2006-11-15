
extern void fn(char const ***a);
char ***b;

void
fn2()
{
    fn(b);
}
