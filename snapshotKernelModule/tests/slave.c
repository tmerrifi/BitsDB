
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "shmUtility.h"
#include "ftrace.h"

#define pageSize 0x1000

#define length (0x1000 * 4)

void writeToByte(unsigned char * mem, int pageId, int byteId, int num){
	*( (int *)(mem + ((pageSize * pageId) + byteId)) ) = num;
}

int getInteger(void * mem, int pageId, int byteId){
	return *( (int *)(mem + ((pageSize * pageId) + byteId)) );
}

void call_msync(void * mem, struct ftracer * tracer, char * message){
	ftrace_on(tracer);
	ftrace_write_to_trace(tracer,message);
	msync(mem,length, MS_SYNC);
	ftrace_write_to_trace(tracer,"SNAP: SLAVE DONE\n");
	ftrace_off(tracer);
}

void start_ftrace(struct ftracer * tracer, char * message){
	ftrace_on(tracer);
	ftrace_write_to_trace(tracer,message);
}

void end_ftrace(struct ftracer * tracer){
	ftrace_write_to_trace(tracer,"SNAP: SLAVE: DONE\n");
	ftrace_off(tracer);
}

void printItOut(void * mem){
	for (int i = 0; i < (length/sizeof(int)); ++i){
		if (i % 500 == 0){
			printf("%d  %p ", *( ((int *)mem) + i ), mem + i);
		}
	}
}

int simpleTest(){
	int testInt = 0;
	struct ftracer * tracer = ftrace_init();
	
	void * mem = coreUtil_openSharedMemory( "snapshot_test1" , (void *)0xA0000000, length, SHM_CLIENT, NULL);	
	sleep(2);	//waiting for master to write to page 1
	
	//start_ftrace(tracer, "Starting read!!!\n");
	testInt = getInteger(mem, 1, 12);
	//end_ftrace(tracer);
	
	printf("slave: initial test int is %d\n", testInt);
	printf("hello there!!!\n");
	call_msync( mem, tracer, "\n\n\nSLAVE msync 1");
	printf("hello there 2!!!\n");
	testInt = getInteger(mem, 1, 12);
	printf("huh? %d\n");
	if (testInt != 55){
		printf("ERROR: Failure on simpleTest after msync, testInt is %d\n\n", testInt);
		return 0;	
	}
	printf("slave: after first msync, test int is %d\n", testInt);
	
	/*sleep(6);	//waiting for master to write to page 1
	msync(mem, length, MS_SYNC);
	testInt = getInteger(mem, 1, 12);
	printf("slave: after second msync, test int is %d\n", testInt);
	if (testInt != 30){
		printf("ERROR: Failure on simpleTest on second test\n\n");
		return 0;
	}*/

	munmap((void *)0xA0000000, length);	//unmap it!
	return 1;
}

int main(int argc, char ** argv){
	if (strncmp(argv[1], "1", 1) == 0) {
		printf("\nTest 1 : %s \n", (simpleTest()) ? "Success" : "Failed");
	}
	
	
}
