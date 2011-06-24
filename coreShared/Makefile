all: shmUtility.o coreUtility.o linkedList.o message.o

message.o: message.c message.h
	gcc -g --std=gnu99 -c message.c -o message.o;

linkedList.o: linkedList.c linkedList.h
	gcc -g --std=gnu99 -c linkedList.c -o linkedList.o;

coreUtility.o: coreUtility.c coreUtility.h
	gcc -g --std=gnu99 -c coreUtility.c -o coreUtility.o -lrt;

shmUtility.o: shmUtility.c
	gcc -g --std=gnu99 -c shmUtility.c -o shmUtility.o -lrt;

clean:
	rm *.o;
