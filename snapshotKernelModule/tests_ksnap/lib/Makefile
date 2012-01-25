

all: staticlib

staticlib: ftrace.o shmUtility.o force_run ksnap_test_sub.o ksnap_test_owner.o memcpy_shm.o ksnap.o time_util.o 
	ar -rvc libsnaptest.a memcpy_shm.o ftrace.o shmUtility.o ksnap_test_sub.o ksnap_test_owner.o ksnap.o time_util.o;\
	sudo cp libsnaptest.a /usr/local/lib/
	sudo cp memcpy_shm.h ftrace.h shmUtility.h time_util.h ksnap_test_sub.h ksnap.h ksnap_test_owner.h /usr/local/include

ksnap_test_sub.o: ksnap_test_sub.c
	gcc -g -std=gnu99 -o ksnap_test_sub.o -c ksnap_test_sub.c -lrt

ksnap_test_owner.o: ksnap_test_owner.c
	gcc -g -std=gnu99 -o ksnap_test_owner.o -c ksnap_test_owner.c -lrt

memcpy_shm.o: ksnap.o memcpy_shm.c memcpy_shm.h
	gcc -g -std=gnu99 -o memcpy_shm.o -c memcpy_shm.c

ksnap.o: ksnap.c
	gcc -g -std=gnu99 -o ksnap.o -c ksnap.c -lrt

time_util.o: time_util.c
	gcc -g -std=gnu99 -o time_util.o -c time_util.c

ftrace.o: ftrace.c
	gcc -g -std=gnu99 -o ftrace.o -c ftrace.c

shmUtility.o: shmUtility.c
	gcc -g -std=gnu99 -o shmUtility.o -c shmUtility.c -lrt

force_run:
	true

clean:
	rm *.o *.a;
