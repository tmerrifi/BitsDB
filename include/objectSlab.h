
#ifndef ObjectSlab_H
#define ObjectSlab_H

typedef struct{
	void * objects;					//slab of objects
	int sizeInObjects;				//how many objects can we hold?
	int sizeOfType;					//the size in bytes of this type
	int shm_fd;						//the shared memory file descriptor
	int slabIncrementSize;
}ObjectSlab;

typedef struct{						//this appears as the first few bytes in the memory mapped file
	int currentObjectsAllocated;
}ObjectSlabHeader;

/******MACRO FUNCTIONS FOR COMMONLY USED STUFF *********/

#define slab_totalBytes(objectCount,sizeOfType) ((objectCount * sizeOfType)+sizeof(ObjectSlabHeader))

#define slab_objectsAllocated(slab) (((ObjectSlabHeader *)(slab->objects))->currentObjectsAllocated)


/*****PUBLIC FUNCTIONS*************************/
ObjectSlab * initObjectSlab(char * segmentName, void * initAddress, int sizeOfType, int slabSize);

void closeObjectSlab(ObjectSlab * slab);

int slab_addObject(ObjectSlab * slab, void * newObject); 

#endif