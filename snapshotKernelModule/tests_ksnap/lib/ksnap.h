
#ifndef KSNAP_H
#define KSNAP_H

#include "shmUtility.h"

typedef struct{
  int size_of_segment;
  void * segment;
  int * fd;
  char * name;
  char * file_name;
}ksnap;


ksnap * ksnap_open(int size_of_segment, char * segment_name, int type_of_commit, int commit_interval, int process_type, void * desired_address);

ksnap * ksnap_create_no_open(int size_of_segment, char * segment_name, int type_of_commit, int commit_interval, int process_type, void * desired_address);

ksnap * ksnap_open_exisiting(char * segment_name);

void ksnap_update(ksnap * snap, int debug_it);

void ksnap_settle(ksnap * snap, int debug_it);

#define KSNAP_OWNER SHM_CORE
#define KSNAP_READER SHM_CLIENT
#define KSNAP_SHARED SHM_SHARED

#define KSNAP_DEBUG 1
#define KSNAP_NO_DEBUG 0

#endif
