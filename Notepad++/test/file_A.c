//----------------------------------------
//
//			C test file A
//
//----------------------------------------

#include "stdio.h"

int dummy(int a, int b)
{
	if (a > b)
		return (a + b);
	else
		return (a - b);
}

int main(void)
{
	// An integer
	int i;
	
	// A char
	char c;
	
		// A long
		long l;
	
	int a = 10, b = 20;
	
	i = dummy(a, b);
	
	return(0);
}
