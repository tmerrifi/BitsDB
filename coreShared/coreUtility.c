
#include <stdlib.h>
#include <string.h>
#include "coreUtility.h"

char * coreUtil_concat(char * graphName, char * objectName)
{
	char * combinedName=malloc(strlen(graphName)+strlen(objectName) + 1);
	strncpy(combinedName,graphName,strlen(graphName));
	strncat(combinedName,"_",1);
	strncat(combinedName,objectName,strlen(objectName));
	return combinedName;
}