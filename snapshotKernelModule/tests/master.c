
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

void printItOut(void * mem){
	for (int i = 0; i < (length/sizeof(int)); ++i){
		if (i % 500 == 0){
			printf("%d  %p ", *( ((int *)mem) + i ), mem + i);
		}
	}
}

void call_msync(void * mem, struct ftracer * tracer, char * message){
	//ftrace_on(tracer);
	//ftrace_write_to_trace(tracer,message);
	msync(mem,length, MS_SYNC);
	//ftrace_write_to_trace(tracer,"SNAP: MASTER DONE\n");
	//ftrace_off(tracer);
}

void start_ftrace(struct ftracer * tracer, char * message){
	ftrace_on(tracer);
	ftrace_write_to_trace(tracer,message);
}

void end_ftrace(struct ftracer * tracer){
	ftrace_write_to_trace(tracer,"SNAP: MASTER: DONE\n");
	ftrace_off(tracer);
}

int simpleTest(){
	int testInt = 0;
	struct ftracer * tracer = ftrace_init();
	start_ftrace(tracer, "MASTER: opening shared memory\n");
	void * mem = coreUtil_openSharedMemory( "snapshot_test1" , (void *)0xA0000000, length, SHM_CORE, NULL);	
	end_ftrace(tracer);
	call_msync(mem, tracer, "\n\n\n\n\n\n\n\n\n\nMASTER my-msync 1");
	testInt = getInteger(mem, 1, 12);
	printf("MASTER initial test int is %d\n", testInt);
	call_msync(mem, tracer, "\n\n\nMASTER my-msync 2");
	
	//call_msync(mem, tracer, "msync from new master blah\n");
	
	start_ftrace(tracer, "\n\n\n\n\n\n\n\nMASTER: starting first write!!!\n");
	writeToByte(mem, 1, 12, 65);
	end_ftrace(tracer);
	call_msync(mem, tracer, "\n\n\nMASTER my-msync 3");
	testInt = getInteger(mem, 1, 12);
	printf("MASTER test int is %d\n", testInt);
	
	sleep(5);
	//writeToByte(mem, 1, 12, 40);
	
	//testInt = getInteger(mem, 1, 12);
	//printf("MASTER test int is %d\n", testInt);
	
	call_msync(mem, tracer, "\n\n\nMASTER my-msync 4");
	//munmap((void *)0xA0000000, length);	//unmap it!
}

void lilTest(char * name){
	struct ftracer * tracer = ftrace_init();
	
	void * mem = coreUtil_openSharedMemory( name , (void *)0xA0000000, length, SHM_CORE, NULL);
	
	int testInt = getInteger(mem, 0, 12);
	printf("test int is .... %d\n", testInt);

	start_ftrace(tracer, "Starting the big write!!!\n");
	writeToByte(mem, 0, 12, 20);
	end_ftrace(tracer);
	
	//printItOut(mem);
	
    testInt = getInteger(mem, 0, 12);
	printf("test int is .... %d\n", testInt);
	
	call_msync(mem, tracer, "msync from brand new master 1\n");
}

int main(int argc, char ** argv){
	if (strncmp(argv[1], "1", 1) == 0){
		simpleTest();
	}
	//lilTest(argv[1]);
}




