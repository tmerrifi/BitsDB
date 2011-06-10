
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <string.h>		//strlen and strcat
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "graphKernel/graph.h"
#include "jsonProcessing.h"
#include "graphKernel/graphUtility.h"
#include "graphKernel/coreUtility.h"
#include "graphKernel/array.h"
#include "graphKernel/list.h"

Graph * mainGraph = NULL;	//TODO: Get rid of this eventually, we'll need a list (or hash table) of graphs

/*********PRIVATE FUNCTION PROTOTYPES****************************/

void * createVertex(yajl_val node);							//create a new vertex from a json node
int addObjectToArray(void * collection, void * object);		//add an object to the objectSlab

/*********PUBLIC FUNCTIONS***************************************/

//DESCRIPTION: This function takes a bunch of vertices in json form and adds them to the graph
int addVertices(Graph * graph, char * json){
	//TODO: eventually take a graph pointer, for now since we only have one graph just use "mainGraph"
	 return json_addFromList(json, mainGraph->vertices, createVertex, addObjectToArray);
}

//DESCRIPTION: This function initializes a brand new graph. If the graph (identified by the identifier graphName)
//already exists then an error is thrown.
//Args: graphName - an unique string identifier for this graph
Graph * initGraph(char * graphName){
	if (!mainGraph){
		mainGraph = malloc(sizeof(Graph));	//TODO: just one graph for now, maybe support several eventually
		
		char * verticesSharedMemoryId=graphUtil_getVertexName(graphName);
		char * edgeSharedMemoryId=graphUtil_getEdgesName(graphName);
		
		mainGraph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CORE);
		mainGraph->edges = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Edge), 5, SHM_CORE);
		
		//mainGraph->vertexSlab=initObjectSlab(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5);
		//mainGraph->edgeSlab=initObjectSlab(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Edge), 5);
	}
	return mainGraph;
}


/*******PRIVATE FUNCTIONS**************/

//DESCRIPTION: Takes a single vertex node from the json tree and creates a new vertex
//ARGS: yajl_val node : A vertex node in json
void * createVertex(yajl_val node){
	Vertex * newVertex = malloc(sizeof(Vertex));
	for (int i = 0; i < 3; ++i){
		yajl_val values = node->u.array.values[i];
		switch(i){
				case 0:
						break;
				case 1: newVertex->lat=values->u.number.d;
						break;
				case 2: newVertex->lon=values->u.number.d;
						break;
		}
	}
	return newVertex;
}

//DESCRIPTION: This is passed to the json processing code so it is not coupled with the object slab.
//ARGS: collection: the objectSlab
//		object: the object to be added to the collection
int addObjectToArray(void * collection, void * object){
		return (array_addObject((Array *)collection, object)) ? 1 : 0;
}