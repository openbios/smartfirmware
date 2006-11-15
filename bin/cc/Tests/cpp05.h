#define Foo -bar

f()
{
  int bar;
  -Foo; /* equivalent to "- -bar", not "--bar" */
}
