
extern void fn(float const a[][3]);

float const v[][3] = { { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 } };

int
fn2()
{
    fn(v);
}
