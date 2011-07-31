
#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include <semaphore.h>

#include "graphKernel/array.h"
#include "graphKernel/list.h"
#include "graphKernel/linkedList.h"

typedef struct{
	CollectionObject base;	//so we can place it in our collections classes
	int neighborsPtr;
}Vertex;

typedef struct{
	ListObject base;	//because the neighbors is a list
	int destVertex;
}Neighbor;

/*The central data structure representing a graph*/
typedef struct{
	Array * vertices;
	Lists * neighbors;
	char * graphName;
	int graphId;
	LinkedList * attributes;
	sem_t * lock;
}Graph;

/*Main function for creating a new graph*/
Graph * graph_init(char * graphName);

/*Get an update of this graph*/
Graph * updateGraph(char * graphName);

/*Add the vertices to the graph from JSON*/
int addVertices(Graph * graph, char * json);

int addNeighbor(Graph * graph, int v1, int v2);

int addVerticesFromCount(Graph * graph, u_int32_t count, int ** indices);

void graph_getSnapShot(Graph * graph);

#define graph_getSemName(gname) coreUtil_concat("sem_", gname)

#endif