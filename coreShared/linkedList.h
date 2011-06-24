
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct _LinkedListNode{
	struct _LinkedListNode * prev;
	struct _LinkedListNode * next;
	void * payload;
}LinkedListNode;

typedef struct {
	LinkedListNode * head;
	LinkedListNode * tail;
}LinkedList;

LinkedList * linkedList_init();

LinkedListNode * linkedList_addNode(LinkedList * list, void * payload);

LinkedListNode * linkedList_removeNode(LinkedList * list, LinkedListNode * targetNode);

LinkedListNode * linkedList_find(LinkedList * list, void * targetPayload, int (*compareFunction)(void * comparePayload, void * targetPayload));

LinkedListNode * linkedList_print(LinkedList * list, void (*printFunction)(void * payload));

void linkedList_free(LinkedList * list);

int linkedList_compare_string(void * str, void * targetStr);

#endif