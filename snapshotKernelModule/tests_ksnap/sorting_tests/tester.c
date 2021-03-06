
#include "rbtdata.h"
#include "rbtree.h"
#include "sort_test.h"
#include "time_util.h"
#include "ksnap.h"
#include "ftrace.h"
#include "subscribers.h"
#include "memcpy_shm.h"
#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>



#define SYNC_TYPE_GLOBAL_LOCK 1
#define SYNC_TYPE_KSNAP 2
#define SYNC_TYPE_MEMCPY 3

struct cmd_options{
  int total_time;
  int numbers_to_generate_between_sleeps;
  int type_of_data_structure;
  int time_to_sleep;
  int sub_time_to_sleep;
  int sub_number;
  int sync_type;
  int segment_size_in_bytes;
  int max_key;
};

struct cmd_options options;

TAILQ_HEAD(tailhead, key_object);
struct tailhead * head, * otherhead;


sem_t * mutex;

struct timespec time_begin;

pthread_mutex_t key_queue_lock;
pthread_cond_t key_queue_cond;

//another thread creates a queue of numbers that will be consumed
//by the main thread that updates the sorted data structure. 
struct key_object{
  struct timespec time_created;
  int key;
  TAILQ_ENTRY(key_object) entries;
};

//creates a key_object and initializes the time_created field to be
//the current time
struct key_object * __create_key_object(int key){
  struct key_object * obj = malloc(sizeof(struct key_object));
  obj->key=key;
  clock_gettime(CLOCK_MONOTONIC, &obj->time_created);
  return obj;
}

int __owner_ended(){
  struct timespec current_time; 
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  unsigned long diff = time_util_time_diff(&time_begin, &current_time);
  unsigned long secs = diff / 1000000;
    return secs > options.total_time;
}

int __stabalized(){
  struct timespec current_time; 
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  //printf("stabalized time %lu  %lu\n", current_time.tv_sec, current_time.tv_nsec);
  unsigned long diff = time_util_time_diff(&time_begin, &current_time);
  unsigned long secs = diff / 1000000;
  return secs > 0;
}


//the function used by the "other thread" to generate random keys
//and throw them into a queue
void * __generate_keys(void * args){
   printf("starting key generation...\n");
  srand(time(NULL));
  //creating the sleep timer to pass to nanosleep
  struct timespec * sleep_timer = time_util_create_timespec(options.time_to_sleep);
  printf("starting key generation...time to sleep is %d\n", options.time_to_sleep);
  int first_time=1;
  struct key_object * obj;

  while(!__owner_ended()){
    pthread_mutex_lock(&key_queue_lock);
    if (first_time){
      //lets fill it
      int ops_to_fill_it = options.max_key;
      //printf("OPS TO FILL IT %d\n", ops_to_fill_it);
      for (int k=0; k<ops_to_fill_it;++k){
	obj=__create_key_object(random() % options.max_key);
	TAILQ_INSERT_TAIL(head, obj, entries);
	first_time=0;
      }
    }
    else{
      //printf("generating....\n\n");
      for(int j=0; j<options.numbers_to_generate_between_sleeps;++j){
	obj=__create_key_object(random() % options.max_key);
	TAILQ_INSERT_TAIL(head, obj, entries);
      }
    }
    pthread_mutex_unlock(&key_queue_lock);
    pthread_cond_signal(&key_queue_cond);
    nanosleep(sleep_timer, NULL);
  }
  pthread_cond_signal(&key_queue_cond);
}

ksnap_test_subs_profile * _get_test_profile(struct sort_tester * tester, void * ksnap_address){
  ksnap_test_subs_profile * test_profile = malloc(sizeof(ksnap_test_subs_profile));
  if (options.sync_type == SYNC_TYPE_GLOBAL_LOCK){
     test_profile->action = subscribers_global_locking;
     test_profile->open_ksnap=0;
  }
  else if (options.sync_type == SYNC_TYPE_KSNAP){
    test_profile->action = subscribers_ksnap;
    test_profile->open_ksnap=1;
  }
  else if (options.sync_type == SYNC_TYPE_MEMCPY){
    test_profile->action = subscribers_memcpy;
    test_profile->open_ksnap=0;
  }
  test_profile->action_data = tester;
  test_profile->desired_address = ksnap_address;
  test_profile->sleep_ms = options.sub_time_to_sleep;
  test_profile->total_time = options.total_time;
  return test_profile;
}

sem_t * open_semaphore(char * name){
  char mutex_name[100];
  sprintf(mutex_name, "%s_global.sem", name);
  printf("opening the semaphore %s\n",mutex_name);
  sem_t * mutex = sem_open(mutex_name,O_CREAT,0644,1);
  return mutex;
}

void acquire(sem_t * mutex, ksnap * snapshot){
  if (options.sync_type == SYNC_TYPE_KSNAP){
    return;
  }
  else if (options.sync_type == SYNC_TYPE_GLOBAL_LOCK){
    sem_wait(mutex);
  }
}

void release(sem_t * mutex, ksnap * snapshot, struct memcpy_shm * memcpy_shm){
  if (options.sync_type == SYNC_TYPE_KSNAP){
    ksnap_settle(snapshot, KSNAP_NO_DEBUG);
  }
  else if (options.sync_type == SYNC_TYPE_GLOBAL_LOCK){
    sem_post(mutex);
  }
  else if (options.sync_type == SYNC_TYPE_MEMCPY){
    memcpy_shm_settle(memcpy_shm, snapshot);
  }
}

void * __consume_keys_and_update(void * args){
  struct key_object * obj;
  struct ftracer * tracer;
  struct stats * stats = stats_init();
  //sort_tester handles everything related to dealing with the sorted datastructure (insertion, deletion, etc...)
  struct sort_tester * tester = sort_tester_factory(options.type_of_data_structure, SORT_TEST_NAME(options), options.segment_size_in_bytes);
   //firing up the sort tester will also initialize the shared memory malloc, which maps in the shared memory.
  //So now we'll find the mapping so we can treat it like a normal ksnap mapping
  ksnap * snapshot = ksnap_open_exisiting(SORT_TEST_NAME(options));
  printf("done opening existing....size is %d\n", snapshot->size_of_segment);
  //now we create the subscribers
  ksnap_test_subs_profile * sub_profile = _get_test_profile(tester, snapshot->segment);
  ksnap_test_subs_init(options.sub_number, 0, snapshot->size_of_segment, 
		       SORT_TEST_NAME(options), sub_profile);
  sem_t * mutex = open_semaphore(SORT_TEST_NAME(options));
  struct memcpy_shm * memcpy_shm =  memcpy_shm_open(snapshot, (void *)0xA0000000);
  if (!snapshot)
    return NULL;
  //creating the sleep timer to pass to nanosleep
  struct timespec * sleep_timer = time_util_create_timespec(1);
  struct timespec current_time;
  int is_stabalized=0;
  while(!__owner_ended()){
    //printf("huh?\n");
    is_stabalized=__stabalized();
    pthread_mutex_lock(&key_queue_lock);
    //now see if anything has been added, if not then wait for a signal from the generator
    while(head->tqh_first==NULL && !__owner_ended()){
      //printf("waiting...\n");
      pthread_cond_wait(&key_queue_cond, &key_queue_lock);
      //printf("done waiting...\n");
    }
    //swap queue ptrs
    void * tmp_ptr = otherhead;
    otherhead=head;
    head=tmp_ptr;
    //printf("head is %p and otherhead is %p\n", head, otherhead);
    pthread_mutex_unlock(&key_queue_lock);

    
    acquire(mutex, snapshot);    
    while (otherhead->tqh_first != NULL){
      //get the current time
      sort_tester_insert(tester, otherhead->tqh_first->key);
      if (is_stabalized){
	/*printf("current time %lu %lu, other time %lu %lu \n", 
	  current_time.tv_sec, current_time.tv_nsec, head->tqh_first->time_created.tv_sec, head->tqh_first->time_created.tv_nsec);*/
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	stats_latency_counter_add(stats, time_util_time_diff(&otherhead->tqh_first->time_created, &current_time));
	stats_total_ops_inc(stats);
      }
      TAILQ_REMOVE(otherhead, otherhead->tqh_first, entries);
    }
    //printf("\n\n\n\n\nowner releasing...\n");
    release(mutex, snapshot, memcpy_shm);    
    //printf("owner done releasing...\n");    }
    
  }
  printf("owner made it out\n");
  //need to wait until the other processes are done
  int status;
  wait(&status);
  printf("outputting stats\n");
  stats_output(stats, "stats_owner.txt");
  printf("done with stats\n");
}


void __process_args(int argc, char ** argv){

 char * optString = "t:o:d:s:S:n:c:b:";
 int opt = getopt(argc,argv, optString);
 while(opt != -1){
   switch(opt){
   case 't':
     options.total_time = atoi(optarg);
     break;
   case 'o':
     options.numbers_to_generate_between_sleeps = atoi(optarg);
     break;
   case 'd':
     if (strcmp(optarg,"rbt")==0){
       options.type_of_data_structure = SORT_TEST_TYPE_RBTREE;
     }
     else if (strcmp(optarg,"list")==0){
       options.type_of_data_structure = SORT_TEST_TYPE_LIST;
     }
     break;
   case 's':
     options.time_to_sleep=atoi(optarg);
     break;
   case 'S':
     options.sub_time_to_sleep=atoi(optarg);
     break;
   case 'n':
     options.sub_number = atoi(optarg);
     break;
   case 'c':
     if (strcmp(optarg,"ksnap")==0){ 
       printf("using ksnap\n");
       options.sync_type = SYNC_TYPE_KSNAP;
     }
     else if (strcmp(optarg,"glock")==0){
       options.sync_type = SYNC_TYPE_GLOBAL_LOCK;
     }
     else if (strcmp(optarg,"memcpy")==0){
       options.sync_type = SYNC_TYPE_MEMCPY;
     }
     break;
   case 'b':
     options.segment_size_in_bytes = atoi(optarg);
     options.max_key = options.segment_size_in_bytes/((sizeof(struct rbtdata)+30);
     printf("max key is %d, size in bytes %d, size of rbtdata %d\n", options.max_key, options.segment_size_in_bytes, sizeof(struct rbtdata));
     break;
   }
   opt = getopt(argc,argv, optString);
 }
}

int main(int argc, char ** argv){

  __process_args(argc, argv);
  clock_gettime(CLOCK_MONOTONIC, &time_begin);
  head = malloc(sizeof(struct tailhead));
  otherhead = malloc(sizeof(struct tailhead));
  TAILQ_INIT(head);
  TAILQ_INIT(otherhead);
  //create a pthread to generate the keys
  pthread_t * generate_thread = malloc(sizeof(pthread_t));
  //create a pthread to consume the keys and update the sorted data structure
  pthread_t * consume_thread = malloc(sizeof(pthread_t));
  //initialize the mutex used for locking down the key queue
  pthread_mutex_init(&key_queue_lock,0);
  pthread_cond_init(&key_queue_cond,0);
  //start the threads
  pthread_create(generate_thread,NULL,__generate_keys,NULL);
  pthread_create(consume_thread,NULL,__consume_keys_and_update,NULL);
  pthread_join(*generate_thread,NULL);
  pthread_join(*consume_thread,NULL);
}

