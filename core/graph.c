
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <string.h>		//strlen and strcat
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

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
	graph=mainGraph;
	sem_wait(graph->lock);
	int counter=0;
	int * _indices = malloc(sizeof(int) * count);	//we need to return the indices to the calling client
	for (int i = 0; i < count; ++i){
		Vertex * newVertex = malloc(sizeof(Vertex));
		newVertex->neighborsPtr = 0;
		void * newArrayObject = array_addObject(graph->vertices, newVertex);	//returns 0 on fail, 1 on success so we can add it up
		if (newArrayObject){
			int index = array_getIndex(graph->vertices, newArrayObject);
			printf("index is %d\n", index);
			_indices[i]=index;													//add our index to the array
			++counter;
		}
	}
	*indices = _indices;	//so we can return our array
	sem_post(graph->lock);
	return counter;
}

//DESCRIPTION: This function initializes a brand new graph. If the graph (identified by the identifier graphName)
//already exists then an error is thrown.
//Args: graphName - an unique string identifier for this graph
Graph * graph_init(char * graphName){
	
	char * verticesSharedMemoryId=graphUtil_getVertexName(graphName);
	char * edgeSharedMemoryId=graphUtil_getEdgesName(graphName);
	
	array_deleteSegment(verticesSharedMemoryId);	//TODO: TEMPORARY for testing's sake!!!!!
	lists_deleteSegment(edgeSharedMemoryId);
	
	mainGraph = malloc(sizeof(Graph));	//TODO: just one graph for now, maybe support several eventually
	mainGraph->graphName = malloc(strlen(graphName));
	strncpy(mainGraph->graphName, graphName, strlen(graphName)); 
	mainGraph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CORE);
	mainGraph->neighbors = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Neighbor), 5, SHM_CORE);
	mainGraph->lock = sem_open( graph_getSemName(mainGraph->graphName),
										O_CREAT, 0777, 1 );
	//get the value of the semaphore to make sure it's not zero
	int sem_value = 0;
	sem_getvalue(mainGraph->lock, &sem_value);
	if (sem_value == 0){
		sem_post(mainGraph->lock); //we can't use this semaphore if it's zero, so increment it	
	}
	
	return mainGraph;
}

int addNeighbor(Graph * graph, int v1, int v2){
	graph=mainGraph;
	sem_wait(graph->lock);
	List * list;
	Vertex * tmpV1 = array_getById(graph->vertices, v1);
	int result = 0;
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
		result = 1;
	}
	
	sem_post(graph->lock);
	return result;
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