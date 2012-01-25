
#include <stdlib.h>
#include "time_util.h"

struct timespec * time_util_create_timespec(int milliseconds){
  struct timespec * t = malloc(sizeof(struct timespec));
  t->tv_sec = (milliseconds/1000 > 0) ? milliseconds/1000 : 0;
  long mill_tmp = (milliseconds/1000 > 0) ? milliseconds-(milliseconds*t->tv_sec) : milliseconds;
  t->tv_nsec = mill_tmp*1000000;
  return t;
}

long time_util_time_diff(struct timespec * start, struct timespec * end){
  unsigned long start_total = start->tv_sec * 1000000 + (start->tv_nsec/1000);
  unsigned long end_total = end->tv_sec * 1000000 + (end->tv_nsec/1000);
  return (end_total - start_total);
}
