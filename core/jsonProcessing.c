
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "yajl/yajl_tree.h"


int json_addFromList(char * json, void * collection, void * (*createObject)(yajl_val node), int (*addObject)(void * collection, void * object)){
	int successfullyAdded=0;
	char errbuf[1024];
	yajl_val node;	// the root node of the structure representing the json tree
	if ( node = yajl_tree_parse((const char *) json, errbuf, sizeof(errbuf))){	//get this party started with some json parsing
		if (node->type == yajl_t_array){	//we should have an array of vertices! Else something is wronge
				int length = node->u.array.len;		//how many vertices do we have?!
				for (int i = 0; i < length; ++i){	//lets step through them all
					void * newObject=createObject(node->u.array.values[i]); //create the new object
					if (newObject){
						 if (addObject(collection,newObject) > 0){
						 	++successfullyAdded;
						 }
					}
				}
		}
	}
}