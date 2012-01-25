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

void sub_memcpy(ksnap * snap, ksnap_test_subs_profile * test_profile){
  
  printf("hello world\n");
  struct timespec * sleep_timer = time_util_create_timespec(500);
  struct memcpy_shm * memcpy_shm =  memcpy_shm_client_open(snap);
  char * seg = (char *)memcpy_shm->segment;
  for (int j=0; j<10; ++j){
    printf("\n\n\n%d:\n",j);
    memcpy_shm_client_update(memcpy_shm);
    for (int i=0; i<snap->size_of_segment; ++i){
      printf(" %d %p", seg[i], seg+i);
    }
    nanosleep(sleep_timer, NULL);
  }
  exit(1);
}


int main(int argc, char ** argv){
  
  struct timespec * sleep_timer = time_util_create_timespec(500);
  ksnap * snapshot = ksnap_open(50, "my_test_segment", COMMIT_ALWAYS, 100, KSNAP_SHARED, (void *)0xA0000000);
  struct memcpy_shm * memcpy_shm =  memcpy_shm_open(snapshot, (void *)0xD0000000);
  
  ksnap_test_subs_profile * test_profile = malloc(sizeof(ksnap_test_subs_profile));
  test_profile->action = sub_memcpy;
  test_profile->desired_address = (void *)0xA0000000;
  test_profile->open_ksnap = 0;

  ksnap_test_subs_init(1, 0, snapshot->size_of_segment, 
		       "my_test_segment", test_profile);
  //printf("here..\n");
  char * seg = (char *)snapshot->segment;
  for (int j=0; j<10; ++j){
    printf("\n\ntest %d...\n", j);
    for (int i=0; i<snapshot->size_of_segment; ++i){
      seg[i]=j*atoi(argv[1]);
      //printf("%d ", *seg);
    }
    memcpy_shm_settle(memcpy_shm, snapshot);  
    nanosleep(sleep_timer, NULL);
  }
}
