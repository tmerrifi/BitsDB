all: core

core: graph.o server.c jsonProcessing.o objectSlab.o
	gcc -DSERVER --std=gnu99 -g -o kernel server.c graph.o jsonProcessing.o objectSlab.o -lyajl

graph.o: graph.c
	gcc -DSERVER --std=gnu99 -g -c graph.c -o graph.o

jsonProcessing.o: jsonProcessing.c jsonProcessing.h
	gcc -DSERVER --std=gnu99 -g -c jsonProcessing.c -o jsonProcessing.o

objectSlab.o: objectSlab.c
	gcc -DSERVER --std=gnu99 -g -c objectSlab.c -o objectSlab.o

clean:
	rm -rf *~ *.dSYM *.o
