#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "graphKernel/graph.h"
#include "graphKernel/collection.h"
#include "graphKernel/array.h"
#include "graphKernel/list.h"

typedef enum{INT_TYPE=1,STRING_TYPE=2, DOUBLE_TYPE}AttributeType;

#define MAX_STRING_SIZE 25

typedef struct{
	char * attributeName;			//the name of this attribute
	Collection * collection;		//where we store the attribute values
	Collection * parentCollection;	//the collection we are an attribute for
}AttributeCollection;


typedef struct{
	CollectionObject base;
	int attribute;
}AttributeInt;

typedef struct{
	CollectionObject base;
	double attribute;
}AttributeDouble;

typedef struct{
	CollectionObject base;
	char attribute[MAX_STRING_SIZE];
}AttributeString;

AttributeCollection * attribute_init(Graph * graph, Collection * collection, char * attributeName, AttributeType attType);

void attribute_addString(AttributeCollection * ac, void * object, char * theAttribute);

void attribute_addInteger(AttributeCollection * ac, void * object, int theAttribute);

void attribute_addIntegerWithIndex(AttributeCollection * ac, int index, int theAttribute);

void attribute_addDoubleWithIndex(AttributeCollection * ac, int index, double theAttribute);

int attribute_getInteger(AttributeCollection * ac, void * object);

double attribute_getDouble(AttributeCollection * ac, void * object);

#endif