#include "dberror.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *RC_OUTPUT;

/*  Output a message to standard output describing the error.*/
void 
printError (RC error)
{
	if (RC_OUTPUT != NULL)
		printf("EC (%i), \"%s\"\n", error, RC_OUTPUT);
	else
		printf("EC (%i)\n", error);
}

char *
errorMessage (RC error)
{
	char *message;

	if (RC_OUTPUT != NULL)
	{
		message = (char *) malloc(strlen(RC_OUTPUT) + 30);
		sprintf(message, "EC (%i), \"%s\"\n", error, RC_OUTPUT);
	}
	else
	{
		message = (char *) malloc(30);
		sprintf(message, "EC (%i)\n", error);
	}

	return message;
}
