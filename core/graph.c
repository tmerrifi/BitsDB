
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
#include "graphKernel/shmUtility.h"

Graph * mainGraph = NULL;	//TODO: Get rid of this eventually, we'll need a list (or hash table) of graphs

/*********PRIVATE FUNCTION PROTOTYPES****************************/

void * createVertex(yajl_val node);							//create a new vertex from a json node
int addObjectToArray(void * collection, void * object);		//add an object to the objectSlab

/*********PUBLIC FUNCTIONS***************************************/

int addVerticesFromCount(Graph * graph, u_int32_t count, int ** indices){
	int counter=0;
	int * _indices = malloc(sizeof(int) * count);	//we need to return the indices to the calling client
	for (int i = 0; i < count; ++i){
		Vertex * newVertex = malloc(sizeof(Vertex));
		newVertex->neighborsPtr = 0;
		void * newArrayObject = array_addObject(mainGraph->vertices, newVertex);	//returns 0 on fail, 1 on success so we can add it up
		if (newArrayObject){
			int index = array_getIndex(mainGraph->vertices, newArrayObject);
			printf("index is %d\n", index);
			_indices[i]=index;													//add our index to the array
			++counter;
		}
	}
	*indices = _indices;	//so we can return our array
	return counter;
}

//DESCRIPTION: This function initializes a brand new graph. If the graph (identified by the identifier graphName)
//already exists then an error is thrown.
//Args: graphName - an unique string identifier for this graph
Graph * initGraph(char * graphName){
	
	char * verticesSharedMemoryId=graphUtil_getVertexName(graphName);
	char * edgeSharedMemoryId=graphUtil_getEdgesName(graphName);
	
	array_deleteSegment(verticesSharedMemoryId);	//TODO: TEMPORARY for testing's sake!!!!!
	lists_deleteSegment(edgeSharedMemoryId);
	
	mainGraph = malloc(sizeof(Graph));	//TODO: just one graph for now, maybe support several eventually
		
	mainGraph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CORE);
	mainGraph->neighbors = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Neighbor), 5, SHM_CORE);
	
	return mainGraph;
}

int addNeighbor(Graph * graph, int v1, int v2){
	graph=mainGraph;
	List * list;
	Vertex * tmpV1 = array_getById(graph->vertices, v1);
	if (tmpV1->neighborsPtr){
		list = lists_getListByIndex(graph->neighbors, index);
	}
	else{
		list = lists_addList(graph->neighbors);
		tmpV1->neighborsPtr = lists_getIndex(graph->neighbors, list);
	}
	Neighbor * neighbor = malloc(sizeof(Neighbor));
	if (list && neighbor){
		neighbor->destVertex = v2;
		lists_addObjectToList(graph->neighbors, list, neighbor);
		return 1;
	}
	else{
		return 0;	
	}
}

/*******PRIVATE FUNCTIONS**************/

//DESCRIPTION: Takes a single vertex node from the json tree and creates a new vertex
//ARGS: yajl_val node : A vertex node in json
/*void * createVertex(yajl_val node){
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
}*/