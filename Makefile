
CLIENT_HEADERS=clientLibrary/clientGraph.h clientLibrary/message.h clientLibrary/attributes.h
GLOBAL_HEADERS=include/graph.h include/kernelMessage.h include/global.h include/objectSlab.h coreShared/shmUtility.h coreShared/coreUtility.h graphShared/graphUtility.h collections/array.h collections/collection.h collections/list.h core/jsonProcessing.h coreShared/message.h coreShared/linkedList.h

all: installHeaders clientLibrary core install

tests: clientLibrary core install
	cd core; sudo ./core & cd ..;
	cd tests; make --always-make all;
	./utilityScripts/killKernels.sh;	

install: coreShared clientLibrary collections core force_look
	cp clientLibrary/libGraphKernelClient.a /usr/local/lib;
	cp collections/libBitsDBClientCollections.a /usr/local/lib;
	cp collections/libBitsDBCoreCollections.a /usr/local/lib;

clientLibrary: installHeaders coreShared graphShared force_look
	cd clientLibrary; make all;

collections: force_look
	cd collections; make libCore; make libClient;

coreShared: force_look	
	cd coreShared; make;

graphShared: force_look
	cd graphShared; make;

core: installHeaders force_look
	cd core; make all;

installHeaders: force_look
	cp $(CLIENT_HEADERS) /usr/local/include/graphKernel; \
	cp $(GLOBAL_HEADERS) /usr/local/include/graphKernel;

force_look : 
	true
