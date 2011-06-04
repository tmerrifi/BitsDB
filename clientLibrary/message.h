
#ifndef MESSAGE_H
#define MESSAGE_H

#include <unistd.h>	//for u_int32_t

//DESCRIPTION: A generic function for sending a message to the graphkernel. It takes
//a message type as an argument (found in kernelMessage.h) and 
//a byte array as the payload
//ARGS: messageType: A 32bit unsigned int that indicates the purpose
//of this message to the kernel.
//		payload: The body of the message, usually json
unsigned char * sendMessage(u_int32_t operationType, char * payload);


#endif