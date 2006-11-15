typedef struct Type
{
    int a;
} Type;

void add(Type const f);

void
func(Type const f)
{
    add(f);
}
