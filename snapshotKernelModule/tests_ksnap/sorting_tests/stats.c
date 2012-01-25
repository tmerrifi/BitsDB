
#include "stats.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct stats * stats_init(){
  struct stats * stats = malloc(sizeof(struct stats));
  stats->total_latency=0;
  stats->latency_ops=0;
  stats->total_ops=0;

}

int stats_latency_counter_add(struct stats * stats, int latency){
  //printf("latency is %d\n", latency);
  stats->total_latency += latency;
  stats->latency_ops++;
}

void stats_total_ops_inc(struct stats * stats){
  stats->total_ops++;
}

int stats_output(struct stats * stats, char * file_name){

  FILE * f = fopen(file_name, "a+");
  char timing[5000];
	
  sprintf(timing, "LATENCY: %lu OPS: %lu\n", (stats->latency_ops) ? (stats->total_latency/stats->latency_ops) : 0, stats->total_ops);
  printf("LATENCY: %lu OPS: %lu\n", (stats->latency_ops) ? (stats->total_latency/stats->latency_ops) : 0, stats->total_ops);
  //sprintf(timing, "OPS: %lu\n", stats->total_ops);

  fwrite(timing, strlen(timing), 1, f);
  fclose(f);

}
