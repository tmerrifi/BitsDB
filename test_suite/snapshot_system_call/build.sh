gcc -std=gnu99 -g -c -o ftrace.o ftrace.c -lrt
gcc -std=gnu99 -D_GNU_SOURCE -g -c -o shmUtility.o shmUtility.c
gcc -g -std=gnu99 -o subscriber subscriber.c shmUtility.o ftrace.o -lrt
gcc -g -std=gnu99 -o tester tester.c shmUtility.o ftrace.o -lrt
