
#include <stdio.h>
#include "linkedList.h"

void printOurString(void * string);

int findString(void * str, void * targetStr);

int main(){

	LinkedList * ourList = linkedList_init();
	
	char * test1 = "testing1";
	char * test2 = "testing2";
	char * test3 = "testing3";
	
	LinkedListNode * one =linkedList_addNode(ourList, test1);
	LinkedListNode * two =linkedList_addNode(ourList, test2);
	LinkedListNode * three = linkedList_addNode(ourList, test3);
	
	linkedList_print(ourList, printOurString);
	printf("\n\n");
	
	LinkedListNode * foundNode = linkedList_find(ourList, test1, findString);
	printf("%s\n\n", (char *)foundNode->payload);
	
	linkedList_removeNode(ourList,two); 
	
	linkedList_print(ourList, printOurString);
	printf("\n\n");
	
	linkedList_removeNode(ourList,one); 
	
	linkedList_print(ourList, printOurString);
	printf("\n\n");
	
	linkedList_removeNode(ourList,three); 
	
	one =linkedList_addNode(ourList, test1);
	
	linkedList_print(ourList, printOurString);
	printf("\n\n");
}


int findString(void * str, void * targetStr){
	if (strcmp((char *)str, (char *)targetStr)==0){
		return 1;	
	}
	else{
		return 0;	
	}
}


void printOurString(void * string){

	printf("%s\n", string);	
	
}