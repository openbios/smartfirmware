
int
foo(int i)
{
    switch (i)
    {
    case 0:
    	return 0;
    case 1:
    	return 1;
    case 2:
    	return 2;
    case 5:
    	return 3;
    case 10:
    	return 4;
    case 20:
    	return 5;
    case 50:
    	return 6;
    case 100:
    	return 7;
    case 200:
    	return 8;
    case 500:
    	return 9;
    case 1000:
    	return 10;
    case 2000:
    	return 11;
    case 5000:
    	return 12;
    }

    return -1;
}
