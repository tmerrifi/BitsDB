#include <stdlib.h>
#include <stdio.h>
#include "graphKernel/graph.h"
#include "graphKernel/array.h"

int main(){
	Graph * graph = initGraph("testBitsDB");
	addVertices(NULL, "vertices");
	Vertex * v = array_getById(graph->vertices, 1);
	//printf("lat: %lf, long: %lf\n", v->lat, v->lon);
	createAttribute(graph, graph->vertices, "label");
	addAttribute(graph, v, "label", "a fake label");
}