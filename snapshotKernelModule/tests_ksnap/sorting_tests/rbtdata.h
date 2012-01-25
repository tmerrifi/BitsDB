
#ifndef RBTDATA_H
#define RBTDATA_H

#include "rbtree.h"

struct rbtdata{
  struct rb_node node;
  int key;
  char buffer[OBJ_SIZE];
};

//like the classical functional programming "map". Traverse the data structure and invoke the map_function on each node
void rbtdata_map(void  * __root, void (*map_function)(int key, void *), void * data);

//search the red-black tree for this key
void *rbtdata_search(void *__root, int key);

//insert into the red-black tree
int rbtdata_insert(void * __root, int key);

//delete a node from the red-black tree
void * rbtdata_delete(void * __root, int key);

//init the data structure
void * rbtdata_init();

#endif
