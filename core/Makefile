all: core

core: graph.o server.c objectSlab.o
	gcc -DSERVER --std=gnu99 -g -o core server.c graph.o objectSlab.o ../graphShared/graphUtility.o ../coreShared/message.o ../coreShared/coreUtility.o ../coreShared/shmUtility.o -lyajl -lrt -lBitsDBCoreCollections

graph.o: graph.c
	gcc -DSERVER --std=gnu99 -g -c graph.c -o graph.o

objectSlab.o: objectSlab.c
	gcc -DSERVER --std=gnu99 -g -c objectSlab.c -o objectSlab.o

clean:
	rm -rf *~ *.dSYM *.o
