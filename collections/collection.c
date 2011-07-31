

#include "collection.h"
#include "graphKernel/coreUtility.h"

/*
DESCRIPTION: Initialize the collection object, this is an abstract class. Open the memory mapped segment and setup all the members
ARGUMENTS: 	collection - The collection object...instantiated in one of the child classes
			segmentName - The name of the shared memory segment we need to open
			initAddress - the initial address where we (hope) the OS maps the collection
			defaultSize - What to ftruncate the file to if we are creating it
*/
void collection_init(Collection * collection, char * segmentName, void * initAddress, int defaultSize, int callingProcess){
	collection->segmentName=segmentName;
	collection->mem=coreUtil_openSharedMemory(segmentName, initAddress, defaultSize, callingProcess, &(collection->fd));
	collection->sizeOfMapping=coreUtil_getFileSize(collection->fd);
	collection->vtable.closeCollection=collection_close;
}

void collection_close(Collection * collection){
	coreUtil_closeSharedMemory(collection->mem, collection->sizeOfMapping, collection->fd);	
}

void * collection_resize(Collection * collection, int newSize){
	collection->mem = coreUtil_resizeSharedMemory(collection->mem, collection->sizeOfMapping, newSize, collection->fd);	
	if (collection->mem != -1){
		collection->sizeOfMapping=newSize;
	}
	return collection->mem;
}

void collection_remove(Collection * collection, CollectionObject * object){
	if (isValid(collection,object) ){		//just making sure this thing isn't already free
		int byteOffsetOfNewFreedObject = (void *)object - collection->mem;
		if (collection_getFirstFree(collection) == NULL){			//is the free list empty?
			collection_getFirstFree(collection) = collection_getLastFree(collection) = byteOffsetOfNewFreedObject;
		}
		else{
			((CollectionObject *)(collection->mem + collection_getLastFree(collection)))->nextFree = byteOffsetOfNewFreedObject;
			collection_getLastFree(collection) = byteOffsetOfNewFreedObject;
		}
	}
}
/*DESC: Is this object currently in use? If so 1 is returned...if it has been freed then we return 0*/
int isValid(Collection * collection, CollectionObject * object){
	return (object->nextFree == NULL && (collection->mem + collection_getFirstFree(collection)) != object );
}

void * collection_findFree(Collection * collection ){
	int firstFree = collection_getFirstFree(collection);
	void * returnVal = NULL;
	if (firstFree > 0){			//do we have something in the free list?
			returnVal=collection->mem + firstFree;		//setup the return ptr
			if (firstFree == collection_getLastFree(collection)){	//there is only one object in the free list
				collection_getFirstFree(collection) = collection_getLastFree(collection) = 0;
			}
			else{
				collection_getFirstFree(collection) = ((CollectionObject *)(collection->mem + firstFree))->nextFree;
			}	
			((CollectionObject *)returnVal)->nextFree=0;		//set the next free to be emtpy
	}
	return returnVal;				//return the ptr 
}

void collection_delete(Collection * collection){
	coreUtil_closeSharedMemory(collection->mem, collection->sizeOfMapping, collection->fd);		//make sure to unmap and close the file first
	coreUtil_removeSharedMemory(collection->segmentName);
}

void collection_deleteSegment(char * segmentName){
	coreUtil_removeSharedMemory(segmentName);
}

int collection_getSizeInBytes(Collection * collection){
	return coreUtil_getFileSize(collection->fd);	
}

