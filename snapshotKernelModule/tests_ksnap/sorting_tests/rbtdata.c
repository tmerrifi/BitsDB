
#include "rbtdata.h"
#include "rbtree.h"
#include "mm.h"

void * rbtdata_init(){
   struct rb_root * rbroot = MM_malloc(sizeof(struct rb_root));
   rbroot->rb_node = NULL;

   return rbroot;
}

void __rbtdata_map(struct rb_node * node, void (*map_function)(int key, void * data), void * data){
  if (!node)
    return;
  else{
    struct rbtdata * rbtdata = container_of(node, struct rbtdata, node);
    __rbtdata_map(node->rb_left, map_function, data);
    map_function(rbtdata->key, data);
    __rbtdata_map(node->rb_right, map_function, data);
  }
}

void rbtdata_map(void  * __root, void (*map_function)(int, void *), void * data){
  struct rb_root * root = (struct rb_root *)__root;
  struct rb_node *node = root->rb_node;
  __rbtdata_map(node, map_function, data);
}

void * rbtdata_search(void  * __root, int key){
  struct rb_root * root = (struct rb_root *)__root;
  struct rb_node *node = root->rb_node;

  while (node) {
    struct rbtdata *data = container_of(node, struct rbtdata, node);
    int result=data->key;
    if (key < result)
      node = node->rb_left;
    else if (key > result)
      node = node->rb_right;
    else
      return data;
  }
  return NULL;
}

void * rbtdata_delete(void * __root, int key){
  struct rb_root * root = (struct rb_root *)__root;
  struct rbtdata * data = rbtdata_search(root, key);
  if (data){
    rb_erase(&data->node, root);
  }
  return data;
}

int rbtdata_insert(void * __root, int key){
  struct rb_root * root = (struct rb_root *)__root;
  struct rbtdata * result;
  if (result = rbtdata_delete(root, key)){
    //printf("deleting!!!!\n");
    MM_free(result);
  }
  else{
    //printf("inserting!!!!\n");
    struct rbtdata * data = MM_malloc(sizeof(struct rbtdata));
    //printf("size of rbtdata %d\n", sizeof(struct rbtdata));
    data->key = key;
    struct rb_node **new = &(root->rb_node), *parent = NULL;
  

    /* Figure out where to put new node */
    while (*new) {
      struct rbtdata *this = container_of(*new, struct rbtdata, node);
      int result = this->key;
      parent = *new;
      if (data->key < result)
	new = &((*new)->rb_left);
      else if (data->key > result)
	new = &((*new)->rb_right);
      else
	return 0;
    }
    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);

    return 1;
  }
}
