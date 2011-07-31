
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <string.h>		//strlen and strcat
#include <fcntl.h>
#include <stdio.h>
#include <semaphore.h>

#include "graphKernel/kernelMessage.h"
#include "graphKernel/graph.h"
#include "graphKernel/message.h"
#include "graphKernel/coreUtility.h"
#include "graphKernel/shmUtility.h"
#include "graphKernel/graphUtility.h"
#include "graphKernel/array.h"
#include "graphKernel/list.h"
#include "graphKernel/jsonProcessing.h"
#include "graphKernel/linkedList.h"
#include "graphKernel/attributes.h"

Graph * mainGraph = NULL;

/*Private functions...definitions here because of no forward definition*/
char * getFileBuffers(char * fileName);

int * addVerticesInCore(Graph * graph, int count);

AttributeCollection * attribute_findAttribute(Graph * graph, char * attributeName);

int findAttributeByName(void * collectionPtr, void * targetName);

/********* PUBLIC FUNCTIONS ***************************/

Graph * graph_init(char * graphName){
	char * verticesSharedMemoryId=graphUtil_getVertexName(graphName);
	char * edgeSharedMemoryId=graphUtil_getEdgesName(graphName);
	
	if (!mainGraph){
		int result = sendMessage(MESSAGE_INIT_GRAPH,graphName,strlen(graphName), NULL);
		if (result == OP_SUCCEESS){
			mainGraph=malloc(sizeof(Graph));
			mainGraph->graphName = malloc(strlen(graphName));
			strncpy(mainGraph->graphName, graphName, strlen(graphName));
			//mainGraph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CLIENT);
			//mainGraph->neighbors = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Neighbor), 5, SHM_CLIENT);
			mainGraph->attributes = linkedList_init();		//attributes is a generic linked list
			mainGraph->lock = sem_open( graph_getSemName(mainGraph->graphName),
										O_CREAT, 0777, 1 );
			graph_getSnapShot(mainGraph);
		}
	}
	return mainGraph;
}

int addVertices(Graph * graph, char * jsonFileName){
	char * verticesJson = NULL;		//pointers to buffers that will store the json strings
	if (jsonFileName){									//try to get both (or either) from the files supplied
		verticesJson = getFileBuffers(jsonFileName);
		json_parseArrayOfObjectAndFindAttributes(verticesJson, graph,attribute_findAttribute,addVerticesInCore);
	}	
}

int addNeighbor(Graph * graph, int v1, int v2){
	//build message
	unsigned char * message = malloc(sizeof(MessageType_InsertNeighbor));
	((MessageType_InsertNeighbor *)message)->v1=v1;
	((MessageType_InsertNeighbor *)message)->v2=v2;
	int result = sendMessage(MESSAGE_INSERT_NEIGHBOR, message, sizeof(MessageType_InsertNeighbor), NULL);
	if (result == OP_SUCCEESS){
		return 1;	
	}
	else{
		return 0;	
	}
}

/***** PRIVATE FUNCTIONS**********/

int * addVerticesInCore(Graph * graph, int count){
	//the purpose of this is to add vertices to the core and return a list of indices
	unsigned char * resultBuffer;
	int result = sendMessage(MESSAGE_INSERT_VERTICES,&count, sizeof(int), &resultBuffer);
	int * indices = NULL;
	if ( result == OP_SUCCEESS ){
		indices = (int *)resultBuffer;
	}
	
	return indices;
}

AttributeCollection * attribute_findAttribute(Graph * graph, char * attributeName){
	AttributeCollection * returnCollection = NULL;
	if (graph){
		LinkedListNode * foundNode = linkedList_find(graph->attributes, coreUtil_concat(graph->graphName, attributeName), findAttributeByName);
		if (foundNode){
			returnCollection = (AttributeCollection *)foundNode->payload;
		}
	}
	
	return returnCollection;
}

int findAttributeByName(void * collectionPtr, void * targetName){
	return linkedList_compare_string( ((AttributeCollection *)collectionPtr)->attributeName, targetName);
}

void graph_getSnapShot(Graph * graph){
		
	sem_wait(graph->lock);
	char * verticesSharedMemoryId=graphUtil_getVertexName(graph->graphName);
	char * edgeSharedMemoryId=graphUtil_getEdgesName(graph->graphName);
	graph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CLIENT);
	graph->neighbors = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Neighbor), 5, SHM_CLIENT);
	
	//array_getNextValidObjectFromIndex(Array * array, int * index, int keepGoingFlag);
	
	Array * newVertices = array_copy(graph->vertices);
	Lists * newNeighbors = lists_copy(graph->neighbors);
	
	array_close(mainGraph->vertices);	//closing these guys unmaps the memory
	array_close((Array *)mainGraph->neighbors);
	
	graph->vertices = newVertices;
	graph->neighbors = newNeighbors;
	
	/*int vertexBytes = collection_getSizeInBytes((Collection *)mainGraph->vertices);
	int neighborsBytes = collection_getSizeInBytes((Collection *)mainGraph->neighbors);
	//perform copy
	void * vertices_mem = malloc(vertexBytes);
	void * neighbors_mem = malloc(neighborsBytes);
	
	memcpy(vertices_mem, mainGraph->vertices->base.mem, vertexBytes);
	memcpy(neighbors_mem, ((Array *)mainGraph->neighbors)->base.mem, neighborsBytes);
	
	Array * vertices = malloc(sizeof(Array));
	List * neighbors = malloc(sizeof(List));
	
	memcpy(vertices, */
	//now we need to unmap our regions
	//mainGraph->vertices->base.mem = newVertices;	//ok, set these back
	//((Array *)mainGraph->neighbors)->base.mem = newNeighbors;
	sem_post(graph->lock);
}

//DESCRIPTION: A helper function for reading in a file and filling up a buffer
char * getFileBuffers(char * fileName){
	char * buff = NULL;
	FILE * file = fopen(fileName,"r");
	if (file){
		/*first get file size*/
		fseek(file,0,SEEK_END);
		int size=ftell(file);
		buff = malloc(size);
		fseek(file,0,0);
		fread(buff, 1, size, file);			
	}
	return buff;
}


