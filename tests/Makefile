
CLIENT_LIB=GraphKernelClient

all: initializationTests

initializationTests: initializationTests.c
	gcc --std=gnu99 -o initializationTests initializationTests.c -l$(CLIENT_LIB) -lrt;

clean:
	rm initializationTests *.o;
