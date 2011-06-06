
#ifndef CORE_UTILITY_H

#define CORE_UTILITY_H

#include <sys/mman.h>	//for memory mapping


//DESCRIPTION: we need to concat the name of the graph with "vertices". This is how we will get the file
//or the shared memory region where the vertices are stored.
//ARGS: graphName: The name of the graph
size_t coreUtil_getFileSize(int fd);

void * coreUtil_openSharedMemory(char * segmentName, void * address, int defaultSize, int callingProcess);

#define SHM_CLIENT 0		//the client can only read
#define SHM_CORE 1			//the core can read/write/create

#endif
