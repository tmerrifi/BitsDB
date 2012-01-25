
#include "sort_test.h"
#include "rbtree.h"
#include "rbtdata.h"
#include "mm.h"

struct sort_tester * __sort_tester_new(){
  struct sorted_data_structure * sds = malloc(sizeof(struct sorted_data_structure));
  struct sort_tester * st = malloc(sizeof(struct sort_tester));
  st->sds = sds;
  sds->size = MM_malloc(sizeof(int));
  return st;
}

struct sort_tester * __sort_tester_new_rbt(int data_structure_type, char * test_name){
  struct sort_tester * st = __sort_tester_new();
  st->sds->sorted_ds_ptr.rbroot = rbtdata_init();
  //st->sds->sorted_ds_ptr.rbroot = malloc(sizeof(struct rb_root));
  //st->sds->sorted_ds_ptr.rbroot->rb_node = NULL;
  st->sds->ds_type = SORT_TEST_TYPE_RBTREE;
  st->insert = rbtdata_insert;
  st->search = rbtdata_search;
  st->delete = rbtdata_delete;
  st->map = rbtdata_map;
  return st;
}

struct sort_tester * __sort_tester_new_list(int data_structure_type, char * test_name, int size_of_segment){
  struct sort_tester * st = __sort_tester_new();
  st->sds->sorted_ds_ptr.list_head = sorted_list_init(size_of_segment);
  st->sds->ds_type = SORT_TEST_TYPE_LIST;
  st->insert = sorted_list_insert;
  st->search = sorted_list_search;
  st->delete = sorted_list_delete;
  st->map = sorted_list_map;
  return st;
}

void * __get_data_structure(struct sorted_data_structure * sds){
  switch (sds->ds_type){
  case SORT_TEST_TYPE_RBTREE:
    return sds->sorted_ds_ptr.rbroot;
  case SORT_TEST_TYPE_LIST:
    return sds->sorted_ds_ptr.list_head;
  default:
    return NULL;
  }
}

struct sort_tester * sort_tester_factory(int data_structure_type, char * test_name, int size_of_segment){
  //Unfortunately need to append snapshot to this test_name
  char * shm_file_name = malloc(100);
  sprintf(shm_file_name, "%s", test_name);
   //create the mmap region
  printf("size of segment....%d\n", size_of_segment);
  MM_create(size_of_segment, shm_file_name);
  printf("created the region and called it %s\n", shm_file_name);
  switch(data_structure_type){
  case SORT_TEST_TYPE_RBTREE:
    return __sort_tester_new_rbt(data_structure_type, test_name);
    break;
  case SORT_TEST_TYPE_LIST:
    return __sort_tester_new_list(data_structure_type, test_name, size_of_segment);
    break;
  default:
    return NULL;
  }
}

void sort_tester_map(struct sort_tester * st, void (*map_function)(int key, void * data), void * data){
  st->map(__get_data_structure(st->sds), map_function, data);
}

void * sort_tester_delete(struct sort_tester * st, int key){
  void * result = st->delete(__get_data_structure(st->sds), key);
  MM_free(result);
}

int sort_tester_insert(struct sort_tester * st, int key){
  st->insert(__get_data_structure(st->sds), key);
}

void * sort_tester_search(struct sort_tester * st, int key){
  
  st->search(__get_data_structure(st->sds), key);
}
