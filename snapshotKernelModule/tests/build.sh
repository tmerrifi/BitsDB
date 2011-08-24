gcc -std=gnu99 -D_GNU_SOURCE -g -c -o shmUtility.o shmUtility.c
gcc -std=gnu99 -g -c -o ftrace.o ftrace.c -lrt
gcc -std=gnu99 -g -o testing main.c shmUtility.o -lrt
gcc -std=gnu99 -g -o slave slave.c shmUtility.o ftrace.o -lrt
gcc -std=gnu99 -g -o master master.c shmUtility.o ftrace.o -lrt