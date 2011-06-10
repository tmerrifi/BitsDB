all: clientLib

clientLib: message.o clientGraph.o
	ar -rcv libGraphKernelClient.a clientGraph.o message.o ../coreShared/coreUtility.o ../graphShared/graphUtility.o;
	
clientGraph.o: clientGraph.c
	gcc -g --std=gnu99 -c clientGraph.c -o clientGraph.o;
	
message.o: message.c
	gcc -g --std=gnu99 -c message.c -o message.o;

clean:
	rm -rf *~ *.dSYM *.a *.o
