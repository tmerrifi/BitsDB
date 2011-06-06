
#include <string.h>
#include <stdlib.h>

//DESCRIPTION: we need to concat the name of the graph with "vertices". This is how we will get the file
//or the shared memory region where the vertices are stored.
//ARGS: graphName: The name of the graph
char * graphUtil_getVertexName(char * graphName){
	char * verticesSharedMemoryId=malloc(strlen(graphName)+strlen("_vertices"));
	strncpy(verticesSharedMemoryId,graphName,strlen(graphName));
	strncat(verticesSharedMemoryId,"_vertices",strlen(graphName)+strlen("_vertices"));
	return verticesSharedMemoryId;
}


//DESCRIPTION: we need to concat the name of the graph with "edges". This is how we will get the file
//or the shared memory region where the edges are stored.
//ARGS: graphName: The name of the graph
char * graphUtil_getEdgesName(char * graphName){
	char * edgeSharedMemoryId=malloc(strlen(graphName)+strlen("_edges"));	
	strncpy(edgeSharedMemoryId,graphName,strlen(graphName));
	strncat(edgeSharedMemoryId,"_edges",strlen(graphName)+strlen("_edges"));	//create a unique id for the edge shared memory
	return edgeSharedMemoryId;
}