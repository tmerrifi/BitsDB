all: clientLib

clientLib: message.o clientGraph.o attributes.o jsonProcessing.o
	ar -rcv libGraphKernelClient.a clientGraph.o ../coreShared/message.o ../coreShared/coreUtility.o ../coreShared/linkedList.o ../graphShared/graphUtility.o attributes.o jsonProcessing.o;

jsonProcessing.o: jsonProcessing.c jsonProcessing.h
	gcc -g --std=gnu99 -c jsonProcessing.c -o jsonProcessing.o;
	
attributes.o: attributes.c
	gcc -g --std=gnu99 -c attributes.c -o attributes.o;

clientGraph.o: clientGraph.c
	gcc -g --std=gnu99 -c clientGraph.c -o clientGraph.o;
clean:
	rm -rf *~ *.dSYM *.a *.o
