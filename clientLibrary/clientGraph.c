
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <string.h>		//strlen and strcat
#include <fcntl.h>
#include <stdio.h>

#include "graphKernel/kernelMessage.h"
#include "graphKernel/graph.h"
#include "message.h"
#include "graphKernel/coreUtility.h"
#include "graphKernel/graphUtility.h"
#include "graphKernel/array.h"
#include "graphKernel/list.h"

Graph * mainGraph = NULL;

/*Private functions...definitions here because of no forward definition*/
char * getFileBuffers(char * fileName);

/********* PUBLIC FUNCTIONS ***************************/

Graph * initGraph(char * graphName){
	char * verticesSharedMemoryId=graphUtil_getVertexName(graphName);
	char * edgeSharedMemoryId=graphUtil_getEdgesName(graphName);
	
	if (!mainGraph){
		KernelMessageHeader * resultHeader = (KernelMessageHeader *)sendMessage(MESSAGE_INIT_GRAPH,graphName);
		if (resultHeader && resultHeader->result == OP_SUCCEESS){
			mainGraph=malloc(sizeof(Graph));
			mainGraph->vertices = array_init(verticesSharedMemoryId, (void *)0xF0000000, sizeof(Vertex), 5, SHM_CLIENT);
			mainGraph->edges = lists_init(edgeSharedMemoryId, (void *)0xD0000000, sizeof(Edge), 5, SHM_CLIENT);
		}
	}
	return mainGraph;
}

int addVertices(Graph * graph, char * jsonFileName){
	char * verticesJson = NULL;		//pointers to buffers that will store the json strings
	if (jsonFileName){									//try to get both (or either) from the files supplied
		verticesJson = getFileBuffers(jsonFileName);
		if (!verticesJson){
			perror("Import of json for vertices failed\n");	
		}
		else{	//import those bad boys!
			sendMessage(MESSAGE_INSERT_VERTICES,verticesJson);
		}
	}	
}

int addEdges(Graph * graph, char * jsonFileName){
	char * edgesJson = NULL;
	if (jsonFileName){
		edgesJson = getFileBuffers(jsonFileName);
		if (!edgesJson){
			perror("Import of json for vertices failed\n");	
		}
	}	
}

/***** PRIVATE FUNCTIONS**********/

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


