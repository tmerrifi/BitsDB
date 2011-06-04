
/*The central data structure representing a graph*/
typedef struct _Graph{
	Vertex * vertices;
	Edge * edges;
}Graph;

Graph * theGraph = NULL;

/*Main function for creating a new graph*/
Graph * initGraph(char * graphName);

/*Add the vertices to the graph from JSON*/
int addVertices(Graph * graph, char * verticesJson);


/*Add the edges to the graph from JSON*/
int addEdges(Graph * graph, char * verticesJson);