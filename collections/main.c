#include <stdio.h>
#include <string.h>

#include "collection.h"
#include "array.h"
#include "list.h"
#include "graphKernel/shmUtility.h"

int test_addNewObjectsSimple();

typedef struct {
	ListObject base;
	int ssn;
	char name[10];
}Student;

int main(){
	
	printf("***Lists Tests****\n");
	printf("test_lists_addObjectsSimple: %d\n", test_lists_addObjectsSimple());
	printf("test_lists_removeListSimple: %d\n", test_lists_removeListSimple());
	printf("test_lists_removeObjects: %d\n", test_lists_removeObjects());
	
	printf("***Array Tests***\n");
	printf("test_addNewObjectsSimple: %d\n", test_addNewObjectsSimple());
	printf("test_addNewObjectsResize: %d\n", test_addNewObjectsResize());
	printf("test_freeObjects: %d\n", test_freeObjects());
	printf("test_reallocateFreedObjects: %d\n", test_reallocateFreedObjects());
}

int test_lists_removeObjects(){
	lists_deleteSegment("testLists");
	Lists * lists = lists_init("testLists", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	List * ourList = lists_addList(lists);
	
	for (int i = 0; i < 10; ++i){
		Student * amber = malloc(sizeof(Student));
		amber->ssn=i;
		strncpy(amber->name, "Amber", 7);
		amber = lists_addObjectToList(lists, ourList, amber);	
	}
	
	Student * s=NULL;
	int counter=0;
	LISTS_FOR_EACH(lists,ourList,s){
		if (counter % 2){
			lists_removeObjectFromList(lists, ourList, s);
		}
		counter++;
	}
	
	s=NULL;
	counter=0;
	LISTS_FOR_EACH(lists,ourList,s){
		if (s->ssn != counter){
			printf("%d %d", s->ssn, counter);
			return 0;
		}	
		counter+=2;
	}
	
	return 1;
}

int test_lists_removeListSimple(){
	lists_deleteSegment("testLists");
	Lists * lists = lists_init("testLists", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	List * ourList = lists_addList(lists);
	
	for (int i = 0; i < 10; ++i){
		Student * amber = malloc(sizeof(Student));
		amber->ssn=i;
		strncpy(amber->name, "Amber", 7);
		amber = lists_addObjectToList(lists, ourList, amber);	
	}

	lists_removeList(lists, ourList);
	Student * s = lists_getFirst(lists,ourList);
	return (s == NULL) ? 1 : 0;
}

int test_lists_addObjectsSimple(){
	lists_deleteSegment("testLists");
	Lists * lists = lists_init("testLists", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	List * ourList = lists_addList(lists);
	
	for (int i = 0; i < 10; ++i){
		Student * amber = malloc(sizeof(Student));
		amber->ssn=i;
		strncpy(amber->name, "Amber", 7);
		amber = lists_addObjectToList(lists, ourList, amber);	
	}
	
	Student * s;
	int counter=0;
	for (Student * s=lists_getFirst(lists,ourList); s!=NULL; s=lists_getNextObject(lists,s)){
		if (s->ssn != counter){ return 0; }
		counter++;
	}
	
	return 1;
}

int test_reallocateFreedObjects(){
	
	array_deleteSegment("testArray");
	Array * theArray = array_init("testArray", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	
	Student * amber = malloc(sizeof(Student));
	amber->ssn=33333;
	strncpy(amber->name, "Amber", 7);
	
	Student * tim = malloc(sizeof(Student));
	tim->ssn=444444;
	strncpy(tim->name, "Tim", 7);
	
	array_addObject(theArray, amber);
	array_addObject(theArray, tim);
	
	amber = array_getById(theArray, 0);
	tim = array_getById(theArray, 1);
	
	array_removeObject(theArray, amber);
	array_removeObject(theArray, tim);
	
	Student * rex = malloc(sizeof(Student));
	rex->ssn=555;
	strncpy(rex->name, "Rex", 7);
	
	Student * hitch = malloc(sizeof(Student));
	hitch->ssn=666;
	strncpy(hitch->name, "Hitchens", 7);
	
	array_addObject(theArray, rex);
	array_addObject(theArray, hitch);
	
	rex = array_getById(theArray, 0);
	hitch = array_getById(theArray, 1); 
	
	return (rex->ssn == 555 && hitch->ssn == 666) ? 1 : 0;
}


int test_freeObjects(){
	array_deleteSegment("testArray");
	Array * theArray = array_init("testArray", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	
	Student * amber = malloc(sizeof(Student));
	amber->ssn=33333;
	strncpy(amber->name, "Amber", 7);
	
	Student * tim = malloc(sizeof(Student));
	tim->ssn=444444;
	strncpy(tim->name, "Tim", 7);
	
	array_addObject(theArray, amber);
	array_addObject(theArray, tim);
	
	amber = array_getById(theArray, 0);
	tim = array_getById(theArray, 1);
	
	array_removeObject(theArray, amber);
	
	amber = array_getById(theArray, 0);
	
	return (amber == NULL) ? 1 : 0;
}

int test_addNewObjectsResize(){
	//Array * theArray = array_init("uyd2", (void *)0xA0000000, sizeof(Student), 2);
	array_deleteSegment("testArray");
	Array * theArray = array_init("testArray", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	
	Student * amber = malloc(sizeof(Student));
	amber->ssn=33333;
	strncpy(amber->name, "Amber", 7);
	
	Student * tim = malloc(sizeof(Student));
	tim->ssn=444444;
	strncpy(tim->name, "Tim", 7);
	
	Student * rex = malloc(sizeof(Student));
	rex->ssn=555;
	strncpy(rex->name, "Rex", 7);
	
	Student * hitch = malloc(sizeof(Student));
	hitch->ssn=666;
	strncpy(hitch->name, "Hitchens", 7);
	
	array_addObject(theArray, amber);
	array_addObject(theArray, tim);
	array_addObject(theArray, rex);
	array_addObject(theArray, hitch);
	
	hitch = array_getById(theArray, 4);
	
	return (hitch->ssn == 666) ? 1 : 0;
}

int test_addNewObjectsSimple(){
	//Array * theArray = array_init("uyd2", (void *)0xA0000000, sizeof(Student), 2);
	array_deleteSegment("testArray");
	Array * theArray = array_init("testArray", (void *)0xA0000000, sizeof(Student), 2, SHM_CORE);
	
	Student * amber = malloc(sizeof(Student));
	amber->ssn=33333;
	strncpy(amber->name, "Amber", 7);
	
	Student * tim = malloc(sizeof(Student));
	tim->ssn=444444;
	strncpy(tim->name, "Tim", 7);
	
	array_addObject(theArray, amber);
	array_addObject(theArray, tim);
	
	amber = array_getById(theArray, 1);
	tim = array_getById(theArray, 2);
	
	return (amber->ssn == 33333 && tim->ssn == 444444) ? 1 : 0;
}

