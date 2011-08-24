
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "shmUtility.h"

int pageSize = 0x1000;

void printItOut(void * mem){
	for (int i = 0; i < 10000; ++i){
		if (i % 1000 == 0){
			printf("output is %d\n", *( ((int *)mem) + i ));
		}
	}
}



void writeToPage(unsigned char * mem, int pageId){
	unsigned char * newPtr = mem + (pageSize * pageId + 10);	//the 10 is just to be sure ;)
	*newPtr = pageId + 10;										//just something to put in there to trigger a page fault
}

int main(int argc, char ** argv){
	
	int num = 555;
	
	char path[100];
	char pathTwo[100];
	
	strcpy(path, "/sys/kernel/debug/tracing/tracing_on");
	strcpy(pathTwo, "/sys/kernel/debug/tracing/trace_marker");
	
	int trace_fd = open(path, O_WRONLY);
	int marker_fd = open(pathTwo, O_WRONLY);
	
	write(trace_fd, "1", 1);
	write(marker_fd, "In critical area\n", 17);
	if (argc > 1){
		num = atoi(argv[1]);
	}
	
	int length = pageSize * 4;
	
	void * mem = coreUtil_openSharedMemory( argv[2] , (void *)0xA0000000, length, SHM_CORE, NULL);
	writeToPage((unsigned char *)mem, 0);
	//munmap((void *)0xA0000000, length);
	msync(mem, length, MS_SYNC);
	
	
	write(marker_fd, "Out 2critical area 2\n", 18);
	lseek(trace_fd, 0, SEEK_SET);
	printf("pid: %d\n", getpid());
	write(trace_fd, "0", 1);
	//sleep(10);
	/*printItOut(mem);
	for (int i = 0; i < 10000; ++i){
		if (i % 1000 == 0){
			printf("hit %d\n", i);
			sleep(1);	
		}
		*( ((int *)mem) + i ) = num;
	}
	
	printItOut(mem);*/
}
