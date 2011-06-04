



#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <string.h>		//strlen and strcat
#include <fcntl.h>
#include <stdio.h>

#include "graphKernel/kernelMessage.h"
#include "graphKernel/graph.h"
#include "message.h"

Graph * mainGraph = NULL;

/*Private functions...definitions here because of no forward definition*/
Graph * createNewGraph(char * graphName);
Graph * loadExistingGraph(char * verticesSharedMemoryId, char * edgeSharedMemoryId);
char * getFileBuffers(char * fileName);


/********* PUBLIC FUNCTIONS ***************************/

Graph * initGraph(char * graphName){
	
	char * edgeSharedMemoryId=malloc(strlen(graphName)+strlen("_edges"));
	char * verticesSharedMemoryId=malloc(strlen(graphName)+strlen("_vertices"));
	
	strncpy(edgeSharedMemoryId,graphName,strlen(graphName));
	strncpy(verticesSharedMemoryId,graphName,strlen(graphName));
	
	strncat(edgeSharedMemoryId,"_edges",strlen(graphName)+strlen("_edges"));	//create a unique id for the edge shared memory
	strncat(verticesSharedMemoryId,"_vertices",strlen(graphName)+strlen("_vertices"));	//create a unique id for the vertices shared memory
	
	KernelMessageHeader * resultHeader = (KernelMessageHeader *)sendMessage(MESSAGE_INIT_GRAPH,graphName);
	if (resultHeader && resultHeader->result == OP_SUCCEESS){
		
	}
	
	loadExistingGraph(verticesSharedMemoryId,edgeSharedMemoryId);
	
	return NULL;
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


Graph * loadExistingGraph(char * verticesSharedMemoryId, char * edgeSharedMemoryId){
	Graph * graph = malloc(sizeof(Graph));	//TODO: just one graph for now, maybe support several eventually
		
	int edgeFd=0;
	int vertexFd=0;
	
	void * edgeMem=NULL;
	void * vertexMem=NULL;    
	      
	if ((vertexFd = shm_open(verticesSharedMemoryId, O_CREAT | O_RDWR, 0777)) > 0){	//start by getting a file descriptor for the shared memory, must be new or we throw an error
		vertexMem = mmap((void *)0xA0000000,1024,PROT_READ|PROT_WRITE,MAP_SHARED,vertexFd,0);	//lets map the file into our address space
		//printf("%p %p", vertexFd, vertexMem);
	}
	
	if ( (edgeFd = shm_open(edgeSharedMemoryId, O_CREAT | O_RDWR, 0777)) > 0){	//start by getting a file descriptor for the shared memory, must be new or we throw an error
		edgeMem = mmap((void *)0x8000, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, edgeFd, 0);	//lets map the file into our address space
	}
	
	if (vertexMem){
		mainGraph = malloc(sizeof(Graph));
		mainGraph->vertices=vertexMem;
		mainGraph->edges=edgeMem;
		return graph;
	}
	else {
		perror("A Graph initialization error occured");
	}
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


