
#ifndef MESSAGE_H
#define MESSAGE_H

#include <unistd.h>	//for u_int32_t

//DESCRIPTION: A generic function for sending a message to the graphkernel. It takes
//a message type as an argument (found in kernelMessage.h) and 
//a byte array as the payload
//ARGS: messageType: A 32bit unsigned int that indicates the purpose
//of this message to the kernel.
//		payload: The body of the message, usually json
int sendMessage(u_int32_t operationType, void * payload, u_int32_t length, unsigned char ** resultPayload);

unsigned char * buildMessage(u_int32_t operationType, char * payload, u_int32_t payloadLen);


void print_debug_bytes(void * ptr, int length);
void print_debug_ints(void * ptr, int length);

#endif