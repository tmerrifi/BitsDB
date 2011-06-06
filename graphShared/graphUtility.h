
#ifndef GRAPH_UTILITY_H

#define GRAPH_UTILITY_H

//DESCRIPTION: we need to concat the name of the graph with "vertices". This is how we will get the file
//or the shared memory region where the vertices are stored.
//ARGS: graphName: The name of the graph
char * graphUtil_getVertexName(char * graphName);

//DESCRIPTION: we need to concat the name of the graph with "edges". This is how we will get the file
//or the shared memory region where the edges are stored.
//ARGS: graphName: The name of the graph
char * graphUtil_getEdgesName(char * graphName);


#endif
