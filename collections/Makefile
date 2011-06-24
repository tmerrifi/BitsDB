all: test

test: list.o array.o collection.o
	gcc --std=gnu99 -g -o test main.c collection.o array.o ../coreShared/shmUtility.o ../graphShared/graphUtility.o list.o -lrt;
	./test

libCore: list.o array.o collection.o
	ar -rcv libBitsDBCoreCollections.a list.o array.o collection.o

libClient: list.o array.o collection.o
	ar -rcv libBitsDBClientCollections.a list.o array.o collection.o

list.o: list.h array.o collection.o
	gcc --std=gnu99 -g -c -o list.o list.c

array.o: collection.h array.c array.h
	gcc --std=gnu99 -g -c -o array.o array.c

collection.o: collection.c collection.h
	gcc --std=gnu99 -g -c -o collection.o collection.c

clean:
	rm *.o *.a test

force_look : 
	true
