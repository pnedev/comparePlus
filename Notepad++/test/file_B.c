//----------------------------------------
//			C test file A
//----------------------------------------

#include "stdio.h"
#include "math.h"

int dummy(int a, int b);

int main(void)
{
	// An integer
	int i;
	
	// A char
	char c, d, e;
	
	// A long
	long l;
	
	int a = 11, b = 20;
	
	i = dummy(a, b);
	
	return(1);
}

int dummy(int a, int b)
{
	if (b > a)
		return (a + b);
	else
		return (a - b);
}