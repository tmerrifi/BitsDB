#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "graphKernel/graph.h"
#include "graphKernel/collection.h"

typedef struct{
	char * attributeName;
	Collection * collection;
}AttributeCollection;

AttributeCollection * createAttribute(Graph * graph, Collection * collection, "label");

addAttribute(graph, v, "label", "a fake label");



#endif