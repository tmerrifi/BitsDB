#include <stdlib.h>
#include <stdio.h>
#include "graphKernel/graph.h"
#include "graphKernel/array.h"
#include "graphKernel/attributes.h"

int main(){
	Graph * graph = graph_init("testBitsDB");
	AttributeCollection * attMyInt = attribute_init(graph, graph->vertices, "myInt", DOUBLE_TYPE);	
	addVertices(graph, "vertices");
	//graph_getSnapShot(graph);
	addNeighbor(graph, 1, 2);
	addNeighbor(graph, 2, 3);
	addNeighbor(graph, 3, 1);
	
	Vertex * v;
	ARRAY_FOR_EACH(graph->vertices,v){		//this should error 
		printf("attribute value is %lf\n", 	attribute_getDouble(attMyInt, v));
		if (v->neighborsPtr){
			Neighbor * n;
			List * list = lists_getListByIndex(graph->neighbors, v->neighborsPtr);
			LISTS_FOR_EACH(graph->neighbors,list,n){
				printf("this neighbor is : %d\n", n->destVertex);
			}		
		} 
	}
}