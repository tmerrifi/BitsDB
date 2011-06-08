
#include <stdio.h>
#include <stdlib.h>

#include "collection.h"
#include "array.h"

/******PUBLIC FUNCTIONS*******************/

Array * array_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize){
	Array * array = malloc(sizeof(Array));
	collection_init((Collection *)array, segmentName, initAddress, slabIncrementSize*sizeOfType + sizeof(ArrayHdr));
	array->sizeOfType=sizeOfType;
	array->slabIncrementSize=slabIncrementSize;
	array->nextFreeSlot=array->base.mem + sizeof(ArrayHdr) + ( ((ArrayHdr *)array->base.mem)->currentObjectsAllocated * sizeOfType);
	return array;
}

void * array_addObject(Array * array, void * object){
	void * target = collection_findFree((Collection *)array);

	if (!target && (array->nextFreeSlot + array->sizeOfType) > (array->base.mem + array->base.sizeOfMapping)){	//are we out of space?!
		if (collection_resize((Collection *)array, array->base.sizeOfMapping + (array->slabIncrementSize * array->sizeOfType)) == NULL){
			return NULL;		
		}
	}
	if (!target){
		target = array->nextFreeSlot;
		array->nextFreeSlot=array->nextFreeSlot+array->sizeOfType;
		((ArrayHdr *)array->base.mem)->currentObjectsAllocated+=1;
	}
	memcpy(target, object, array->sizeOfType);
	((CollectionObject *)target)->nextFree=0;	//make sure this is 0 since it indicates whether it is valid.
	return target;
}

int array_removeObject(Array * array, void * object){
	collection_remove((Collection *)array, object);
}

void array_close(Array * array){
	collection_close((Collection *)array);	
}

void * array_getById(Array * array, int id){
	void * result = array->base.mem + (array->sizeOfType * id) + sizeof(ArrayHdr); 	
	return (isValid((Collection *)array, (CollectionObject *)result)) ? result : NULL;
}

void array_delete(Array * array){
	collection_delete((Collection *)array);	
}

void array_deleteSegment(char * segmentName){
	collection_deleteSegment(segmentName);
}


