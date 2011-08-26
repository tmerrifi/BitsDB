
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <semaphore.h>

#include "shmUtility.h"
#include "ftrace.h"

struct snap_message{
	unsigned int number;
	unsigned int page;
	unsigned int byte_offset;
	unsigned int exit_flag;
};


void call_msync(void * mem, struct ftracer * tracer, char * message, int length){
	ftrace_on(tracer);
	ftrace_write_to_trace(tracer,message);
	msync(mem,length, MS_SYNC);
	ftrace_write_to_trace(tracer,"SNAP: SUBSCRIBER DONE\n");
	ftrace_off(tracer);
}


int main(int argc, char ** argv){

	struct ftracer * tracer = ftrace_init();
	//printf("I'm a child, arg 2 is %s\n", argv[1]);
	char mq_name[15];
	sprintf(mq_name, "/mq_%s", argv[1]);
	//open mq
	mqd_t new_mq = mq_open(mq_name, O_RDONLY);	
	if (new_mq == -1){	
		exit(1);
	}
	
	//open mq to the main process
	mqd_t main_mq = mq_open("/mq_main_snap_tester", O_RDWR);
	if (main_mq == -1){	
		exit(1);
	}
	
	sem_t * shm_sem = sem_open("sem_snapshot_sys_call_test", O_RDWR);
	
	struct snap_message message;
	
	int error_count = 0;
	
	//ftrace_on(tracer);
	//ftrace_write_to_trace(tracer,"\n\n\n\n\nSUBSCRIBER: opening shared memory\n");
	void * mem = coreUtil_openSharedMemory( "snapshot_test_shm" , (void *)0xA0000000, atoi(argv[2]), SHM_CLIENT, NULL);
	//ftrace_write_to_trace(tracer,"SUBSCRIBER: done opening shared memory\n");
	//ftrace_off(tracer);
	printf("done opening\n");
	while(1){
		//printf("trying to receive\n");
		mq_receive(new_mq, (char *)&message, sizeof(struct snap_message), NULL);
		//printf("got something\n");
		
		//semaphore
		if (message.exit_flag){
			if (error_count)
				printf("FAILED: %d errors occured in process %s\n", error_count, argv[1]);
			else
				printf("\nSUCCESS: No errors in process %s\n", argv[1]);
			sem_close(shm_sem);
			exit(0);	
		}
		else{
			sem_wait(shm_sem);
			printf("got something....done waiting\n");
			char ftrace_message[200];
			sprintf(ftrace_message, "\n\n\n\n\n\n\n\nWTF!!! CALLING MSYNC FOR PROCESS %s\n\n", argv[1]);
			call_msync(mem,tracer, ftrace_message, atoi(argv[2]));
			printf("Just returned from msync\n");
			
			int byte_offset = message.page * (1<<12) + message.byte_offset;
			
			//ftrace_on(tracer);
			//ftrace_write_to_trace(tracer,"\n\n\n\n\nSUBSCRIBER: reading from shared mem!!!\n");
			int * target = (int *)(mem + byte_offset);
			//ftrace_write_to_trace(tracer,"SUBSCRIBER: done reading shared memory\n");
			//ftrace_off(tracer);
			
			
			//printf("SUB: read at %p, val is %d\n", target, *target);
			//printf("looking at to %p, val is %d, expected value is %d\n", target, *target, message.number);
			if (*target != message.number){
				++error_count;	
			}
			sem_post(shm_sem);
			
			struct snap_message message;
			message.exit_flag = 1;
			printf("sending to main queue!\n");
			mq_send(main_mq, (char *)&message, sizeof(struct snap_message), 0);
			printf("done sending to main queue!\n");
		}
	}
}