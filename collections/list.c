
#include <stdio.h>
#include <stdlib.h>

#include "collection.h"
#include "array.h"
#include "list.h"

Lists * lists_init(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize){
	Lists * lists = malloc(sizeof(Lists));
	return (Lists *)(array_init(segmentName, initAddress, sizeOfType, slabIncrementSize));
}

List * lists_addList(Lists * lists){
	ListObject * list = malloc(sizeof(ListObject));
	list->meta_un.listHeader.head = list->meta_un.listHeader.tail = 0;
	return (List *)array_addObject((Array *)lists,list);
}

void * lists_addObjectToList(Lists * lists, List * list, void * object){
	void * newObject = array_addObject((Array *)lists,object);
	if (newObject){
		((ListObject *)newObject)->meta_un.ptrs.next = 0;
		((ListObject *)newObject)->meta_un.ptrs.prev = 0;
		
		int offsetFromStart = newObject - lists->base.mem;
		if (!list->meta_un.listHeader.head){
			//insert at the head
			list->meta_un.listHeader.head = list->meta_un.listHeader.tail = offsetFromStart;
		}	
		else{
			ListObject * oldTail = (ListObject *)(lists->base.mem + list->meta_un.listHeader.tail);
			oldTail->meta_un.ptrs.next = offsetFromStart;
			((ListObject *)newObject)->meta_un.ptrs.prev = list->meta_un.listHeader.tail;
			list->meta_un.listHeader.tail=offsetFromStart;
		}
	}
	return newObject + sizeof(ListObject);
}

void * lists_getNextObject(Lists * lists, void * object){
	ListObject * listObj = (ListObject * )object;
	if (listObj->meta_un.ptrs.next){
		return (void *)(lists->base.mem + listObj->meta_un.ptrs.next);
	}
	else{
		return NULL;	
	}
}

void * lists_getFirst(Lists * lists, List * list){
	if (list->meta_un.listHeader.head && isValid((Collection *)lists, list)){
		return (void *)(lists->base.mem + list->meta_un.listHeader.head);
	}
}

int lists_removeList(Lists * lists, List * list){
	collection_remove((Collection *)lists, (CollectionObject *)list);
}

int lists_removeObjectFromList(Lists * lists, List * list, void * object){
	ListObject * listObj = (ListObject *)object;
	if (listObj->meta_un.ptrs.next && listObj->meta_un.ptrs.prev){							//we are in the middle of the list
		ListObject * prevObj = (ListObject *)(lists->base.mem + listObj->meta_un.ptrs.prev);
		prevObj->meta_un.ptrs.next = listObj->meta_un.ptrs.next;
	}
	else if (listObj->meta_un.ptrs.next){				//we are the head of the linked list and there is a next object
		list->meta_un.listHeader.head = listObj->meta_un.ptrs.next;		//point the head of the list to the next object
	}
	else if (listObj->meta_un.ptrs.prev){
		ListObject * prevObj = (ListObject *)(lists->base.mem + listObj->meta_un.ptrs.prev);
		prevObj->meta_un.ptrs.next = 0;
	}
	else{												//we are the only node in the list
		list->meta_un.listHeader.head = 0;
	}
	collection_remove((Collection *)lists, (CollectionObject *)listObj);
}


void lists_delete(Lists * lists){
	collection_delete((Collection *)lists);	
}

void lists_deleteSegment(char * segmentName){
	collection_deleteSegment(segmentName);
}


