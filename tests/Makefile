
CLIENT_LIB=GraphKernelClient

all: initializationTests

initializationTests: initializationTests.c
	gcc --std=gnu99 -o initializationTests initializationTests.c ../graphShared/graphUtility.o ../coreShared/coreUtility.o -l$(CLIENT_LIB) -lrt -lBitsDBCoreCollections;

clean:
	rm initializationTests *.o;
