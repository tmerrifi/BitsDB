
#ifndef JSON_PROCESSING_H
#define JSON_PROCESSING_H

#include "yajl/yajl_tree.h"

int json_addFromList(char * json, void * collection, void * (*createObject)(yajl_val), int (*addObject)(void *, void *));

#endif