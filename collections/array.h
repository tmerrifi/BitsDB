
#ifndef ARRAY_H
#define ARRAY_H

#include "collection.h"

typedef struct{
	Collection base;
	int slabIncrementSize;		//we increment the collection by this much when we run out of space
	int sizeOfType;				//size of the type we are storing
	void * nextFreeSlot;		//If the free list is empty, where to next?
}Array;

/*This structure appears as the first few bytes in the shared memory segment and in the file*/
typedef struct{
	CollectionHdr base;
	int currentObjectsAllocated;
}ArrayHdr;

Array * array_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize);

void * array_addObject(Array * array, void * object);

int array_removeObject(Array * array, void * object);

void array_close(Array * array);

void * array_getById(Array * array, int id);

void array_delete(Array * array);

void array_deleteSegment(char * segmentName);

#endif