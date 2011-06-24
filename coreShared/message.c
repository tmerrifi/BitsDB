
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/kernelMessage.h"
#include "../include/global.h"
#include "message.h"

void print_debug_bytes(void * ptr, int length){
	unsigned char * tmpptr = (unsigned char *)ptr;
	for (int i = 0; i < length; ++i){
		printf("%d  ", *tmpptr++);	
	}	
	printf("\n\n\n");
}

void print_debug_ints(void * ptr, int length){
	int * tmpptr = (unsigned char *)ptr;
	for (int i = 0; i < length; ++i){
		printf("%d  ", *tmpptr++);	
	}	
	printf("\n\n\n");
}

//DESCRIPTION: A function to build a message. Puts a header on it and appends
//the payload to the end. This gets sent over ipc (socket) to the kernel.
//ARGS: operationType: A 32bit unsigned int that indicates the purpose
//of this message to the kernel.
//		payload: The body of the message, usually json
unsigned char * buildMessage(u_int32_t operationType, char * payload, u_int32_t payloadLen){
	unsigned char * message = malloc(payloadLen + sizeof(KernelMessageHeader));
	memset(message,0,payloadLen + sizeof(KernelMessageHeader));
	
	((KernelMessageHeader *)message)->operationType=operationType;
	((KernelMessageHeader *)message)->length=payloadLen;
	
	memcpy(message+sizeof(KernelMessageHeader), payload, payloadLen);
	return message;
}

//DESCRIPTION: A generic function for sending a message to the graphkernel. It takes
//a message type as an argument (found in kernelMessage.h) and 
//a byte array as the payload
//ARGS: operationType: A 32bit unsigned int that indicates the purpose
//of this message to the kernel.
//		payload: The body of the message, usually json
int sendMessage(u_int32_t operationType, void * payload, u_int32_t length, unsigned char ** resultPayload){
	
	int result=OP_FAILED;	//-1 means the operation has failed
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	//first get the socket, TCP for stream behavior
	if (sock < 0){
		perror("Error creating socket");
		return result;
	}
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");		//TODO: make this a param or something
	
	int conn_result = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));	//Connect to the server
    if (conn_result < 0){
    	perror("error connecting\n");
    	return result;	
    }
   
	unsigned char * message = (unsigned char *)buildMessage(operationType, payload, length);	//build the message header
	int sentBytes = send(sock, message, ((KernelMessageHeader *)message)->length + sizeof(KernelMessageHeader), 0);
	if (sentBytes > 0)	//the size is calculated from the payload size in the header
    {
    	printf("Message was sent %d\n", sentBytes);
    	unsigned char * resultBuffer = malloc(sizeof(KernelMessageHeader));
    	if (recv(sock, resultBuffer, sizeof(KernelMessageHeader), 0) > 0){
    		printf("Got a result\n");
    		int payloadLength = ((KernelMessageHeader *)resultBuffer)->length;
    		result = ((KernelMessageHeader *)resultBuffer)->operationType;
    		printf("payload length is %d and result is %d\n", payloadLength, result);
    		if (payloadLength > 0){
    			resultBuffer = malloc(payloadLength);
    			if (recv(sock, resultBuffer, payloadLength, 0) >= payloadLength){
    				printf("Got payload\n");
    				//print_debug_ints(resultBuffer, payloadLength/sizeof(int));
    				*resultPayload = resultBuffer;
    			}
    		}
    	}
    }
    
    close(sock);
	return result;
}