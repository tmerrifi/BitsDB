

#ifndef SORTTEST_H
#define SORTTEST_H

#include "rbtdata.h"
#include "rbtree.h"
#include "list.h"

#define SORT_TEST_TYPE_RBTREE 1
#define SORT_TEST_TYPE_LIST 2

//append a snapshot if it is using ksnap, total hack
#define SORT_TEST_NAME(o) ((o.sync_type==2)?"snapshot_sort_test":"sort_test")

struct sorted_data_structure{
  union{
    struct rb_root * rbroot;
    struct sorted_list_head * list_head;
  }sorted_ds_ptr;
  int * size;
  int ds_type;
};

//generic data structure, used for doing basic operations on sorted data structures
struct sort_tester{
  struct sorted_data_structure * sds;
  void * (*search) (void * data_structure, int key);
  int (*insert) (void * data_structure, int key);
  void * (*delete) (void * data_structure, int key);
  void (*map) (void * data_structure, void (*map_function)(int key, void *), void * data);
};


struct sort_tester * sort_tester_factory(int data_structure_type, char * test_name, int size_of_segment);

int sort_tester_insert(struct sort_tester * st, int key);

void * sort_tester_delete(struct sort_tester * st, int key);

void * sort_tester_search(struct sort_tester * st, int key);

void sort_tester_map(struct sort_tester * st, void (*map_function)(int key, void *), void * data);

#endif
