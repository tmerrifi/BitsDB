
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>	//for memory mapping
#include <sys/stat.h>

#include "graphKernel/objectSlab.h"

/*****PRIVATE FUNCTION PROTOTYPES*******************/
int resizeSlab(ObjectSlab * slab, int newSize);
void openSharedMemory(ObjectSlab * slab, char * segmentName, int * sizeInBytes);
void * mapSharedMemory(ObjectSlab * slab, int initBytes, void * initAddress);


/********PUBLIC FUNCTIONS*****************************/

void closeObjectSlab(ObjectSlab * slab){
	if (munmap(slab->objects, slab_totalBytes(slab->sizeInObjects,slab->sizeOfType)) == -1){
		perror("unmapping (closing) the slab failed");
	}
	close(slab->shm_fd);
}

//DESCRIPTION: Initalize the slab object. This is an abstraction for a contiguous block of objects that can be index into using
//the id of the object. The slab works exclusively with mmap'd regions. Thus it takes a file descriptor and does an ftruncate if
//it needs to resize the region. Also, the slab is a homogenous storage device, so it only stores the same type (size) of objects
ObjectSlab * initObjectSlab(char * segmentName, void * initAddress, int sizeOfType, int slabIncrementSize){
	ObjectSlab * slab = malloc(sizeof(ObjectSlab));
	if (slab) {
		int sizeInBytes=0;
		openSharedMemory(slab,segmentName,&sizeInBytes);
		if (slab->shm_fd == -1){
			perror("creating shared memory segment failed");
			return NULL;
		}
		if (sizeInBytes == 0){
			sizeInBytes=slabIncrementSize * sizeOfType + sizeof(ObjectSlabHeader);
			int result = ftruncate(slab->shm_fd, sizeInBytes);	//make the file bigger
			if (result==-1){
				perror("An error occured when trying to truncate a file");	
				return NULL;
			}
		}
		slab->objects = mapSharedMemory(slab, sizeInBytes, initAddress);
		if (!slab->objects){
			perror("failed to map the segment into the address space");
			return NULL;
		}
		
		slab->slabIncrementSize = slabIncrementSize;
		slab->sizeInObjects = (sizeInBytes - sizeof(ObjectSlabHeader))/sizeOfType;
		slab->sizeOfType = sizeOfType;
	}
	return slab;
}

int slab_addObject(ObjectSlab * slab, void * newObject){
	if (slab->sizeInObjects <= slab_objectsAllocated(slab)){								//the slab is too small to handle the new object
			if (resizeSlab(slab,slab->slabIncrementSize + slab_objectsAllocated(slab)) < 0){	//attempt to resize the slab by SLAB_INCREMENT
				return -1;																	//it failed, returning -1
			}
			slab->sizeInObjects += slab->slabIncrementSize;									//it worked, so up the available size
	}
	
	memcpy(slab->objects + (slab->sizeOfType * slab_objectsAllocated(slab)) ,newObject, slab->sizeOfType);	//cpy the new object into the slab
	slab_objectsAllocated(slab) += 1;
	return 1;
}


/*********PRIVATE FUNCTIONS*******************************/

//DESCRIPTION: Map the shared memory segment
void openSharedMemory(ObjectSlab * slab, char * segmentName, int * sizeInBytes){
	int fd = shm_open(segmentName, O_RDWR, 0777);
	if (fd < 0){
			fd = shm_open(segmentName,  O_CREAT | O_RDWR, 0777);
	}
	else{
		struct stat fileStats;
		if (fstat(fd,&fileStats) != -1){
			*sizeInBytes=(fileStats.st_size > 0) ? fileStats.st_size : 0;
			printf("size is %d\n", *sizeInBytes);
		}
		else{
			perror("filestats failed\n");	
		}
	}
	
	slab->shm_fd=fd;
}

//DESCRIPTION: Map the shared memory segment
void * mapSharedMemory(ObjectSlab * slab, int initBytes, void * initAddress){
	if (slab->shm_fd > 0){
		void * mem = mmap(initAddress, initBytes, PROT_READ | PROT_WRITE, MAP_SHARED,slab->shm_fd, 0);	//now map it into memory
		if ((int)mem != -1){
			return mem;
		}
		else{
			perror("An error occured when mapping a shared memory segment");
		}
	}
	return NULL;
}

int resizeSlab(ObjectSlab * slab, int newSizeInObjects){
	int result=-1;
	int newSizeInBytes=slab_totalBytes(newSizeInObjects,slab->sizeOfType);
	
	if (munmap(slab->objects, slab_totalBytes(slab->sizeInObjects,slab->sizeOfType)) == -1){
		perror("unmapping (resizing) the slab failed");
	}
	else if (ftruncate(slab->shm_fd, newSizeInBytes) == -1) {
		perror("error resizing the file (ftruncate)");
	}
	else {
		void * mem = mmap(slab->objects, newSizeInBytes, PROT_READ | PROT_WRITE, MAP_SHARED, slab->shm_fd, 0);	//now map it into memory
		if ((int)mem == -1){
			perror("An error occured when mapping a shared memory segment in resize for object slab.");
		}
		else{
			slab->objects=mem;
			slab->sizeInObjects=newSizeInObjects;
		}	
		result=1;
	}
	return result;
}






