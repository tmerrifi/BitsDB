
#ifndef COLLECTION_H
#define COLLECTION_H

#include <stdlib.h>

typedef struct Collection;

struct CollectionVTable{
	void (*closeCollection)(struct Collection * collection);
};

/*DESCRIPTION: The structure representing the base class of collections. Every child class
that will have all of these fields in addition to their own*/
typedef struct{
	int fd;						//the file descriptor
	char * segmentName;			//the name of the shared mem segment
	void * mem;					//where the objects are stored
	int sizeOfMapping;			//The current size (in bytes) of the mapping
	struct CollectionVTable vtable;
}Collection;

/*DESCRIPTION: The base class for the header that is stored in the first few bytes of each memory area*/
typedef struct{
	int firstFree;				//this points to the first free object. It has to be an int (in bytes) so that it is position independent
	int lastFree;
}CollectionHdr;

/*DESCRIPTION: The base class for the objects stored in the collection.*/
typedef struct{
	int nextFree;			//if this object is in the free list, then this points to the next free object				
}CollectionObject;


#define collection_getFirstFree(collection) ((CollectionHdr *)(collection->mem))->firstFree

#define collection_getLastFree(collection) ((CollectionHdr *)(collection->mem))->lastFree
/*
DESCRIPTION: Initialize the collection object, this is an abstract class. Open the memory mapped segment and setup all the members
ARGUMENTS: 	collection - The collection object...instantiated in one of the child classes
			segmentName - The name of the shared memory segment we need to open
			initAddress - the initial address where we (hope) the OS maps the collection
			sizeOfType - The size of the type we are storing
			slabIncrementSize - when we run out of space, how many objects worth should we increase it?
*/
void collection_init(Collection * collection, char * segmentName, void * initAddress, int defaultSize, int callingProcess);

void * collection_resize(Collection * collection, int newSize);

void collection_close(Collection * collection);

void collection_remove(Collection * collection, CollectionObject * object);

void collection_delete(Collection * collection);

void collection_deleteSegment(char * segmentName);

int isValid(Collection * collection, CollectionObject * object);

void * collection_findFree(Collection * collection);

#endif