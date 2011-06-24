
#include <stdlib.h>

#include "attributes.h"
#include "graphKernel/coreUtility.h"
#include "graphKernel/array.h"
#include "graphKernel/collection.h"
#include "graphKernel/shmUtility.h"
#include "graphKernel/linkedList.h"
/*
DESCRIPTION: 

*/
AttributeCollection * attribute_init(Graph * graph, Collection * collection, char * attributeName, AttributeType attType){
	
	AttributeCollection * attributeColl = malloc(sizeof(AttributeCollection));
	attributeColl->attributeName = coreUtil_concat(graph->graphName, attributeName);
	attributeColl->parentCollection = collection;
	
	int sizeOfType = 0;
	CollectionType collType;
	 
	switch (attType){
		case STRING_TYPE :
			sizeOfType=sizeof(AttributeString);
			collType=coll_array;
			break;
		case INT_TYPE : 
			sizeOfType=sizeof(AttributeInt);
			collType=coll_array;
			break;
		case DOUBLE_TYPE :
			sizeOfType=sizeof(AttributeDouble);
			collType=coll_array;
	}
	if (collType==coll_array){
		//create the new array that will store this exciting new attribute
		attributeColl->collection= (Collection *)array_init(attributeColl->attributeName, 0, sizeOfType, 
															(collection->sizeOfMapping) ? collection->sizeOfMapping : DEFAULT_ARR_INC_SIZE, 
															SHM_CORE);
	}
	//add the the graph's attribute list
	linkedList_addNode(graph->attributes, attributeColl);
	
	return (attributeColl && attributeColl->collection) ? attributeColl : NULL;
}


void attribute_addString(AttributeCollection * ac, void * object, char * theAttribute){
		return NULL;
}

void attribute_addInteger(AttributeCollection * ac, void * object, int theAttribute){
	//integers are stored in the array type...so grab the int
	if (ac && ac->collection && object){
		//first compute offset for vertex
		int offset = array_getIndex(ac->parentCollection, object);
		attribute_addIntegerWithIndex(ac, offset, theAttribute);
	}
}

void attribute_addIntegerWithIndex(AttributeCollection * ac, int index, int theAttribute){
	//integers are stored in the array type...so grab the int
	if (ac && ac->collection){
		//first compute offset for vertex
		AttributeInt * newAttribute = malloc(sizeof(AttributeInt));
		newAttribute->attribute=theAttribute;
		//now insert into that position
		array_addObjectByIndex(ac->collection, newAttribute, index);
	}
}


void attribute_addDouble(AttributeCollection * ac, void * object, double theAttribute){
	//integers are stored in the array type...so grab the int
	if (ac && ac->collection && object){
		//first compute offset for vertex
		int offset = array_getIndex(ac->parentCollection, object);
		attribute_addIntegerWithIndex(ac, offset, theAttribute);
	}
}

void attribute_addDoubleWithIndex(AttributeCollection * ac, int index, double theAttribute){
	//integers are stored in the array type...so grab the int
	void * tmp;
	if (ac && ac->collection){
		//first compute offset for vertex
		AttributeDouble * newAttribute = malloc(sizeof(AttributeInt));
		newAttribute->attribute=theAttribute;
		//now insert into that position
		tmp = array_addObjectByIndex(ac->collection, newAttribute, index);
	}
}

double attribute_getDouble(AttributeCollection * ac, void * object){
	if (ac && object){
		int offset = array_getIndex(ac->parentCollection, object);
		AttributeDouble * doubleObject = array_getObjectFromIndex(ac->collection, offset);
		return (doubleObject) ? doubleObject->attribute : 0;
	}	
	else{
		return 0;	
	}
}

int attribute_getInteger(AttributeCollection * ac, void * object){
	if (ac && object){
		int offset = array_getIndex(ac->parentCollection, object);
		AttributeInt * intObject = array_getObjectFromIndex(ac->collection, offset);
		return (intObject) ? intObject->attribute : 0;
	}	
	else{
		return 0;	
	}
}



