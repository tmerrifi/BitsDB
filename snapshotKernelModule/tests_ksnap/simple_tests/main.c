
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "ksnap_test_sub.h"
#include "ksnap_test_owner.h"

#define PAGE_SIZE (1<<12)

int main(){

  ksnap_test_owner_profile owner_profile;

  owner_profile.sleep_ms=5; 
  owner_profile.bytes_per_page=5; 
  owner_profile.total_num_of_writes=3;
  owner_profile.writes_between_sleep=5; 
  owner_profile.writes_between_settled=5; 
  
  //start the 
  ksnap_test_owner_init(ksnap_test_owner_type_snap, PAGE_SIZE*5, 
			  "asegment", &owner_profile);
  
  //first, create a test profile
  ksnap_test_subs_profile profile;
  profile.sleep_ms=500;
  profile.bytes_per_page=5;
  profile.total_num_of_reads=3;
  profile.reads_between_sleep=5;
  profile.reads_between_updates=5;

  //fire up the readers
  printf("huh?\n");
  ksnap_test_subs_init(1, ksnap_test_sub_type_snap, PAGE_SIZE*5, "asegment", &profile);
}


