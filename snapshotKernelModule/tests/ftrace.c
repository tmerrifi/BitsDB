
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "ftrace.h"

struct ftracer * ftrace_init(){
	struct ftracer * tracer = malloc(sizeof(struct ftracer));	
	if (tracer){
		tracer->fd_onoff = open("/sys/kernel/debug/tracing/tracing_on", O_WRONLY);
		tracer->fd_marker = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);	
	}
	return tracer;
}

void ftrace_on(struct ftracer * tracer){
	if (tracer){
		write(tracer->fd_onoff, "1", 1);
		lseek(tracer->fd_onoff,0,SEEK_SET);
	}
	else{
		perror("your tracer is NULL");	
	}
}

void ftrace_off(struct ftracer * tracer){
	if (tracer){
		write(tracer->fd_onoff, "0", 1);
		lseek(tracer->fd_onoff,0,SEEK_SET);
	}
	else{
		perror("your tracer is NULL");	
	}
}

void ftrace_write_to_trace(struct ftracer * tracer, char * str){
	if (tracer){
		write(tracer->fd_marker, str, strlen(str));
	}
	else{
		perror("your tracer is NULL");	
	}
}