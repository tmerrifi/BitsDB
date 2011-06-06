
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "coreUtility.h"

size_t coreUtil_getFileSize(int fd){
	size_t sizeInBytes=0;
	struct stat fileStats;
	if (fstat(fd,&fileStats) != -1){
		sizeInBytes=(fileStats.st_size > 0) ? fileStats.st_size : 0;
	}
	return sizeInBytes;
}

void * coreUtil_openSharedMemory(char * segmentName, void * address, int defaultSize, int callingProcess){
	int open_flags = (callingProcess == SHM_CLIENT) ?  O_RDONLY : O_CREAT | O_RDWR;
	int mmap_prots = (callingProcess == SHM_CLIENT) ? PROT_READ : PROT_READ|PROT_WRITE;
	void * mem = NULL;
	int fd = shm_open(segmentName, open_flags, 0777);
	if (fd != -1){
		int sizeInBytes=coreUtil_getFileSize(fd);
		if (sizeInBytes == 0 && callingProcess == SHM_CORE){
			ftruncate(fd, defaultSize);
			sizeInBytes=defaultSize;
		}
		if (sizeInBytes > 0){
			mem = mmap(address,sizeInBytes,mmap_prots,MAP_SHARED,fd,0);
		}	
	}
	return mem;
}

