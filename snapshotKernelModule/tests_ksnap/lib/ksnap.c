
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ksnap.h"
#include "shmUtility.h"
#include "ftrace.h"

//open up a snapshot, only to be called once on a snapshot
ksnap * ksnap_open(int size_of_segment, char * segment_name, int type_of_commit, int commit_interval, int process_type, void * desired_address){
  ksnap * snap = malloc(sizeof(ksnap));
  snap->name = malloc(100);
  snap->file_name = malloc(100);
  sprintf(snap->file_name, "%s.mem", segment_name);
  sprintf(snap->name, "%s", segment_name);
  
  printf("segment name is .... %s\n", snap->file_name);
  snap->size_of_segment = size_of_segment;
  snap->segment = coreUtil_openSharedMemory( snap->file_name , desired_address, size_of_segment, process_type, snap->fd, type_of_commit, commit_interval);
  return snap;
}

//create the ksnap object, but don't open the segment yet
ksnap * ksnap_create_no_open(int size_of_segment, char * segment_name, int type_of_commit, int commit_interval, int process_type, void * desired_address){
  ksnap * snap = malloc(sizeof(ksnap));
  snap->name = malloc(100);
  snap->file_name = malloc(100);
  sprintf(snap->file_name, "%s.mem", segment_name);
  sprintf(snap->name, "%s", segment_name);
  
  printf("segment name is .... %s\n", snap->file_name);
  snap->size_of_segment = size_of_segment;
  snap->segment = desired_address;
  return snap;
}

//what if we want to work with a segment that is already opened in our address space? This is useful for when we are using the
//malloc library and we don't have control (or full control) of the mmap() call and where it maps to. So, we have to go find the address
//so we can still call msync on it.
ksnap * ksnap_open_exisiting(char * segment_name){
  FILE * fp;
  ksnap * snap;
  int pid = getpid();
  //a buffer for the command we need to run (a shell script with some input params)
  char command[200];
  //a buffer for storing the segment address we get
  char segment_address[20];
  sprintf(command,"find_snapshot_mapping.sh %d %s\n", pid, segment_name); 
  printf("looking for %s %d\n", segment_name, pid);
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("failed to find snapshot, could not execute external shell.\n" );
    return NULL;
  }

  //get the segment address, the formula just *2 because it's in hex and 
  //-3 because proc doesn't seem to output the last 2 bytes (since it's obviously word aligned I guess)
  if (fgets(segment_address, (sizeof(void *)*2)-3, fp)==NULL){
    perror("failed to find existing snapshot\n");
    return NULL;
  }
  else{ 
    snap = malloc(sizeof(ksnap));
    snap->segment = (void *)strtol(segment_address, NULL, 16);
    snap->name = segment_name;
  }
  //now we need to get the size, so we grab the end address. There is a space in the output so the formula changes by 1
  if (fgets(segment_address, (sizeof(void *)*2)-2, fp)==NULL){
    perror("failed to find existing snapshot\n");
    return NULL;
  }
  else{
    unsigned long end_address = strtol(segment_address, NULL, 16);
    snap->size_of_segment=end_address - (unsigned long)snap->segment;
  }

  /* close */
  pclose(fp);
  return snap;
}

//lets update and get a new view of the snapshot
void ksnap_update(ksnap * snap, int debug_it){
  struct ftracer * tracer;
  if (debug_it){
    tracer=ftrace_init(tracer);
    ftrace_on(tracer);
    ftrace_write_to_trace(tracer, "\n\nSNAPSHOT:CALLING UPDATE\n\n\n");
  }
  msync(snap->segment,snap->size_of_segment, MS_SYNC);
  if (debug_it){
    ftrace_write_to_trace(tracer, "\n\nSNAPSHOT:DONE CALLING UPDATE\n\n\n");
    ftrace_off(tracer);
    ftrace_close(tracer);
  }
}

void ksnap_settle(ksnap * snap, int debug_it){
  struct ftracer * tracer;
  if (debug_it){
    tracer=ftrace_init(tracer);
    ftrace_on(tracer);
    ftrace_write_to_trace(tracer, "\n\nSNAPSHOT:CALLING SETTLE,\n\n\n"); 
  }
  msync(snap->segment,snap->size_of_segment, MS_SYNC);
  if (debug_it){
    ftrace_write_to_trace(tracer, "\n\nSNAPSHOT:DONE CALLING SETTLE\n\n\n");
    ftrace_off(tracer);
    ftrace_close(tracer);
  }
}
