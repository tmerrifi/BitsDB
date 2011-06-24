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

#include "../include/graph.h"
#include "../include/kernelMessage.h"
#include "graphKernel/message.h"
#include "../include/global.h"
#include "jsonProcessing.h"

#ifndef FD_COPY
#define FD_COPY(f, t)   (void)(*(t) = *(f))
#endif

typedef struct{
	int socket;
	KernelMessageHeader * messageHeader;
	void * messagePayload;
	int payloadLength;
	int messageToSend;
}ClientMessage;

ClientMessage theClient;

//a generic funtion to check if an error has occured, print it out, and exit.
static void checkAndExit(int fd, char * errorMsg){
	if (fd < 0){
		perror(errorMsg);
		exit(1);	
	}	
}

//reusing the addr allows us to restart the server quickly even if connection are still active on the socket.
//The site gives a good explanation: http://www.unixguide.net/network/socketfaq/4.5.shtml
void setupReuseAddr(int server_sock){
	int yes=1;
	int result = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	checkAndExit(result, "SERVER: Error setting reuse addr setting on socket");
}

struct sockaddr_in * setupSocket(){
	struct sockaddr_in * addr = malloc(sizeof(struct sockaddr_in)); 	// internet socket address data structure
	addr->sin_family = AF_INET;
	addr->sin_port = htons(SERVER_PORT); // byte order is significant
	addr->sin_addr.s_addr = INADDR_ANY; // listen to all interfaces	
	return addr;
}

//DESCRIPTION: 
void demultiplexOperation(ClientMessage * clientMessage){
	int insert_counter=0;
	int operationType=clientMessage->messageHeader->operationType;
	int * indices = NULL;
	int result = 0;
	clientMessage->messageHeader=calloc(1,sizeof(KernelMessageHeader));		//reallocate a messageHeader to send back
	clientMessage->messageToSend=1;											//mark it ready to send
	
	switch(operationType){
		case MESSAGE_INIT_GRAPH:
			if (initGraph((char *)(clientMessage->messagePayload))){	//now initialize the graph...for create, the payload is just the name
				printf("Successfully created graph!\n");
				clientMessage->messageHeader->operationType=OP_SUCCEESS;
				clientMessage->messagePayload = NULL;
				clientMessage->payloadLength = 0;
			}
			else
			{
				printf("Creation failed\n");	
				clientMessage->messageHeader->operationType=OP_FAILED;
			}
			break;
		case MESSAGE_INSERT_VERTICES:
			insert_counter = addVerticesFromCount(NULL, *((u_int32_t *)(clientMessage->messagePayload)), &indices);
			if (insert_counter > 0){	//now initialize the graph...for create, the payload is just the name
				printf("Successfully add %d vertices\n", insert_counter);
				clientMessage->messageHeader->operationType=OP_SUCCEESS;
				clientMessage->messagePayload = indices;
				clientMessage->payloadLength = insert_counter * sizeof(int);
			}
			else
			{
				printf("Failed to add vertices\n");
				clientMessage->messageHeader->operationType=OP_FAILED;	
			}
			break;
		case MESSAGE_INSERT_NEIGHBOR:
			result = addNeighbor(NULL, 
							((MessageType_InsertNeighbor *)clientMessage->messagePayload)->v1, 
							((MessageType_InsertNeighbor *)clientMessage->messagePayload)->v2);
			if (result){
				clientMessage->messageHeader->operationType=OP_SUCCEESS;
				clientMessage->messagePayload = NULL;
				clientMessage->payloadLength = 0;
			}
			else{
				printf("Insert Neighbor Failed\n");
				clientMessage->messageHeader->operationType=OP_FAILED;
			}
		default:
			printf("Nutin\n");	
	}
}

void printOutBytes(unsigned char * b, int length){
	for (int i = 0; i < length; ++i){
		printf("%d\n", *(b+i));	
	}
	printf("Done\n");
}

//DESCRIPTION: The main server function that listens for requests over a raw IP socket.
//Args:None
void server(){
	int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	checkAndExit(server_sock,"SERVER: Error creating socket");
	
	struct sockaddr_in * addr=setupSocket();
	int res = bind(server_sock, (struct sockaddr*)addr, sizeof(struct sockaddr));
	checkAndExit(res,"SERVER: Error binding socket to address.");
	
	setupReuseAddr(server_sock);
	
	/* workaround for funny OS X bug - need to start listening without select */
	res=fcntl(server_sock, F_SETFL, O_NONBLOCK);
  	if(res<0) { perror("fcntl"); } 
	if (listen (server_sock, 1) < 0) { perror ("listen"); exit(1); } 
	fcntl(server_sock, F_SETFL, 0);
	
	/* initializing data structure for select call */
	fd_set readset, writeset;
	FD_ZERO(&readset);	//zero out the fd set
	FD_ZERO(&writeset);
	FD_SET(server_sock,&readset);
	
	while(1){
		fd_set rdyset, wset;
		FD_COPY(&readset,&rdyset);					//we need to keep around the original set and have this copy to see who is ready
		FD_COPY(&writeset,&wset);
		int rdy = select(FD_SETSIZE,&rdyset,&wset,0,0);	//wait for something to happen	
		if(FD_ISSET(server_sock,&rdyset)) {	//we have a new message
			printf("got a connection\n");
			int sock;
			struct sockaddr_in remote_addr;
			unsigned int socklen = sizeof(remote_addr); 
			sock = accept(server_sock, (struct sockaddr*)&remote_addr, &socklen);
			checkAndExit(sock,"SERVER: Error getting socket for the client.");
			theClient.socket=sock;
			printf("newsocket is %d\n", sock);
			FD_SET(sock,&readset);
			FD_SET(sock,&writeset);
		}
		if(FD_ISSET(theClient.socket,&rdyset)) {
			printf("socket is %d\n", theClient.socket);
			int bufferSize = (theClient.messageHeader != NULL) ? theClient.messageHeader->length : sizeof(KernelMessageHeader);
			unsigned char * buf=malloc(bufferSize);
			memset(buf,0,bufferSize);
			int rec_count = recv(theClient.socket,buf,bufferSize,0);
			printf("rec_count %d, buffersize %d\n", rec_count, bufferSize);
			if (rec_count > 0 && theClient.messageHeader == NULL){			//we got the header
				theClient.messageHeader = (KernelMessageHeader *)buf;
			}
			else if (rec_count > 0){		
				theClient.messagePayload=buf;									//we got the payload
				demultiplexOperation(&theClient);
				free(buf);
				printf("done with read\n");
			}
			else{														//need to shut down the socket
				theClient.messageHeader=NULL;
				close(theClient.socket);
				FD_CLR(theClient.socket,&readset);
				FD_CLR(theClient.socket,&writeset);
				printf("closed connection\n");
			}
		}
		if (FD_ISSET(theClient.socket,&wset)){
			if (theClient.messageToSend){
				unsigned char * theMessage = 
					buildMessage(theClient.messageHeader->operationType, (char *)theClient.messagePayload, theClient.payloadLength);
				
				int send_count = send(theClient.socket,theMessage,theClient.payloadLength  + sizeof(KernelMessageHeader),0);
				if (send_count >= 0){
						print_debug_ints(theMessage + sizeof(KernelMessageHeader), theClient.payloadLength);
						free(theClient.messageHeader);
						theClient.messageHeader=NULL;
						theClient.messageToSend=0;
						printf("sent result of size %d\n", send_count);
				}
			}
		}
	}
}

int main(){
	printf("started server\n");
	memset(&theClient,0,sizeof(ClientMessage));
	server();
}