#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "yajl/yajl_tree.h"

typedef struct _Vertex{
	char * label;
	double lat;
	double lon;
}Vertex;

Vertex * createVertex(yajl_val node){
	Vertex * newVertex = malloc(sizeof(Vertex));
	for (int i = 0; i < 3; ++i){
		yajl_val values = node->u.array.values[i];
		switch(i){
				case 0: newVertex->label=values->u.string;
						break;
				case 1: newVertex->lat=values->u.number.d;
						break;
				case 2: newVertex->lon=values->u.number.d;
						break;
		}
	}	
	return newVertex;
}

int main()
{
	char errbuf[1024];
	FILE * file = fopen("vertices","r");
	/*first get file size*/
	fseek(file,0,SEEK_END);
	int size=ftell(file);
	char * buff = malloc(size);
	fseek(file,0,0);
	fread(buff, 1, size, file);
	printf("done with initial read in\n"); 
    /* null plug buffers */
    errbuf[0] = 0;
	yajl_val node;
	if ( node = yajl_tree_parse((const char *) buff, errbuf, sizeof(errbuf))){	//get this party started with some json parsing
		if (node->type == yajl_t_array){
				int length = node->u.array.len;
				for (int i = 0; i < length; ++i){
					if (i % 10000 == 0)
						printf("%d\n", i);
					Vertex * v = createVertex(node->u.array.values[i]);
					//printf("%s, %lf, %lf\n", v->label, v->lat, v->lon);
				}
			}
	}
	free(buff);
	
}