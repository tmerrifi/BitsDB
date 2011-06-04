
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

//DESCRIPTION: A function to build a message. Puts a header on it and appends
//the payload to the end. This gets sent over ipc (socket) to the kernel.
//ARGS: operationType: A 32bit unsigned int that indicates the purpose
//of this message to the kernel.
//		payload: The body of the message, usually json
static unsigned char * buildMessage(u_int32_t operationType, char * payload){
	u_int32_t payloadLen=strlen(payload);
	printf("%s, %d\n", payload, payloadLen);
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
unsigned char * sendMessage(u_int32_t operationType, char * payload){
	
	unsigned char * result=NULL;	//-1 means the operation has failed
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
   
	unsigned char * message = (unsigned char *)buildMessage(operationType, payload);	//build the message header
	if (send(sock, message, ((KernelMessageHeader *)message)->length + sizeof(KernelMessageHeader), 0) > 0)	//the size is calculated from the payload size in the header
    {
    	printf("Message was sent\n");
    	unsigned char * resultBuffer = malloc(sizeof(KernelMessageHeader));
    	if (recv(sock, resultBuffer, sizeof(KernelMessageHeader), 0) > 0){
    		printf("Got a result\n");
    		result=resultBuffer;
    	}
    }
    
    close(sock);
	return result;
}