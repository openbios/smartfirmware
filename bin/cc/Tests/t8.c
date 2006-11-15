
int a;
int b;

void
foo()
{
    switch (a)
    {
    case 1:
    	b = 5;
	break;
    case 2:
    	b = 4;
	break;
    case 3:
    	b = 3;
	break;
    case 4:
    	b = 2;
	break;
    case 5:
    	b = 1;
	break;
    default:
    	b = 0;
	break;
    }
}
