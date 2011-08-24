
#include <stdlib.h>
#include "linkedList.h"


LinkedList * linkedList_init(){
	LinkedList * list = malloc(sizeof(LinkedList));
	list->head=NULL;
	list->tail=NULL;
	return list;
}

LinkedListNode * linkedList_addNode(LinkedList * list, void * payload){
	LinkedListNode * newNode = NULL;
	if (list){
		newNode = malloc(sizeof(LinkedListNode));
		newNode->next=NULL;
		newNode->prev=NULL;
		newNode->payload=payload;
		if (list->head == NULL){
			list->head=newNode;
			list->tail=newNode;
		}	
		else{
			list->tail->next = newNode;
			newNode->prev = list->tail;
			list->tail = newNode;
		}
		list->length+=1;
	}
	return newNode;
}

LinkedListNode * linkedList_removeNode(LinkedList * list, LinkedListNode * targetNode){
	if (list && targetNode){	
		if (targetNode->next && targetNode->prev){
			targetNode->prev->next = targetNode->next;
			targetNode->next->prev = targetNode->prev;
		}
		else if (targetNode->next){
			list->head = targetNode->next;
			targetNode->next->prev = NULL;
		}
		else if (targetNode->prev){
			targetNode->prev->next = NULL;	
		}
		else{
			list->head=NULL;
			list->tail=NULL;	
		}
		list->length-=1;
		free(targetNode);
	}
}	
	
LinkedListNode * linkedList_find(LinkedList * list, void * targetPayload, int (*compareFunction)(void * comparePayload, void * targetPayload)){
	if (list && list->head){
		LinkedListNode * tmp = list->head;
		while (tmp){
			if (compareFunction(tmp->payload, targetPayload)){
				return tmp;	
			}
			tmp=tmp->next;	
		}
	}
}

LinkedListNode * linkedList_print(LinkedList * list, void (*printFunction)(void * payload)){
	if (list && list->head){
		LinkedListNode * tmp = list->head;
		while (tmp){
			printFunction(tmp->payload);
			tmp=tmp->next;	
		}
	}
}

void linkedList_free(LinkedList * list){
	if (list && list->head){
		LinkedListNode * tmp = list->head;
		LinkedListNode * target;
		while (tmp){
			target = tmp;
			tmp=tmp->next;
			free(target);
		}
	}
	free(list);
}

/***********Comparison Functions**************/

int linkedList_compare_string(void * str, void * targetStr){
	if (strcmp((char *)str, (char *)targetStr)==0){
		return 1;	
	}
	else{
		return 0;	
	}
}

int linkedList_compare_ptr(void * ptr, void * targetPtr){
	if (ptr==targetPtr){
		return 1;	
	}	
	else{
		return 0;	
	}
}






