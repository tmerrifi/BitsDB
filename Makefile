
CLIENT_HEADERS=clientLibrary/clientGraph.h clientLibrary/message.h
GLOBAL_HEADERS=include/graph.h include/kernelMessage.h include/global.h include/objectSlab.h

all: installHeaders clientLibrary kernel install

tests: clientLibrary kernel install
	cd kernel; sudo ./kernel & cd ..;
	cd tests; make --always-make all;
	./utilityScripts/killKernels.sh;	

install: clientLibrary kernel force_look
	cp clientLibrary/libGraphKernelClient.a /usr/local/lib;

clientLibrary: installHeaders force_look
	cd clientLibrary; make all;
	
kernel: installHeaders force_look
	cd kernel; make all;

installHeaders: force_look
	cp $(CLIENT_HEADERS) /usr/local/include/graphKernel; \
	cp $(GLOBAL_HEADERS) /usr/local/include/graphKernel;

force_look : 
	true
