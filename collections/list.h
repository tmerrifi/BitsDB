
#ifndef LIST_H
#define LIST_H

#include "collection.h"
#include "array.h"

typedef Array Lists;

typedef struct{
	CollectionObject base;
	union{
		struct{
			int head;
			int tail;
		} listHeader;
		struct{
			int next;
			int prev;	
		} ptrs;
	} meta_un;
}ListObject;

typedef ListObject List;

typedef struct{
	CollectionHdr base;
}ListsHdr;

Lists * lists_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize, int callingProcess);

List * lists_addList(Lists * lists);

void * lists_addObjectToList(Lists * lists, List * list, void * object);

void * lists_getNextObject(Lists * lists, void * object);

void lists_delete(Lists * lists);

void lists_deleteSegment(char * segmentName);

void * lists_getFirst(Lists * lists, List * list);

int lists_removeList(Lists * lists, List * list);

int lists_removeObjectFromList(Lists * lists, List * list, void * object);

#define LISTS_FOR_EACH(lists,ourList,i)\
		for (i=lists_getFirst(lists,ourList); i!=NULL; i=lists_getNextObject(lists,i))

#endif