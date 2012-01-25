

#ifndef STATS_H
#define STATS_H


struct stats{
  unsigned long total_latency;
  unsigned long latency_ops;
  unsigned long total_ops;
};

struct stats * stats_init();

int stats_latency_counter_add(struct stats * stats, int latency);

int stats_output(struct stats * stats, char * file_name);

void stats_total_ops_inc(struct stats * stats);

#endif
