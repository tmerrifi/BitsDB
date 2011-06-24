
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "yajl/yajl_tree.h"
#include "graphKernel/attributes.h"

int json_parseArrayOfObjectAndFindAttributes(char * json, void * dataBase, 
											 void * (*findAttributeFunction)(void * database, char * attributeName), 
											 int * (*createNewObjects) (void * dataBase, int count)){
	
	int successfullyAdded=0;
	char errbuf[1024];
	int vertexCount = 0;
	int * indexArray = NULL;
	yajl_val node;	// the root node of the structure representing the json tree
	if ( node = yajl_tree_parse((const char *)json, errbuf, sizeof(errbuf))){	//get this party started with some json parsing
		if (node->type == yajl_t_array){	//we should have an array of vertices! Else something is wrong
				vertexCount = node->u.array.len;		//how many vertices do we have?!
				if (vertexCount > 0){
					indexArray = createNewObjects(dataBase, vertexCount);	
					for (int i = 0; i < vertexCount; ++i){	//lets step through them all
						yajl_val objectNode = node->u.array.values[i];
						if (objectNode->type == yajl_t_object){
							char ** keys = objectNode->u.object.keys;
							int keyLength = objectNode->u.object.len;
							for (int k = 0; k < keyLength; ++k){
								//okay, now go through each key / value pair
								char * key = keys[k];
								AttributeCollection * ac = findAttributeFunction(dataBase,key);
								yajl_val value = objectNode->u.object.values[k];
								switch(value->type){
									case yajl_t_number:
										if (YAJL_IS_INTEGER(value)){
												attribute_addIntegerWithIndex(ac,i,(int)value->u.number.i);
										}	
										else if (YAJL_IS_DOUBLE(value)){
											attribute_addDoubleWithIndex(ac,indexArray[i],(int)value->u.number.d);		
										}
										break;
								}
							}
						}
					}
				}
		}
	}
	else{
		printf("parsing error: %s\n", errbuf);	
	}
}


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