#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	
	char *str; 
	
	printf("Put command: \n") ;
	scanf("%s", str);
	
	system("bash -c 'echo %s > /proc/dogdoor'", str);

	exit(0);
}	

