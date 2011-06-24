
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>	//for memory mapping
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "shmUtility.h"

size_t coreUtil_getFileSize(int fd){
	size_t sizeInBytes=0;
	struct stat fileStats;
	if (fstat(fd,&fileStats) != -1){
		sizeInBytes=(fileStats.st_size > 0) ? fileStats.st_size : 0;
	}
	return sizeInBytes;
}

void * coreUtil_openSharedMemory(char * segmentName, void * address, int defaultSize, int callingProcess, int * fd){
	int open_flags, mmap_prots;
	
	switch(callingProcess){
		case(SHM_CREATE_ONLY):	
			open_flags=O_CREAT | O_RDWR | O_EXCL;
			mmap_prots=PROT_READ|PROT_WRITE;
			break;
		case(SHM_CORE):
			open_flags=O_CREAT | O_RDWR;
			mmap_prots=PROT_READ|PROT_WRITE;
			break;
		case(SHM_CLIENT):
			open_flags=O_RDONLY;
			mmap_prots=PROT_READ;
			break;
	}
	
	void * mem = NULL;
	
	int tmp_fd = 0;		//to allow people to pass in NULL just in case they don't care about getting back the file descriptor
	if (!fd){		
		fd = &tmp_fd;		
	}
	
	*fd = shm_open(segmentName, open_flags, 0777);
	if (*fd != -1){
		int sizeInBytes=coreUtil_getFileSize(*fd);
		if (sizeInBytes == 0 && callingProcess == SHM_CORE){
			ftruncate(*fd, defaultSize);
			sizeInBytes=defaultSize;
		}
		if (sizeInBytes > 0){
			mem = mmap(address,sizeInBytes,mmap_prots,MAP_SHARED,*fd,0);
		}	
	}
	return mem;
}

void coreUtil_closeSharedMemory(void * addr, int mappingSize, int fd){
	if (munmap(addr, mappingSize) == -1){
		perror("unmapping (closing) the slab failed");
	}
	close(fd);	
}

void * coreUtil_resizeSharedMemory(void * addr, int currentMappingSize, int newMappingSize, int fd){
	void * mem = NULL;
	if (munmap(addr, currentMappingSize) == -1){
		perror("unmapping (closing) the slab failed");
	}
	else if (ftruncate(fd, newMappingSize) == -1) {
		perror("error resizing the file (ftruncate)");
	}
	else {
		mem = mmap(addr, newMappingSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	//now map it into memory
		if ((int)mem == -1){
			perror("An error occured when mapping a shared memory segment in resize for object slab.");
		}
	}
	
	return mem;
}

void coreUtil_removeSharedMemory(char * segmentName){
	if (shm_unlink(segmentName) == -1){
		perror("Error unlinking");
	}
}




