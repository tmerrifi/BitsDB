
#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include "graphKernel/objectSlab.h"

typedef struct{
	double lat;
	double lon;
}Vertex;

typedef struct{
	Vertex * v1;
	Vertex * v2;
}Edge;

/*The central data structure representing a graph*/
typedef struct{
	#ifdef SERVER
		ObjectSlab * vertexSlab;
		ObjectSlab * edgeSlab;
	#else
		Vertex * vertices;
		Edge * edges;
	#endif
	
	char * graphName;
	int graphId;
}Graph;

/*Main function for creating a new graph*/
Graph * initGraph(char * graphName);

/*Get an update of this graph*/
Graph * updateGraph(char * graphName);

/*Add the vertices to the graph from JSON*/
int addVertices(Graph * graph, char * json);


/*Add the edges to the graph from JSON*/
int addEdges(Graph * graph, char * json);

#endif