
#ifndef FTRACE_H
#define FTRACE_H

struct ftracer{
	int fd_onoff;
	int fd_marker;	
};

struct ftracer * ftrace_init();

void ftrace_on(struct ftracer * tracer);

void ftrace_off(struct ftracer * tracer);

void ftrace_write_to_trace(struct ftracer * tracer, char * str);

void ftrace_close(struct ftracer * tracer);

#endif
