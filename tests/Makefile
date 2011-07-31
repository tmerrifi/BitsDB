
CLIENT_LIB=GraphKernelClient

all: initializationTests

initializationTests: initializationTests.c
	gcc --std=gnu99 -g -o initializationTests initializationTests.c ../graphShared/graphUtility.o ../coreShared/coreUtility.o ../coreShared/shmUtility.o  -l$(CLIENT_LIB) -lrt -lBitsDBCoreCollections -lyajl; 

clean:
	rm initializationTests;
