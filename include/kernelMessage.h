
#ifndef KERNEL_MESSAGE
#define KERNEL_MESSAGE

#include <unistd.h>
#include <string.h>

#define MESSAGE_INIT_GRAPH 1
#define	MESSAGE_INSERT_VERTICES 2
#define MESSAGE_INSERT_EDGES 3

#define OP_FAILED 0
#define OP_SUCCEESS 1

typedef struct {
	u_int32_t graphId;			//the integer id representing this graph
	u_int32_t operationType;	//what is the purpose of this message?
	u_int32_t length;			//how many bytes long is this message?
	u_int32_t result;			//the result of the message?
	u_int32_t vertexCount;
	u_int32_t edgeCount;
}KernelMessageHeader;

#endif