
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sched.h>

#include "shmUtility.h"
#include "ftrace.h"

struct snap_message{
	unsigned int number;
	unsigned int page;
	unsigned int byte_offset;
	unsigned int exit_flag;
};

struct child_process{
	pid_t pid;
	mqd_t mqd;	
};

struct operation{
	int tst_int;
	int page;	
};

/*we're going to write a bunch of numbers to pages, so we need the numbers up front*/
struct operation * get_random_ops(int num_of_writes, int num_of_pages){
	srandom(time(NULL));
	struct operation * arr_of_ops = malloc(sizeof(struct operation) * num_of_writes);
	
	for (int i = 0; i < num_of_writes; ++i){
		arr_of_ops[i].tst_int =  (random() % (1<<12)) & 0xFFFFFFFC;
		printf("the test int is %u\n", arr_of_ops[i].tst_int);
		arr_of_ops[i].page = random() % num_of_pages;
	}
	
	return arr_of_ops;
}

mqd_t open_mqueue(int number){
	char * mq_name = malloc(10 + strlen("mq_"));
	sprintf(mq_name, "/mq_%d", number);
	/*setup attributes*/
	struct mq_attr attr;
  	attr.mq_flags   = 0;
  	attr.mq_maxmsg  = 10;
  	attr.mq_msgsize = sizeof(struct snap_message);
  	attr.mq_curmsgs = 0;

	
	mqd_t new_mq = mq_open(mq_name, O_CREAT | O_RDWR, 0777, &attr);
	return new_mq;
}

mqd_t open_tester_mqueue(){
	/*setup attributes*/
	struct mq_attr attr;
  	attr.mq_flags   = 0;
  	attr.mq_maxmsg  = 10;
  	attr.mq_msgsize = sizeof(struct snap_message);
  	attr.mq_curmsgs = 0;
	mqd_t new_mq = mq_open("/mq_main_snap_tester", O_CREAT | O_RDWR, 0777, &attr);
	return new_mq;
}

struct child_process * create_processes(int num_of_subscribers, int length){
	struct child_process * children = malloc(sizeof(struct child_process) * num_of_subscribers);
	pid_t childPid;
	char * argvec[10];
	char len[20];
	argvec[0]="subscriber";
	sprintf(len, "%d", length);
	argvec[2]=len;
	argvec[3]=NULL;
	char * digit = malloc( num_of_subscribers / 10 + 1 );	//make sure we have enough space for all the digits we have.
	
	for (int i = 0; i < num_of_subscribers; ++i){
		sprintf(digit,"%d", i);		//lets send this number to the child process
		argvec[1] = digit;
		switch(childPid = fork()){
			case -1:
				perror("pid creation failed\n");
				break;	
			case 0:
				execv("subscriber", argvec);
				break;
			default:
				children[i].pid=childPid;
				children[i].mqd=open_mqueue(i);
		}
	}
	return children;
}

void kill_children(struct child_process * children, int num_of_subscribers){
	
	for (int i = 0; i < num_of_subscribers; ++i){
		struct snap_message message;
		message.exit_flag = 1;
		int result = mq_send(children[i].mqd, (char *)&message, sizeof(struct snap_message), 0);
	}
}

void * open_shared_mem(struct ftracer * tracer, int num_of_pages, int turn_on_ftrace){

	if (turn_on_ftrace){
		ftrace_on(tracer);
		ftrace_write_to_trace(tracer,"\n\n\n\n\n\nTESTER: opening shared memory\n");
	}
	void * mem = coreUtil_openSharedMemory( "snapshot_test_shm" , (void *)0xA0000000, (1<<12) * num_of_pages, SHM_CORE, NULL);
	if (turn_on_ftrace){
		ftrace_write_to_trace(tracer,"TESTER: done opening shared memory\n");
		ftrace_off(tracer);	
	}
	return mem;
}

void write_to_mem(struct ftracer * tracer, int * target_mem, int turn_on_ftrace){
	
	if (turn_on_ftrace){
		ftrace_on(tracer);
		ftrace_write_to_trace(tracer,"\n\n\n\n\n\nTESTER: writing to mem\n");
	}
	*target_mem = (int)(random() % 1000);
	if (turn_on_ftrace){
		ftrace_write_to_trace(tracer,"TESTER: DONE writing to mem\n");
		ftrace_off(tracer);	
	}
}


int main(int argc, char ** argv){
	
	if (argc < 6){
		printf("usage: ./tester <number_of_writes> <size_of_mapping_in_pages> <number_of_subscribers> <frequeny_of_testing> <y_or_n_for_ftrace>\n");	
		exit(1);
	}
	int num_of_writes = atoi(argv[1]);	//how many writes should the owner perform?
	int num_of_pages = atoi(argv[2]);
	int num_of_subscribers = atoi(argv[3]);
	int frequency_of_updates = atoi(argv[4]); 
	int turn_on_ftrace = (strncmp("y",argv[5],1)) ? 0 : 1;
	int length = (1<<12) * num_of_pages;	//length in bytes
	struct ftracer * tracer = ftrace_init();
	mqd_t main_queue = open_tester_mqueue();
	
	//open the semaphore
	sem_t * shm_sem =  sem_open("sem_snapshot_sys_call_test", O_CREAT | O_RDWR, 0777, 1);
	if (shm_sem == SEM_FAILED){
		perror("sem_open failed: ");
		exit(EXIT_FAILURE);	
	}
	
	void * mem = open_shared_mem(tracer, num_of_pages, turn_on_ftrace);

	struct operation * arr_of_random_ops = get_random_ops(num_of_writes,num_of_pages);	//get all our random numbers

	struct child_process * children = create_processes(num_of_subscribers, length);
	
	
	for (int i = 0; i < num_of_writes; ++i){
		int process = random() % num_of_subscribers;
		struct snap_message message;
		
		sem_wait(shm_sem);
		
		int * target_mem = ((int *)(mem + (arr_of_random_ops[i].page * (1<<12) + arr_of_random_ops[i].tst_int)));
		
		printf("value is %d\n", *target_mem);	//important, won't work without the read first. Eventually we'll fix.
		write_to_mem(tracer, target_mem, turn_on_ftrace);
		
		if (i % frequency_of_updates == 0){
			int test_index = (i > 0) ? random() % i : 0;
			//printf("wrote to %p, val is %d\n", target_mem, *target_mem);
			message.number = *target_mem;
			message.page = arr_of_random_ops[i].page;
			message.byte_offset = arr_of_random_ops[i].tst_int;
			message.exit_flag = 0;
			sem_post(shm_sem);
			printf("sending\n");
			//sleep(30);
			int result = mq_send(children[process].mqd, (char *)&message, sizeof(struct snap_message), 0);
			printf("main queue blocking\n");
			mq_receive(main_queue, (char *)&message, sizeof(struct snap_message), NULL);
			printf("main queue done blocking\n");
			//printf("got something: exit_flag is %d\n", message.exit_flag);
		}
		else{
			sem_post(shm_sem);
		}
		
	}
	
	kill_children(children, num_of_subscribers);
	sem_close(shm_sem);
	return 0;
}



