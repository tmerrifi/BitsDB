
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

#define DEFAULT_ARR_INC_SIZE 5

Array * array_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize, int callingProcess);

void * array_addObject(Array * array, void * object);

void * array_getObjectFromIndex(Array * array, int index);

void * array_addObjectByIndex(Array * array, void * object, int index);

int array_removeObject(Array * array, void * object);

void array_close(Array * array);

void * array_getById(Array * array, int id);

int array_getIndex(Array * array, void * object);

void array_delete(Array * array);

void array_deleteSegment(char * segmentName);

void * array_getNextValidObjectFromIndex(Array * array, int * index, int keepGoingFlag);

#define ARRAY_FOR_EACH(arr,arr_obj)\
		int i = 1;\
  		arr_obj=array_getNextValidObjectFromIndex(arr,&i, 1);\
		for (; i <= ((ArrayHdr *)arr->base.mem)->currentObjectsAllocated; arr_obj=array_getNextValidObjectFromIndex(arr,&i, 1) )

#endif


