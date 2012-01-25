
#ifndef MEMCPY_SHM_H
#define MEMCPY_SHM_H

#include <semaphore.h>

#include "ksnap.h"

struct memcpy_shm{
  void * segment;
  ksnap * snapshot;
  sem_t * mutex;
  char * file_name;
};

#define MEMCPY_SHM_SUFFIX "memcpy_shm"

struct memcpy_shm * memcpy_shm_open(ksnap * snapshot, void * address);

struct memcpy_shm * memcpy_shm_client_open(ksnap * snapshot);

void memcpy_shm_client_update(struct memcpy_shm * memcpy_shm);

void memcpy_shm_settle(struct memcpy_shm * memcpy_shm, ksnap * snapshot);

#endif
