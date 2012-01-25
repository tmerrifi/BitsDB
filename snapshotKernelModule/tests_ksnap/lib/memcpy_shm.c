
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>

#include "shmUtility.h"
#include "ksnap.h"
#include "memcpy_shm.h"

sem_t * __open_semaphore(char * name, int owner){
  char mutex_name[120];
  sprintf(mutex_name, "%s_%s.sem", name, MEMCPY_SHM_SUFFIX);
  sem_t * mutex = sem_open(mutex_name,((owner)? O_CREAT : 0),0644,1);
  return mutex;
}

struct memcpy_shm * memcpy_shm_open(ksnap * snapshot, void * address){
  struct memcpy_shm * shm = malloc(sizeof(struct memcpy_shm));
  //create the new file name
  char file_name[120];
  sprintf(file_name,"%s_%s",snapshot->name, MEMCPY_SHM_SUFFIX);
  printf("opening %s...\n", file_name);
  shm->segment = coreUtil_openSharedMemory(file_name, address, snapshot->size_of_segment, SHM_SHARED, NULL, COMMIT_NONE, 0);
  shm->snapshot = snapshot;
  shm->mutex = __open_semaphore(snapshot->name, 1);
  return shm;
}

struct memcpy_shm * memcpy_shm_client_open(ksnap * snapshot){
    //first, try and unmap the current snapshot
  munmap(snapshot->segment,snapshot->size_of_segment);
  struct memcpy_shm * shm = malloc(sizeof(struct memcpy_shm));
  //create the new file name
  shm->file_name=malloc(120);
  sprintf(shm->file_name,"%s_%s",snapshot->name, MEMCPY_SHM_SUFFIX);
  //map the new segment where the old one was
  shm->mutex = __open_semaphore(snapshot->name, 0);
  shm->snapshot = snapshot;
  printf("client opening %s...\n", shm->file_name);
  memcpy_shm_client_update(shm);

  return shm;
}

void memcpy_shm_client_update(struct memcpy_shm * memcpy_shm){
  printf("unmapping\n");
  munmap(memcpy_shm->snapshot->segment,memcpy_shm->snapshot->size_of_segment);
  printf("done unmapping\n");
  sem_wait(memcpy_shm->mutex);
  printf("going for it\n");
  memcpy_shm->segment = coreUtil_openSharedMemory(memcpy_shm->file_name, memcpy_shm->snapshot->segment, 
						  memcpy_shm->snapshot->size_of_segment, SHM_CLIENT, NULL, COMMIT_NONE, 0);
  printf("done\n");
  sem_post(memcpy_shm->mutex);
  printf("after mutex\n");
}

void memcpy_shm_settle(struct memcpy_shm * memcpy_shm, ksnap * snapshot){
  sem_wait(memcpy_shm->mutex);
  memcpy(memcpy_shm->segment, snapshot->segment, snapshot->size_of_segment);
  sem_post(memcpy_shm->mutex);
}
