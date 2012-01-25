#ifndef LIST_H
#define LIST_H

#include <sys/queue.h>

#define LIST_DIRECTION_UP 1
#define LIST_DIRECTION_DOWN 0

struct sorted_list_entry{
  int key;
  CIRCLEQ_ENTRY(sorted_list_entry) entry;
  char buffer[OBJ_SIZE];
};

//CIRCLEQ_HEAD(sorted_list_head, sorted_list_entry);

struct sorted_list_head {                                                        
  struct sorted_list_entry *cqh_first;
  struct sorted_list_entry *cqh_last;
  int direction;
  int max_list_size;
  int list_size;
};

//like the classical functional programming "map". Traverse the data structure and invoke the map_function on each node
void sorted_list_map(void  * list, void (*map_function)(int key, void * data), void * data);

//search the red-black tree for this key
void * sorted_list_search(void * list, int key);

//insert into the red-black tree
int sorted_list_insert(void * list, int key);

//delete a node from the red-black tree
void * sorted_list_delete(void * list, int key);

//init the data structure
void * sorted_list_init();

#endif
