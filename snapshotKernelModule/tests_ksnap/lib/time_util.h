
#include <time.h>

struct timespec * time_util_create_timespec(int milliseconds);

long time_util_time_diff(struct timespec * start, struct timespec * end);
