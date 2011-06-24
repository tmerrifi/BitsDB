#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection.h"
#include "array.h"

/******PUBLIC FUNCTIONS*******************/

Array * array_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize, int callingProcess){
	Array * array = malloc(sizeof(Array));
	collection_init((Collection *)array, segmentName, initAddress, slabIncrementSize*sizeOfType + sizeof(ArrayHdr), callingProcess);
	array->sizeOfType=sizeOfType;
	array->slabIncrementSize=slabIncrementSize;
	
	if (((ArrayHdr *)array->base.mem)->currentObjectsAllocated == 0){	//lets start it off at 1 so a pointer of 0 will indicate NULL
		((ArrayHdr *)array->base.mem)->currentObjectsAllocated = 1;	
	}
	
	array->nextFreeSlot=array->base.mem + sizeof(ArrayHdr) + ( ((ArrayHdr *)array->base.mem)->currentObjectsAllocated * sizeOfType);
	
	return array;
}

void * array_addObjectByAddress(Array * array, void * object, void * target, int fromFreeList){
	
	if ((target + array->sizeOfType) > (array->base.mem + array->base.sizeOfMapping)){	//are we out of space?!
		
		int diff = (target + array->sizeOfType) - (array->base.mem + array->base.sizeOfMapping);	//how much space do we need?
		int inc = (array->slabIncrementSize * array->sizeOfType);	//the default incremement
		if (diff > (array->slabIncrementSize * array->sizeOfType)){	//if it is not enough, add the difference to insure we have what we need
			inc += diff;
		}
		if (collection_resize((Collection *)array, array->base.sizeOfMapping + inc) == NULL){		//perform the resize
			return NULL;		
		}
		array->nextFreeSlot= target + array->sizeOfType;
		((ArrayHdr *)array->base.mem)->currentObjectsAllocated+=array_getIndex(array, target);
	}
	else if (!fromFreeList){
		array->nextFreeSlot= target + array->sizeOfType;
		((ArrayHdr *)array->base.mem)->currentObjectsAllocated+=1;
	}	
	
	memcpy(target, object, array->sizeOfType);
	((CollectionObject *)target)->nextFree=0;	//make sure this is 0 since it indicates whether it is valid.
	return target;
}

void * array_addObjectByIndex(Array * array, void * object, int index){
	return array_addObjectByAddress(array, object, array_getObjectFromIndex(array, index), 0);
}


void * array_addObject(Array * array, void * object){
	void * target = collection_findFree((Collection *)array);
	int fromFree = 1;
	if (!target){
		target = array->nextFreeSlot;
		fromFree = 0;
	}

	return array_addObjectByAddress(array, object, target, fromFree);
}

void * array_getObjectFromIndex(Array * array, int index){
	array_getNextValidObjectFromIndex(array, &index, 0);
}

void * array_getNextValidObjectFromIndex(Array * array, int * index, int keepGoingFlag){
	
	if (*index > ((ArrayHdr *)array->base.mem)->currentObjectsAllocated ){
		return NULL;	
	}
	else{
		void * obj = array->base.mem + sizeof(ArrayHdr) + (*index * array->sizeOfType);	
		int valid = isValid(array,obj);
		if (!valid && keepGoingFlag){
			*index = *index + 1;
			void * obj = array_getObjectFromIndex(array, index);			//not valid? try the next object	
		}	
		else{
			if (!valid){
				obj=NULL;	
			}
			*index = *index + 1;
		}
		return obj;
	}
}


/*DESCRIPTION: calculate the index into the array of this vertex*/
int array_getIndex(Array * array, void * object){
	return (object && isValid(array, object)) ? ((object - array->base.mem - sizeof(ArrayHdr))/array->sizeOfType) : NULL;
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


